/*
 *  This sketch shows basic info about the configuration
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
    EEPROMr.add_by_name("eeprom2");

    // Or add them by subtype (it will search and add all partitions with that subtype)
    //EEPROMr.add_by_subtype(0x99);

    // Offset where the magic bytes will be stored (last 16 bytes block)
    EEPROMr.offset(0xFF0);

    // Look for the most recent valid data and populate the memory buffer
    EEPROMr.begin(4096);

    // -------------------------------------------------------------------------

    Serial.printf("[EEPROM] Reserved partitions : %u\n", EEPROMr.size());
    Serial.printf("[EEPROM] Partition size      : %u\n", EEPROMr.length());
    Serial.printf("[EEPROM] Parititions in use  : ");
    for (uint32_t i = 0; i < EEPROMr.size(); i++) {
        if (i>0) Serial.print(", ");
        Serial.print(EEPROMr.name(i));
    }
    Serial.println();
    Serial.printf("[EEPROM] Current partition   : %s\n", EEPROMr.current());

}

void loop() {}
