/**
 * LINE FOLLOWER - COLOR ONLY
 *
 * Follows black line using color sensor.
 * At forks: RED > GREEN, same color = take RIGHT
 *
 * Uses confidence-based color detection (2/3 channels = valid match)
 */

// --- Motor Pins ---
const int IN1 = 8;
const int IN2 = 7;
const int IN3 = 5;
const int IN4 = 4;
const int ENA = 9;
const int ENB = 3;

// --- Color Sensor ---
const int S0 = A0;
const int S1 = A1;
const int S2 = A2;
const int S3 = A3;
const int COLOR_OUT = A4;

const int READ_TIMEOUT = 50000;

// --- STATISTICAL THRESHOLDS (mean ± 3σ from calibration data) ---
// Each color has: mean and range for R, G, B

// WHITE: R=15.4±0.5, G=17.3±0.5, B=14.6±0.5
const int WHITE_R_MIN = 12, WHITE_R_MAX = 18;
const int WHITE_G_MIN = 14, WHITE_G_MAX = 20;
const int WHITE_B_MIN = 12, WHITE_B_MAX = 18;

// BLACK: R=122±2.8, G=134.7±3.0, B=108.6±2.5
const int BLACK_R_MIN = 110, BLACK_R_MAX = 135;
const int BLACK_G_MIN = 122, BLACK_G_MAX = 148;
const int BLACK_B_MIN = 98, BLACK_B_MAX = 120;

// RED: R=21.5±2.2, G=90.2±9.3, B=65.7±6.7
const int RED_R_MIN = 12, RED_R_MAX = 32;
const int RED_G_MIN = 55, RED_G_MAX = 125;
const int RED_B_MIN = 40, RED_B_MAX = 92;

// GREEN: R=77.6±5.9, G=40.9±1.5, B=61.6±3.6
const int GREEN_R_MIN = 55, GREEN_R_MAX = 100;
const int GREEN_G_MIN = 34, GREEN_G_MAX = 50;
const int GREEN_B_MIN = 48, GREEN_B_MAX = 75;

// BLUE: R=113.5±5.0, G=78.1±2.7, B=40.7±1.2
const int BLUE_R_MIN = 95, BLUE_R_MAX = 132;
const int BLUE_G_MIN = 65, BLUE_G_MAX = 90;
const int BLUE_B_MIN = 35, BLUE_B_MAX = 48;

// --- Timing ---
const int TURN_90_DELAY = 500;
const int MOVE_DELAY = 100;

// --- Colors ---
enum Color { BLACK, RED, GREEN, WHITE, BLUE, UNKNOWN };

// --- State ---
int lastR = 0, lastG = 0, lastB = 0;
Color lastColor = UNKNOWN;
int searchForwardMs = 150;  // How far to go forward after sweep (increases over time)

// --- Color History ---
const int HISTORY_SIZE = 8;          // Number of readings to keep
Color colorHistory[HISTORY_SIZE];     // Circular buffer
int historyIndex = 0;                 // Current position in buffer
unsigned long lastHistoryTime = 0;    // Last time we recorded
const int HISTORY_INTERVAL = 40;      // Record every 40ms

// How many bad readings in a row before we sweep
const int BAD_THRESHOLD = 4;

// ========== SETUP ==========

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

  // Color sensor
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(COLOR_OUT, INPUT);
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  // Initialize history to UNKNOWN
  for (int i = 0; i < HISTORY_SIZE; i++) {
    colorHistory[i] = UNKNOWN;
  }

  Serial.println(F("=== LINE FOLLOWER ==="));
  Serial.println(F("Confidence + history smoothing"));
  Serial.println(F("Starting in 2s..."));
  delay(2000);
  Serial.println(F("GO!"));
}

// ========== MOTORS ==========

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnLeft90() {
  turnLeft();
  delay(TURN_90_DELAY);
  stopMotors();
}

void turnRight90() {
  turnRight();
  delay(TURN_90_DELAY);
  stopMotors();
}

