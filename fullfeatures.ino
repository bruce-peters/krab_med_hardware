#include <ESP32Servo.h>

// Pin assignments
const int BUTTONIN = 12;                   // Button input
const int BUTTONLIGHT = 14;                // Button LED
const int LEDS[] = {4, 16, 17, 5, 18, 19}; // 6 LEDs
const int NUM_LEDS = 6;
const int SERVO_PIN = 27;
const int SPEAKER_PIN = 13;

// Servo object
Servo myServo;

// State variable
bool buttonPressed = false;

void setup() {
  // Initialize button and button LED
  pinMode(BUTTONIN, INPUT_PULLUP); // Active LOW
  pinMode(BUTTONLIGHT, OUTPUT);
  digitalWrite(BUTTONLIGHT, HIGH); // LED on initially

  // Initialize 6 LEDs
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LEDS[i], OUTPUT);
    digitalWrite(LEDS[i], LOW); // start OFF
  }

  // Initialize servo
  myServo.attach(SERVO_PIN);
  myServo.write(0); // Servo starts down

  // Initialize speaker
  pinMode(SPEAKER_PIN, OUTPUT);
}

void loop() {
  // Check button (active LOW)
  if (!buttonPressed && digitalRead(BUTTONIN) == LOW) {
    buttonPressed = true; // Only trigger once

    // Turn off button light
    digitalWrite(BUTTONLIGHT, LOW);

    // Turn on 6 LEDs
    for (int i = 0; i < NUM_LEDS; i++) {
      digitalWrite(LEDS[i], HIGH);
    }

    for (int freq = 1000; freq <= 2000; freq += 50) {
      tone(SPEAKER_PIN, freq);
      delay(5);
    }
    noTone(SPEAKER_PIN);

    delay(500);
    // Move servo up (180 degrees)
    myServo.write(90);

    // Play a simple chirp on speaker
  }
}
