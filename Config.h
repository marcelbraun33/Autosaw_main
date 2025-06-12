#pragma once

#include <ClearCore.h>

// === Serial Configuration ===
#define USB_BAUD            115200
#define GENIE_BAUD          115200
#define GENIE_SERIAL_PORT Serial0  // Optional, or just use Serial1 directly

// === Motor Connections ===
#define MOTOR_SPINDLE       ConnectorM0   // Velocity mode (MCVC)
#define MOTOR_FENCE_X       ConnectorM3   // Step/Dir
#define MOTOR_TABLE_Y       ConnectorM2   // Step/Dir
#define MOTOR_ROTARY_Z      ConnectorM1   // Step/Dir

// === Pendant Selector Switch (Position Dial) ===
#define SELECTOR_PIN_X        ConnectorIO1   // Jog Fence
#define SELECTOR_PIN_Y        ConnectorIO2   // Jog Table
#define SELECTOR_PIN_Z        ConnectorIO3   // Jog Rotary
#define SELECTOR_PIN_SETUP    ConnectorIO4   // Setup AutoCut
#define SELECTOR_PIN_AUTOCUT  ConnectorIO5   // AutoCut Active
#define GENIE_RESET_PIN 6

// === Pendant Range Selector (X1/X10/X100) ===
#define RANGE_PIN_X10         ConnectorA11   // X10 range switch
#define RANGE_PIN_X100        ConnectorA10   // X100 range switch
// X1 is implied when both A10 and A11 are LOW

// === E-Stop and Relay Control ===
#define ESTOP_INPUT_PIN       ConnectorA12   // E-stop (active LOW)
#define SAFETY_RELAY_OUTPUT   ConnectorIO0   // Relay control

// === Encoder Jog Wheel ===
#define JOG_ENCODER_INPUT     EncoderIn

// === Motor Configuration Constants ===
#define COUNTS_PER_REV_SPINDLE    12800.0f
#define FENCE_STEPS_PER_INCH      4065.0f
#define TABLE_STEPS_PER_INCH      4065.0f
#define ROTARY_STEPS_PER_DEGREE   11.3778f  // Adjust as needed
#define MAX_X_INCHES 7.0f   // for example
#define MAX_Y_INCHES 7.0f   // your table depth

// === RPM Control ===
#define SPINDLE_MAX_RPM       4000.0f
#define RPM_MIN               0.0f
#define RPM_MAX               4000.0f
#define RPM_STEP              10
#define RPM_SCALE             1.0f

// === Jog Encoder Scaling ===
#define ENCODER_COUNTS_PER_CLICK  4
#define JOG_MULTIPLIER_X1         1
#define JOG_MULTIPLIER_X10        10
#define JOG_MULTIPLIER_X100       100

// === Form IDs ===
#define FORM_HOMING         0   // Form0
#define FORM_JOG_X          1   // Form1 (Jog Fence)
#define FORM_SEMI_AUTO      2   // Form2
#define FORM_SETTINGS       3   // Form3
#define FORM_SPLASH         4   // Form4
#define FORM_AUTOCUT        5   // Form5
#define FORM_JOG_Y          6   // Form6 (Jog Table)
#define FORM_MANUAL_MODE    7   // Form7
#define FORM_JOG_Z          8   // Form8 (Rotary)

// ─── Genie Object Definitions ────────────────────────────────────────────────────

// — LED Digits —
// Form1 (Jog Fence X)
#define LEDDIGITS_SAW_POSITION             0   // Form1 - Leddigits0
#define LEDDIGITS_STOCK_LENGTH             1   // Form1 - Leddigits1
#define LEDDIGITS_CUT_THICKNESS            2   // Form1 - Leddigits2
#define LEDDIGITS_INCREMENT                3   // Form1 - Leddigits3
#define LEDDIGITS_TOTAL_SLICES             4   // Form1 - Leddigits4
#define LEDDIGITS_SLICE_COUNTER            5   // Form1 - Leddigits5
#define LED_NEGATIVE_INDICATOR             0   // Form1 - Led0

// Form2 (Semi Auto)
#define LEDDIGITS_RPM_DISPLAY              6   // Form2 - Leddigits6
#define LEDDIGITS_FEED_OVERRIDE            7   // Form2 - Leddigits7
#define LEDDIGITS_CUT_PRESSURE             27  // Form2 - Leddigits27
#define LEDDIGITS_THICKNESS_F2             28  // Form2 - Leddigits28
#define LEDDIGITS_DISTANCE_TO_GO_F2        29  // Form2 - Leddigits29
#define LEDDIGITS_TARGET_FEEDRATE          32  // Form2 - Leddigits32
#define LED_READY                          6   // Form2 - Led6


