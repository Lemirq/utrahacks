/**
 * ============================================================================
 * COLOR SENSOR CALIBRATION - CSV OUTPUT
 * ============================================================================
 *
 * Outputs CSV format for logging to file.
 * Use the Python script (log_colors.py) to capture data.
 *
 * ============================================================================
 */

// Color Sensor Pins
const int S0 = A0;
const int S1 = A1;
const int S2 = A2;
const int S3 = A3;
const int COLOR_OUT = A4;

const int READ_TIMEOUT = 50000;
const int READ_INTERVAL = 200;  // Read every 200ms

unsigned long readingCount = 0;

void setup() {
  Serial.begin(9600);

  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(COLOR_OUT, INPUT);

  // Set frequency scaling to 20%
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  delay(1000);

  // CSV Header
  Serial.println("reading,red,green,blue,timestamp");
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
  int red = getRedValue();
  int green = getGreenValue();
  int blue = getBlueValue();

  readingCount++;

  // Output CSV: reading,red,green,blue,timestamp
  Serial.print(readingCount);
  Serial.print(",");
  Serial.print(red);
  Serial.print(",");
  Serial.print(green);
  Serial.print(",");
  Serial.print(blue);
  Serial.print(",");
  Serial.println(millis());

  delay(READ_INTERVAL);
}
