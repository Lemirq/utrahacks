/**
 * ============================================================================
 * LINE FOLLOWING ROBOT WITH OBSTACLE AVOIDANCE
 * ============================================================================
 *
 * DESCRIPTION:
 * A two-wheeled robot that follows a black line and navigates colored paths.
 * The robot uses a color sensor to detect line colors and an ultrasonic
 * sensor for obstacle detection on the red (obstacle course) path.
 *
 * HARDWARE:
 * - Arduino UNO R4 WiFi (ABX00080)
 * - L298N Motor Driver
 * - 2x JGA25-370 DC Motors (12V, 50RPM)
 * - V575 Color Sensor (TCS3200 compatible)
 * - HY-SRF05 Ultrasonic Sensor
 * - 9V Battery (or 12V power supply)
 *
 * NAVIGATION RULES:
 * 1. Always follow the BLACK line
 * 2. At a fork with DIFFERENT colors: RED has priority over GREEN
 * 3. At a fork with SAME colors: RIGHT has priority
 * 4. RED path = Obstacle course (use ultrasonic to avoid obstacles)
 * 5. GREEN path = Clear path (just follow)
 *
 * OBSTACLE AVOIDANCE STRATEGY (for RED path):
 * 1. Detect obstacle with ultrasonic sensor
 * 2. Turn right 90 degrees
 * 3. Move forward
 * 4. Turn left 90 degrees
 * 5. Check for obstacles again (recursive)
 * 6. When clear: forward, left, forward, right, continue on path
 *
 * AUTHOR: UltraHacks Team
 * DATE: January 2026
 * ============================================================================
 */

// ============================================================================
// PIN DEFINITIONS
// ============================================================================

// --- Motor A (Left Motor) ---
const int IN1 = 8;   // Motor A direction pin 1
const int IN2 = 7;   // Motor A direction pin 2

// --- Motor B (Right Motor) ---
const int IN3 = 5;   // Motor B direction pin 1
const int IN4 = 4;   // Motor B direction pin 2

// --- Ultrasonic Sensor (HY-SRF05) ---
const int TRIG = 11;  // Trigger pin
const int ECHO = 12;  // Echo pin

// --- Color Sensor (V575 / TCS3200) ---
const int S0 = A0;         // Frequency scaling pin 0
const int S1 = A1;         // Frequency scaling pin 1
const int S2 = A2;         // Color filter selection pin 0
const int S3 = A3;         // Color filter selection pin 1
const int COLOR_OUT = A4;  // Frequency output pin

// ============================================================================
// CONFIGURATION CONSTANTS
// ============================================================================

// --- Motor Timing (adjust based on your robot) ---
const int TURN_90_DELAY = 500;      // Time in ms to turn 90 degrees
const int MOVE_UNIT_DELAY = 300;    // Time in ms to move one "unit" forward
const int MOTOR_SPEED_DELAY = 10;   // Small delay for motor stability

// --- Ultrasonic Sensor ---
const int OBSTACLE_THRESHOLD = 15;  // Distance in cm to consider as obstacle
const int ULTRASONIC_TIMEOUT = 30000;  // Timeout in microseconds

// --- Color Sensor Thresholds (CALIBRATED) ---
// Based on calibration data collected on 2026-01-31
// Lower values = stronger color detection (more light reflected)
//
// Calibration Results:
// WHITE: R=15-19, G=16-20, B=5-6   (all low - high reflectance)
// RED:   R=26-35, G=107-121, B=18-22 (R lowest, G very high)
// GREEN: R=79-85, G=39-42, B=17-19 (G lower than R)
// BLUE:  R=40-55, G=37-44, B=10-12 (B lowest)
// BLACK: R=126-140, G=157-163, B=43-45 (all high - low reflectance)

// WHITE detection: all values very low
const int WHITE_R_MAX = 25;
const int WHITE_G_MAX = 25;
const int WHITE_B_MAX = 10;

// BLACK detection: all values high
const int BLACK_R_MIN = 100;
const int BLACK_G_MIN = 140;
const int BLACK_B_MIN = 35;

// RED detection: R is low (26-35), G is very high (107-121)
const int RED_R_MIN = 20;
const int RED_R_MAX = 45;
const int RED_G_MIN = 90;  // G must be high for red

// GREEN detection: R=79-85, G=39-42
const int GREEN_R_MIN = 65;
const int GREEN_R_MAX = 100;
const int GREEN_G_MIN = 30;
const int GREEN_G_MAX = 55;

