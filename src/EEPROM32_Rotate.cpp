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

#include "Arduino.h"
#include "EEPROM32_Rotate.h"
#include <esp_spi_flash.h>
#include <esp_partition.h>

// -----------------------------------------------------------------------------
// PUBLIC *NEW* METHODS
// -----------------------------------------------------------------------------

/**
 * @brief Add a partition by name to the partition pool
 * @param {const char*} name    Name of the partition in the partition table
 * @returns {uint8_t}           Number of partitions add (0 or 1)
 */
uint8_t EEPROM32_Rotate::add_by_name(const char* name) {

    // Is it already in the pool?
    if (_exists(name)) return 0;

    // Find partitions
    const esp_partition_t * part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, name);
    if (part == NULL) return 0;

    // Store in _user_defined_size the size of the smallest partition
    if ((0 == _user_defined_size) || (part->size < _user_defined_size)) {
        _user_defined_size = part->size;
    }

    // Create a record
    partition_t p;
    p.name = strdup(name);
    p.part = part;

    // Add it to the pool
    _partitions.push_back(p);

    return 1;

}

/**
 * @brief Add a partition by subtype to the partition pool
 * @param {uint8_t} subtype     Subtype of the partitions to add
 * @returns {uint8_t}           Number of partitions add (0 or 1)
 */
uint8_t EEPROM32_Rotate::add_by_subtype(uint8_t subtype) {

    uint8_t count = 0;

    // Get an iterator to all partitions with the given subtype
    esp_partition_iterator_t iterator = esp_partition_find(ESP_PARTITION_TYPE_DATA, (esp_partition_subtype_t) subtype, NULL);
    if (iterator == NULL) return count;

    do {

        // Get the partition info
        const esp_partition_t* part = esp_partition_get(iterator);

        // Is it already in the pool?
        if (_exists(part->label)) continue;

        // Store in _user_defined_size the size of the smallest partition
        if ((0 == _user_defined_size) || (part->size < _user_defined_size)) {
            _user_defined_size = part->size;
        }

        // Create a record
        partition_t p;
        p.name = strdup(part->label);
        p.part = part;

        // Add it to the pool
        _partitions.push_back(p);

        count++;

    } while ((iterator = esp_partition_next(iterator)));

    // Release the iterator
    esp_partition_iterator_release(iterator);

    return count;

}

/**
 * @brief Defines the offset inside the memory buffer where the magic bytes will be stored.
 * The library uses 3 bytes to track last valid partition, so there must be at least 3
 * bytes available in the memory buffer from the offset onwards.
 * Must be called before the begin method but after adding all the partitions.
 * @param {uint8_t} offset      Offset
 * @returns {bool}              True if seccessfully set
 */
bool EEPROM32_Rotate::offset(uint16_t offset) {
    if (offset + 3 > _user_defined_size) return false;
    _offset = offset;
    return true;
}

/**
 * @brief Returns the number of partitions used for rotating EEPROM.
 * @returns {uint8_t}           Partition pool size
 */
uint8_t EEPROM32_Rotate::size() {
    return _partitions.size();
}

/**
 * @brief Returns the partition name for the give index.
 * @returns {char *}            Partition name
 */
const char * EEPROM32_Rotate::name(uint8_t index) {
    if (index >= _partitions.size()) return "";
    return _partitions[index].name;
}

/**
 * @brief Returns the partition name whose contents match those of the EEPROM data buffer.
 * @returns {char *}            Current partition name
 */
const char * EEPROM32_Rotate::current() {
    if (_partitions.size() == 0) return "";
    return _partitions[_partition_index].name;
}

/**
 * @brief Dumps the EEPROM data to the given stream in a human-friendly way.
 * @param {Stream &}  debug     Stream to dump the data to
 * @param {uint8_t} index       Partition index to dump (default to current partition)
 */
void EEPROM32_Rotate::dump(Stream & debug, uint8_t index) {

    if (index >= _partitions.size()) return;

    const esp_partition_t * part = _partitions[index].part;

    char ascii[17];
    memset(ascii, ' ', 16);
    ascii[16] = 0;

    debug.printf("\n         ");
    for (uint16_t i = 0; i <= 0x0F; i++) {
        debug.printf("%02X ", i);
    }
    debug.printf("\n------------------------------------------------------");

    uint8_t * data = new uint8_t[16];

    for (uint16_t address = 0; address < _size; address++) {

        if ((address % 16) == 0) {
            esp_partition_read(part, address, (void *) data, 16);
            if (address > 0) {
                debug.print(ascii);
                memset(ascii, ' ', 16);
            }
            debug.printf("\n0x%04X:  ", address);
        }

        uint8_t b = data[address % 16];
        if (31 < b && b < 127) ascii[address % 16] = (char) b;
        debug.printf("%02X ", b);

        yield();

    }

    delete[] data;

    debug.print(ascii);
    debug.printf("\n\n");

}

// -----------------------------------------------------------------------------
// OVERWRITTEN METHODS
// -----------------------------------------------------------------------------

