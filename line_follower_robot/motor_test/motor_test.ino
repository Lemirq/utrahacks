/**
 * SIMPLE MOTOR TEST
 *
 * Commands:
 *   f - Forward
 *   b - Backward
 *   l - Left
 *   r - Right
 *   s - Stop
 */

const int IN1 = 8;
const int IN2 = 7;
const int IN3 = 5;
const int IN4 = 4;

void setup() {
  Serial.begin(9600);
  Serial.println("================================================");
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  Serial.println("MOTOR TEST");
  Serial.println("f=forward  b=back  l=left  r=right  s=stop");
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();

    switch (cmd) {
      case 'f':
        Serial.println("FORWARD");
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
        break;

      case 'b':
        Serial.println("BACKWARD");
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
        break;

      case 'l':
        Serial.println("LEFT");
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
        break;

      case 'r':
        Serial.println("RIGHT");
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
        break;

      case 's':
        Serial.println("STOP");
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
        break;
    }
  }
}