// BLUE detection: R=40-55, G=37-44, B=10-12
const int BLUE_R_MIN = 35;
const int BLUE_R_MAX = 65;
const int BLUE_G_MIN = 30;
const int BLUE_G_MAX = 55;
const int BLUE_B_MAX = 15;

// --- Color Sensor Timing ---
const int COLOR_READ_TIMEOUT = 50000;  // Timeout for pulseIn

// ============================================================================
// ENUMS FOR READABILITY
// ============================================================================

/**
 * Detected colors from the color sensor
 */
enum Color {
  COLOR_BLACK,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_WHITE,
  COLOR_BLUE,
  COLOR_UNKNOWN
};

/**
 * Robot movement directions
 */
enum Direction {
  DIR_FORWARD,
  DIR_BACKWARD,
  DIR_LEFT,
  DIR_RIGHT,
  DIR_STOP
};

// ============================================================================
// GLOBAL STATE VARIABLES
// ============================================================================

bool onRedPath = false;  // Track if we're on the obstacle course path

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  // Initialize serial for debugging
  Serial.begin(9600);
  Serial.println("=================================");
  Serial.println("LINE FOLLOWER ROBOT INITIALIZING");
  Serial.println("=================================");

  // --- Initialize Motor Pins ---
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  stopMotors();
  Serial.println("[OK] Motors initialized (IN1=8, IN2=7, IN3=5, IN4=4)");

  // --- Initialize Ultrasonic Sensor ---
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  Serial.println("[OK] Ultrasonic sensor initialized (TRIG=11, ECHO=12)");

  // --- Initialize Color Sensor ---
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(COLOR_OUT, INPUT);

  // Set frequency scaling to 20% (S0=HIGH, S1=LOW)
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);
  Serial.println("[OK] Color sensor initialized (S0=A0, S1=A1, S2=A2, S3=A3, OUT=A4)");

  Serial.println("=================================");
  Serial.println("INITIALIZATION COMPLETE");
  Serial.println("Starting in 2 seconds...");
  Serial.println("=================================\n");

  delay(2000);
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Read the current color under the sensor
  Color currentColor = readColor();

  // Log current state
  logColor(currentColor);

  // --- Main Navigation Logic ---
  switch (currentColor) {

    case COLOR_BLACK:
      // Follow the black line - go straight
      moveForward();
      break;

    case COLOR_RED:
      // Red path detected - enter obstacle course mode
      Serial.println("[NAV] RED path detected - Obstacle course mode");
      onRedPath = true;
      handleRedPath();
      break;

    case COLOR_GREEN:
      // Green path detected - clear path, just follow
      Serial.println("[NAV] GREEN path detected - Clear path mode");
      onRedPath = false;
      moveForward();
      break;

    case COLOR_BLUE:
      // Blue detected - additional functionality (customize as needed)
      Serial.println("[NAV] BLUE detected - Custom action");
      // TODO: Add your blue path behavior here
      moveForward();
      break;

    case COLOR_WHITE:
      // Lost the line - need to find it again
      Serial.println("[NAV] WHITE detected - Searching for line...");
      findLine();
      break;

    case COLOR_UNKNOWN:
    default:
      // Unknown color - stop and reassess
      Serial.println("[NAV] UNKNOWN color - Stopping to reassess");
      stopMotors();
      delay(100);
      break;
  }

  delay(50);  // Small delay for stability
}

// ============================================================================
// MOTOR CONTROL FUNCTIONS
// ============================================================================

/**
 * Move both motors forward
 */
