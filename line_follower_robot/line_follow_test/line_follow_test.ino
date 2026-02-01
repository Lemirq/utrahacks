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

// --- Ultrasonic Sensor ---
const int TRIG = 11;
const int ECHO = 12;
const int ULTRASONIC_TIMEOUT = 30000;

// --- IR Obstacle Sensors ---
// TODO: Update these pins to match your wiring
const int IR_LEFT = 2;   // Left-facing IR sensor
const int IR_RIGHT = 6;  // Right-facing IR sensor

// --- Obstacle Detection Thresholds ---
const int OBSTACLE_DIST_CM = 15;      // Stop if obstacle closer than this (ultrasonic)
const bool IR_ACTIVE_LOW = true;      // true if IR outputs LOW when obstacle detected

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

  // Ultrasonic sensor
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // IR obstacle sensors
  pinMode(IR_LEFT, INPUT);
  pinMode(IR_RIGHT, INPUT);

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

// Slow in-place turns using PWM for search sweeps
void turnLeftSlow(int speed) {
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnRightSlow(int speed) {
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void restoreFullSpeed() {
  digitalWrite(ENA, HIGH);
  digitalWrite(ENB, HIGH);
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

// ========== OBSTACLE DETECTION ==========

// Get distance from ultrasonic sensor in cm (-1 if no echo)
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

// Check if left IR sensor detects obstacle
bool obstacleLeft() {
  bool reading = digitalRead(IR_LEFT);
  return IR_ACTIVE_LOW ? !reading : reading;
}

// Check if right IR sensor detects obstacle
bool obstacleRight() {
  bool reading = digitalRead(IR_RIGHT);
  return IR_ACTIVE_LOW ? !reading : reading;
}

// Check if front ultrasonic detects obstacle
bool obstacleFront() {
  long dist = getDistance();
  if (dist < 0) return false;  // No reading = no obstacle
  return dist < OBSTACLE_DIST_CM;
}

// Check all obstacle sensors, returns: 0=clear, 1=left, 2=right, 3=both/front
int checkObstacles() {
  bool front = obstacleFront();
  bool left = obstacleLeft();
  bool right = obstacleRight();

  if (front) return 3;           // Front blocked
  if (left && right) return 3;   // Both sides blocked
  if (left) return 1;            // Left blocked
  if (right) return 2;           // Right blocked
  return 0;                      // Clear
}

// Handle obstacle - returns true if obstacle was handled
bool handleObstacle() {
  int obstacle = checkObstacles();

  if (obstacle == 0) return false;  // No obstacle

  stopMotors();

  Serial.print(F("[OBSTACLE] Detected: "));
  switch (obstacle) {
    case 1: Serial.println(F("LEFT")); break;
    case 2: Serial.println(F("RIGHT")); break;
    case 3: Serial.println(F("FRONT/BOTH")); break;
  }

  // Avoid obstacle
  switch (obstacle) {
    case 1:  // Left blocked - veer right
      turnRight();
      delay(200);
      break;
    case 2:  // Right blocked - veer left
      turnLeft();
      delay(200);
      break;
    case 3:  // Front blocked - back up and turn
      // Back up
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
      delay(300);
      stopMotors();
      // Turn right to find new path
      turnRight();
      delay(400);
      break;
  }

  stopMotors();
  return true;
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

// Check if color is valid (something we can follow)
bool isValidColor(Color c) {
  return c == BLACK || c == RED || c == GREEN || c == BLUE;
}

// Slow 90° sweep in one direction with continuous color checking
// Returns true if path color found (and stops immediately)
// goLeft: true = turn left, false = turn right
bool slowSweep90(bool goLeft, int durationMs, int checkIntervalMs, int speed) {
  unsigned long start = millis();
  const char* direction = goLeft ? "LEFT" : "RIGHT";

  Serial.print(F("[SWEEP] Starting "));
  Serial.print(direction);
  Serial.print(F(" 90deg (speed="));
  Serial.print(speed);
  Serial.println(F(")"));

  // Start slow in-place rotation
  if (goLeft) {
    turnLeftSlow(speed);
  } else {
    turnRightSlow(speed);
  }

  int checksPerformed = 0;

  // Continuously check while rotating
  while (millis() - start < (unsigned long)durationMs) {
    Color c = readColor();
    checksPerformed++;

    if (isValidColor(c)) {
      stopMotors();
      restoreFullSpeed();
      Serial.print(F("[SWEEP] "));
      Serial.print(direction);
      Serial.print(F(" found "));
      Serial.print(colorName(c));
      Serial.print(F(" after "));
      Serial.print(millis() - start);
      Serial.print(F("ms ("));
      Serial.print(checksPerformed);
      Serial.println(F(" checks)"));
      return true;
    }

    delay(checkIntervalMs);
  }

  // Completed without finding
  stopMotors();
  restoreFullSpeed();
  Serial.print(F("[SWEEP] "));
  Serial.print(direction);
  Serial.print(F(" complete - nothing found ("));
  Serial.print(checksPerformed);
  Serial.println(F(" checks)"));
  return false;
}

void findLine() {
  Serial.print(F("[SEARCH] Lost at RGB="));
  Serial.print(lastR);
  Serial.print(F(","));
  Serial.print(lastG);
  Serial.print(F(","));
  Serial.println(lastB);

  stopMotors();

  // === TUNABLE CONSTANTS ===
  const int SWEEP_DURATION_MS = 1200;  // Time for 90° rotation (adjust for your motors)
  const int CHECK_INTERVAL_MS = 10;    // Check color every 10ms while rotating
  const int SWEEP_SPEED = 80;          // PWM speed (0-255), lower = slower

  // Slow 90° LEFT sweep with continuous color checking
  if (slowSweep90(true, SWEEP_DURATION_MS, CHECK_INTERVAL_MS, SWEEP_SPEED)) {
    moveForward();
    delay(150);
    return;
  }

  // If left didn't find it, try right (180° from current position = 90° right of original)
  if (slowSweep90(false, SWEEP_DURATION_MS * 2, CHECK_INTERVAL_MS, SWEEP_SPEED)) {
    moveForward();
    delay(150);
    return;
  }

  // Fallback: return to roughly center and move forward
  Serial.println(F("[SEARCH] Nothing found - moving forward"));
  slowSweep90(true, SWEEP_DURATION_MS, CHECK_INTERVAL_MS, SWEEP_SPEED);  // Back to center-ish
  moveForward();
  delay(200);
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
  // Check for obstacles first - takes priority over line following
  if (handleObstacle()) {
    return;  // Obstacle was handled, skip line following this cycle
  }

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
      moveForward();
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
