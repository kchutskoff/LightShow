
/*
This is an example of how simple driving a Neopixel can be
This code is optimized for understandability and changability rather than raw speed
More info at https://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/
*/

// Change this to be at least as long as your pixel string (too long will work fine, just be a little slower)

#include "MessageHandler.h"
#include "HSV.h"
#include <eeprom.h>
#define PIXELS 106  // Number of pixels in the string
#define PIXELS_LESS_SPECIAL 103
#define SPECIAL_1_SAME 100
#define SPECIAL_2_SAME 76
#define SPECIAL_3_SAME 88

// These values depend on which pin your string is connected to and what board you are using 
// More info on how to find these at http://www.arduino.cc/en/Reference/PortManipulation

// These values are for the pin that connects to the Data Input pin on the LED strip. They correspond to...

// Arduino Yun:     Digital Pin 8
// DueMilinove/UNO: Digital Pin 12
// Arduino MeagL    PWM Pin 4

// You'll need to look up the port/bit combination for other boards. 


// steps of 50, 200 = 0, 1000 = 15, 15 steps
// value is ((200 - H) / 50) + 16 * ((200 - L) / 50)
// some values will be un-needed, some will also be congruent (65ns per nop)

// t1h = 550ns to 850ns		t1l = 450ns to 750ns
// t0h = 200 to 500ns		t0l = 650ns to 950ns

// h+l = 650ns to 1850ns
// pref high = 800 + 600
// pref low = 300 + 800

// t0 = 200-500 + 650-950
// t1 = 550-850 + 450-750

// Note that you could also include the DigitalWriteFast header file to not need to to this lookup.

#define PIXEL_PORT  PORTD  // Port of the pin the pixels are connected to
#define PIXEL_DDR   DDRD   // Port of the pin the pixels are connected to
#define PIXEL_BIT   6      // Bit of the pin the pixels are connected to

#pragma region LED VARIABLE TIMING DECLARATIONS

#define NS_PER_SEC (1000000000L)          // Note that this has to be SIGNED since we want to be able to check for negative values of derivatives

#define CYCLES_PER_SEC (F_CPU)

#define NS_PER_CYCLE ( NS_PER_SEC / CYCLES_PER_SEC )

#define NS_TO_CYCLES(n) ( (n) / NS_PER_CYCLE )

#define LED_SEND_SIGNAL(SEND_PORT, SEND_BIT, ON_TIME, OFF_TIME) do{ asm volatile ( "sbi %[port], %[bit] \n\t.rept %[onCycles] \n\tnop \n\t.endr \n\tcbi %[port], %[bit] \n\t.rept %[offCycles] \n\tnop \n\t.endr \n\t" :: [port] "I" (_SFR_IO_ADDR(SEND_PORT)), [bit] "I" (SEND_BIT), [onCycles] "I" (NS_TO_CYCLES(ON_TIME) - 2), [offCycles] "I" (NS_TO_CYCLES(OFF_TIME) - 2) ); }while(0)

extern "C" {
	typedef void(*LEDTimingFunc) (void);
}