/**
 * @brief Loads 'size' bytes of data into memory for EEPROM emulation from the
 * latest valid partition in the partition pool
 * @param {size_t} size         Data size to read
 */
void EEPROM32_Rotate::begin(size_t size) {

    if (_partitions.size() == 0) return;

    uint32_t best_index = 0;
    uint8_t best_value = 0xFF;
    bool first = true;

    for (uint32_t index = 0; index < _partitions.size(); index++) {

        // load the partition data
        _name = _partitions[index].name;
        EEPROMClass::begin(size);

        // get partition value
        uint8_t value = read(_offset + EEPROM32_ROTATE_COUNTER_OFFSET);
        DEBUG_EEPROM32_ROTATE("Magic value for partition #%u is %u\n", index, value);

        // validate content
        if (!_check_crc()) {
            DEBUG_EEPROM32_ROTATE("Partition #%u has not passed the CRC check\n", index);
            continue;
        }

        // if this is the first valid partitions we are reading
        if (first) {

            first = false;
            best_index = index;
            best_value = value;

        // else compare values
        } else {

            // This new partition is newer if...
            bool newer = ((value < best_value) and (best_value - value) > 128) or \
                         ((value > best_value) and (value - best_value) < 128);

            if (newer) {
                best_index = index;
                best_value = value;
            }

        }

    }

    // Re-read the data from the best index partition
    _partition_index = best_index;
    _name = _partitions[_partition_index].name;
    EEPROMClass::begin(size);
    _partition_value = best_value;

    DEBUG_EEPROM32_ROTATE("Current partition is #%u (%s)\n", _partition_index, _name);
    DEBUG_EEPROM32_ROTATE("Current magic value is #%u\n", _partition_value);

}

/**
 * @brief Writes data from memory to the next partition in the partition pool
 * and flags it as current partition.
 * @returns {bool}      True if successfully written
 */
bool EEPROM32_Rotate::commit() {

    if (_partitions.size() == 0) return false;

    // Check if we are really going to write
    if (!_user_defined_size) return false;
    if (!_dirty) return true;
    if (!_data) return false;

    // Backup current values
    uint8_t index_backup = _partition_index;
    uint8_t value_backup = _partition_value;

    // Update partition for next write
    _partition_index = (_partition_index + 1) % _partitions.size();
    _name = _partitions[_partition_index].name;
    _partition_value++;

    DEBUG_EEPROM32_ROTATE("Writing to partition #%u (%s)\n", _partition_index, _name);
    DEBUG_EEPROM32_ROTATE("Writing magic value #%u\n", _partition_value);

    // Update the counter & crc bytes
    uint16_t crc = _calculate_crc();
    write(_offset + EEPROM32_ROTATE_CRC_OFFSET, (crc >> 8) & 0xFF);
    write(_offset + EEPROM32_ROTATE_CRC_OFFSET + 1, crc & 0xFF);
    write(_offset + EEPROM32_ROTATE_COUNTER_OFFSET, _partition_value);

    // Perform the commit
    bool ret = EEPROMClass::commit();

    // If commit failed restore values
    if (!ret) {

        DEBUG_EEPROM32_ROTATE("Commit to partition #%u failed, restoring\n", _partition_index);

        // Restore values
        _partition_index = index_backup;
        _partition_value = value_backup;
        _name = _partitions[_partition_index].name;

    }

    return ret;

}

// -----------------------------------------------------------------------------
// PRIVATE METHODS
// -----------------------------------------------------------------------------

/**
 * @brief Checks if a partition is already in the pool
 * @param {const char *} name       Name to look for
 * @returns {bool}                  True if it is already in the pool
 * @protected
 */
bool EEPROM32_Rotate::_exists(const char * name) {
    for (uint8_t i=0; i<_partitions.size(); i++) {
        if (strcmp(name, _partitions[i].name) == 0) return true;
    }
    return false;
}

/**
 * @brief Calculates the CRC of the data in memory (except for the magic bytes)
 * @returns {uint16_t}          CRC
 * @protected
 */
uint16_t EEPROM32_Rotate::_calculate_crc() {
    uint16_t crc = 0;
    for (uint16_t address = 0; address < _user_defined_size; address++) {
        if (_offset <= address && address <= _offset + 2) continue;
        crc = crc + read(address);
    }
    return crc;
}

/**
 * @brief Compares the CRC of the data in memory against the stored one
 * @returns {bool}              True if they match, so data is OK
 * @protected
 */
bool EEPROM32_Rotate::_check_crc() {
    uint16_t calculated = _calculate_crc();
    uint16_t stored =
        (read(_offset + EEPROM32_ROTATE_CRC_OFFSET) << 8) +
        read(_offset + EEPROM32_ROTATE_CRC_OFFSET + 1);
    DEBUG_EEPROM32_ROTATE("Calculated CRC: 0x%04X\n", calculated);
    DEBUG_EEPROM32_ROTATE("Stored CRC    : 0x%04X\n", stored);
    return (calculated == stored);
}
