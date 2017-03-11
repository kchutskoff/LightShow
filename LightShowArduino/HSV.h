#pragma once
/*
The MIT License (MIT)

Copyright (c) 2013 FastLED

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

// lifted from fastled

typedef struct RgbColor
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} RgbColor;

typedef struct HsvColor
{
	uint8_t h;
	uint8_t s;
	uint8_t v;
} HsvColor;

#define APPLY_DIMMING(X) (X)
#define HSV_SECTION_3 (0x40)

uint8_t scale8(uint8_t i, uint8_t scale) {
	asm volatile(
		// Multiply 8-bit i * 8-bit scale, giving 16-bit r1,r0
		"mul %0, %1          \n\t"
		// Add i to r0, possibly setting the carry flag
		"add r0, %0         \n\t"
		// load the immediate 0 into i (note, this does _not_ touch any flags)
		"ldi %0, 0x00       \n\t"
		// walk and chew gum at the same time
		"adc %0, r1          \n\t"
		"clr __zero_reg__    \n\t"

		: "+a" (i)      /* writes to i */
		: "a"  (scale)  /* uses scale */
		: "r0", "r1"    /* clobbers r0, r1 */);

	/* Return the result */
	return i;
}

void HsvToRgb(HsvColor& hsv, RgbColor& rgb)
{
	uint8_t hue, saturation, value;

	hue = scale8(hsv.h, 191);
	saturation = hsv.s;
	value = hsv.v;

	// Saturation more useful the other way around
	saturation = 255 - saturation;
	uint8_t invsat = APPLY_DIMMING(saturation);

	// Apply dimming curves
	value = APPLY_DIMMING(value);

	// The brightness floor is minimum number that all of
	// R, G, and B will be set to, which is value * invsat
	uint8_t brightness_floor;

	asm volatile(
		"mul %[value], %[invsat]            \n"
		"mov %[brightness_floor], r1        \n"
		: [brightness_floor] "=r" (brightness_floor)
		: [value] "r" (value),
		[invsat] "r" (invsat)
		: "r0", "r1"
		);

	// The color amplitude is the maximum amount of R, G, and B
	// that will be added on top of the brightness_floor to
	// create the specific hue desired.
	uint8_t color_amplitude = value - brightness_floor;

	// Figure how far we are offset into the section of the
	// color wheel that we're in
	uint8_t offset = hue & (HSV_SECTION_3 - 1);  // 0..63
	uint8_t rampup = offset * 4; // 0..252


								 // compute color-amplitude-scaled-down versions of rampup and rampdown
	uint8_t rampup_amp_adj;
	uint8_t rampdown_amp_adj;

	asm volatile(
		"mul %[rampup], %[color_amplitude]       \n"
		"mov %[rampup_amp_adj], r1               \n"
		"com %[rampup]                           \n"
		"mul %[rampup], %[color_amplitude]       \n"
		"mov %[rampdown_amp_adj], r1             \n"
		: [rampup_amp_adj] "=&r" (rampup_amp_adj),
		[rampdown_amp_adj] "=&r" (rampdown_amp_adj),
		[rampup] "+r" (rampup)
		: [color_amplitude] "r" (color_amplitude)
		: "r0", "r1"
		);


	// add brightness_floor offset to everything
	uint8_t rampup_adj_with_floor = rampup_amp_adj + brightness_floor;
	uint8_t rampdown_adj_with_floor = rampdown_amp_adj + brightness_floor;


	// keep gcc from using "X" as the index register for storing
	// results back in the return structure.  AVR's X register can't
	// do "std X+q, rnn", but the Y and Z registers can.
	// if the pointer to 'rgb' is in X, gcc will add all kinds of crazy
	// extra instructions.  Simply killing X here seems to help it
	// try Y or Z first.
	asm volatile(""  : : : "r26", "r27");


	if (hue & 0x80) {
		// section 2: 0x80..0xBF
		rgb.r = rampup_adj_with_floor;
		rgb.g = brightness_floor;
		rgb.b = rampdown_adj_with_floor;
	}
	else {
		if (hue & 0x40) {
			// section 1: 0x40..0x7F
			rgb.r = brightness_floor;
			rgb.g = rampdown_adj_with_floor;
			rgb.b = rampup_adj_with_floor;
		}
		else {
			// section 0: 0x00..0x3F
			rgb.r = rampdown_adj_with_floor;
			rgb.g = rampup_adj_with_floor;
			rgb.b = brightness_floor;
		}
	}

	asm volatile("clr __zero_reg__  \n\t" : : : "r1");
}
