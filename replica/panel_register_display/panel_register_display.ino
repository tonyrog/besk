// BESK panel lights - swedens first "real" (non relay based) computer 
//  MD     0 1 2 3     ... 36 37 38 39
//  MR     0 1 2 3     ... 36 37 38 39
//  AR  00 0 1 2 3     ... 36 37 38 39  40
//  AS     11 ... 0  7...0 OP  11...0 KR
//  150 LEDS!  (add extra "unkown" 13 more)
//

#define LED_MD_0  0    // 40 leds 0..39
#define LED_MR_0  40   // 40 leds 40..79
#define LED_AR_00 80   //  1 led 80
#define LED_AR_0  81   // 40 leds 81..120
#define LED_AR_40 121  //  1 led 121
#define LED_AS_11 122  // 12 leds 122..133
#define LED_OP_7  134  //  8 leds 134..141
#define LED_KR_11 142  // 12 leds 142..153

typedef uint64_t helord_t;  // 40 bits used bit 39 is sign bit (bit 0) bit 0 is 2^-39 (bit 39)

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN    2

// How many NeoPixels are attached to the Arduino? (currently using 154)
#define LED_COUNT (5*60)

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

uint32_t orange = strip.Color(0xff, 0x44, 0x00);
uint32_t black = strip.Color(0x00, 0x00, 0x00); // ? or is there a off?
uint32_t green = strip.Color(0x00, 0xff, 0x00);
uint32_t red = strip.Color(0xff, 0x00, 0x00);

helord_t MD = 0xF888801234;
helord_t MR = 0;
helord_t AR = 0x5555555555;
uint8_t  AR_00 = 1;
uint8_t  AR_40 = 0;
uint16_t AS = 0x108;        // address
uint8_t  OP = 0x56;        // 8 bit (7-bit used)
uint16_t KR = 0x100;        // IP - kontroll register

void draw_bit(int p, unsigned int x)
{
  strip.setPixelColor(p, x ? orange : black);
}

// Draw besk register (fixnum bit0 is sign bit)
void draw_besk_reg(helord_t x, int p)
{
  int i;

  for (i = 0; i < 40; i++) {
      draw_bit(i+p, (x >> 39) & 1);
      x <<= 1;
  }
}

// Draw regular register (n-bits)
void draw_reg(unsigned int x, int p, int n)
{
  int i;

  for (i = n-1; i >= 0; i--) {
      draw_bit(i+p, (x & 1));
  }
}

void draw_state()
{
  draw_besk_reg(MD, LED_MD_0);
  draw_besk_reg(MR, LED_MR_0);
  draw_reg(AS, LED_AS_11, 12); draw_reg(OP, LED_OP_7,8); draw_reg(KR, LED_KR_11, 12);

  // strip.setPixelColor(0, green);
  strip.setPixelColor(154, red);
}

// setup() function -- runs once at startup --------------------------------

void setup() {
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
}


// loop() function -- runs repeatedly as long as board is on ---------------

void loop() {
  // Fill along the length of the strip in various colors...
  AR ^= 0xAAAAAAAAAA;
  MD = (MD + 1) & 0xFFFFFFFFFF;
  MR = (MR + 7) & 0xFFFFFFFFFF;
  AS = (AS + 1) & 0x7FF;
  OP = (OP + 3) & 0x7F;
  KR = (KR + 1) & 0x7FF;
  draw_state();
  strip.show(); 
  delay(100);
}

