// -*- c++ -*-
// Control 18 hex displays using 4 I2C wire buses

#include "i2c.h"

typedef struct {
    i2c_t bus;
    char* name;  // name of display type
    int addr;    // first address
    int n;       // number of displays found on bus
    uint8_t map; // bitmap of displays found
    uint32_t speed; 
} bus_t;


// #define I2C_DEMO
#define SPEED 100000

i2c_t   ARl_bus;   // i2c bus for ARl displays
int     ARl_n;     // number of ARl displays found
uint8_t ARl_map;   // bit map of ARl displays found


#define ARl_scl   22
#define ARl_sda   23
#define ARl1_pwr  24
#define ARl2_pwr  25
#define ARl3_pwr  26
#define ARl4_pwr  27
#define ARl5_pwr  28
// avail 29
#define ARl_SPEED SPEED

i2c_t   ARh_bus;     // i2c bus for ARh displays
int     ARh_n;     // number of ARh displays found
uint8_t ARh_map;   // bit map of ARh displays found

#define ARh_scl   30
#define ARh_sda   31
#define ARh1_pwr  32
#define ARh2_pwr  33
#define ARh3_pwr  34
#define ARh4_pwr  35
#define ARh5_pwr  36
// avail 37
#define ARh_SPEED SPEED

i2c_t   AS_bus;
int     AS_n;
uint8_t AS_map;
int     OP_n;
uint8_t OP_map;

#define AS_scl   38
#define AS_sda   39
#define AS1_pwr  40
#define AS2_pwr  41
#define AS3_pwr  42
#define OP1_pwr  43
#define OP2_pwr  44
// avail 45
#define AS_SPEED SPEED

i2c_t   KR_bus;
int     KR_n;
uint8_t KR_map;

#define KR_scl   46
#define KR_sda   47
#define KR1_pwr  48
#define KR2_pwr  49
#define KR3_pwr  50
// avail 51,52,53

#define KR_SPEED SPEED

int ARl_pwr_pin[5] = { ARl1_pwr, ARl2_pwr, ARl3_pwr, ARl4_pwr, ARl5_pwr};
int ARh_pwr_pin[5] = { ARh1_pwr, ARh2_pwr, ARh3_pwr, ARh4_pwr, ARh5_pwr};
int AS_pwr_pin[5]  = { AS1_pwr, AS2_pwr, AS3_pwr, OP1_pwr, OP2_pwr};
int KR_pwr_pin[5]  = { KR1_pwr, KR2_pwr, KR3_pwr, 0, 0};

uint32_t ARl = 0x00000; // 20 bit value
uint32_t ARh = 0x00000; // 20 bit value
uint16_t AS  = 0x123;   // 12 bit value
uint8_t  OP  = 0x2A;    // 8 bit value
uint16_t KR  = 0x120;   // 12 bit value

/*
bus_t bus[] = {
	{ &ARl_bus, "ARl", 0, 5, &ARl_map, ARl_SPEED },
	{ &ARh_bus, "ARh", 0, 5, &ARh_map, ARh_SPEED },
	{ &AS_bus,  "AS",  0, 5, &AS_map,  AS_SPEED  },
	{ &KR_bus,  "KR",  0, 3, &KR_map,  KR_SPEED  },
	{ NULL, NULL, 0, 0, NULL, 0 }
};
*/


void setup_i2c(i2c_t* i2c, int scl, int sda, uint32_t speed)
{
    memset(i2c, 0, sizeof(i2c_t));
    i2c->sda_pin = sda;
    i2c->scl_pin = scl;
    i2c_init(i2c, speed);    
}

void setup_pwr_pins(int pin[5])
{
    int i;
    for (i = 0; i < 5; i++) {
        if (pin[i] != 0) {
	    pinMode(pin[i], OUTPUT);
	    digitalWrite(pin[i], LOW);
	}
    }
}

int probe_displays(i2c_t* i2c, const char *regname, int addr, int num,
		   uint8_t* map)
{
    int found = 0;
    *map = 0;
    while (num > 0) {
	if (i2c_test(i2c, 0x20+addr)) {
	    Serial.print("Found display ");
	    Serial.print(regname);
	    Serial.print(" ");
	    Serial.println(addr);
	    *map |= (1 << addr);
	    found++;
	}
	addr++;
	num--;
    }
    return found;
}

int pcf8575_set_mask(i2c_t* i2c, int addr, uint16_t mask)
{
    int r;
    uint8_t buf[2];
    buf[0] = mask & 0xFF;
    buf[1] = mask >> 8;
    r = i2c_write(i2c, 0x20+addr, buf, 2);
    return r;
}