// Form3 (Settings)
#define LEDDIGITS_DIAMETER_SETTINGS        8   // Form3 - Leddigits8
#define LEDDIGITS_THICKNESS_SETTINGS       9   // Form3 - Leddigits9
#define LEDDIGITS_RPM_SETTINGS            10   // Form3 - Leddigits10
#define LEDDIGITS_FEEDRATE_SETTINGS       11   // Form3 - Leddigits11
#define LEDDIGITS_RAPID_SETTINGS          12   // Form3 - Leddigits12
#define LEDDIGITS_CUT_PRESSURE_SETTINGS   26   // Form3 - Leddigits26

// Form5 (Autocut Active)
#define LEDDIGITS_FEED_OVERRIDE_F5        13   // Form5 - Leddigits13
#define LEDDIGITS_RPM_F5                  14   // Form5 - Leddigits14
#define LEDDIGITS_STOCK_LENGTH_F5         15   // Form5 - Leddigits15
#define LEDDIGITS_THICKNESS_F5            16   // Form5 - Leddigits16
#define LEDDIGITS_CUTTING_POSITION_F5     17   // Form5 - Leddigits17
#define LEDDIGITS_DISTANCE_TO_GO_F5       18   // Form5 - Leddigits18
#define LEDDIGITS_CUT_PRESSURE_F5         30   // Form5 - Leddigits30
#define LEDDIGITS_TOTAL_SLICES_F5         31   // Form5 - Leddigits31
#define WINBUTTON_ADJUST_MAX_SPEED_F5     45   // Form5 - Winbutton45 (Adjust Max Speed f5)


// Form6 (Jog Table Y)
#define LEDDIGITS_STOCK_END_Y             19   // Form6 - Leddigits19
#define LEDDIGITS_TABLE_POSITION_Y        20   // Form6 - Leddigits20
#define LEDDIGITS_CUT_LENGTH_Y            21   // Form6 - Leddigits21
#define LEDDIGITS_CUT_STOP_Y              22   // Form6 - Leddigits22
#define LEDDIGITS_RETRACT_DISTANCE        25   // Form6 - Leddigits25
#define LED_AT_START_POSITION_Y           1    // Form6 - Led1

// Form7 (Manual Mode)
#define LEDDIGITS_MANUAL_RPM              23   // Form7 - Leddigits23

// Form8 (Rotary Z)
#define LEDDIGITS_ROTARY_RPM              24   // Form8 - Leddigits24

// LED Indicators
#define LED_FEED_RATE_OFFSET_F2           2   // Form2
#define LED_FEED_RATE_OFFSET_F5           3   // Form5
#define LED_CUT_PRESSURE_OFFSET_F2        4   // Form2
#define LED_CUT_PRESSURE_OFFSET_F5        5   // Form5

// — Gauges —
#define IGAUGE_SEMIAUTO_LOAD_METER        0   // Form2 - IGauge0
#define IGAUGE_SEMIAUTO_CUT_PRESSURE      1   // Form2 - IGauge1
#define IGAUGE_AUTOCUT_FEED_PRESSURE      2   // Form5 - IGauge2
#define IGAUGE_AUTOCUT_LOAD_METER         3   // Form5 - IGauge3
#define IGAUGE_MANUAL_LOAD_METER          4   // Form7 - IGauge4

// — Slider —
#define ISLIDER_CONTRAST                  0   // Form3 - ISliderD0

// — WinButtons —
// Form1 (Jog Fence X)
#define WINBUTTON_CAPTURE_ZERO             0   // Form1 - Winbutton0
#define WINBUTTON_CAPTURE_STOCK_LENGTH     1   // Form1 - Winbutton1
#define WINBUTTON_ACTIVATE_JOG             2   // Form1 - Winbutton2
#define WINBUTTON_CAPTURE_INCREMENT        3   // Form1 - Winbutton3
#define WINBUTTON_INC_PLUS                 4   // Form1 - Winbutton4
#define WINBUTTON_INC_MINUS                5   // Form1 - Winbutton5
#define WINBUTTON_DIVIDE_SET               6   // Form1 - Winbutton6
#define WINBUTTON_SET_STOCK_LENGTH         7   // Form1 - Winbutton7
#define WINBUTTON_SET_CUT_THICKNESS        8   // Form1 - Winbutton8
#define WINBUTTON_SET_TOTAL_SLICES         9   // Form1 - Winbutton9
#define WINBUTTON_GO_TO_ZERO              47   // Form1 - Winbutton47
#define WINBUTTON_SET_STOCK_SLICES_X_INC  51   // Form1 - Winbutton51 (New in your HMI)

