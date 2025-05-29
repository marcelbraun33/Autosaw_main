// === ArduinoCompat.h ===
#pragma once

// This header must be included before any STL includes to prevent macro conflicts

// Backup Arduino min/max macros if they exist
#ifdef min
#define ARDUINO_MIN_MACRO min
#undef min
#endif

#ifdef max
#define ARDUINO_MAX_MACRO max
#undef max
#endif

#ifdef atomic_load
#undef atomic_load
#endif

#ifdef atomic_exchange
#undef atomic_exchange
#endif

// Add other Arduino macro conflicts as needed