void LED_SendSignal_200_650(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 200, 650); }	void LED_SendSignal_200_700(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 200, 700); }	void LED_SendSignal_200_750(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 200, 750); }	void LED_SendSignal_200_800(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 200, 800); }	void LED_SendSignal_200_850(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 200, 850); }	void LED_SendSignal_200_900(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 200, 900); }	void LED_SendSignal_200_950(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 200, 950); }
void LED_SendSignal_250_650(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 250, 650); }	void LED_SendSignal_250_700(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 250, 700); }	void LED_SendSignal_250_750(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 250, 750); }	void LED_SendSignal_250_800(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 250, 800); }	void LED_SendSignal_250_850(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 250, 850); }	void LED_SendSignal_250_900(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 250, 900); }	void LED_SendSignal_250_950(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 250, 950); }
void LED_SendSignal_300_650(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 300, 650); }	void LED_SendSignal_300_700(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 300, 700); }	void LED_SendSignal_300_750(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 300, 750); }	void LED_SendSignal_300_800(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 300, 800); }	void LED_SendSignal_300_850(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 300, 850); }	void LED_SendSignal_300_900(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 300, 900); }	void LED_SendSignal_300_950(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 300, 950); }
void LED_SendSignal_350_650(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 350, 650); }	void LED_SendSignal_350_700(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 350, 700); }	void LED_SendSignal_350_750(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 350, 750); }	void LED_SendSignal_350_800(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 350, 800); }	void LED_SendSignal_350_850(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 350, 850); }	void LED_SendSignal_350_900(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 350, 900); }	void LED_SendSignal_350_950(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 350, 950); }
void LED_SendSignal_400_650(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 400, 650); }	void LED_SendSignal_400_700(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 400, 700); }	void LED_SendSignal_400_750(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 400, 750); }	void LED_SendSignal_400_800(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 400, 800); }	void LED_SendSignal_400_850(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 400, 850); }	void LED_SendSignal_400_900(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 400, 900); }	void LED_SendSignal_400_950(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 400, 950); }
void LED_SendSignal_450_650(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 450, 650); }	void LED_SendSignal_450_700(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 450, 700); }	void LED_SendSignal_450_750(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 450, 750); }	void LED_SendSignal_450_800(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 450, 800); }	void LED_SendSignal_450_850(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 450, 850); }	void LED_SendSignal_450_900(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 450, 900); }	void LED_SendSignal_450_950(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 450, 950); }
void LED_SendSignal_500_650(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 500, 650); }	void LED_SendSignal_500_700(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 500, 700); }	void LED_SendSignal_500_750(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 500, 750); }	void LED_SendSignal_500_800(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 500, 800); }	void LED_SendSignal_500_850(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 500, 850); }	void LED_SendSignal_500_900(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 500, 900); }	void LED_SendSignal_500_950(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 500, 950); }
void LED_SendSignal_550_450(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 550, 450); }	void LED_SendSignal_550_500(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 550, 500); }	void LED_SendSignal_550_550(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 550, 550); }	void LED_SendSignal_550_600(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 550, 600); }	void LED_SendSignal_550_650(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 550, 650); }	void LED_SendSignal_550_700(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 550, 700); }	void LED_SendSignal_550_750(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 550, 750); }
void LED_SendSignal_600_450(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 600, 450); }	void LED_SendSignal_600_500(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 600, 500); }	void LED_SendSignal_600_550(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 600, 550); }	void LED_SendSignal_600_600(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 600, 600); }	void LED_SendSignal_600_650(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 600, 650); }	void LED_SendSignal_600_700(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 600, 700); }	void LED_SendSignal_600_750(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 600, 750); }
void LED_SendSignal_650_450(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 650, 450); }	void LED_SendSignal_650_500(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 650, 500); }	void LED_SendSignal_650_550(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 650, 550); }	void LED_SendSignal_650_600(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 650, 600); }	void LED_SendSignal_650_650(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 650, 650); }	void LED_SendSignal_650_700(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 650, 700); }	void LED_SendSignal_650_750(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 650, 750); }
void LED_SendSignal_700_450(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 700, 450); }	void LED_SendSignal_700_500(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 700, 500); }	void LED_SendSignal_700_550(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 700, 550); }	void LED_SendSignal_700_600(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 700, 600); }	void LED_SendSignal_700_650(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 700, 650); }	void LED_SendSignal_700_700(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 700, 700); }	void LED_SendSignal_700_750(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 700, 750); }
void LED_SendSignal_750_450(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 750, 450); }	void LED_SendSignal_750_500(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 750, 500); }	void LED_SendSignal_750_550(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 750, 550); }	void LED_SendSignal_750_600(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 750, 600); }	void LED_SendSignal_750_650(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 750, 650); }	void LED_SendSignal_750_700(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 750, 700); }	void LED_SendSignal_750_750(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 750, 750); }
void LED_SendSignal_800_450(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 800, 450); }	void LED_SendSignal_800_500(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 800, 500); }	void LED_SendSignal_800_550(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 800, 550); }	void LED_SendSignal_800_600(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 800, 600); }	void LED_SendSignal_800_650(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 800, 650); }	void LED_SendSignal_800_700(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 800, 700); }	void LED_SendSignal_800_750(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 800, 750); }
void LED_SendSignal_850_450(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 850, 450); }	void LED_SendSignal_850_500(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 850, 500); }	void LED_SendSignal_850_550(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 850, 550); }	void LED_SendSignal_850_600(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 850, 600); }	void LED_SendSignal_850_650(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 850, 650); }	void LED_SendSignal_850_700(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 850, 700); }	void LED_SendSignal_850_750(void) { LED_SEND_SIGNAL(PIXEL_PORT, PIXEL_BIT, 850, 750); }

