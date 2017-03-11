
// hello world (testing email)
/*
This is an example of how simple driving a Neopixel can be
This code is optimized for understandability and changability rather than raw speed
More info at http://wp.josh.com/2014/05/11/ws2812-neopixels-made-easy/
*/

// Change this to be at least as long as your pixel string (too long will work fine, just be a little slower)

#include "MessageHandler.h"
#include "HSV.h"
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

// Note that you could also include the DigitalWriteFast header file to not need to to this lookup.

#define PIXEL_PORT  PORTD  // Port of the pin the pixels are connected to
#define PIXEL_DDR   DDRD   // Port of the pin the pixels are connected to
#define PIXEL_BIT   6      // Bit of the pin the pixels are connected to

// These are the timing constraints taken mostly from the WS2812 datasheets 
// These are chosen to be conservative and avoid problems rather than for maximum throughput 

#define T1H  800    // Width of a 1 bit in ns
// 800
#define T1L  600    // Width of a 1 bit in ns
// 600
#define T0H  400    // Width of a 0 bit in ns
// 200
#define T0L  800    // Width of a 0 bit in ns
// 900
#define RES 80000    // Width of the low gap between bits to cause a frame to latch

// Here are some convience defines for using nanoseconds specs to generate actual CPU delays

#define NS_PER_SEC (1000000000L)          // Note that this has to be SIGNED since we want to be able to check for negative values of derivatives

#define CYCLES_PER_SEC (F_CPU)

#define NS_PER_CYCLE ( NS_PER_SEC / CYCLES_PER_SEC )

#define NS_TO_CYCLES(n) ( (n) / NS_PER_CYCLE )

// Actually send a bit to the string. We must to drop to asm to enusre that the complier does
// not reorder things and make it so the delay happens in the wrong place.

inline void sendBit(bool bitVal) {

	if (bitVal) {        // 0 bit

		asm volatile (
			"sbi %[port], %[bit] \n\t"        // Set the output bit
			".rept %[onCycles] \n\t"                                // Execute NOPs to delay exactly the specified number of cycles
			"nop \n\t"
			".endr \n\t"
			"cbi %[port], %[bit] \n\t"                              // Clear the output bit
			".rept %[offCycles] \n\t"                               // Execute NOPs to delay exactly the specified number of cycles
			"nop \n\t"
			".endr \n\t"
			::
			[port]    "I" (_SFR_IO_ADDR(PIXEL_PORT)),
			[bit]   "I" (PIXEL_BIT),
			[onCycles]  "I" (NS_TO_CYCLES(T1H) - 2),    // 1-bit width less overhead  for the actual bit setting, note that this delay could be longer and everything would still work
			[offCycles]   "I" (NS_TO_CYCLES(T1L) - 2)     // Minimum interbit delay. Note that we probably don't need this at all since the loop overhead will be enough, but here for correctness

			);

	}
	else {          // 1 bit

					// **************************************************************************
					// This line is really the only tight goldilocks timing in the whole program!
					// **************************************************************************


		asm volatile (
			"sbi %[port], %[bit] \n\t"        // Set the output bit
			".rept %[onCycles] \n\t"        // Now timing actually matters. The 0-bit must be long enough to be detected but not too long or it will be a 1-bit
			"nop \n\t"                                              // Execute NOPs to delay exactly the specified number of cycles
			".endr \n\t"
			"cbi %[port], %[bit] \n\t"                              // Clear the output bit
			".rept %[offCycles] \n\t"                               // Execute NOPs to delay exactly the specified number of cycles
			"nop \n\t"
			".endr \n\t"
			::
			[port]    "I" (_SFR_IO_ADDR(PIXEL_PORT)),
			[bit]   "I" (PIXEL_BIT),
			[onCycles]  "I" (NS_TO_CYCLES(T0H) - 2),
			[offCycles] "I" (NS_TO_CYCLES(T0L) - 2)

			);

	}

	// Note that the inter-bit gap can be as long as you want as long as it doesn't exceed the 5us reset timeout (which is A long time)
	// Here I have been generous and not tried to squeeze the gap tight but instead erred on the side of lots of extra time.
	// This has thenice side effect of avoid glitches on very long strings becuase 


}


