/**
 * SENSOR & MOTOR TEST - WASD + SERVO CONTROLS
 *
 * MOTORS:
 *   W = Forward
 *   A = Left
 *   S = Backward
 *   D = Right
 *   X = Stop
 *
 * SERVO:
 *   Q = Rotate left (-15 deg)
 *   E = Rotate right (+15 deg)
 *   R = Center (90 deg)
 */

#include <Servo.h>

// --- Motor Pins ---
const int IN1 = 8;
const int IN2 = 7;
const int IN3 = 5;
const int IN4 = 4;
const int ENA = 9;
const int ENB = 3;

// --- Servo ---
const int SERVO_PIN = 13;
Servo servo;
int servoAngle = 90;

// --- Ultrasonic Sensor ---
const int TRIG = 11;
const int ECHO = 12;

// --- Color Sensor ---
const int S0 = A0;
const int S1 = A1;
const int S2 = A2;
const int S3 = A3;
const int COLOR_OUT = A4;

const int READ_TIMEOUT = 50000;
const int ULTRASONIC_TIMEOUT = 30000;

int loopCount = 0;
char currentDirection = 'X';

void setup() {
  Serial.begin(9600);

  // Motors
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  digitalWrite(ENA, HIGH);
  digitalWrite(ENB, HIGH);
  stopMotors();

  // Servo
  servo.attach(SERVO_PIN);
  servo.write(servoAngle);

  // Ultrasonic
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // Color sensor
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(COLOR_OUT, INPUT);
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  Serial.println("================================");
  Serial.println("  WASD + SERVO CONTROL");
  Serial.println("================================");
  Serial.println("W/A/S/D = Move   X = Stop");
  Serial.println("Q = Servo left   E = Servo right   R = Center");
  Serial.println();
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  currentDirection = 'X';
}

void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  currentDirection = 'W';
}

void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  currentDirection = 'S';
}

void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  currentDirection = 'A';
}

void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  currentDirection = 'D';
}

long getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long duration = pulseIn(ECHO, HIGH, ULTRASONIC_TIMEOUT);
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

int getRedValue() {
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  return pulseIn(COLOR_OUT, LOW, READ_TIMEOUT);
}

int getGreenValue() {
  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);
  return pulseIn(COLOR_OUT, LOW, READ_TIMEOUT);
}

int getBlueValue() {
  digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);
  return pulseIn(COLOR_OUT, LOW, READ_TIMEOUT);
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();

    switch (cmd) {
      // Motors
      case 'w': case 'W':
        moveForward();
        Serial.println(">>> FORWARD");
        break;
      case 'a': case 'A':
        turnLeft();
        Serial.println(">>> LEFT");
        break;
      case 's': case 'S':
        moveBackward();
        Serial.println(">>> BACKWARD");
        break;
      case 'd': case 'D':
        turnRight();
        Serial.println(">>> RIGHT");
        break;
      case 'x': case 'X': case ' ':
        stopMotors();
        Serial.println(">>> STOP");
        break;

      // Servo
      case 'q': case 'Q':
        servoAngle -= 15;
        if (servoAngle < 0) servoAngle = 0;
        servo.write(servoAngle);
        Serial.print(">>> SERVO: ");
        Serial.println(servoAngle);
        break;
      case 'e': case 'E':
        servoAngle += 15;
        if (servoAngle > 180) servoAngle = 180;
        servo.write(servoAngle);
        Serial.print(">>> SERVO: ");
        Serial.println(servoAngle);
        break;
      case 'r': case 'R':
        servoAngle = 90;
        servo.write(servoAngle);
        Serial.println(">>> SERVO: 90 (center)");
        break;
    }
  }

  loopCount++;

  // Print sensor readings every 10 loops
  if (loopCount % 10 == 0) {
    long dist = getDistance();
    int r = getRedValue();
    int g = getGreenValue();
    int b = getBlueValue();

    Serial.print("[");
    Serial.print(currentDirection);
    Serial.print("] Servo:");
    Serial.print(servoAngle);
    Serial.print("  Dist:");
    if (dist < 0) Serial.print("---");
    else Serial.print(dist);
    Serial.print("cm  R:");
    Serial.print(r);
    Serial.print(" G:");
    Serial.print(g);
    Serial.print(" B:");
    Serial.println(b);
  }

  delay(100);
}