// Form2 (Semi-Auto)
#define WINBUTTON_SPINDLE_ON              10   // Form2 - Winbutton10
#define WINBUTTON_FEED_TO_STOP            11   // Form2 - Winbutton11
#define WINBUTTON_FEED_HOLD               12   // Form2 - Winbutton12
#define WINBUTTON_EXIT_FEED_HOLD          13   // Form2 - Winbutton13
#define WINBUTTON_INC_PLUS_F2             14   // Form2 - Winbutton14
#define WINBUTTON_STOCK_ZERO              15   // Form2 - Winbutton15 (renamed from ADJUST_FEED)
#define WINBUTTON_SETTINGS_SEMI           16   // Form2 - Winbutton16
#define WINBUTTON_ADJUST_CUT_PRESSURE     41   // Form2 - Winbutton41
#define WINBUTTON_HOME_F2                 42   // Form2 - Winbutton42 (renamed from TABLE_TO_POSITION)
#define WINBUTTON_INC_MINUS_F2            50   // Form2 - Winbutton50
#define WINBUTTON_ADJUST_MAX_SPEED        32   // Form2 - Winbutton32

// Form3 (Settings)
#define WINBUTTON_SET_RPM_SETTINGS        17   // Form3 - Winbutton17
#define WINBUTTON_SET_FEEDRATE_SETTINGS   18   // Form3 - Winbutton18
#define WINBUTTON_SET_RAPID_SETTINGS      19   // Form3 - Winbutton19
#define WINBUTTON_SET_DIAMETER_SETTINGS   20   // Form3 - Winbutton20
#define WINBUTTON_SET_THICKNESS_SETTINGS  21   // Form3 - Winbutton21
#define WINBUTTON_BACK                    38   // Form3 - Winbutton38
#define WINBUTTON_SET_CUT_PRESSURE_F3     40   // Form3 - Winbutton40

// Form5 (Autocut Active)
#define WINBUTTON_END_CYCLE_F5            22   // Form5 - Winbutton22
#define WINBUTTON_SLIDE_HOLD_F5           23   // Form5 - Winbutton23
#define WINBUTTON_SPINDLE_F5              24   // Form5 - Winbutton24 (renamed from ADJUST_FEEDRATE_F5)
#define WINBUTTON_START_AUTOFEED_F5       25   // Form5 - Winbutton25
#define WINBUTTON_SETTINGS_F5             26   // Form5 - Winbutton26
#define WINBUTTON_ADJUST_CUT_PRESSURE_F5  43   // Form5 - Winbutton43
#define WINBUTTON_MOVE_TO_START_POSITION  44   // Form5 - Winbutton44


// Form6 (Jog Table Y)
#define WINBUTTON_CAPTURE_CUT_END_F6      27   // Form6 - Winbutton27
#define WINBUTTON_ACTIVATE_JOG_Y_F6       28   // Form6 - Winbutton28
#define WINBUTTON_SET_WITH_MPG_F6         29   // Form6 - Winbutton29
#define WINBUTTON_CAPTURE_CUT_START_F6    30   // Form6 - Winbutton30
#define WINBUTTON_JOG_TO_START            34   // Form6 - Winbutton34
#define WINBUTTON_SET_RETRACT_WITH_MPG_F6 39   // Form6 - Winbutton39
#define WINBUTTON_JOG_TO_END              48   // Form6 - Winbutton48
#define WINBUTTON_JOG_TO_RETRACT          49   // Form6 - Winbutton49

// Form7 (Manual Mode)
//#define WINBUTTON_ACTIVATE_PENDANT        32   // Form7 - Winbutton32
#define WINBUTTON_ACTIVATE_HOMING         33   // Form7 - Winbutton33
#define WINBUTTON_SPINDLE_TOGGLE_F7       31   // Form7 - Winbutton31
#define WINBUTTON_SETTINGS_F7             35   // Form7 - Winbutton35

// Form8 (Rotary Z)
#define WINBUTTON_SET_RPM_F8              36   // Form8 - Winbutton36
#define WINBUTTON_ENABLE_F8               37   // Form8 - Winbutton37


// === EncoderPositionTracker Configuration ===
// This section defines constants for the absolute position tracking system

// Encoder steps per inch (using existing motor calibration values for consistency)
#define ENCODER_X_STEPS_PER_INCH  FENCE_STEPS_PER_INCH  // X-axis (fence) encoder resolution
#define ENCODER_Y_STEPS_PER_INCH  TABLE_STEPS_PER_INCH  // Y-axis (table) encoder resolution

// Position verification tolerance (inches)
#define ENCODER_POSITION_TOLERANCE  0.002f  // Maximum acceptable difference between commanded and actual position

// Encoder position verification mode
#define ENCODER_VERIFICATION_ENABLED  true    // Set to false to disable position verification

// Debug settings
#define ENCODER_DEBUG_LOGGING  true    // Enable/disable detailed position logging
#define ENCODER_LOG_INTERVAL   1000    // Milliseconds between position log messages

// Motor references for encoder position tracking
#define MOTOR_SAW_Y  MOTOR_TABLE_Y     // Clarifying Y-axis motor reference for encoder tracking
