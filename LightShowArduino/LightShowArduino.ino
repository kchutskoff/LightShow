bool lastWasSpecial = false;
bool lastWasHeader = false;
bool isSending = false;
bool isReading = false;

#define NOP() do { __asm__ volatile ("nop"); } while (0)

enum Commands : uint8_t {
	FRM,
	FRM_RESP
};

void readSerial() {
	int available = 0;
	while (Serial.available() > 0) {
		uint8_t read = Serial.read();
		if (lastWasHeader) {
			lastWasHeader = false;
			// this is the type, send off a message arrival
			if (read == Commands::FRM_RESP) {
				isReading = true;
				onFrameResponse();
			}
			else {
				isReading = false;
			}
		}
		else if (lastWasSpecial) {
			lastWasSpecial = false;
			if (read == 0xFF) {
				// start of packet
				lastWasHeader = true;
			}
			else if (read == 0x00) {
				isReading = false;
			}
		}
		else if (read == 0x55) {
			lastWasSpecial = true;
		}
	}
}

bool hasNextByte() {
	return isReading && Serial.available() > 0;
}

#define MAX_ATTEMPTS (10)

uint8_t getNextByte() {
	if (isReading) {
		uint8_t attempts = MAX_ATTEMPTS + 1;
		while (--attempts != 0) {
			if (Serial.available() > 0) {
				attempts = MAX_ATTEMPTS + 1;
				uint8_t read = Serial.read();
				if (lastWasSpecial) {
					if (read == 0xFE) {
						// escaped
						return 0x55;
					}
					else if (read == 0x00) {
						// end of packet
						isReading = false;
						return 0x00;
					}
					else {
						// ignore
						continue;
					}
				}
				if (read == 0x55) {
					// escape character
					lastWasSpecial = true;
					continue;
				}
				else {
					return read;
				}
			}
		}
		return 0x00;
	}
	return 0x00;
}

bool beginSend(uint8_t messageType) {
	if (!isSending) {
		isSending = true;
		Serial.write((uint8_t)0x55);
		Serial.write((uint8_t)0xFF);
		sendByte(messageType);
		return true;
	}
	return false;
}
bool sendByte(uint8_t data) {
	if (isSending) {
		Serial.write(data);
		if (data == 0x55) {
			Serial.write((byte)0xFE);
		}
		return true;
	}
	return false;
}
bool endSend() {
	if (isSending) {
		Serial.write((uint8_t)0x55);
		Serial.write((uint8_t)0x00);
		Serial.flush();
		isSending = false;
		return true;
	}
	return false;
}

#define PIXEL_PORT  PORTD  // Port of the pin the pixels are connected to
#define PIXEL_DDR   DDRD   // Port of the pin the pixels are connected to
#define PIXEL_BIT   6      // Bit of the pin the pixels are connected to

void ledsetup() {
	// Set the specified pin up as digital out
	bitSet(PIXEL_DDR, PIXEL_BIT);
}

uint64_t lastFrameMillis = 0;
uint64_t pingDropFrameEvery = 250; // if we don't get a frame after 1000ms, send message for a new one
uint8_t resetCountAfterDrops = 10; // if we don't get a frame after 10 pings, reset the data count
uint8_t droppedFrameCount = 0; // count the number of dropped frames
uint16_t currentDataCount = 0; // amount of data to request for
uint16_t maxDataCount = 1000; // our message buffer is only 1024
uint8_t currentDataCountIndex = 0; // number of times we sent the current data count
uint8_t maxDataCountIndex = 20; // send each amount of data 50 times in a row before moving up

void requestFrame() {
	uint8_t high = (currentDataCount >> 8);
	uint8_t low = currentDataCount;
	beginSend(Commands::FRM);
	sendByte(high);
	sendByte(low);
	endSend();
}

void onFrameResponse() {
	droppedFrameCount = 0;
	uint8_t byteToRead;
	for (uint16_t i = 0; i < currentDataCount; ++i) {
		byteToRead = getNextByte();
	}
	while (getNextByte() != 0x00);
	cli();
	for (uint16_t i = 0; i < currentDataCount; ++i) {
		// pretend we did some processing on LEDs
		for (uint8_t j = 0; j < 8; ++j) {
			NOP();
			NOP();
			NOP();
			NOP();
			NOP();
			NOP();
			NOP();
		}
	}
	sei();
	// how many times have we sent this data size?
	currentDataCountIndex++;
	if (currentDataCountIndex >= maxDataCountIndex) {
		// enough, go up a data size and reset the counter
		currentDataCountIndex = 0;
		currentDataCount++;
		if (currentDataCount >= maxDataCount) {
			currentDataCount = 0; // hit the max, go back to zero
		}
	}
	lastFrameMillis = millis();
	requestFrame();
}


// the setup function runs once when you press reset or power the board
void setup() {
	ledsetup();
	Serial.begin(1000000);
}

// the loop function runs over and over again until power down or reset
void loop() {
	readSerial();
	if (millis() - lastFrameMillis >= pingDropFrameEvery) {
		// missed a frame, so ask for a new one
		requestFrame();
		lastFrameMillis = millis();
		droppedFrameCount++;
		if (droppedFrameCount >= resetCountAfterDrops) {
			// dropped twoo many frames, restart count at 1 again
			currentDataCount = 0;
			currentDataCountIndex = 0;
		}
	}
}