void SetTiming(void** f, byte t) {
	switch (t) {
	case 0x00: *f = LED_SendSignal_200_650; break;	case 0x01: *f = LED_SendSignal_200_700; break;	case 0x02: *f = LED_SendSignal_200_750; break;	case 0x03: *f = LED_SendSignal_200_800; break;	case 0x04: *f = LED_SendSignal_200_850; break;	case 0x05: *f = LED_SendSignal_200_900; break;	case 0x06: *f = LED_SendSignal_200_950; break;
	case 0x10: *f = LED_SendSignal_250_650; break;	case 0x11: *f = LED_SendSignal_250_700; break;	case 0x12: *f = LED_SendSignal_250_750; break;	case 0x13: *f = LED_SendSignal_250_800; break;	case 0x14: *f = LED_SendSignal_250_850; break;	case 0x15: *f = LED_SendSignal_250_900; break;	case 0x16: *f = LED_SendSignal_250_950; break;
	case 0x20: *f = LED_SendSignal_300_650; break;	case 0x21: *f = LED_SendSignal_300_700; break;	case 0x22: *f = LED_SendSignal_300_750; break;	case 0x23: *f = LED_SendSignal_300_800; break;	case 0x24: *f = LED_SendSignal_300_850; break;	case 0x25: *f = LED_SendSignal_300_900; break;	case 0x26: *f = LED_SendSignal_300_950; break;
	case 0x30: *f = LED_SendSignal_350_650; break;	case 0x31: *f = LED_SendSignal_350_700; break;	case 0x32: *f = LED_SendSignal_350_750; break;	case 0x33: *f = LED_SendSignal_350_800; break;	case 0x34: *f = LED_SendSignal_350_850; break;	case 0x35: *f = LED_SendSignal_350_900; break;	case 0x36: *f = LED_SendSignal_350_950; break;
	case 0x40: *f = LED_SendSignal_400_650; break;	case 0x41: *f = LED_SendSignal_400_700; break;	case 0x42: *f = LED_SendSignal_400_750; break;	case 0x43: *f = LED_SendSignal_400_800; break;	case 0x44: *f = LED_SendSignal_400_850; break;	case 0x45: *f = LED_SendSignal_400_900; break;	case 0x46: *f = LED_SendSignal_400_950; break;
	case 0x50: *f = LED_SendSignal_450_650; break;	case 0x51: *f = LED_SendSignal_450_700; break;	case 0x52: *f = LED_SendSignal_450_750; break;	case 0x53: *f = LED_SendSignal_450_800; break;	case 0x54: *f = LED_SendSignal_450_850; break;	case 0x55: *f = LED_SendSignal_450_900; break;	case 0x56: *f = LED_SendSignal_450_950; break;
	case 0x60: *f = LED_SendSignal_500_650; break;	case 0x61: *f = LED_SendSignal_500_700; break;	case 0x62: *f = LED_SendSignal_500_750; break;	case 0x63: *f = LED_SendSignal_500_800; break;	case 0x64: *f = LED_SendSignal_500_850; break;	case 0x65: *f = LED_SendSignal_500_900; break;	case 0x66: *f = LED_SendSignal_500_950; break;
	case 0x70: *f = LED_SendSignal_550_450; break;	case 0x71: *f = LED_SendSignal_550_500; break;	case 0x72: *f = LED_SendSignal_550_550; break;	case 0x73: *f = LED_SendSignal_550_600; break;	case 0x74: *f = LED_SendSignal_550_650; break;	case 0x75: *f = LED_SendSignal_550_700; break;	case 0x76: *f = LED_SendSignal_550_750; break;
	case 0x80: *f = LED_SendSignal_600_450; break;	case 0x81: *f = LED_SendSignal_600_500; break;	case 0x82: *f = LED_SendSignal_600_550; break;	case 0x83: *f = LED_SendSignal_600_600; break;	case 0x84: *f = LED_SendSignal_600_650; break;	case 0x85: *f = LED_SendSignal_600_700; break;	case 0x86: *f = LED_SendSignal_600_750; break;
	case 0x90: *f = LED_SendSignal_650_450; break;	case 0x91: *f = LED_SendSignal_650_500; break;	case 0x92: *f = LED_SendSignal_650_550; break;	case 0x93: *f = LED_SendSignal_650_600; break;	case 0x94: *f = LED_SendSignal_650_650; break;	case 0x95: *f = LED_SendSignal_650_700; break;	case 0x96: *f = LED_SendSignal_650_750; break;
	case 0xA0: *f = LED_SendSignal_700_450; break;	case 0xA1: *f = LED_SendSignal_700_500; break;	case 0xA2: *f = LED_SendSignal_700_550; break;	case 0xA3: *f = LED_SendSignal_700_600; break;	case 0xA4: *f = LED_SendSignal_700_650; break;	case 0xA5: *f = LED_SendSignal_700_700; break;	case 0xA6: *f = LED_SendSignal_700_750; break;
	case 0xB0: *f = LED_SendSignal_750_450; break;	case 0xB1: *f = LED_SendSignal_750_500; break;	case 0xB2: *f = LED_SendSignal_750_550; break;	case 0xB3: *f = LED_SendSignal_750_600; break;	case 0xB4: *f = LED_SendSignal_750_650; break;	case 0xB5: *f = LED_SendSignal_750_700; break;	case 0xB6: *f = LED_SendSignal_750_750; break;
	case 0xC0: *f = LED_SendSignal_800_450; break;	case 0xC1: *f = LED_SendSignal_800_500; break;	case 0xC2: *f = LED_SendSignal_800_550; break;	case 0xC3: *f = LED_SendSignal_800_600; break;	case 0xC4: *f = LED_SendSignal_800_650; break;	case 0xC5: *f = LED_SendSignal_800_700; break;	case 0xC6: *f = LED_SendSignal_800_750; break;
	case 0xD0: *f = LED_SendSignal_850_450; break;	case 0xD1: *f = LED_SendSignal_850_500; break;	case 0xD2: *f = LED_SendSignal_850_550; break;	case 0xD3: *f = LED_SendSignal_850_600; break;	case 0xD4: *f = LED_SendSignal_850_650; break;	case 0xD5: *f = LED_SendSignal_850_700; break;	case 0xD6: *f = LED_SendSignal_850_750; break;
	}
}

