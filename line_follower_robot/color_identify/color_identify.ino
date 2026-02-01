/**
 * ============================================================================
 * COLOR IDENTIFIER - Real-time color detection
 * ============================================================================
 *
 * Displays the currently detected color using calibrated thresholds.
 * Open Serial Monitor at 9600 baud to see results.
 *
 * ============================================================================
 */

// Color Sensor Pins
const int S0 = A0;
const int S1 = A1;
const int S2 = A2;
const int S3 = A3;
const int COLOR_OUT = A4;

// --- CALIBRATED THRESHOLDS ---
// WHITE: R=15-19, G=16-20, B=5-6
const int WHITE_R_MAX = 25;
const int WHITE_G_MAX = 25;
const int WHITE_B_MAX = 10;

// BLACK: R=126-140, G=157-163, B=43-45
const int BLACK_R_MIN = 100;
const int BLACK_G_MIN = 140;
const int BLACK_B_MIN = 35;

// RED: R=26-35, G=107-121, B=18-22
const int RED_R_MIN = 20;
const int RED_R_MAX = 45;
const int RED_G_MIN = 90;

// GREEN: R=79-85, G=39-42, B=17-19
const int GREEN_R_MIN = 65;
const int GREEN_R_MAX = 100;
const int GREEN_G_MIN = 30;
const int GREEN_G_MAX = 55;

// BLUE: R=40-55, G=37-44, B=10-12
const int BLUE_R_MIN = 35;
const int BLUE_R_MAX = 65;
const int BLUE_G_MIN = 30;
const int BLUE_G_MAX = 55;
const int BLUE_B_MAX = 15;

const int READ_TIMEOUT = 50000;

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

  Serial.println("================================");
  Serial.println("    COLOR IDENTIFIER READY      ");
  Serial.println("================================");
  Serial.println();
  delay(1000);
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

String identifyColor(int r, int g, int b) {
  // WHITE: All values very low
  if (r < WHITE_R_MAX && g < WHITE_G_MAX && b < WHITE_B_MAX) {
    return "WHITE";
  }

  // BLACK: All values high
  if (r > BLACK_R_MIN && g > BLACK_G_MIN && b > BLACK_B_MIN) {
    return "BLACK";
  }

  // RED: R is low, G is very high
  if (r >= RED_R_MIN && r <= RED_R_MAX && g >= RED_G_MIN) {
    return "RED";
  }

  // GREEN: R is medium-high, G is medium
  if (r >= GREEN_R_MIN && r <= GREEN_R_MAX &&
      g >= GREEN_G_MIN && g <= GREEN_G_MAX) {
    return "GREEN";
  }

  // BLUE: R is medium, G is medium, B is very low
  if (r >= BLUE_R_MIN && r <= BLUE_R_MAX &&
      g >= BLUE_G_MIN && g <= BLUE_G_MAX &&
      b <= BLUE_B_MAX) {
    return "BLUE";
  }

  return "UNKNOWN";
}

void loop() {
  int r = getRedValue();
  int g = getGreenValue();
  int b = getBlueValue();

  String color = identifyColor(r, g, b);

  // Clear line and print
  Serial.print("Color: ");
  Serial.print(color);
  Serial.print("    (R:");
  Serial.print(r);
  Serial.print(" G:");
  Serial.print(g);
  Serial.print(" B:");
  Serial.print(b);
  Serial.println(")");

  delay(300);
}
