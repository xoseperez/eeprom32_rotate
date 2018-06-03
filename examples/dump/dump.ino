/*
 *  This sketch dumps the default EEPROM sector memory
 */

#include <EEPROM32_Rotate.h>

EEPROM32_Rotate EEPROMr;

void setup() {

    // Init DEBUG --------------------------------------------------------------

    Serial.begin(115200);
    delay(2000);
    Serial.println();
    Serial.println();

    // Init EEPROM32_Rotate ----------------------------------------------------

    // You can add partitions manually by name
    EEPROMr.add_by_name("eeprom");

    // Offset where the magic bytes will be stored (last 16 bytes block)
    EEPROMr.offset(0xFF0);

    // Look for the most recent valid data and populate the memory buffer
    EEPROMr.begin(4096);

    // -------------------------------------------------------------------------

    Serial.printf("[EEPROM] Dumping data for partition %s\n", EEPROMr.current());
    EEPROMr.dump(Serial);

}

void loop() {}