// ========== COLOR SENSOR ==========

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

// Check if value is in range
bool inRange(int val, int minVal, int maxVal) {
  return val >= minVal && val <= maxVal;
}

// Calculate confidence for a color (0-3 channels matching)
int getConfidence(int r, int g, int b,
                  int rMin, int rMax,
                  int gMin, int gMax,
                  int bMin, int bMax) {
  int confidence = 0;
  if (inRange(r, rMin, rMax)) confidence++;
  if (inRange(g, gMin, gMax)) confidence++;
  if (inRange(b, bMin, bMax)) confidence++;
  return confidence;
}

const char* colorName(Color c) {
  switch(c) {
    case BLACK: return "BLACK";
    case RED: return "RED";
    case GREEN: return "GREEN";
    case WHITE: return "WHITE";
    case BLUE: return "BLUE";
    default: return "???";
  }
}

Color readColor() {
  int r = getRedValue();
  int g = getGreenValue();
  int b = getBlueValue();

  lastR = r;
  lastG = g;
  lastB = b;

  // Calculate confidence for each color
  int confWhite = getConfidence(r, g, b, WHITE_R_MIN, WHITE_R_MAX, WHITE_G_MIN, WHITE_G_MAX, WHITE_B_MIN, WHITE_B_MAX);
  int confBlack = getConfidence(r, g, b, BLACK_R_MIN, BLACK_R_MAX, BLACK_G_MIN, BLACK_G_MAX, BLACK_B_MIN, BLACK_B_MAX);
  int confRed = getConfidence(r, g, b, RED_R_MIN, RED_R_MAX, RED_G_MIN, RED_G_MAX, RED_B_MIN, RED_B_MAX);
  int confGreen = getConfidence(r, g, b, GREEN_R_MIN, GREEN_R_MAX, GREEN_G_MIN, GREEN_G_MAX, GREEN_B_MIN, GREEN_B_MAX);
  int confBlue = getConfidence(r, g, b, BLUE_R_MIN, BLUE_R_MAX, BLUE_G_MIN, BLUE_G_MAX, BLUE_B_MIN, BLUE_B_MAX);

  // Find best match (need at least 2/3 confidence)
  int bestConf = 1;  // Minimum threshold: 2 channels must match
  Color bestColor = UNKNOWN;

  if (confWhite > bestConf) { bestConf = confWhite; bestColor = WHITE; }
  if (confBlack > bestConf) { bestConf = confBlack; bestColor = BLACK; }
  if (confRed > bestConf) { bestConf = confRed; bestColor = RED; }
  if (confGreen > bestConf) { bestConf = confGreen; bestColor = GREEN; }
  if (confBlue > bestConf) { bestConf = confBlue; bestColor = BLUE; }

  // Handle ties: prefer BLACK for line following
  if (bestColor == UNKNOWN) {
    if (confBlack == 2) bestColor = BLACK;
    else if (confWhite == 2) bestColor = WHITE;
    else if (confRed == 2) bestColor = RED;
    else if (confGreen == 2) bestColor = GREEN;
    else if (confBlue == 2) bestColor = BLUE;
  }

  return bestColor;
}