inline void sendByte(uint8_t byte) {

	for (uint8_t bit = 0; bit < 8; bit++) {

		sendBit(bitRead(byte, 7));                // Neopixel wants bit in highest-to-lowest order
												  // so send highest bit (bit #7 in an 8-bit byte since they start at 0)
		byte <<= 1;                                    // and then shift left so bit 6 moves into 7, 5 moves into 6, etc

	}
}

/*

The following three functions are the public API:

ledSetup() - set up the pin that is connected to the string. Call once at the begining of the program.
sendPixel( r g , b ) - send a single pixel to the string. Call this once for each pixel in a frame.
show() - show the recently sent pixel on the LEDs . Call once per frame.

*/


// Set the specified pin up as digital out

void ledsetup() {

	bitSet(PIXEL_DDR, PIXEL_BIT);

}

inline void sendPixel(uint8_t r, uint8_t g, uint8_t b) {

	sendByte(g);          // Neopixel wants colors in green then red then blue order
	sendByte(r);
	sendByte(b);

}

// each frame should take under 33ms to receive (115200 baud, 10 bits per byte, theoretical max of 36 fps)
uint64_t lastFrameMillis = 0;
uint64_t pingDropFrameEvery = 50; // if we don't get a frame after 50ms, send message for a new one
uint8_t runOwnAfter = 2; // if we don't get a frame after 2 pings, transition to our own logic
uint8_t droppedFrameCount = 0; // count the number of dropped frames
uint64_t lastOwnMillis = 0; // last time since our own frame
uint64_t ownFrameDelay = 32; // number of ms between our own frame
uint8_t ownFrameIndex = 0; // offset of own frame (for animation)


// Just wait long enough without sending any bots to cause the pixels to latch and display the last sent frame

void show() {
	_delay_us((RES / 1000UL) + 1);       // Round up since the delay must be _at_least_ this long (too short might not work, too long not a problem)
}

MessageHandlerClass msg = MessageHandlerClass(Serial);
enum Commands : uint8_t {
	FRM,
	FRM_RESP
};


void requestFrame() {
	msg.beginSend(Commands::FRM);
	msg.sendByte((uint8_t)PIXELS);
	msg.endSend();
}

void onFrameResponse() {
	droppedFrameCount = 0;
	lastFrameMillis = millis();
	cli();
	for (int i = 0; i < PIXELS; ++i) {
		sendByte(msg.getNextByte());
		sendByte(msg.getNextByte());
		sendByte(msg.getNextByte());
	}
	sei();
	// don't need this as we are likely going to take longer to get the next frame anyways
	//show();
	requestFrame();
}

void runOwnFrame() {
	uint8_t curHue = ++ownFrameIndex;
	int16_t errorHue = 2 * 255 - PIXELS_LESS_SPECIAL;
	RgbColor rgb;
	HsvColor hsv;
	hsv.s = 255;
	hsv.v = 50;
	uint8_t special1_hue;
	uint8_t special2_hue;
	uint8_t special3_hue;
	cli();
	for (int i = 0; i < PIXELS_LESS_SPECIAL; ++i) {
		hsv.h = curHue;
		HsvToRgb(hsv, rgb);
		sendPixel(rgb.r, rgb.g, rgb.b);
		if (i == SPECIAL_1_SAME) {
			special1_hue = curHue;
		}
		else if (i == SPECIAL_2_SAME) {
			special2_hue = curHue;
		}
		else if (i == SPECIAL_3_SAME) {
			special3_hue = curHue;
		}
		//LEDS[i * 3 + 0] = rgb.g;
		//LEDS[i * 3 + 1] = rgb.r;
		//LEDS[i * 3 + 2] = rgb.b;
		while (errorHue > 0) {
			curHue++;
			errorHue -= PIXELS_LESS_SPECIAL;
		}
		errorHue += 255;
	}
	hsv.v = 255;
	hsv.h = special1_hue;
	HsvToRgb(hsv, rgb);
	sendPixel(rgb.r, rgb.g, rgb.b);

	hsv.h = special2_hue;
	HsvToRgb(hsv, rgb);
	sendPixel(rgb.r, rgb.g, rgb.b);

	hsv.h = special3_hue;
	HsvToRgb(hsv, rgb);
	sendPixel(rgb.r, rgb.g, rgb.b);

	sei();
	//for (int i = 0; i < PIXELS * 3; ++i) {
	//	sendByte(LEDS[i]);
	//}
	
}

// the setup function runs once when you press reset or power the board
void setup() {
	ledsetup();
	Serial.begin(115200);
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
