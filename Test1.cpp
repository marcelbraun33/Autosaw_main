#include <ClearCore.h>
#include <genieArduinoDEV.h>

#define GENIE_SERIAL_CONNECTOR ConnectorCOM0
#define GENIE_SERIAL_STREAM Serial0
#define GENIE_RESET_PIN 6
#define GENIE_BAUD 115200

Genie genie;

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("Testing Genie display...");

    GENIE_SERIAL_CONNECTOR.Mode(Connector::RS232);
    GENIE_SERIAL_CONNECTOR.PortOpen();
    GENIE_SERIAL_STREAM.begin(GENIE_BAUD);

    pinMode(GENIE_RESET_PIN, OUTPUT);
    digitalWrite(GENIE_RESET_PIN, LOW);
    delay(100);
    digitalWrite(GENIE_RESET_PIN, HIGH);
    delay(5000);

    if (genie.Begin(GENIE_SERIAL_STREAM)) {
        Serial.println("Genie display initialized.");
        genie.SetForm(0);
        delay(2000);
        genie.SetForm(1);
        delay(500);
        int currentForm = genie.ReadObject(GENIE_OBJ_FORM, 0);
        Serial.print("Current form: ");
        Serial.println(currentForm);
    } else {
        Serial.println("Failed to initialize Genie display.");
    }
}

void loop() {
    // Empty loop for testing
}