#pragma endregion

uint8_t GetTimingByte(uint16_t high_ns, uint16_t low_ns) {
	if (high_ns >= 200 && high_ns <= 500) {
		uint8_t high_byte = (((high_ns - 200) / 50) & 0x0F) << 4;
		if (low_ns >= 650 && low_ns <= 950) {
			uint8_t low_byte = (((low_ns - 650) / 50) & 0x0F);
			return high_byte | low_byte;
		}
	}
	else if (high_ns >= 550 && high_ns <= 850) {
		uint8_t high_byte = (((high_ns - 200) / 50) & 0x0F) << 4;
		if (low_ns >= 450 && low_ns <= 750) {
			uint8_t low_byte = (((low_ns - 450) / 50) & 0x0F);
			return high_byte | low_byte;
		}
	}
	return 0xFF;
}

LEDTimingFunc Send1BitFunc = nullptr; // 200 = C8
LEDTimingFunc Send0BitFunc = nullptr; // 44 = 2C

void Set1Timing(byte timing) {
	SetTiming((void**)&Send1BitFunc, timing);
}
void Set0Timing(byte timing) {
	SetTiming((void**)&Send0BitFunc, timing);
}

inline void _sendByteVariable(uint8_t byte) {
	if (byte & (0x80)) { Send1BitFunc(); }
	else { Send0BitFunc(); }

	if (byte & (0x40)) { Send1BitFunc(); }
	else { Send0BitFunc(); }

	if (byte & (0x20)) { Send1BitFunc(); }
	else { Send0BitFunc(); }

	if (byte & (0x10)) { Send1BitFunc(); }
	else { Send0BitFunc(); }

	if (byte & (0x08)) { Send1BitFunc(); }
	else { Send0BitFunc(); }

	if (byte & (0x04)) { Send1BitFunc(); }
	else { Send0BitFunc(); }

	if (byte & (0x02)) { Send1BitFunc(); }
	else { Send0BitFunc(); }

	if (byte & (0x01)) { Send1BitFunc(); }
	else { Send0BitFunc(); }
}

void SendPixelVariable(byte r, byte g, byte b) {
	_sendByteVariable(g);          // Neopixel wants colors in green then red then blue order
	_sendByteVariable(r);
	_sendByteVariable(b);
}


void ledsetup() {
	// Set the specified pin up as digital out
	bitSet(PIXEL_DDR, PIXEL_BIT);
}

RgbColor LEDs[256];

uint64_t lastFrameMillis = 0;
uint64_t pingDropFrameEvery = 1000; // if we don't get a frame after 100ms, send message for a new one
uint8_t runOwnAfter = 2; // if we don't get a frame after 2 pings, transition to our own logic
uint8_t droppedFrameCount = 0; // count the number of dropped frames
uint64_t lastOwnMillis = 0; // last time since our own frame
uint64_t ownFrameDelay = 16; // number of ms between our own frame
uint8_t ownFrameIndex = 0; // offset of own frame (for animation)

