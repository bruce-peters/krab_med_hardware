import serial
import time
import threading
import sys
import json
import urllib.request
import urllib.error
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

# === Update this line to match your ESP32's port ===
# On macOS, it’ll look like: /dev/tty.usbserial-XXXXX or /dev/cu.usbmodemXXXX
# On Windows: COM3, COM4, etc.
PORT = "/dev/cu.usbserial-0001"
BAUD = 115200

DISPENSE_URL = "http://127.0.0.1:5800/api/hardware/dispense"
DISPENSE_COMPARTMENT = 1
DISPENSE_MEDICATION_ID = "550e8400-e29b-41d4-a716-446655440000"

def post_dispense_event():
    payload = {
        "compartment": DISPENSE_COMPARTMENT,
        "medication_id": DISPENSE_MEDICATION_ID,
    }
    data = json.dumps(payload).encode()
    req = urllib.request.Request(
        DISPENSE_URL,
        data=data,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=3) as resp:
            body = resp.read().decode(errors="ignore")
            print(f"[Host] Dispense POST status={resp.status} body={body[:200]}")
    except urllib.error.HTTPError as e:
        try:
            err_body = e.read().decode(errors="ignore")
        except Exception:
            err_body = ""
        print(f"[Host] Dispense POST failed HTTP {e.code}: {err_body[:200]}")
    except Exception as e:
        print(f"[Host] Dispense POST error: {e}")

def reader_loop(ser: serial.Serial, stop_event: threading.Event):
    while not stop_event.is_set():
        try:
            line = ser.readline().decode(errors="ignore").strip()
            if line:
                print(f"[ESP32] {line}")
                # Optional: react to button press
                if line == "BUTTON_PRESSED":
                    print("[Host] Received button press.")
                    threading.Thread(target=post_dispense_event, daemon=True).start()
        except Exception as e:
            print(f"[Reader] Error: {e}")
            break

def send_command(ser: serial.Serial, cmd: str):
    ser.write((cmd + "\n").encode())
    ser.flush()

def start_http_server(ser: serial.Serial, write_lock: threading.Lock, host: str = "127.0.0.1", port: int = 5050):
    class Handler(BaseHTTPRequestHandler):
        def _read_json(self):
            try:
                length = int(self.headers.get('Content-Length', '0'))
                if length:
                    data = self.rfile.read(length)
                    return json.loads(data.decode(errors='ignore'))
            except Exception:
                return None
            return None

        def _send_json(self, code: int, payload: dict):
            body = json.dumps(payload).encode()
            self.send_response(code)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def do_POST(self):  # noqa: N802 (match BaseHTTPRequestHandler signature)
            path = self.path.rstrip('/')
            tokens = [tok for tok in path.split('/') if tok]
            payload = self._read_json() or {}

            # Utility to send a serial command safely
            def send(cmd: str):
                with write_lock:
                    send_command(ser, cmd)
                return cmd

            cmd = None
            status = 200
            # /servo/open, /servo/close
            if tokens == ["servo", "open"]:
                cmd = send("SERVO_UP")
            elif tokens == ["servo", "close"]:
                cmd = send("SERVO_DOWN")
            # /speaker
            elif tokens and tokens[0] == "speaker" and len(tokens) == 1:
                cmd = send("CHIRP")
            # /button/on, /button/off (button light)
            elif tokens == ["button", "on"]:
                cmd = send("BUTTON_LIGHT_ON")
            elif tokens == ["button", "off"]:
                cmd = send("BUTTON_LIGHT_OFF")
            # /lcd/clear and /lcd/print
            elif tokens == ["lcd", "clear"]:
                cmd = send("LCD_CLEAR")
            elif tokens == ["lcd", "print"]:
                # JSON body: {"line1": "text", "line2": "text"}
                line1 = str(payload.get("line1", "")) if payload else ""
                line2 = str(payload.get("line2", "")) if payload else ""
                if line1:
                    send(f"LCD_PRINT1 {line1}")
                if line2:
                    send(f"LCD_PRINT2 {line2}")
                cmd = "LCD_PRINT"
            # /led/on and /led/off (all LEDs), optional JSON: {"led_id": n}
            elif tokens == ["led", "on"]:
                led_id = payload.get("led_id")
                if isinstance(led_id, int) and 1 <= led_id <= 6:
                    cmd = send(f"LED{led_id}_ON")
                else:
                    cmd = send("ON")
            elif tokens == ["led", "off"]:
                led_id = payload.get("led_id")
                if isinstance(led_id, int) and 1 <= led_id <= 6:
                    cmd = send(f"LED{led_id}_OFF")
                else:
                    cmd = send("OFF")
            # /led{n}/on and /led{n}/off (n=1..6)
            elif len(tokens) == 2 and tokens[0].startswith("led") and tokens[0][3:].isdigit():
                n = int(tokens[0][3:])
                if 1 <= n <= 6 and tokens[1] in ("on", "off"):
                    cmd = send(f"LED{n}_" + ("ON" if tokens[1] == "on" else "OFF"))
                else:
                    status = 404
            else:
                status = 404

            if status == 200 and cmd:
                self._send_json(200, {"status": "ok", "command": cmd})
            else:
                self._send_json(404, {"status": "error", "error": "Unknown endpoint"})

        def log_message(self, format: str, *args):  # silence default logging
            return

    httpd = ThreadingHTTPServer((host, port), Handler)
    th = threading.Thread(target=httpd.serve_forever, daemon=True)
    th.start()
    print(f"HTTP server listening on http://{host}:{port}")
    return httpd

if __name__ == "__main__":
    ser = serial.Serial(PORT, BAUD, timeout=1)
    time.sleep(2)  # Wait for ESP32 reset
    print("Connected to", PORT)

    stop_event = threading.Event()
    t = threading.Thread(target=reader_loop, args=(ser, stop_event), daemon=True)
    t.start()

    write_lock = threading.Lock()
    httpd = start_http_server(ser, write_lock, host="127.0.0.1", port=5050)

    help_text = (
        "Commands: ON, OFF, BLINK, SERVO_UP, SERVO_DOWN, "
        "BUTTON_LIGHT_ON, BUTTON_LIGHT_OFF, CHIRP, "
        "LED1_ON..LED6_ON, LED1_OFF..LED6_OFF, Q to quit"
    )
    print(help_text)

    try:
        while True:
            cmd = input(
                "Enter command (type HELP to list, Q to quit): "
            ).strip().upper()
            if cmd in ("H", "HELP"):
                print(help_text)
                continue
            if cmd == "Q":
                break
            if not cmd:
                continue
            send_command(ser, cmd)
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        stop_event.set()
        time.sleep(0.1)
        if httpd:
            httpd.shutdown()
            time.sleep(0.1)
        ser.close()
        print("Disconnected.")
