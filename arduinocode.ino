#include <ESP32Servo.h>
#include <LiquidCrystal.h>

// === Pin Assignments ===
const int LED1 = 4;
const int LED2 = 16;
const int LED3 = 17;
const int LED4 = 5;
const int LED5 = 18;
const int LED6 = 19;

const int BUTTONIN = 12;
const int BUTTONLIGHT = 14;

const int SERVO_PIN = 27;
const int SPEAKER_PIN = 13;

// Array of all LED pins for easier looping
const int leds[] = {LED1, LED2, LED3, LED4, LED5, LED6};
const int numLeds = 6;

// Servo
Servo myServo;

// Button debounce and state
int stableButtonState = HIGH; // using INPUT_PULLUP (HIGH idle)
int lastButtonReading = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelayMs = 50;

// Servo angles
const int SERVO_DOWN_ANGLE = 0;
const int SERVO_UP_ANGLE = 90;

// LCD pins (from lcd.ino)
const int RS = 22;
const int EN = 23;
const int D4 = 26;
const int D5 = 25;
const int D6 = 33;
const int D7 = 32;

// Initialize the LCD object (RS, EN, D4, D5, D6, D7)
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

void lcdPrintLine(int line, const String &text) {
  int row = line == 0 ? 0 : 1;
  String clipped = text;
  if (clipped.length() > 16) {
    clipped = clipped.substring(0, 16);
  }
  lcd.setCursor(0, row);
  // Clear the line by printing spaces
  lcd.print("                ");
  lcd.setCursor(0, row);
  lcd.print(clipped);
}

void setup() {
  Serial.begin(115200);

  // Setup LED pins
  for (int i = 0; i < numLeds; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
  }

  // Setup button LED
  pinMode(BUTTONLIGHT, OUTPUT);
  digitalWrite(BUTTONLIGHT, HIGH);

  // Setup button input (active LOW)
  pinMode(BUTTONIN, INPUT_PULLUP);

  // Setup servo
  myServo.attach(SERVO_PIN);
  myServo.write(SERVO_DOWN_ANGLE);

  // Setup speaker
  pinMode(SPEAKER_PIN, OUTPUT);

  // Setup LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcdPrintLine(0, "Sat, October 25");
  lcdPrintLine(1, "6:02 pm");

  Serial.println(
      "ESP32 ready. Commands: ON, OFF, BLINK, SERVO_UP, SERVO_DOWN, "
      "BUTTON_LIGHT_ON, BUTTON_LIGHT_OFF, CHIRP, LED1_ON..LED6_ON, "
      "LED1_OFF..LED6_OFF, LCD_CLEAR, LCD_PRINT1 <text>, LCD_PRINT2 <text>");
}

void setAllLeds(bool state) {
  for (int i = 0; i < numLeds; i++) {
    digitalWrite(leds[i], state ? HIGH : LOW);
  }
}

void blinkAllLeds(int times, int delayMs) {
  for (int t = 0; t < times; t++) {
    setAllLeds(HIGH);
    delay(delayMs);
    setAllLeds(LOW);
    delay(delayMs);
  }
}

void setLedIndexState(int zeroBasedIndex, bool state) {
  if (zeroBasedIndex < 0 || zeroBasedIndex >= numLeds)
    return;
  digitalWrite(leds[zeroBasedIndex], state ? HIGH : LOW);
}

void playChirp() {
  for (int freq = 1000; freq <= 2000; freq += 50) {
    tone(SPEAKER_PIN, freq);
    delay(5);
  }
  noTone(SPEAKER_PIN);
}

void loop() {
  // Debounced button read and edge detect (active LOW)
  int reading = digitalRead(BUTTONIN);
  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelayMs) {
    if (reading != stableButtonState) {
      stableButtonState = reading;
      if (stableButtonState == LOW) {
        Serial.println("BUTTON_PRESSED");
      }
    }
  }
  lastButtonReading = reading;

  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Remove extra spaces or newlines

    if (command.equalsIgnoreCase("ON")) {
      setAllLeds(true);
      Serial.println("LEDs ON");
    } else if (command.equalsIgnoreCase("OFF")) {
      setAllLeds(false);
      Serial.println("LEDs OFF");
    } else if (command.equalsIgnoreCase("BLINK")) {
      blinkAllLeds(5, 300);
      Serial.println("LEDs BLINKED");
    } else if (command.equalsIgnoreCase("SERVO_UP")) {
      myServo.write(SERVO_UP_ANGLE);
      Serial.println("SERVO UP");
    } else if (command.equalsIgnoreCase("SERVO_DOWN")) {
      myServo.write(SERVO_DOWN_ANGLE);
      Serial.println("SERVO DOWN");
    } else if (command.equalsIgnoreCase("BUTTON_LIGHT_ON")) {
      digitalWrite(BUTTONLIGHT, HIGH);
      Serial.println("BUTTON LIGHT ON");
    } else if (command.equalsIgnoreCase("BUTTON_LIGHT_OFF")) {
      digitalWrite(BUTTONLIGHT, LOW);
      Serial.println("BUTTON LIGHT OFF");
    } else if (command.equalsIgnoreCase("CHIRP")) {
      playChirp();
      Serial.println("CHIRP PLAYED");
    } else if (command.startsWith("LED")) {
      // Expected formats: LED1_ON .. LED6_ON, LED1_OFF .. LED6_OFF
      int indexCharPos = 3; // after "LED"
      if (command.length() >= 5) {
        int ledIndexOneBased =
            command.substring(indexCharPos, indexCharPos + 1).toInt();
        if (ledIndexOneBased >= 1 && ledIndexOneBased <= 6) {
          bool turnOn = command.endsWith("_ON");
          bool turnOff = command.endsWith("_OFF");
          if (turnOn || turnOff) {
            setLedIndexState(ledIndexOneBased - 1, turnOn);
            Serial.print("LED");
            Serial.print(ledIndexOneBased);
            Serial.println(turnOn ? " ON" : " OFF");
          } else {
            Serial.println(
                "Invalid LED command format. Use LEDn_ON or LEDn_OFF");
          }
        } else {
          Serial.println("LED index out of range (1-6)");
        }
      } else {
        Serial.println("Invalid LED command");
      }
    } else if (command.equalsIgnoreCase("LCD_CLEAR")) {
      lcd.clear();
      Serial.println("LCD CLEARED");
    } else if (command.startsWith("LCD_PRINT1 ")) {
      String text = command.substring(11);
      lcdPrintLine(0, text);
      Serial.println("LCD LINE1 UPDATED");
    } else if (command.startsWith("LCD_PRINT2 ")) {
      String text = command.substring(11);
      lcdPrintLine(1, text);
      Serial.println("LCD LINE2 UPDATED");
    } else {
      Serial.print("Unknown command: ");
      Serial.println(command);
    }
  }
}
