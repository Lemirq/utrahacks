# Line Following Robot with Obstacle Avoidance

A two-wheeled Arduino robot that follows colored lines and navigates obstacles autonomously.

## Overview

This robot follows a black line and makes decisions at colored forks. It handles two types of paths:
- **Red Path**: Obstacle course with boxes to navigate around
- **Green Path**: Clear path to shooting practice area

## Hardware Requirements

| Component | Model | Quantity |
|-----------|-------|----------|
| Microcontroller | Arduino UNO R4 WiFi (ABX00080) | 1 |
| Motor Driver | L298N | 1 |
| DC Motors | JGA25-370 (12V, 50RPM) | 2 |
| Color Sensor | V575 (TCS3200 compatible) | 1 |
| Ultrasonic Sensor | HY-SRF05 | 1 |
| Power Supply | 9V Battery or 12V adapter | 1 |
| Breadboard | Standard | 1 |
| Jumper Wires | Male-to-Male, Male-to-Female | Various |

## Wiring Diagram

### L298N Motor Driver

| L298N Pin | Arduino Pin | Notes |
|-----------|-------------|-------|
| IN1 | 8 | Motor A direction |
| IN2 | 7 | Motor A direction |
| IN3 | 5 | Motor B direction |
| IN4 | 4 | Motor B direction |
| ENA | Jumper ON | Full speed (no PWM) |
| ENB | Jumper ON | Full speed (no PWM) |
| +12V | Battery (+) | 9-12V power |
| GND | Battery (-) + Arduino GND | Common ground |

**Motor Connections:**
- Motor A (Left) → OUT1, OUT2
- Motor B (Right) → OUT3, OUT4

### HY-SRF05 Ultrasonic Sensor

| Sensor Pin | Arduino Pin |
|------------|-------------|
| VCC | 5V |
| TRIG | 11 |
| ECHO | 12 |
| GND | GND |
| OUT | Not connected |

### V575 Color Sensor

| Sensor Pin | Arduino Pin |
|------------|-------------|
| VCC | 5V |
| S0 | A0 |
| S1 | A1 |
| S2 | A2 |
| S3 | A3 |
| OUT | A4 |
| GND | GND |

## Navigation Rules

### Fork Decision Priority

1. **Different colors at fork**: RED takes priority over GREEN
2. **Same color at fork**: RIGHT takes priority

### Path Types

| Color | Path Type | Behavior |
|-------|-----------|----------|
| Black | Main line | Follow straight |
| Red | Obstacle course | Use ultrasonic to avoid obstacles |
| Green | Clear path | Follow to destination |
| White | Off track | Search for line |

## Obstacle Avoidance Algorithm

When an obstacle is detected on the red path:

```
1. Turn RIGHT 90 degrees
2. Move FORWARD one unit
3. Turn LEFT 90 degrees
4. Check for obstacle again
   - If blocked: REPEAT from step 1
   - If clear: Continue to step 5
5. Move FORWARD (past obstacle)
6. Turn LEFT 90 degrees
7. Move FORWARD (toward line)
8. Turn RIGHT 90 degrees
9. Continue following path
```

```
    OBSTACLE
    +-----+
    |     |
    |     |
    +-----+
       ^
       |
    [Robot starts here]
       |
       v
    +----> (right)
           |
           v
    <----+ (left turn)
    |
    v
    Continue...
```

## Calibration

### Color Sensor Calibration

The color detection thresholds need to be calibrated for your environment:

```cpp
// In line_follower_robot.ino, adjust these values:
const int BLACK_THRESHOLD = 100;  // Increase if black not detected
const int RED_MIN = 20;           // Adjust based on your red color
const int RED_MAX = 60;
const int GREEN_MIN = 20;         // Adjust based on your green color
const int GREEN_MAX = 60;
```

**How to calibrate:**
1. Upload the code
2. Open Serial Monitor (9600 baud)
3. Place sensor over each color and note the R, G, B values
4. Adjust thresholds accordingly

### Motor Timing Calibration

Adjust these values based on your robot's speed and weight:

```cpp
const int TURN_90_DELAY = 500;     // Time to turn 90 degrees (ms)
const int MOVE_UNIT_DELAY = 300;   // Time to move one unit (ms)
```

**How to calibrate:**
1. Set `TURN_90_DELAY` to 500ms
2. Run the robot and observe 90-degree turns
3. Increase value if turning too little, decrease if too much
4. Repeat for `MOVE_UNIT_DELAY` with forward movement

## Serial Monitor Output

Connect via USB and open Serial Monitor at 9600 baud to see:

```
=================================
LINE FOLLOWER ROBOT INITIALIZING
=================================
[OK] Motors initialized (IN1=8, IN2=7, IN3=5, IN4=4)
[OK] Ultrasonic sensor initialized (TRIG=11, ECHO=12)
[OK] Color sensor initialized (S0=A0, S1=A1, S2=A2, S3=A3, OUT=A4)
=================================
INITIALIZATION COMPLETE
Starting in 2 seconds...
=================================

[COLOR] R:45 G:120 B:130
[DETECTED] BLACK
[MOTOR] Moving FORWARD
[ULTRASONIC] Distance: 25 cm
...
```

## File Structure

```
line_follower_robot/
├── line_follower_robot.ino   # Main Arduino sketch
└── README.md                 # This file
```

## Troubleshooting

### Motors not spinning
- Check ENA/ENB jumpers are ON
- Verify battery is connected to L298N +12V and GND
- Ensure Arduino GND is connected to L298N GND
- Check motor wire connections at OUT terminals

### Color sensor returning 0
- Check wiring (especially VCC and GND)
- Sensor may be too far from the surface (ideal: 1-2cm)
- Ensure adequate lighting

### Ultrasonic sensor always returns 999
- Check TRIG and ECHO wiring
- Ensure nothing is blocking the sensor
- Verify 5V power connection

### Robot turns wrong direction
- Swap motor wires at OUT1/OUT2 or OUT3/OUT4
- Or swap IN1/IN2 or IN3/IN4 pin assignments in code

### Robot overshoots turns
- Decrease `TURN_90_DELAY` value
- Check if one motor is faster than the other

## Future Improvements

- [ ] Add PID control for smoother line following
- [ ] Implement speed control via PWM
- [ ] Add encoder feedback for precise movements
- [ ] Add LCD display for status
- [ ] Implement servo control for shooting mechanism

## License

MIT License - UltraHacks Team 2026
