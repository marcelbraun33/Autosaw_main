#pragma once

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#define NOMINMAX

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
#define FORM_SPLASH         4
#define FORM_MANUAL_MODE    7
#define FORM_HOMING         0
#define FORM_JOG_X          1
#define FORM_JOG_Y          6
#define FORM_JOG_Z          8
#define FORM_SEMI_AUTO      2
#define FORM_AUTOCUT        5
#define FORM_SETTINGS       3

// ─── Genie Object Definitions ────────────────────────────────────────────────────

// — LED Digits —
#define LEDDIGITS_SAW_POSITION             0   // Form1
#define LEDDIGITS_STOCK_LENGTH             1   // Form1
#define LEDDIGITS_CUT_THICKNESS            2   // Form1
#define LEDDIGITS_INCREMENT                3   // Form1
#define LEDDIGITS_TOTAL_SLICES             4   // Form1
#define LEDDIGITS_SLICE_COUNTER            5   // Form1
#define LED_NEGATIVE_INDICATOR             0   // Form1

#define LEDDIGITS_RPM_DISPLAY              6   // Form2
#define LEDDIGITS_FEED_OVERRIDE            7   // Form2
#define LEDDIGITS_CUT_PRESSURE             27  // Form2 - New from object sheet
#define LEDDIGITS_THICKNESS_F2             28  // Form2 - New from object sheet
#define LEDDIGITS_DISTANCE_TO_GO_F2        29  // Form2 - New from object sheet
#define LEDDIGITS_CUTTING_POSITION_F2      32  // Form2 - New from object sheet
#define LEDDIGITS_CUTTING_TOTAL_F2         33  // Form2 - New from object sheet

#define LEDDIGITS_DIAMETER_SETTINGS        8   // Form3
#define LEDDIGITS_THICKNESS_SETTINGS       9   // Form3
#define LEDDIGITS_RPM_SETTINGS            10   // Form3
#define LEDDIGITS_FEEDRATE_SETTINGS       11   // Form3
#define LEDDIGITS_RAPID_SETTINGS          12   // Form3
#define LEDDIGITS_CUT_PRESSURE_SETTINGS   26   // Form3 - New from object sheet

#define LEDDIGITS_FEED_OVERRIDE_F5        13   // Form5
#define LEDDIGITS_RPM_F5                  14   // Form5
#define LEDDIGITS_STOCK_LENGTH_F5         15   // Form5
#define LEDDIGITS_THICKNESS_F5            16   // Form5
#define LEDDIGITS_CUTTING_POSITION_F5     17   // Form5 - Updated name to match sheet
#define LEDDIGITS_DISTANCE_TO_GO_F5       18   // Form5 - Updated name to match sheet
#define LEDDIGITS_CUT_PRESSURE_F5         30   // Form5 - New from object sheet
#define LEDDIGITS_TOTAL_SLICES_F5         31   // Form5 - New from object sheet

#define LEDDIGITS_STOCK_END_Y             19   // Form6
#define LEDDIGITS_TABLE_POSITION_Y        20   // Form6
#define LEDDIGITS_CUT_LENGTH_Y            21   // Form6
#define LEDDIGITS_CUT_STOP_Y              22   // Form6
#define LEDDIGITS_RETRACT_DISTANCE        25   // Form6 - New from object sheet

// Define LED IDs for indicators
#ifndef LED_AT_START_POSITION_Y
#define LED_AT_START_POSITION_Y 1  // Form6 - LED indicator for table at start position
#endif

#ifndef LED_FEED_RATE_OFFSET_F2
#define LED_FEED_RATE_OFFSET_F2 2  // Form2 - Feed rate offset indicator
#endif

#ifndef LED_CUT_PRESSURE_OFFSET_F2
#define LED_CUT_PRESSURE_OFFSET_F2 4  // Form2 - Cut pressure offset indicator
#endif

#ifndef LED_FEED_RATE_OFFSET_F5
#define LED_FEED_RATE_OFFSET_F5 3  // Form5 - Feed rate offset indicator
#endif

#ifndef LED_CUT_PRESSURE_OFFSET_F5
#define LED_CUT_PRESSURE_OFFSET_F5 5  // Form5 - Cut pressure offset indicator
#endif

#define LEDDIGITS_MANUAL_RPM              23   // Form7
#define LEDDIGITS_MANUAL_UNUSED           24   // Form7 (if any)

