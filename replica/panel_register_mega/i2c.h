//
// Bit Bang I2C library
// Copyright (c) 2018 BitBank Software, Inc.
// Written by Larry Bank (bitbank@pobox.com)
// Project started 10/12/2018
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef __I2C__
#define __I2C__

// On Linux, use it as C code, not C++
#if !defined(ARDUINO) && defined(__cplusplus)
extern "C" {
#endif

typedef struct
{
    uint8_t sda_pin;
    uint8_t sda_state;
    uint8_t scl_pin;
    // uint8_t pad;
    uint32_t delay;
} i2c_t;

//
// Initialize the I2C BitBang library
// Pass the pin numbers used for SDA and SCL
// as well as the clock rate in Hz
//
void i2c_init(i2c_t* i2c, uint32_t speed);
int i2c_begin(i2c_t* i2c, uint8_t addr, uint8_t read);
void i2c_end(i2c_t* i2c);

int i2c_write_byte(i2c_t* i2c, uint8_t val);
//
// Read N bytes
//
int i2c_read(i2c_t* i2c, uint8_t addr, uint8_t* data, int len);
//
// Read N bytes starting at a specific I2C internal register
//
int i2c_read_register(i2c_t* i2c, uint8_t addr, uint8_t reg, uint8_t* data, int len);
//
// Write I2C data
// quits if a NACK is received and returns 0
// otherwise returns the number of bytes written
//
int i2c_write(i2c_t* i2c, uint8_t addr, uint8_t* data, int len);
//
// Scans for I2C devices on the bus
// returns a bitmap of devices which are present (128 bits = 16 bytes, LSB first)
//
// Test if an address responds
// returns 0 if no response, 1 if it responds
//
uint8_t i2c_test(i2c_t* i2c, uint8_t addr);

// A set bit indicates that a device responded at that address
//
void i2c_scan(i2c_t* i2c, uint8_t* map);

#if !defined(ARDUINO) && defined(__cplusplus)
}
#endif

#endif //__BITBANG_I2C__

