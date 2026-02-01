// Motors go forward immediately on power-up

const int IN1 = 8;
const int IN2 = 7;
const int IN3 = 5;
const int IN4 = 4;
const int ENA = 9;
const int ENB = 3;

void setup() {
  Serial.begin(9600);
  Serial.println("MOTOR FORWARD TEST");

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  // ENABLE MOTORS
  digitalWrite(ENA, HIGH);
  digitalWrite(ENB, HIGH);

  // GO FORWARD
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  Serial.println("Motors should be running!");
}

void loop() {
  // runs forever
}
