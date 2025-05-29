// === CutParameters.h ===
#pragma once

/// Parameters for a cutting sequence
struct CutParameters {
    float sliceEnd;        // Y-position to feed to
    float feedRate;        // Feed rate in IPM
    float retractDistance; // Distance to retract after cut
    int totalSlices;       // Number of slices to make
};
