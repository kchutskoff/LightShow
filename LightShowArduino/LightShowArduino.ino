bool lastWasSpecial = false;
bool lastWasHeader = false;
bool isSending = false;
bool isReading = false;

#define NOP() do { __asm__ volatile ("nop"); } while (0)

enum Commands : uint8_t {
	FRM,
	FRM_RESP
};

#define MAX_ATTEMPTS (1000UL)

uint8_t readPacket() {
	if (isReading == false) {
		return 0xEE;
	}
	unsigned long started = millis();
	while (Serial.available() == 0) {
		if (millis() - started > 1000UL) {
			isReading = false;
			return 0xEE;
		}
	}
	if (isReading) {
		uint16_t attempts = MAX_ATTEMPTS + 1UL;
		while (--attempts != 0) {
			if (Serial.available() > 0) {
				attempts = MAX_ATTEMPTS + 1UL;
				uint8_t read = Serial.read();
				if (read == 0xFF) {
					// done packet
					isReading = false;
					return read;
				}
			}
			NOP();
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
uint64_t pingDropFrameEvery = 1000; // if we don't get a frame after 1000ms, send message for a new one
uint8_t resetCountAfterDrops = 10; // if we don't get a frame after 10 pings, reset the data count
uint8_t droppedFrameCount = 0; // count the number of dropped frames
uint16_t currentDataCount = 10; // amount of data to request for
uint16_t maxDataCount = 1000; // our message buffer is only 1024
uint8_t currentDataCountIndex = 0; // number of times we sent the current data count
uint8_t maxDataCountIndex = 5; // send each amount of data 50 times in a row before moving up

uint8_t latestframeID = 0;

void requestFrame() {
	uint8_t high = (currentDataCount >> 8);
	uint8_t low = currentDataCount;
	beginSend(Commands::FRM);
	sendByte(++latestframeID);
	sendByte(high);
	sendByte(low);
	endSend();
	isReading = true;
}

// the setup function runs once when you press reset or power the board
void setup() {
	ledsetup();
	Serial.begin(1000000);
}

// the loop function runs over and over again until power down or reset
void loop() {
	uint8_t read = readPacket();
	if (read == 0xEE) {
		// fucked up, restart
		currentDataCount = 10;
		currentDataCountIndex = 0;
	}
	else if (read == 0x00) {
		// ran out of data, go down one
		if (currentDataCount > 10) {
			currentDataCount--;
		}
		currentDataCountIndex = 0;
	}
	else if (read == 0xFF) {
		// good, go up one
		currentDataCountIndex++;
		if (currentDataCountIndex > maxDataCountIndex) {
			currentDataCountIndex = 0;
			if (currentDataCount < maxDataCount) {
				currentDataCount++;
			}
		}
	}
	requestFrame();
}