int pcf8575_get_mask(i2c_t* i2c, int addr, uint16_t* mask)
{
    uint8_t buf[2];
    int r = i2c_read(i2c, 0x20+addr, buf, 2);
    *mask = buf[0] | (buf[1] << 8);
    return r;
}

// write value of nbits to displays addr0-addr1 write value
// low nibble first
void write_reg(i2c_t* i2c, uint8_t n, uint8_t map,
	       uint32_t val, int nbits, int addr)
{
    if (n) {
	while(nbits > 0) {
	    if (map & (1 << addr)) {
		uint16_t mask = (1 << (val & 0xf));
		int r = pcf8575_set_mask(i2c, addr, ~mask);
		if (r == 2) {
		    Serial.print("Wrote ");
		    Serial.println(mask, HEX);
		}
	    }
	    val >>= 4;
	    nbits -= 4;
	    addr++;
	}
    }
}

void setup()
{
    Serial.begin(115200);

    pinMode(13, OUTPUT);    // activity LED
    digitalWrite(13, LOW);  // turn off LED
    setup_i2c(&ARh_bus, ARh_scl, ARh_sda, ARh_SPEED);
    setup_i2c(&ARl_bus, ARl_scl, ARl_sda, ARl_SPEED);
    setup_i2c(&AS_bus, AS_scl, AS_sda, AS_SPEED);
    setup_i2c(&KR_bus, KR_scl, KR_sda, KR_SPEED);

    setup_pwr_pins(ARl_pwr_pin);
    setup_pwr_pins(ARh_pwr_pin);
    setup_pwr_pins(AS_pwr_pin);
    setup_pwr_pins(KR_pwr_pin);
}

void init_displays()
{
    int addr;
    Serial.println("Probe start");

    ARl_n = probe_displays(&ARl_bus, "ARl", 0, 5, &ARl_map);
    ARh_n = probe_displays(&ARh_bus, "ARh", 0, 5, &ARh_map);
    AS_n  = probe_displays(&AS_bus,  "AS",  0, 3,  &AS_map);
    OP_n  = probe_displays(&AS_bus,  "OP",  3, 2,  &OP_map);
    KR_n  = probe_displays(&KR_bus,  "KR",  0, 3,  &KR_map);

    Serial.print("Probe done, found ");
    Serial.print(ARl_n+ARh_n+AS_n+OP_n+KR_n);
    Serial.println(" displays");

    if (ARl_n) {
	for (addr = 0; addr < 5; addr++) {
	    if (ARl_map & (1 << addr))
		pcf8575_set_mask(&ARl_bus, addr, 0xffff);
	}
    }
    if (ARh_n) {
	for (addr = 0; addr < 5; addr++) {
	    if (ARh_map & (1 << addr))
		pcf8575_set_mask(&ARh_bus, addr, 0xffff);
	}
    }
    if (AS_n) {
	for (addr = 0; addr < 3; addr++) {
	    if (AS_map & (1 << addr))
		pcf8575_set_mask(&AS_bus, addr, 0xffff);
	}
    }
    if (OP_n) {
	for (addr = 3; addr < 5; addr++) {
	    if (OP_map & (1 << addr))
		pcf8575_set_mask(&AS_bus, addr, 0xffff); // OP is same bus as AS
	}
    } 
    if (KR_n) {
	for (addr = 0; addr < 3; addr++)
	    if (KR_map & (1 << addr))
		pcf8575_set_mask(&KR_bus, addr, 0xffff);
    }
}


void loop()
{
    static int once = 1;
    static int state = HIGH;
    if (once) {
	once = 0;
	digitalWrite(13, HIGH);  // turn off LED
	init_displays();
	state = LOW;
    }
    digitalWrite(13, state);
    write_reg(&ARl_bus, ARl_n, ARl_map, ARl, 20, 0);
    write_reg(&ARh_bus, ARh_n, ARh_map, ARh, 20, 0);
    write_reg(&AS_bus,  AS_n, AS_map, AS,  12, 0);
    write_reg(&AS_bus,  OP_n, OP_map, OP,   8, 3);
    write_reg(&KR_bus,  KR_n, KR_map, KR,  12, 0);
    // put your main code here, to run repeatedly:
    state = !state;
    digitalWrite(13, state);  // turn off LED

    // DEMO only
    ARl = (ARl+1) & 0xfffff;
    if ((ARl & 0x0001F) == 0) {
	// switch between two digit 1 and 2
	ARh = (ARh < 2) ? 2 : 1;
    }
    /*    
    AS = (AS+1) & 0x7ff;
    OP ^= 0x15;
    KR = (AS & 0x400) ? AS : KR+1;
    */
    delay(100);
}
