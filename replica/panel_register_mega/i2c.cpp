//
// Bit Bang I2C library
// Copyright (c) 2018-2019 BitBank Software, Inc.
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

#include <Arduino.h>

#include "i2c.h"

// define to enable debug output on the SCL/SDA pin without loading the bus
#define I2C_DEBUG
#define INPUT_MODE INPUT_PULLUP
//
// Initialize the I2C BitBang library
// Pass the pin numbers used for SDA and SCL
// as well as the clock rate in Hz
//
void i2c_init(i2c_t* i2c, uint32_t speed)
{
    if (i2c == NULL)
	return;
    pinMode(i2c->sda_pin, OUTPUT);
    pinMode(i2c->scl_pin, OUTPUT);
    digitalWrite(i2c->sda_pin, LOW); // setting low = enabling as outputs
    digitalWrite(i2c->scl_pin, LOW);
#ifndef I2C_DEBUG
    pinMode(i2c->sda_pin, INPUT);    // let the lines float (tri-state)
    pinMode(i2c->scl_pin, INPUT);
#endif
    // For now, we only support 100, 400 or 800K clock rates
    // all other values default to 100K
    if (speed >= 1000000)
	i2c->delay = 0; // the code execution is enough delay
    else if (speed >= 800000)
	i2c->delay = 1;
    else if (speed >= 400000)
	i2c->delay = 2;
    else if (speed >= 100000)
	i2c->delay = 10;
    else
	i2c->delay = (uint16_t)(1000000 / speed);
}

inline void scl_high(uint8_t pin)
{
#ifdef I2C_DEBUG
    digitalWrite(pin, HIGH);
#else
    pinMode(pin, INPUT_MODE);
#endif
}

inline void scl_low(uint8_t pin)
{
#ifdef I2C_DEBUG    
    digitalWrite(pin, LOW);
#else
    pinMode(pin, OUTPUT);
#endif
}

inline uint8_t sda_read(uint8_t pin, uint8_t* state)
{
    uint8_t r;
#ifdef I2C_DEBUG
    pinMode(pin, INPUT_MODE);
    r = digitalRead(pin);  // not really
    pinMode(pin, OUTPUT);
#else
    // assume the pin is already an input
    r = digitalRead(pin);
#endif
    return r;
}

inline void sda_high(uint8_t pin, uint8_t* state)
{
    if (*state == 0) {
#ifdef I2C_DEBUG
	digitalWrite(pin, HIGH);
#else
	pinMode(pin, INPUT_MODE);
#endif
	*state = 1;
    }
}

inline void sda_low(uint8_t pin, uint8_t* state)
{
    if (*state == 1) {
#ifdef I2C_DEBUG
	digitalWrite(pin, LOW);
#else
	pinMode(pin, OUTPUT);
#endif
	*state = 0; // eliminate glitches
    }
}

static inline int i2c_byte_out(i2c_t *i2c, uint8_t b)
{
    uint8_t i, ack;
    uint8_t sda_pin = i2c->sda_pin;
    uint8_t scl_pin = i2c->scl_pin; // in case of bad C compiler
    uint8_t sda_state = i2c->sda_state;
    uint32_t delay = i2c->delay;
    
    for (i=0; i<8; i++) {
	if (b & 0x80)
	    sda_high(sda_pin,&sda_state); // set data line to 1
	else
	    sda_low(sda_pin,&sda_state); // set data line to 0
	scl_high(scl_pin); // clock high (slave latches data)
	delayMicroseconds(delay);
	scl_low(scl_pin); // clock low
	b <<= 1;
	delayMicroseconds(delay);
    }
    // read ack bit
    sda_high(sda_pin, &sda_state); // set data line for reading
    scl_high(scl_pin); // clock line high
    delayMicroseconds(delay);
    ack = sda_read(sda_pin, &sda_state);
    scl_low(scl_pin); // clock low
    delayMicroseconds(delay);
    sda_low(sda_pin, &sda_state); // data low
    i2c->sda_state = sda_state;
    return (ack == 0) ? 1:0; // a low ACK bit means success
}

// send bit b 8 times to the I2C bus
static inline int i2c_byte_out_fast(i2c_t* i2c, uint8_t b)
{
    uint8_t i, ack;
    uint8_t sda_pin = i2c->sda_pin;
    uint8_t scl_pin = i2c->scl_pin; // in case of bad C compiler
    uint8_t sda_state = i2c->sda_state;
    uint32_t delay = i2c->delay;
    
    if (b & 0x80)
	sda_high(sda_pin,&sda_state); // set data line to 1
    else
	sda_low(sda_pin,&sda_state); // set data line to 0
    for (i=0; i<8; i++) {
	scl_high(scl_pin); // clock high (slave latches data)
	delayMicroseconds(delay);
	scl_low(scl_pin); // clock low
	delayMicroseconds(delay);
    }
    // read ack bit
    sda_high(sda_pin,&sda_state); // set data line for reading
    scl_high(scl_pin); // clock line high
    delayMicroseconds(delay);
    ack = sda_read(sda_pin,&sda_state);
    scl_low(scl_pin); // clock low
    delayMicroseconds(delay); // DEBUG - delay/2
    sda_low(sda_pin,&sda_state); // data low
    i2c->sda_state = sda_state;
    return (ack == 0) ? 1:0; // a low ACK bit means success
}

