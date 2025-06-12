// ...existing code...

// REMOVE or COMMENT OUT these macros if present:
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// Comment out or remove any macro definitions like these:
#if 0
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#endif

// If you need min/max, use std::min/std::max from <algorithm> instead.

// ...existing code...
