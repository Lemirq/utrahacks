// Motor A
const int ENA = 9;
const int IN1 = 8;
const int IN2 = 7;

// Motor B
const int ENB = 3;
const int IN3 = 5;
const int IN4 = 4;

void setup() {
  Serial.begin(9600);
  Serial.println("Starting motor control...");

  // Motor A
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  Serial.println("Motor A pins initialized (ENA=9, IN1=8, IN2=7)");

  // Motor B
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  Serial.println("Motor B pins initialized (ENB=3, IN3=5, IN4=4)");

  Serial.println("Setup complete!\n");
}

void loop() {
  // Both motors forward
  Serial.println(">>> FORWARD");
  Serial.println("Motor A: IN1=HIGH, IN2=LOW, ENA=255");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 255);

  Serial.println("Motor B: IN3=HIGH, IN4=LOW, ENB=255");
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, 255);
  Serial.println("Running forward for 3 seconds...\n");
  delay(3000);

  // Both motors stop
  Serial.println(">>> STOP");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  Serial.println("All direction pins LOW");
  Serial.println("Stopped for 1 second...\n");
  delay(1000);

  // Both motors reverse
  Serial.println(">>> REVERSE");
  Serial.println("Motor A: IN1=LOW, IN2=HIGH, ENA=255");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENA, 255);

  Serial.println("Motor B: IN3=LOW, IN4=HIGH, ENB=255");
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENB, 255);
  Serial.println("Running reverse for 3 seconds...\n");
  delay(3000);

  // Both motors stop
  Serial.println(">>> STOP");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  Serial.println("All direction pins LOW");
  Serial.println("Stopped for 1 second...\n");
  delay(1000);
}
