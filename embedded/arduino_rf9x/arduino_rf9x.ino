//#include <FlashAsEEPROM.h>


#include "feather_m0.h"
//#include "../../libsodium/src/libsodium/include/sodium.h"
#include "libsodium_include/sodium.h"
//#include "../../packet.hpp"
#include "packet.hpp"


int16_t packetnum = 0;  // packet counter, we increment per xmission
int8_t flags = 255;
uint8_t fragment = 0;
unsigned long sendtime, delta;
uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t len;

unsigned char dataReceiverPublicKey[crypto_sign_PUBLICKEYBYTES];
unsigned char txSharedKey[crypto_kx_SESSIONKEYBYTES];
unsigned char rxSharedKey[crypto_kx_SESSIONKEYBYTES];

unsigned char publicKey[crypto_sign_PUBLICKEYBYTES];
unsigned char privateKey[crypto_sign_SECRETKEYBYTES];

RH_RF95 rf95(RFM95_CS, RFM95_INT);


// TODO:

void setup(void) {
	pinMode(RFM95_RST, OUTPUT);
	digitalWrite(RFM95_RST, HIGH);

	Serial.begin(115200);
	while (!Serial) { // TODO: Change to boot without Serial
		delay(50);
	}

	delay(100);

	// Reset RF95 Radio
	digitalWrite(RFM95_RST, LOW);
	delay(10);
	digitalWrite(RFM95_RST, HIGH);
	delay(10);

	while (!rf95.init()) {
		Serial.println("LoRa radio init failed");
		while (1); // TODO: Make more robust? Flash led?
	}

	Serial.println("LoRa radio initialized");

	if (!rf95.setFrequency(RF95_FREQ)) {
		Serial.println("setFrequency failed");
		while (1); // TODO: Make more robust? Flash led?
	}

	Serial.print("Set Freq to: ");
	Serial.println(RF95_FREQ);

	// The default transmitter power is 13dBm, using PA_BOOST.
	// If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
	// you can set transmitter powers from 5 to 23 dBm:
	rf95.setTxPower(23, false);

	rf95.setThisAddress(DEVICE_ID);

	generateKeys(); // TODO
}

// TODO: Flash Memory stuff
// TODO: Data Receiver Key

void loop(void) {
	char sensorData[156]; // TODO: Remove magic number

	memcpy(sensorData, "Sensor Data", 12);

	sendData(sensorData, 12);

	delay(DATA_SEND_INTERVAL);
}



void encryptData(struct packet_data *pd, char* data, uint8_t dataLen) {
	crypto_stream_xchacha20_xor( // TODO: Check return value?
		pd->data,
		(unsigned char*) data,
		dataLen,
		pd->crypto_nonce,
		txSharedKey
	);
}



void signPacket(struct packet *p) {
	crypto_sign_state state;
	uint8_t maskedFlags;

	crypto_sign_init(&state); // TODO/NOTE: The example code omitted semicolons...

	//crypto_sign_update(&state, &p->to, sizeof(p->to)); // TODO: To field is ignored so any gateway can process this
	crypto_sign_update(&state, (unsigned char*) &p->from, sizeof(p->from)); // TODO: Probably should convert these to network byte order
	crypto_sign_update(&state, &p->id, sizeof(p->id));
	crypto_sign_update(&state, &p->fragment, sizeof(p->fragment));
	maskedFlags = p->flags & 0xF;
	crypto_sign_update(&state, &maskedFlags, sizeof(maskedFlags));
	crypto_sign_update(&state, &p->len, sizeof(p->len));

	// TODO: When multi packet messages are supported, the
	// signatures are for the stream of messages and this
	// function needs to be modified accordingly.

	if (maskedFlags == PACKET_TYPE_DATA_FIRST) {
		crypto_sign_update(
			&state,
			p->payload.data.crypto_nonce,
			sizeof(p->payload.data.crypto_nonce)
		);
		crypto_sign_update(
			&state,
			p->payload.data.data,
			p->len
		);
	} else {
		crypto_sign_update(
			&state,
			p->payload.data.data,
			p->len
		);
	}

	crypto_sign_final_create(
		&state,
		p->payload.data.signature,
		NULL,
		privateKey
	);
}



void transmitData(struct packet *p) {
	struct packet recvP;

	Serial.println("Transmitting packet");

	rf95.setHeaderTo(p->to);
	rf95.setHeaderFrom(p->from);
	rf95.setHeaderFlags(p->flags);
	rf95.setHeaderFragment(p->fragment);

	rf95.send((uint8_t *) p->payload.bytes, p->len);
	rf95.waitPacketSent();

	if (rf95.waitAvailableTimeout(REPLY_TIMEOUT)) {

		if (rf95.recv((uint8_t*) recvP.payload.bytes, &recvP.len)) {
			Serial.println("Received reply");

			recvP.from     = rf95.headerFrom();
			recvP.to       = rf95.headerTo();
			recvP.id       = rf95.headerId();
			recvP.fragment = rf95.headerFragment();
			recvP.flags    = rf95.headerFlags();
			recvP.rssi     = rf95.lastRssi();

			processReply(&recvP);

		} else {
			Serial.println("Receive failed");
		}
	} else {
		Serial.println("No reply received within timeout");
	}

	rf95.sleep();
}



void processReply(struct packet *p) {
	// TODO
	return;
}



bool sendData(char *data, uint8_t dataLen) {
	struct packet p;

	//Currently, only a max of 156 bytes can be sent
	if (dataLen > 156) return false;

	p.to = 0;
	p.from = DEVICE_ID;
	p.flags = PACKET_TYPE_DATA_FIRST;
	p.id = 0;
	p.fragment = 0;
	p.len = dataLen;

	randombytes_buf(p.payload.data.crypto_nonce, crypto_secretbox_NONCEBYTES);
	encryptData(&p.payload.data, data, dataLen);
	signPacket(&p);
	transmitData(&p);

	return true;
}


void generateKeys(void) {
	crypto_kx_keypair(publicKey, privateKey);
	generateSharedKeys(); // TODO: Check return value
}



bool generateSharedKeys(void) {
	if (crypto_kx_server_session_keys(rxSharedKey, txSharedKey, publicKey, privateKey, dataReceiverPublicKey) != 0) {
		return false;
	}
	return true;
}