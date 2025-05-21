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

#define LEDDIGITS_RPM_DISPLAY              6   // Form2
#define LEDDIGITS_FEED_OVERRIDE            7   // Form2

#define LEDDIGITS_DIAMETER_SETTINGS        8   // Form3
#define LEDDIGITS_THICKNESS_SETTINGS       9   // Form3
#define LEDDIGITS_RPM_SETTINGS            10   // Form3
#define LEDDIGITS_FEEDRATE_SETTINGS       11   // Form3
#define LEDDIGITS_RAPID_SETTINGS          12   // Form3

#define LEDDIGITS_FEED_OVERRIDE_F5        13   // Form5
#define LEDDIGITS_RPM_F5                  14   // Form5
#define LEDDIGITS_STOCK_LENGTH_F5         15   // Form5
#define LEDDIGITS_THICKNESS_F5            16   // Form5
#define LEDDIGITS_REMAINING_SLICES_F5     17   // Form5
#define LEDDIGITS_TOTAL_SLICES_F5         18   // Form5

#define LEDDIGITS_STOCK_END_Y             19   // Form6
#define LEDDIGITS_TABLE_POSITION_Y        20   // Form6
#define LEDDIGITS_CUT_LENGTH_Y            21   // Form6
#define LEDDIGITS_CUT_STOP_Y              22   // Form6

#define LEDDIGITS_MANUAL_RPM              23   // Form7
#define LEDDIGITS_MANUAL_UNUSED           24   // Form7 (if any)
#define LEDDIGITS_UNUSED_25               25   // Form7 (if any)

// — Gauges —
#define IGAUGE_SEMIAUTO_LOAD_METER        0   // Form2
#define IGAUGE_SEMIAUTO_FEED_PRESSURE     1   // Form2
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
#define WINBUTTON_ADVANCE_INCREMENT        14
#define WINBUTTON_ADJUST_FEED              15
#define WINBUTTON_SETTINGS_SEMI            16

// Form3 (Settings)
#define WINBUTTON_SET_RPM_SETTINGS         17
#define WINBUTTON_SET_FEEDRATE_SETTINGS    18
#define WINBUTTON_SET_RAPID_SETTINGS       19
#define WINBUTTON_SET_DIAMETER_SETTINGS    20
#define WINBUTTON_SET_THICKNESS_SETTINGS   21
#define WINBUTTON_BACK                      38

// Form5 (Autocut Active)
#define WINBUTTON_END_CYCLE_F5             22
#define WINBUTTON_SLIDE_HOLD_F5            23
#define WINBUTTON_ADJUST_FEEDRATE_F5       24
#define WINBUTTON_START_AUTOFEED_F5        25
#define WINBUTTON_SETTINGS_F5              26

// Form6 (Jog Table)
#define WINBUTTON_CAPTURE_CUT_END_F6       27
#define WINBUTTON_ACTIVATE_JOG_Y_F6        28
#define WINBUTTON_SET_WITH_MPG_F6          29
#define WINBUTTON_CAPTURE_CUT_START_F6     30I am going to show you 

// Form7 (Manual Mode)
#define WINBUTTON_SPINDLE_TOGGLE_F7        31
#define WINBUTTON_ACTIVATE_PENDANT         32
#define WINBUTTON_ACTIVATE_HOMING          33
#define WINBUTTON_SET_RPM_F7               34
#define WINBUTTON_SETTINGS_F7              35

// Form8 (Rotary)
#define WINBUTTON_SET_RPM_F8               36
#define WINBUTTON_ENABLE_F8                37