void moveForward() {
  Serial.println("[MOTOR] Moving FORWARD");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

/**
 * Move both motors backward
 */
void moveBackward() {
  Serial.println("[MOTOR] Moving BACKWARD");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

/**
 * Turn left (left motor backward, right motor forward)
 */
void turnLeft() {
  Serial.println("[MOTOR] Turning LEFT");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

/**
 * Turn right (left motor forward, right motor backward)
 */
void turnRight() {
  Serial.println("[MOTOR] Turning RIGHT");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

/**
 * Stop both motors
 */
void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

/**
 * Turn right 90 degrees
 */
void turnRight90() {
  Serial.println("[MOTOR] Turning RIGHT 90 degrees");
  turnRight();
  delay(TURN_90_DELAY);
  stopMotors();
  delay(MOTOR_SPEED_DELAY);
}

/**
 * Turn left 90 degrees
 */
void turnLeft90() {
  Serial.println("[MOTOR] Turning LEFT 90 degrees");
  turnLeft();
  delay(TURN_90_DELAY);
  stopMotors();
  delay(MOTOR_SPEED_DELAY);
}

/**
 * Move forward one unit distance
 */
void moveForwardUnit() {
  Serial.println("[MOTOR] Moving forward one unit");
  moveForward();
  delay(MOVE_UNIT_DELAY);
  stopMotors();
  delay(MOTOR_SPEED_DELAY);
}

// ============================================================================
// ULTRASONIC SENSOR FUNCTIONS
// ============================================================================

/**
 * Get distance from ultrasonic sensor in centimeters
 *
 * @return Distance in cm, or 999 if no echo received
 */
long getDistance() {
  // Send trigger pulse
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  // Read echo pulse duration with timeout
  long duration = pulseIn(ECHO, HIGH, ULTRASONIC_TIMEOUT);

  // Calculate distance (speed of sound = 0.034 cm/us)
  if (duration == 0) {
    return 999;  // No echo received
  }

  long distance = duration * 0.034 / 2;

  Serial.print("[ULTRASONIC] Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  return distance;
}

/**
 * Check if there's an obstacle in front
 *
 * @return true if obstacle detected within threshold
 */
bool isObstacleDetected() {
  long distance = getDistance();
  return (distance > 0 && distance < OBSTACLE_THRESHOLD);
}

// ============================================================================
// COLOR SENSOR FUNCTIONS
// ============================================================================

/**
 * Read the red component from color sensor
 * Lower value = more red detected
 */
int getRedValue() {
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  return pulseIn(COLOR_OUT, LOW, COLOR_READ_TIMEOUT);
}

/**
 * Read the green component from color sensor
 * Lower value = more green detected
 */
int getGreenValue() {
  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);
  return pulseIn(COLOR_OUT, LOW, COLOR_READ_TIMEOUT);
}

/**
 * Read the blue component from color sensor
 * Lower value = more blue detected
 */
int getBlueValue() {
  digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);
  return pulseIn(COLOR_OUT, LOW, COLOR_READ_TIMEOUT);
}

/**
 * Read and classify the current color
 * Uses calibrated thresholds from sensor testing
 *
 * @return Color enum value
 */
Color readColor() {
  int r = getRedValue();
  int g = getGreenValue();
  int b = getBlueValue();

  Serial.print("[COLOR] R:");
  Serial.print(r);
  Serial.print(" G:");
  Serial.print(g);
  Serial.print(" B:");
  Serial.println(b);

  // --- WHITE: All values very low (high reflectance) ---
  // Calibrated: R=15-19, G=16-20, B=5-6
  if (r < WHITE_R_MAX && g < WHITE_G_MAX && b < WHITE_B_MAX) {
    return COLOR_WHITE;
  }

  // --- BLACK: All values high (low reflectance) ---
  // Calibrated: R=126-140, G=157-163, B=43-45
  if (r > BLACK_R_MIN && g > BLACK_G_MIN && b > BLACK_B_MIN) {
    return COLOR_BLACK;
  }

  // --- RED: R is low, G is very high ---
  // Calibrated: R=26-35, G=107-121, B=18-22
  // Key identifier: G is much higher than R
  if (r >= RED_R_MIN && r <= RED_R_MAX && g >= RED_G_MIN) {
    return COLOR_RED;
  }

  // --- GREEN: R is medium-high, G is medium ---
  // Calibrated: R=79-85, G=39-42, B=17-19
  // Key identifier: R > 65 and G is between 30-55
  if (r >= GREEN_R_MIN && r <= GREEN_R_MAX &&
      g >= GREEN_G_MIN && g <= GREEN_G_MAX) {
    return COLOR_GREEN;
  }

  // --- BLUE: R is medium, G is medium, B is very low ---
  // Calibrated: R=40-55, G=37-44, B=10-12
  // Key identifier: B < 15 and R is between 35-65
  if (r >= BLUE_R_MIN && r <= BLUE_R_MAX &&
      g >= BLUE_G_MIN && g <= BLUE_G_MAX &&
      b <= BLUE_B_MAX) {
    return COLOR_BLUE;
  }

  return COLOR_UNKNOWN;
}

/**
 * Log the detected color to serial
 */
void logColor(Color color) {
  Serial.print("[DETECTED] ");
  switch (color) {
    case COLOR_BLACK:  Serial.println("BLACK"); break;
    case COLOR_RED:    Serial.println("RED"); break;
    case COLOR_GREEN:  Serial.println("GREEN"); break;
    case COLOR_WHITE:  Serial.println("WHITE"); break;
    case COLOR_BLUE:   Serial.println("BLUE"); break;
    default:           Serial.println("UNKNOWN"); break;
  }
}

// ============================================================================
// NAVIGATION FUNCTIONS
// ============================================================================

/**
 * Handle navigation on the red (obstacle course) path
 * Uses ultrasonic sensor to detect and avoid obstacles
 */
void handleRedPath() {
  moveForward();

  while (onRedPath) {
    // Check for obstacles
    if (isObstacleDetected()) {
      Serial.println("[OBSTACLE] Obstacle detected! Initiating avoidance...");
      avoidObstacle();
    }

    // Check current color to see if we're still on red path
    Color currentColor = readColor();

    if (currentColor == COLOR_BLACK) {
      // Back on main line, continue forward
      moveForward();
    } else if (currentColor == COLOR_GREEN) {
      // Switched to green path
      Serial.println("[NAV] Exiting red path, entering green path");
      onRedPath = false;
      break;
    } else if (currentColor == COLOR_WHITE) {
      // Lost the line
      findLine();
    }

    delay(50);
  }
}

/**
 * Obstacle avoidance routine
 * Strategy: Right, Forward, Left, Check, Repeat if needed
 * Then: Forward, Left, Forward, Right to return to path
 */
void avoidObstacle() {
  Serial.println("[AVOID] Starting obstacle avoidance routine");

  // Step 1: Turn right 90 degrees
  turnRight90();

  // Step 2: Move forward (parallel to obstacle)
  moveForwardUnit();

  // Step 3: Turn left 90 degrees (facing original direction)
  turnLeft90();

  // Step 4: Check if still blocked
  if (isObstacleDetected()) {
    Serial.println("[AVOID] Still blocked, repeating avoidance...");
    avoidObstacle();  // Recursive call
    return;
  }

  // Step 5: Clear! Return to path
  Serial.println("[AVOID] Path clear, returning to line...");

  // Move forward past the obstacle
  moveForwardUnit();

  // Turn left to face back toward the line
  turnLeft90();

  // Move forward toward the line
  moveForwardUnit();

  // Turn right to align with the path
  turnRight90();

  Serial.println("[AVOID] Obstacle avoidance complete");
}

/**
 * Handle fork in the path
 * Priority: RED over GREEN, RIGHT over LEFT (if same color)
 */
void handleFork() {
  Serial.println("[FORK] Fork detected, checking paths...");

  stopMotors();
  delay(100);

  // Look right
  turnRight90();
  Color rightColor = readColor();
  Serial.print("[FORK] Right path color: ");
  logColor(rightColor);

  // Return to center
  turnLeft90();

  // Look left
  turnLeft90();
  Color leftColor = readColor();
  Serial.print("[FORK] Left path color: ");
  logColor(leftColor);

  // Return to center
  turnRight90();

  // Decision logic
  if (leftColor == rightColor) {
    // Same color - prefer RIGHT
    Serial.println("[FORK] Same colors - taking RIGHT path");
    turnRight90();
    moveForward();
  } else if (leftColor == COLOR_RED && rightColor != COLOR_RED) {
    // Left is RED, right is not - take RED (left)
    Serial.println("[FORK] Left is RED - taking LEFT path");
    turnLeft90();
    onRedPath = true;
    moveForward();
  } else if (rightColor == COLOR_RED && leftColor != COLOR_RED) {
    // Right is RED, left is not - take RED (right)
    Serial.println("[FORK] Right is RED - taking RIGHT path");
    turnRight90();
    onRedPath = true;
    moveForward();
  } else {
    // Default: take right
    Serial.println("[FORK] Default - taking RIGHT path");
    turnRight90();
    moveForward();
  }
}

/**
 * Search for the line when it's lost
 * Sweeps left and right to find the black line
 */
void findLine() {
  Serial.println("[SEARCH] Searching for line...");

  stopMotors();

  // Try turning right a bit
  for (int i = 0; i < 5; i++) {
    turnRight();
    delay(100);
    stopMotors();

    if (readColor() == COLOR_BLACK) {
      Serial.println("[SEARCH] Line found on RIGHT");
      return;
    }
  }

  // Return to center and try left
  for (int i = 0; i < 10; i++) {
    turnLeft();
    delay(100);
    stopMotors();

    if (readColor() == COLOR_BLACK) {
      Serial.println("[SEARCH] Line found on LEFT");
      return;
    }
  }

  // Return to center if not found
  for (int i = 0; i < 5; i++) {
    turnRight();
    delay(100);
  }
  stopMotors();

  Serial.println("[SEARCH] Line not found, moving forward to retry");
  moveForwardUnit();
}

// ============================================================================
// END OF CODE
// ============================================================================
