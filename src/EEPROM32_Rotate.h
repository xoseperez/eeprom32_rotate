/*

EEPROM32 Rotate

EEPROM wrapper for ESP32 that handles partition rotation

Copyright (C) 2018 by Xose Pérez <xose dot perez at gmail dot com>

The EEPROM32_Rotate library is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

The EEPROM32_Rotate library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the EEPROM32_Rotate library.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef EEPROM32_ROTATE_H
#define EEPROM32_ROTATE_H

#include <EEPROM.h>
#include <Stream.h>
#include <vector>
#include <esp_partition.h>

#ifdef DEBUG_EEPROM32_ROTATE_PORT
#define DEBUG_EEPROM32_ROTATE(...) DEBUG_EEPROM32_ROTATE_PORT.printf( __VA_ARGS__ )
#else
#define DEBUG_EEPROM32_ROTATE(...)
#endif

#define EEPROM32_ROTATE_CRC_OFFSET        0   // 2 bytes long
#define EEPROM32_ROTATE_COUNTER_OFFSET    2   // 1 byte long

typedef struct {
    char * name;
    const esp_partition_t * part;
} partition_t;

class EEPROM32_Rotate: public EEPROMClass {

    public:


        EEPROM32_Rotate(): EEPROMClass() {
            _user_defined_size = 0;
        };

        uint8_t add_by_name(const char* name);
        uint8_t add_by_subtype(uint8_t subtype);
        bool offset(uint16_t offset);
        uint8_t size();

        void begin(size_t size);
        bool commit();

        const char * name(uint8_t index);
        const char * current();
        void dump(Stream & debug, uint8_t index = 0);
        void _auto(uint8_t subtype);

    protected:

        std::vector<partition_t> _partitions;

        uint16_t _offset = 0;
        uint8_t _partition_index = 0;
        uint8_t _partition_value = 0;

        bool _exists(const char * name);
        uint16_t _calculate_crc();
        bool _check_crc();

};

#endif // EEPROM32_ROTATE_H