// — Gauges —
#define IGAUGE_SEMIAUTO_LOAD_METER        0   // Form2
#define IGAUGE_SEMIAUTO_CUT_PRESSURE      1   // Form2 - Updated name to match sheet
#define IGAUGE_AUTOCUT_FEED_PRESSURE      2   // Form5
#define IGAUGE_AUTOCUT_LOAD_METER         3   // Form5
#define IGAUGE_MANUAL_LOAD_METER          4   // Form7

// — Slider —
#define ISLIDER_CONTRAST                   0   // Form3

// — WinButtons —
// Form1 (Jog Fence)
#define WINBUTTON_CAPTURE_ZERO             0
#define WINBUTTON_CAPTURE_STOCK_LENGTH     1
#define WINBUTTON_ACTIVATE_JOG             2
#define WINBUTTON_CAPTURE_INCREMENT        3
#define WINBUTTON_INC_PLUS                 4
#define WINBUTTON_GO_TO_ZERO              47   // Form1 - New from object sheet
#define WINBUTTON_INC_MINUS                5
#define WINBUTTON_DIVIDE_SET               6
#define WINBUTTON_SET_STOCK_LENGTH         7
#define WINBUTTON_SET_CUT_THICKNESS        8
#define WINBUTTON_SET_TOTAL_SLICES         9

// Form2 (Semi-Auto)
#define WINBUTTON_SPINDLE_ON               10
#define WINBUTTON_FEED_TO_STOP             11
#define WINBUTTON_FEED_HOLD                12
#define WINBUTTON_EXIT_FEED_HOLD           13
#define WINBUTTON_INC_PLUS_F2              14  // Updated based on object sheet
#define WINBUTTON_ADJUST_FEED              15
#define WINBUTTON_SETTINGS_SEMI            16
#define WINBUTTON_ADJUST_CUT_PRESSURE      41  // Form2 - New from object sheet
#define WINBUTTON_TABLE_TO_POSITION        42  // Form2 - New from object sheet
#define WINBUTTON_INC_MINUS_F2             50  // Form2 - New from object sheet

// Form3 (Settings)
#define WINBUTTON_SET_RPM_SETTINGS         17
#define WINBUTTON_SET_FEEDRATE_SETTINGS    18
#define WINBUTTON_SET_RAPID_SETTINGS       19
#define WINBUTTON_SET_DIAMETER_SETTINGS    20
#define WINBUTTON_SET_THICKNESS_SETTINGS   21
#define WINBUTTON_BACK                     38
#define WINBUTTON_SET_CUT_PRESSURE_F3      40  // Form3 - New from object sheet
#define WINBUTTON_APPLY_OFFSET_PRESSURE    45  // Form3 - New from object sheet
#define WINBUTTON_APPLY_OFFSET_FEEDRATE    46  // Form3 - New from object sheet

// Form5 (Autocut Active)
#define WINBUTTON_END_CYCLE_F5             22
#define WINBUTTON_SLIDE_HOLD_F5            23
#define WINBUTTON_ADJUST_FEEDRATE_F5       24
#define WINBUTTON_START_AUTOFEED_F5        25
#define WINBUTTON_SETTINGS_F5              26
#define WINBUTTON_ADJUST_CUT_PRESSURE_F5   43  // Form5 - New from object sheet
#define WINBUTTON_MOVE_TO_START_POSITION   44  // Form5 - New from object sheet

// Form6 (Jog Table)
#define WINBUTTON_CAPTURE_CUT_END_F6       27
#define WINBUTTON_ACTIVATE_JOG_Y_F6        28
#define WINBUTTON_SET_WITH_MPG_F6          29
#define WINBUTTON_CAPTURE_CUT_START_F6     30
#define WINBUTTON_JOG_TO_START             34  // Form6 - Updated based on object sheet
#define WINBUTTON_SET_RETRACT_WITH_MPG_F6  39
#define WINBUTTON_JOG_TO_END               48  // Form6 - New from object sheet
#define WINBUTTON_JOG_TO_RETRACT           49  // Form6 - New from object sheet

// Form7 (Manual Mode)
#define WINBUTTON_SPINDLE_TOGGLE_F7        31
#define WINBUTTON_ACTIVATE_PENDANT         32
#define WINBUTTON_ACTIVATE_HOMING          33
#define WINBUTTON_SET_RPM_F7               34
#define WINBUTTON_SETTINGS_F7              35

// Form8 (Rotary)
#define WINBUTTON_SET_RPM_F8               36
#define WINBUTTON_ENABLE_F8                37
