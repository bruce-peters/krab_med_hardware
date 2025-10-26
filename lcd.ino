#include <LiquidCrystal.h>

// Define LCD pin connections
const int RS = 22;
const int EN = 23;
const int D4 = 26;
const int D5 = 25;
const int D6 = 33;
const int D7 = 32;

// Initialize the LCD object (RS, EN, D4, D5, D6, D7)
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

void setup() {
  // Initialize LCD in 16x2 mode
  lcd.begin(16, 2);

  // Display test messages
  lcd.setCursor(0, 0);
  lcd.print("LCD Test Running");

  lcd.setCursor(0, 1);
  lcd.print("Hello ESP32!");

  delay(2000); // Wait 2 seconds
  lcd.clear();

  // Display counting test
  for (int i = 0; i < 20; i++) {
    lcd.setCursor(0, 0);
    lcd.print("Counting: ");
    lcd.print(i);
    lcd.setCursor(0, 1);
    lcd.print("All pins OK!");
    delay(500);
  }

  lcd.clear();
  lcd.print("Done!");
}

void loop() {
  // Nothing — stays showing "Done!"
}
