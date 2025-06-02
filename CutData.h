#pragma once

    static const int kMaxCuts = 100;

    struct CutData {
        // X-axis (cut sequence) parameters
        float cutPositions[kMaxCuts] = { 0.0f }; // Array of cut positions (inches)
        int numCuts = 0;                       // Number of valid cut positions
        float positionZero = 0.0f;   // Captured zero position for X
        bool useStockZero = false;   // Whether to use the captured zero

        float stockLength = 0.0f;              // Length of the stock (inches)
        float increment = 0.0f;                // Distance between cuts (inches)
        float thickness = 0.0f;                // Cut thickness (inches)
        int totalSlices = 0;                   // Total number of slices/cuts

        // Y-axis (cut location) parameters
        float cutStartPoint = 0.0f;            // Y coordinate for cut start (inches)
        float cutEndPoint = 0.0f;              // Y coordinate for cut end (inches)
        float retractDistance = 0.0f;          // Retract distance after cut (inches)
        float cutLength = 0.0f;                // Length of the cut (inches)
        float jogIncrement = 0.01f;            // Default value, can be set from JogXScreen
        
        // Cut pressure override flag
        float cutPressure = 0.0f;              // Current cut pressure value
        bool cutPressureOverride = false;      // Whether cut pressure has been overridden
    };