//
// Receive a byte and read the ack bit
// if we get a NACK (negative acknowledge) return 0
// otherwise return 1 for success
//
static inline uint8_t i2c_byte_in(i2c_t* i2c, uint8_t last)
{
    uint8_t i;
    uint8_t sda_pin = i2c->sda_pin;
    uint8_t scl_pin = i2c->scl_pin; // in case of bad C compiler
    uint8_t sda_state = i2c->sda_state;
    uint32_t delay = i2c->delay;    
    uint8_t b = 0;

    sda_high(sda_pin,&sda_state); // set data line as input
    for (i=0; i<8; i++) {
	delayMicroseconds(delay); // wait for data to settle
	scl_high(scl_pin); // clock high (slave latches data)
	b <<= 1;
	if (sda_read(sda_pin,&sda_state) != 0) // read the data bit
	    b |= 1; // set data bit
	scl_low(scl_pin); // cloc low
    } // for i
    if (last)
        sda_high(sda_pin,&sda_state); // last byte sends a NACK
    else
        sda_low(sda_pin,&sda_state);
    scl_high(scl_pin); // clock high
    delayMicroseconds(delay);
    scl_low(scl_pin); // clock low to send ack
    delayMicroseconds(delay);
    sda_low(sda_pin,&sda_state); // data low
    i2c->sda_state = sda_state;    
    return b;
}

inline int i2c_begin(i2c_t *i2c, uint8_t addr, uint8_t rd)
{
    sda_low(i2c->sda_pin,&i2c->sda_state); // data line low first
    delayMicroseconds(i2c->delay);
    scl_low(i2c->scl_pin); // then clock line low is a START signal
    addr <<= 1;
    if (rd)
	addr++; // set read bit
    // send the slave address and R/W bit
    return i2c_byte_out(i2c, addr);
}

//
// Send I2C STOP condition
//
inline void i2c_end(i2c_t* i2c)
{
    uint8_t sda_pin = i2c->sda_pin;
    uint8_t scl_pin = i2c->scl_pin; // in case of bad C compiler
    uint8_t sda_state = i2c->sda_state;
    uint32_t delay = i2c->delay;    

    sda_low(sda_pin, &sda_state); // data line low
    delayMicroseconds(delay);
    scl_high(scl_pin); // clock high
    delayMicroseconds(delay);
    sda_high(sda_pin, &sda_state); // data high
    delayMicroseconds(delay);
    i2c->sda_state = sda_state;
}

inline int i2c_write_byte(i2c_t* i2c, uint8_t val)
{
    if (val == 0xff || val == 0)
	return i2c_byte_out_fast(i2c, val);
    else
	return i2c_byte_out(i2c, val);
}


static inline int i2c_write(i2c_t* i2c, uint8_t* data, int len)
{
    int rc = 1;
    int old_len = len;

    while (len && (rc == 1)) {
	uint8_t b = *data++;
	rc = i2c_write_byte(i2c, b);
	if (rc == 1) // success
	    len--;
    }
    return (rc == 1) ? (old_len - len) : 0; // 0 indicates bad ack from sending a byte
}

static inline void i2c_read(i2c_t* i2c, uint8_t* data, int len)
{
    while (len--)
	*data++ = i2c_byte_in(i2c, (len == 0));
}


//
// Test a specific I2C address to see if a device responds
// returns 0 for no response, 1 for a response
//
uint8_t i2c_test(i2c_t *i2c, uint8_t addr)
{
    uint8_t response = 0;

    if (i2c_begin(i2c, addr, 0)) // try to write to the given address
	response = 1;
    i2c_end(i2c);
    return response;
}

//
// Scans for I2C devices on the bus
// returns a bitmap of devices which are present (128 bits = 16 bytes, LSB first)
// A set bit indicates that a device responded at that address
//
void i2c_scan(i2c_t *i2c, uint8_t *map)
{
    int i;
    for (i=0; i<16; i++) // clear the bitmap
	map[i] = 0;
    for (i=1; i<128; i++) { // try every address
	if (i2c_test(i2c, i)) {
	    map[i >> 3] |= (1 << (i & 7));
	}
    }
}

//
// Write I2C data
// quits if a NACK is received and returns 0
// otherwise returns the number of bytes written
//
int i2c_write(i2c_t *i2c, uint8_t addr, uint8_t* data, int len)
{
    int rc = 0;
  
    rc = i2c_begin(i2c, addr, 0);
    if (rc == 1) // slave sent ACK for its address
	rc = i2c_write(i2c, data, len);
    i2c_end(i2c);
    return rc; // returns the number of bytes sent or 0 for error
}

//
// Read N bytes starting at a specific I2C internal register
// returns 1 for success, 0 for error
//
int i2c_read_register(i2c_t* i2c, uint8_t addr, uint8_t reg, uint8_t* data, int len)
{
    int rc;
    rc = i2c_begin(i2c, addr, 0); // start a write operation
    if (rc == 1) { // slave sent ACK for its address
	rc = i2c_write(i2c, &reg, 1); // write the register we want to read from
	if (rc == 1) {
	    i2c_end(i2c);
	    rc = i2c_begin(i2c, addr, 1); // start a read operation
	    if (rc == 1)
		i2c_read(i2c, data, len);
	}
    }
    i2c_end(i2c);
    return rc; // returns 1 for success, 0 for error
}
//
// Read N bytes
//
int i2c_read(i2c_t* i2c, uint8_t addr, uint8_t* data, int len)
{
    int rc;
    rc = i2c_begin(i2c, addr, 1);
    if (rc == 1) // slave sent ACK for its address
	i2c_read(i2c, data, len);
    i2c_end(i2c);
    return rc; // returns 1 for success, 0 for error
}