// Read color with debug logging
Color readColorDebug(const char* context) {
  int r = getRedValue();
  int g = getGreenValue();
  int b = getBlueValue();

  lastR = r;
  lastG = g;
  lastB = b;

  int confWhite = getConfidence(r, g, b, WHITE_R_MIN, WHITE_R_MAX, WHITE_G_MIN, WHITE_G_MAX, WHITE_B_MIN, WHITE_B_MAX);
  int confBlack = getConfidence(r, g, b, BLACK_R_MIN, BLACK_R_MAX, BLACK_G_MIN, BLACK_G_MAX, BLACK_B_MIN, BLACK_B_MAX);
  int confRed = getConfidence(r, g, b, RED_R_MIN, RED_R_MAX, RED_G_MIN, RED_G_MAX, RED_B_MIN, RED_B_MAX);
  int confGreen = getConfidence(r, g, b, GREEN_R_MIN, GREEN_R_MAX, GREEN_G_MIN, GREEN_G_MAX, GREEN_B_MIN, GREEN_B_MAX);
  int confBlue = getConfidence(r, g, b, BLUE_R_MIN, BLUE_R_MAX, BLUE_G_MIN, BLUE_G_MAX, BLUE_B_MIN, BLUE_B_MAX);

  int bestConf = 1;
  Color bestColor = UNKNOWN;

  if (confWhite > bestConf) { bestConf = confWhite; bestColor = WHITE; }
  if (confBlack > bestConf) { bestConf = confBlack; bestColor = BLACK; }
  if (confRed > bestConf) { bestConf = confRed; bestColor = RED; }
  if (confGreen > bestConf) { bestConf = confGreen; bestColor = GREEN; }
  if (confBlue > bestConf) { bestConf = confBlue; bestColor = BLUE; }

  if (bestColor == UNKNOWN) {
    if (confBlack == 2) bestColor = BLACK;
    else if (confWhite == 2) bestColor = WHITE;
    else if (confRed == 2) bestColor = RED;
    else if (confGreen == 2) bestColor = GREEN;
    else if (confBlue == 2) bestColor = BLUE;
  }

  // Log detection with confidence
  Serial.print(F("["));
  Serial.print(context);
  Serial.print(F("] RGB="));
  Serial.print(r);
  Serial.print(F(","));
  Serial.print(g);
  Serial.print(F(","));
  Serial.print(b);
  Serial.print(F(" -> "));
  Serial.print(colorName(bestColor));
  Serial.print(F(" (W:"));
  Serial.print(confWhite);
  Serial.print(F(" K:"));
  Serial.print(confBlack);
  Serial.print(F(" R:"));
  Serial.print(confRed);
  Serial.print(F(" G:"));
  Serial.print(confGreen);
  Serial.print(F(" B:"));
  Serial.print(confBlue);
  Serial.println(F(")"));

  return bestColor;
}

// ========== COLOR HISTORY ==========

void addToHistory(Color c) {
  colorHistory[historyIndex] = c;
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;
}

// Count how many of the last N readings are "bad" (not BLACK/RED/GREEN/BLUE)
int countRecentBad(int lastN) {
  int badCount = 0;
  for (int i = 0; i < lastN && i < HISTORY_SIZE; i++) {
    int idx = (historyIndex - 1 - i + HISTORY_SIZE) % HISTORY_SIZE;
    Color c = colorHistory[idx];
    if (c == WHITE || c == UNKNOWN) {
      badCount++;
    }
  }
  return badCount;
}

// Check if we've been on track recently (any BLACK in last N)
bool wasOnTrackRecently(int lastN) {
  for (int i = 0; i < lastN && i < HISTORY_SIZE; i++) {
    int idx = (historyIndex - 1 - i + HISTORY_SIZE) % HISTORY_SIZE;
    if (colorHistory[idx] == BLACK) {
      return true;
    }
  }
  return false;
}

// Should we trigger a sweep? Only if consistently lost
bool shouldSweep() {
  int recentBad = countRecentBad(BAD_THRESHOLD);
  return recentBad >= BAD_THRESHOLD;
}

// ========== LINE FOLLOWING ==========