MessageHandlerClass msg = MessageHandlerClass(Serial);
enum Commands : uint8_t {
	FRM,
	FRM_RESP
};


void requestFrame() {
	msg.beginSend(Commands::FRM);
	msg.sendByte((uint8_t)40);
	msg.endSend();
}

void onFrameResponse() {
	droppedFrameCount = 0;
	RgbColor rgb;
	for (int i = 0; i < 40; ++i) {
		rgb.r = msg.getNextByte();
		rgb.g = msg.getNextByte();
		rgb.b = msg.getNextByte();
		LEDs[i] = rgb;
	}
	cli();
	for (uint8_t i = 0; i < 40; ++i) {
		rgb = LEDs[i];
		SendPixelVariable(rgb.r, rgb.g, rgb.b);
	}
	sei();
	lastFrameMillis = millis();
	requestFrame();
	
	
}

void runOwnFrame() {
	uint8_t curHue = ++ownFrameIndex;
	int16_t errorHue = 2 * 255 - PIXELS_LESS_SPECIAL;
	RgbColor rgb;
	HsvColor hsv;
	hsv.s = 255;
	hsv.v = 50;
	uint8_t special1hue;
	uint8_t special2hue;
	uint8_t special3hue;
	for (uint8_t i = 0; i < PIXELS_LESS_SPECIAL; ++i) {
		hsv.h = curHue;
		HsvToRgb(hsv, rgb);
		LEDs[i] = rgb;
		if (i == SPECIAL_1_SAME) {
			special1hue = curHue;
		}
		else if (i == SPECIAL_2_SAME) {
			special2hue = curHue;
		}
		else if (i == SPECIAL_3_SAME) {
			special3hue = curHue;
		}		
		while (errorHue > 0) {
			curHue++;
			errorHue -= PIXELS;
		}
		errorHue += 255;
	}

	hsv.v = 255;
	hsv.h = special1hue;
	HsvToRgb(hsv, rgb);
	LEDs[PIXELS_LESS_SPECIAL + 0] = rgb;

	hsv.h = special2hue;
	HsvToRgb(hsv, rgb);
	LEDs[PIXELS_LESS_SPECIAL + 1] = rgb;

	hsv.h = special3hue;
	HsvToRgb(hsv, rgb);
	LEDs[PIXELS_LESS_SPECIAL + 2] = rgb;

	cli();
	for (uint8_t i = 0; i < PIXELS; ++i) {
		rgb = LEDs[i];
		SendPixelVariable(rgb.r, rgb.g, rgb.b);
	}
	sei();	
}

enum EEPROM_Addresses {
	Head_1,
	Head_2,
	Head_3,
	Head_4,
	Timing_0,
	Timing_1,
	LED_Count_H,
	LED_Count_L
};

// the setup function runs once when you press reset or power the board
void setup() {
	// load previous values from eeprom
	if (EEPROM[EEPROM_Addresses::Head_1] == 0x4b 
		&& EEPROM[EEPROM_Addresses::Head_2] == 0x79 
		&& EEPROM[EEPROM_Addresses::Head_3] == 0x6c
		&& EEPROM[EEPROM_Addresses::Head_4] == 0x65) {
		// we have a header for previous values, load them
		Set1Timing(EEPROM[EEPROM_Addresses::Timing_1]);
		Set0Timing(EEPROM[EEPROM_Addresses::Timing_0]);
	}
	else {
		// use default values
		Set0Timing(GetTimingByte(200, 700));
		Set1Timing(GetTimingByte(600, 450));
	}
	ledsetup();
	Serial.begin(1000000);
	msg.addHandler(Commands::FRM_RESP, onFrameResponse);
}

// the loop function runs over and over again until power down or reset
void loop() {
	msg.readSerial();
	if (millis() - lastFrameMillis >= pingDropFrameEvery) {
		// missed a frame, so ask for a new one
		requestFrame();
		lastFrameMillis = millis();
		if (droppedFrameCount < runOwnAfter) {
			// still counting number of frames left
			droppedFrameCount++;
			lastOwnMillis = 0;
		}
	}

	if (droppedFrameCount >= runOwnAfter && millis() - lastOwnMillis >= ownFrameDelay) {
		// dropped enough frames and its been enough time since our last frame, run a new frame ourself
		lastOwnMillis = millis();
		runOwnFrame();
	}
}
