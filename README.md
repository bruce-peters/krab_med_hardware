# krab_med_hardware

Firmware and host-side bridge for the physical medicine box that goes with [krab_med_bot](https://github.com/bruce-peters/krab_med_bot). An ESP32 drives six compartment LEDs, a servo for the lid, a lit push button, a speaker, and a 16x2 LCD. A Python script on the computer talks to the ESP32 over USB serial, exposes the box as a small local HTTP API, and tells the krab_med_bot server when the button is pressed.

## Files

| File | What it is |
|---|---|
| `arduinocode.ino` | Main ESP32 firmware. Accepts text commands over serial (115200 baud) and prints `BUTTON_PRESSED` when the button is pressed |
| `cerial.py` | Host bridge: serial terminal + HTTP server on `127.0.0.1:5050` + forwards button presses to krab_med_bot |
| `hardware_interface.py` | Copy of krab_med_bot's `server/controllers/hardware_interface.py`, with the controller URL set to the bridge (`http://localhost:5050`). It is a module for the bot server, not a standalone script |
| `fullfeatures.ino` | Standalone demo sketch with no serial control: pressing the button turns on all LEDs, chirps, and raises the servo |
| `lcd.ino` | LCD wiring test sketch |

## Wiring (ESP32 GPIO numbers, as defined in the sketches)

| Part | GPIO |
|---|---|
| Compartment LEDs 1-6 | 4, 16, 17, 5, 18, 19 |
| Push button input (to GND, uses `INPUT_PULLUP`, active LOW) | 12 |
| Push button light | 14 |
| Servo signal | 27 (0° = closed, 90° = open) |
| Speaker | 13 |
| LCD 16x2 (4-bit mode): RS, EN, D4, D5, D6, D7 | 22, 23, 26, 25, 33, 32 |

## Flash the ESP32

Not compiled or flashed here (no board, and the ESP32 toolchain did not fit in this sandbox's disk). The steps below are the standard ones for these libraries.

**Arduino IDE**
1. Add the ESP32 boards package: *Boards Manager* → install **esp32 by Espressif Systems** (if it is not listed, add `https://espressif.github.io/arduino-esp32/package_esp32_index.json` under *Preferences → Additional boards manager URLs*).
2. *Library Manager* → install **ESP32Servo** and **LiquidCrystal**.
3. Open `arduinocode.ino` (the IDE will offer to put it in an `arduinocode/` folder), select your ESP32 board (for example *ESP32 Dev Module*) and its port, then *Upload*.

**arduino-cli**
```bash
arduino-cli core install esp32:esp32 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install ESP32Servo LiquidCrystal
mkdir -p arduinocode && cp arduinocode.ino arduinocode/
arduino-cli compile --fqbn esp32:esp32:esp32 arduinocode
arduino-cli upload -p <your-serial-port> --fqbn esp32:esp32:esp32 arduinocode
```

### Serial commands (`arduinocode.ino`)
`ON`, `OFF`, `BLINK` (all LEDs) · `LED1_ON` … `LED6_ON`, `LED1_OFF` … `LED6_OFF` · `SERVO_UP`, `SERVO_DOWN` · `BUTTON_LIGHT_ON`, `BUTTON_LIGHT_OFF` · `CHIRP` · `LCD_CLEAR`, `LCD_PRINT1 <text>`, `LCD_PRINT2 <text>`

## Run the bridge (`cerial.py`)

Requires Python 3 and pyserial.
```bash
pip install pyserial
```
1. Edit `PORT` at the top of `cerial.py` to your ESP32's serial port (macOS: `/dev/cu.usbserial-XXXX`, Windows: `COM3`, Linux: `/dev/ttyUSB0`).
2. Run it:
   ```bash
   python cerial.py
   ```
   You can type serial commands (above) at the prompt; `Q` quits.

While it runs, it also:
- **Serves an HTTP API on `http://127.0.0.1:5050`** (all `POST`): `/servo/open`, `/servo/close`, `/speaker`, `/button/on`, `/button/off`, `/led/on`, `/led/off` (optional JSON `{"led_id": 1-6}`, otherwise all LEDs), `/led1/on` … `/led6/off`, `/lcd/clear`, `/lcd/print` (JSON `{"line1": "...", "line2": "..."}`).
- **Forwards button presses to krab_med_bot**: on `BUTTON_PRESSED` it POSTs `{"compartment": 1, "medication_id": ...}` to `http://127.0.0.1:5800/api/hardware/dispense`. Start the bot server on that port (`uvicorn server.main:app --port 5800`) or change `DISPENSE_URL`.

## What was tested here

- `python -m py_compile cerial.py hardware_interface.py` passes (Python 3.12).
- `cerial.py`'s HTTP API and button forwarding were run against a loopback serial port in place of the ESP32: every endpoint above wrote the expected serial command, unknown paths return 404, and a simulated `BUTTON_PRESSED` line produced a successful `POST /api/hardware/dispense` to krab_med_bot running in mock hardware mode on port 5800.
- Not tested: the sketches (no compile, no board), real serial hardware.

## Known gaps

- krab_med_bot's `HARDWARE_MODE=production` expects replies with `"status": "success"` and extra endpoints (`GET /servo/status`, `/led/blink`, `/led/clear`) that `cerial.py` does not provide (it replies `"status": "ok"`). Use the bot in mock hardware mode with this bridge, or extend the bridge.
- The LCD start-up text in `arduinocode.ino` is a fixed date and time (`Sat, October 25` / `6:02 pm`), not a clock.
