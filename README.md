# EEPROM32 Rotate

This is a wrapper around the Arduino Core for ESP32 EEPROM library that handles partition rotating while keeping (almost) full compatibility with the original API.

If you are using the ESP8266, visit the repository for the ESP8266 version of this library here: https://github.com/xoseperez/eeprom_rotate

[![version](https://img.shields.io/badge/version-0.9.4-brightgreen.svg)](CHANGELOG.md)
[![travis](https://travis-ci.org/xoseperez/eeprom32_rotate.svg?branch=master)](https://travis-ci.org/xoseperez/eeprom32_rotate)
[![codacy](https://img.shields.io/codacy/grade/73a1774d4563493dbad4ebfaa55e0568/master.svg)](https://www.codacy.com/app/xoseperez/eeprom32_rotate/dashboard)
[![license](https://img.shields.io/github/license/xoseperez/EEPROM32_Rotate.svg)](LICENSE)
<br />
[![donate](https://img.shields.io/badge/donate-PayPal-blue.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=xose%2eperez%40gmail%2ecom&lc=US&no_note=0&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donate_LG%2egif%3aNonHostedGuest)
[![twitter](https://img.shields.io/twitter/follow/xoseperez.svg?style=social)](https://twitter.com/intent/follow?screen_name=xoseperez)

The emulated EEPROM in the ESP32 uses one SPI flash memory partition to store the data. Due to the nature of this flash memory (NOR) a full partition erase must be done prior to write any new data. If a power failure (intended or not) happens during this process the partition data is lost.

Also, writing data to a NOR memory can be done byte by byte but only to change a 1 to a 0. The only way to turn 0s to 1s is to perform a sector erase which turns all memory positions in that sector to 1. But sector erasing must be done in full sectors, thus wearing out the flash memory faster.

A way to overcome this is to use more than one partition to store data and check on boot which one has the latest valid data.

This is what this library does.

**NOTICE**: Please note EEPROM emulation in the ESP32 is no longer maintained and only supported for backwards compatibility. Moreover, the latest 1.0.3 SDK version breaks this library. Therefore, this library will no longer be maintained either.

## How does it work?

Instead of using a single partition to persist the data from the emulated EEPROM, this library uses a number of partitions to do so: a partition pool.

The library overwrites two methods of the original one: `begin` and `commit`.

The `begin` method will load the data from all the partitions in the partition pool one after the other trying to figure out which one has the **latest valid information**. To do
this it checks two values:

* A 2-bytes CRC
* A 1-byte auto-increment number

These values are stored in a certain position in the partition (at the very beginning by default but the user can choose another position with the `offset` method).

The CRC is calculated based on the contents of the partition (except for those special 3 bytes). If the calculated CRC matches that stored in the partition then the library checks the auto-increment and selects the partition with the most recent number (taking overflows into account, of course).

Those special values are stored by the overwritten `commit` method prior to the actual commit.

With every commit, the library will hop to the next partition. This way, in case of a power failure in the middle of a commit, the CRC for that partition will fail and the library will use the data in the latest known-good partition.

Notice this is not the same as using different EEPROM objects. Because they are isolated instances with different data and each one of them uses memory to store the contents of SPIFFS, thus eating a lot of RAM. This library creates one single object and one single memory buffer instead, but rotates the backend partition used to persist the data.

## Partition table

This library (in practice) requires a custom partition table to work. You could use any partition in the default partition scheme but you will want to create a custom one defining as many partitions for EEPROM as you'd like to.

This is not difficult at all, using PlatformIO. If anyone knows how to do it with the Arduino IDE (not touching the core files) then, please, let me know.

### PlatformIO

PlatformIO lets you define a CSV file for each environment in your platformio.ini file. Like this:

```
[env:nano32]
platform = espressif32
board = nano32
framework = arduino
board_build.partitions = partition-table.csv
build_flags = -DNO_GLOBAL_EEPROM
```

This CSV file defines the different partitions in the SPI flash memory:

```
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x140000,
app1,     app,  ota_1,   0x150000,0x140000,
eeprom0,  data, 0x99,    0x290000,0x1000,
eeprom1,  data, 0x99,    0x291000,0x1000,
spiffs,   data, spiffs,  0x292000,0x16E000,
```

Here you can see that we have added two partitions for EEPROM (eeprom0 and eeprom1). You will be adding later these partitions to the EEPROM_Rotate partition pool like this:

```
EEPROM32_Rotate EEPROM;
EEPROM.add_by_name("eeprom0");
EEPROM.add_by_name("eeprom1");
```

Now your emulated EEPROM will be rotating between those two partitions, reducing the wearing out of the flash memory and preventing failure.

## API

The library inherits form the Arduino Core for ESP32 EEPROM library, but it uses a slightly different API. Differences are in the constructor. The original EEPROM library for ESP32 has different constructor signatures but only valid at the moment is:

```
EEPROMClass EEPROM("eeprom0");
```

To make this library API more consistent, I have decided to change the way you create and 'populate' an object:

```
EEPROM32_Rotate EEPROM;
EEPROM.add_by_name("eeprom0");
```

Now you may find obvious how to add more partitions to the pool.

```
EEPROM32_Rotate EEPROM;
EEPROM.add_by_name("eeprom0");
EEPROM.add_by_name("eeprom1");
EEPROM.add_by_name("eeprom2");
EEPROM.add_by_name("eeprom3");
```

Actually, if all partitions have the same data subtype (usually 0x99, thou this is only a convention) then it's a lot easier to add them all:

```
EEPROM32_Rotate EEPROM;
EEPROM.add_by_subtype(0x99);
```

Now you can use it like a regular EEPROM object:

```
EEPROM32_Rotate EEPROM;
EEPROM.add_by_subtype(0x99);
uint8_t b = EEPROM.read(0);
EEPROM.write(0, b+1);
EEPROM.commit();
```

Remember than the stock EEPROM library for ESP32 has a bunch of convenient methods like `readLong`, `readBytes`, `writeString`,... you can also use those!

In addition, the EEPROM32_Rotate library exposes a set of new methods to configure the partition rotating and performing other special actions:

#### uint8_t add_by_name(const char* name)

Adds a new partition to the partition pool using its name. Returns 1 if successfully added, 0 otherwise. If a partition is already in the pool it's not added again.

Please notice that the 'length()' method of the class will return the size of the smallest partition. Also, notice that the size of the memory buffer must not be bigger than the smallest partition. There is no check for this.

#### bool add_by_subtype(uint8_t subtype)

Adds all partitions with the given subtype to the pool. Returns the number of partitions added. If a partition is already in the pool it's not added again.

Please notice that the 'length()' method of the class will return the size of the smallest partition. Also, notice that the size of the memory buffer must not be bigger than the smallest partition. There is no check for this.

#### void offset(uint8_t offset)

Define the offset in the memory buffer where the special auto-increment and CRC values will be stored. The default value is 0. This special data uses 3 bytes of space in the emulated EEPROM memory buffer. This value must not be bigger than the size of the memory buffer minus 3 bytes.

#### uint8_t size()

Returns the number of partitions used for rotating EEPROM.

#### char * name(uint8_t index)

Returns the name of the n-th partition in the pool.

#### char * current()

Returns the name of the current partition, that whose contents match those of the EEPROM memory buffer.

#### void dump(Stream & debug, uint8_t index) | void dump(Stream & debug)

Dumps the EEPROM data to the given stream in a human-friendly way. You can specify the index for the partition to dump the data, otherwise it will use the current partition. It will only dump the size of the memory buffer.

## Notes

### Disabling the original global EEPROM object

The original EEPROM library automatically instantiates an EEPROM object that's
already available to use. This consumes little memory (since the data buffer is
only created and populated when calling `begin`). But anyway if you don't want to
have a unused object around you can disable the object instantiation by using
the `NO_GLOBAL_EEPROM` build flag.

## License

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
