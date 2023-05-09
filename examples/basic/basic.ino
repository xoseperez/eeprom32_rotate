/*
 *  This sketch shows sector hoping across reboots
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

    uint8_t data;
    uint8_t quantity;
    quantity = 10;
    Serial.printf("Position 0: 0x%02X\n", EEPROMr.read(0));
    Serial.printf("Position 1: 0x%02X\n", EEPROMr.read(1));
    Serial.printf("Position 2: 0x%02X\n", EEPROMr.read(2));
    Serial.printf("Position 5: 0x%02X\n", EEPROMr.read(5));
    Serial.printf("Position 6: 0x%02X\n", EEPROMr.read(6));
    Serial.printf("Data      : 0x%02X\n", data = EEPROMr.read(5));

    Serial.println();
    Serial.printf("Writing 0x%02X to data\n", quantity);
    //EEPROMr.write(0, data + 1);
    EEPROMr.writeUShort(5, 10);

    Serial.println();
    Serial.printf("Commit %s\n", EEPROMr.commit() ? "OK" : "KO");
    Serial.printf("Position 0: 0x%02X\n", EEPROMr.read(0));
    Serial.printf("Position 1: 0x%02X\n", EEPROMr.read(1));
    Serial.printf("Position 2: 0x%02X\n", EEPROMr.read(2));
    Serial.printf("Position 5: 0x%02X\n", EEPROMr.read(5));
    Serial.printf("Position 6: 0x%02X\n", EEPROMr.read(6));
    Serial.printf("Data      : 0x%02X\n", data = EEPROMr.read(5));

    Serial.println("2ND TIME WRITING TO EEPROM");
    quantity = quantity +5;

    Serial.printf("Position 0: 0x%02X\n", EEPROMr.read(0));
    Serial.printf("Position 1: 0x%02X\n", EEPROMr.read(1));
    Serial.printf("Position 2: 0x%02X\n", EEPROMr.read(2));
    Serial.printf("Position 5: 0x%02X\n", EEPROMr.read(5));
    Serial.printf("Position 6: 0x%02X\n", EEPROMr.read(6));
    Serial.printf("Data      : 0x%02X\n", data = EEPROMr.read(6));
    
    
    Serial.printf("Data      : 0x%02X\n", data = EEPROMr.read(0));

    Serial.println();
    Serial.printf("Writing 0x%02X to data\n", quantity);
    //EEPROMr.write(0, data + 1);
    EEPROMr.writeUShort(6, quantity);

    Serial.println();
    Serial.printf("Commit %s\n", EEPROMr.commit() ? "OK" : "KO");
    Serial.printf("Position 0: 0x%02X\n", EEPROMr.read(0));
    Serial.printf("Position 1: 0x%02X\n", EEPROMr.read(1));
    Serial.printf("Position 2: 0x%02X\n", EEPROMr.read(2));
    Serial.printf("Position 5: 0x%02X\n", EEPROMr.read(5));
    Serial.printf("Position 6: 0x%02X\n", EEPROMr.read(6));
    Serial.printf("Data      : 0x%02X\n", data = EEPROMr.read(6));
    Serial.printf("Data      : 0x%02X\n", data = EEPROMr.read(0));

}

void loop() {}