void findLine() {
  Serial.print(F("[SEARCH] Lost at RGB="));
  Serial.print(lastR);
  Serial.print(F(","));
  Serial.print(lastG);
  Serial.print(F(","));
  Serial.println(lastB);

  stopMotors();

  // Try right
  for (int i = 0; i < 5; i++) {
    turnRight();
    delay(80);
    stopMotors();
    if (readColor() == BLACK) {
      Serial.println(F("[SEARCH] Found RIGHT - advancing"));
      // Found it! Move forward optimistically
      moveForward();
      delay(250);
      searchForwardMs = 150;  // Reset since we found it
      return;
    }
  }

  // Try left (go back past center)
  for (int i = 0; i < 10; i++) {
    turnLeft();
    delay(80);
    stopMotors();
    if (readColor() == BLACK) {
      Serial.println(F("[SEARCH] Found LEFT - advancing"));
      // Found it! Move forward optimistically
      moveForward();
      delay(250);
      searchForwardMs = 150;  // Reset since we found it
      return;
    }
  }

  // Return to center
  for (int i = 0; i < 5; i++) {
    turnRight();
    delay(80);
  }
  stopMotors();

  // Didn't find it - move forward with increasing distance
  Serial.print(F("[SEARCH] Not found, advancing "));
  Serial.print(searchForwardMs);
  Serial.println(F("ms"));

  moveForward();
  delay(searchForwardMs);
  stopMotors();

  // Increase forward distance for next sweep (up to a max)
  if (searchForwardMs < 500) {
    searchForwardMs += 50;
  }
}

void handleFork() {
  Serial.print(F("[FORK] Triggered at RGB="));
  Serial.print(lastR);
  Serial.print(F(","));
  Serial.print(lastG);
  Serial.print(F(","));
  Serial.println(lastB);

  stopMotors();
  delay(100);

  // Check right
  turnRight90();
  Color rightColor = readColorDebug("FORK-R");

  // Back to center
  turnLeft90();

  // Check left
  turnLeft90();
  Color leftColor = readColorDebug("FORK-L");

  // Back to center
  turnRight90();

  // Decision: RED > GREEN, same = RIGHT
  Serial.print(F("[FORK] L="));
  Serial.print(colorName(leftColor));
  Serial.print(F(" R="));
  Serial.print(colorName(rightColor));

  if (leftColor == rightColor) {
    Serial.println(F(" -> Same, take RIGHT"));
    turnRight90();
  } else if (leftColor == RED && rightColor != RED) {
    Serial.println(F(" -> RED on left, take LEFT"));
    turnLeft90();
  } else if (rightColor == RED && leftColor != RED) {
    Serial.println(F(" -> RED on right, take RIGHT"));
    turnRight90();
  } else {
    Serial.println(F(" -> Default RIGHT"));
    turnRight90();
  }

  moveForward();
  delay(200);
}

// ========== MAIN LOOP ==========

void loop() {
  Color current = readColor();

  // Add to history periodically
  unsigned long now = millis();
  if (now - lastHistoryTime >= HISTORY_INTERVAL) {
    addToHistory(current);
    lastHistoryTime = now;
  }

  // Only log on color change
  if (current != lastColor) {
    Serial.print(F("[NAV] "));
    Serial.print(colorName(lastColor));
    Serial.print(F(" -> "));
    Serial.print(colorName(current));
    Serial.print(F(" RGB="));
    Serial.print(lastR);
    Serial.print(F(","));
    Serial.print(lastG);
    Serial.print(F(","));
    Serial.println(lastB);
    lastColor = current;
  }

  switch (current) {
    case BLACK:
      moveForward();
      searchForwardMs = 150;  // Reset search distance when on track
      break;

    case RED:
    case GREEN:
      handleFork();
      break;

    case BLUE:
      Serial.println(F("[NAV] BLUE - stopping"));
      stopMotors();
      delay(1000);
      break;

    case WHITE:
    case UNKNOWN:
    default:
      // Don't immediately sweep - check if consistently lost
      if (shouldSweep()) {
        Serial.print(F("[NAV] Lost ("));
        Serial.print(countRecentBad(HISTORY_SIZE));
        Serial.print(F("/"));
        Serial.print(HISTORY_SIZE);
        Serial.print(F(" bad): RGB="));
        Serial.print(lastR);
        Serial.print(F(","));
        Serial.print(lastG);
        Serial.print(F(","));
        Serial.println(lastB);
        findLine();
      } else {
        // Probably just noise - keep going, we were on track recently
        moveForward();
      }
      break;
  }

  delay(50);
}
