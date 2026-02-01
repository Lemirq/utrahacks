/*
 * Servo Motor Test Firmware
 * Tower Pro Micro Servo 9g (SG90)
 * Pin: 13
 */

#include <Servo.h>

const int SERVO_PIN = 13;

Servo servo;

void setup() {
  Serial.begin(9600);
  servo.attach(SERVO_PIN);

  Serial.println("Servo Test Started");
  Serial.println("Commands: 0-180 (angle), s (sweep), c (center)");

  // Start at center position
  servo.write(90);
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input == "s") {
      // Sweep test
      Serial.println("Sweeping...");
      for (int angle = 0; angle <= 180; angle += 5) {
        servo.write(angle);
        delay(50);
      }
      for (int angle = 180; angle >= 0; angle -= 5) {
        servo.write(angle);
        delay(50);
      }
      Serial.println("Sweep complete");
    }
    else if (input == "c") {
      // Center position
      servo.write(90);
      Serial.println("Centered at 90");
    }
    else {
      // Try to parse as angle
      int angle = input.toInt();
      if (angle >= 0 && angle <= 180) {
        servo.write(angle);
        Serial.print("Set angle: ");
        Serial.println(angle);
      } else {
        Serial.println("Invalid input. Use 0-180, 's', or 'c'");
      }
    }
  }

  delay(10);
}
