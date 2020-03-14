//#include <FlashAsEEPROM.h>


#include "feather-m0.h"
//#include "../../libsodium/src/libsodium/include/sodium.h"
#include "libsodium_include/sodium.h"
//#include "../../packet.hpp"
#include "packet.hpp"


int16_t packetnum = 0; // packet counter, we increment per xmission
int8_t flags = 255;
uint8_t fragment = 0;
unsigned long sendtime, delta;
uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t len;

unsigned char dataReceiverPublicKey[crypto_sign_PUBLICKEYBYTES];

unsigned char txSharedKey[crypto_kx_SESSIONKEYBYTES];
unsigned char rxSharedKey[crypto_kx_SESSIONKEYBYTES];

unsigned char publicKey[crypto_kx_PUBLICKEYBYTES];
unsigned char privateKey[crypto_kx_SECRETKEYBYTES];

unsigned char signPublicKey[crypto_sign_PUBLICKEYBYTES];
unsigned char signPrivateKey[crypto_sign_SECRETKEYBYTES];

struct randombytes_implementation randImplM0 = {
	.implementation_name = customRandName,
	.random = customRand,
	.stir = customRandSeed,
	.uniform = NULL,
	.buf = customRandBytes,
	.close = NULL
};

RH_RF95 rf95(RFM95_CS, RFM95_INT);




void
hexPrint(unsigned char* s, uint16_t len)
{
	int c;
	for (uint16_t i = 0; i < len; i++)
	{
		c = (int)s[i];
		if (c < 16)
			Serial.print('0');
		Serial.print(c, HEX);
	}
}


bool
hexToBin(unsigned char* dest, const char* hex, uint16_t hexLen)
{
	if (hexLen % 2 == 1)
		return false;
	for (uint16_t i = 0; i < hexLen; i += 2)
	{
		char s[3];
		s[0] = hex[i];
		s[1] = hex[i + 1];
		s[2] = 0;
		dest[i / 2] = (unsigned char)strtol(s, NULL, 16);
	}
	return true;
}

// TODO:

void
setup(void)
{

	pinMode(RFM95_RST, OUTPUT);
	digitalWrite(RFM95_RST, HIGH);

	Serial.begin(115200);
	while (!Serial)
	{ // TODO: Change to boot without Serial
		delay(50);
	}

	delay(100);

	// Reset RF95 Radio
	digitalWrite(RFM95_RST, LOW);
	delay(10);
	digitalWrite(RFM95_RST, HIGH);
	delay(10);

	while (!rf95.init())
	{
		Serial.println("LoRa radio init failed");
		while (1)
			; // TODO: Make more robust? Flash led?
	}

	Serial.println("LoRa radio initialized");

	if (!rf95.setFrequency(RF95_FREQ))
	{
		Serial.println("setFrequency failed");
		while (1)
			; // TODO: Make more robust? Flash led?
	}

	Serial.print("Set Freq to: ");
	Serial.println(RF95_FREQ);

	// The default transmitter power is 13dBm, using PA_BOOST.
	// If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
	// you can set transmitter powers from 5 to 23 dBm:
	rf95.setTxPower(23, false);

	rf95.setThisAddress(DEVICE_ID);

	randombytes_set_implementation(&randImplM0);

	Serial.println("sodium_init()");
	if (sodium_init() == -1)
	{
		while (1)
			; // TODO: More robust solution?
	}


	//generateKeys(); // TODO

	if (!hexToBin(dataReceiverPublicKey, DATA_RECEIVER_PUBLIC_KEY, crypto_sign_PUBLICKEYBYTES * 2))
	{
		Serial.println("Error loading data receiver public key!");
	}

	if (!hexToBin(publicKey, PUBLIC_KEY, crypto_kx_PUBLICKEYBYTES * 2))
	{
		Serial.println("Error loading existing public key!");
	}
	if (!hexToBin(privateKey, PRIVATE_KEY, crypto_kx_SECRETKEYBYTES * 2))
	{
		Serial.println("Error loading existing private key!");
	}

	if (!hexToBin(signPublicKey, SIGN_PUBLIC_KEY, crypto_sign_PUBLICKEYBYTES * 2))
	{
		Serial.println("Error loading existing signing public key!");
	}
	if (!hexToBin(signPrivateKey, SIGN_PRIVATE_KEY, crypto_sign_SECRETKEYBYTES * 2))
	{
		Serial.println("Error loading existing signing private key!");
	}

	/*
	//Serial.print("Data Receiver Key Str: ");
	//Serial.println(DATA_RECEIVER_PUBLIC_KEY);
	Serial.print("Data Receiver Key Hex: ");
	hexPrint(dataReceiverPublicKey, crypto_sign_PUBLICKEYBYTES);
	Serial.println();

	//Serial.print("Public Key Str: ");
	//Serial.println(PUBLIC_KEY);
	Serial.print("Public Key Hex: ");
	hexPrint(publicKey, crypto_kx_PUBLICKEYBYTES);
	Serial.println();

	//Serial.print("Private Key Str: ");
	//Serial.println(PRIVATE_KEY);
	Serial.print("Private Key Hex: ");
	hexPrint(privateKey, crypto_kx_SECRETKEYBYTES);
	Serial.println();

	//Serial.print("Signing Public Key Str: ");
	//Serial.println(SIGN_PUBLIC_KEY);
	Serial.print("Signing Public Key Hex: ");
	hexPrint(signPublicKey, crypto_sign_PUBLICKEYBYTES);
	Serial.println();

	//Serial.print("Signing Private Key Str: ");
	//Serial.println(SIGN_PRIVATE_KEY);
	Serial.print("Signing Private Key Hex: ");
	hexPrint(signPrivateKey, crypto_sign_SECRETKEYBYTES);
	Serial.println();
	*/

	generateSharedKeys(); // TODO: Check return value

	Serial.println("\nDone setup()");
}

// TODO: Flash Memory stuff
// TODO: Data Receiver Key

void
loop(void)
{
	char sensorData[156]; // TODO: Remove magic number
	Serial.println("Begin main loop...");

	memcpy(sensorData, "This is some sensor data /-/-", 30);

	sendData(sensorData, 30);

	Serial.println("Returned to main loop and waiting for next iteration...");

	delay(DATA_SEND_INTERVAL);
}



void
encryptData(struct packet_data* pd, char* data, uint8_t dataLen)
{
	Serial.println("encryptData()");

	//memcpy(pd->data, data, dataLen); // TODO REMOVE
	crypto_stream_xchacha20_xor( // TODO: Check return value?
		pd->data,
		(unsigned char*) data,
		dataLen,
		pd->crypto_nonce,
		rxSharedKey
	);
}



void
signPacket(struct packet* p)
{
	crypto_sign_state state;
	uint8_t maskedFlags;

	Serial.println("signPacket()");

	crypto_sign_init(&state); // TODO/NOTE: The example code omitted semicolons...
	//crypto_sign_update(&state, &p->to, sizeof(p->to)); // TODO: To field is ignored so any gateway can process this
	crypto_sign_update(&state, (unsigned char*)&p->from, sizeof(p->from)); // TODO: Probably should convert these to network byte order
	crypto_sign_update(&state, &p->id, sizeof(p->id));
	crypto_sign_update(&state, &p->fragment, sizeof(p->fragment));
	maskedFlags = p->flags & 0xF;
	crypto_sign_update(&state, &maskedFlags, sizeof(maskedFlags));
	crypto_sign_update(&state, &p->len, sizeof(p->len));

	// TODO: When multi packet messages are supported, the
	// signatures are for the stream of messages and this
	// function needs to be modified accordingly.

	if (maskedFlags == PACKET_TYPE_DATA_FIRST)
	{
		crypto_sign_update(
			&state,
			p->payload.data.crypto_nonce,
			sizeof(p->payload.data.crypto_nonce));
		crypto_sign_update(
			&state,
			p->payload.data.data,
			p->len - 13 - sizeof(p->payload.data.crypto_nonce) - sizeof(p->payload.data.signature) // TODO Change this on LoRaTRX
		);
	}
	else
	{
		crypto_sign_update(
			&state,
			p->payload.data.data,
			p->len - 13);
	}

	crypto_sign_final_create(
		&state,
		p->payload.data.signature,
		NULL,
		signPrivateKey);
	Serial.print("Signature: ");
	hexPrint(p->payload.data.signature, crypto_sign_BYTES);

	Serial.println();
}



void
transmitData(struct packet* p)
{
	struct packet recvP;

	Serial.println("transmitData():1");

	rf95.setHeaderTo(p->to);
	rf95.setHeaderFrom(p->from);
	rf95.setHeaderId(p->id);
	rf95.setHeaderFlags(p->flags);
	rf95.setHeaderFragment(p->fragment);

	// rf95.send((uint8_t *) p->payload.bytes, 13 + p->len); // TODO: Magic numbers
	rf95.send((uint8_t*)p->payload.bytes, p->len);
	rf95.waitPacketSent();
	Serial.println("transmitData(): Packet Sent!");

	if (rf95.waitAvailableTimeout(REPLY_TIMEOUT))
	{

		if (rf95.recv((uint8_t*)recvP.payload.bytes, &recvP.len))
		{
			Serial.println("Received reply");

			recvP.from = rf95.headerFrom();
			recvP.to = rf95.headerTo();
			recvP.id = rf95.headerId();
			recvP.fragment = rf95.headerFragment();
			recvP.flags = rf95.headerFlags();
			recvP.rssi = rf95.lastRssi();

			processReply(&recvP);
		}
		else
		{
			Serial.println("Receive failed");
		}
	}
	else
	{
		Serial.println("No reply received within timeout");
	}

	rf95.sleep();
}



void
processReply(struct packet* p)
{
	// TODO
	Serial.println("processReply()");
	return;
}



bool
sendData(char* data, uint8_t dataLen)
{
	struct packet p;

	//Currently, only a max of 154 bytes can be sent
	if (dataLen > 154)
		return false;


	p.to = 0;
	p.from = DEVICE_ID;
	p.flags = PACKET_TYPE_DATA_FIRST;
	p.id = 0;
	p.fragment = 0;
	// TODO: Magic number
	p.len = 13 + dataLen + sizeof(p.payload.data.crypto_nonce) + sizeof(p.payload.data.signature); // TODO: This is only valid for PACKET_TYPE_DATA_FIRST

	//randombytes_buf(p.payload.data.crypto_nonce, crypto_secretbox_NONCEBYTES);
	customRandBytes(p.payload.data.crypto_nonce, crypto_secretbox_NONCEBYTES);
	encryptData(&p.payload.data, data, dataLen);
	signPacket(&p);
	transmitData(&p);

	return true;
}



void
generateKeys(void)
{
	Serial.println("generateKeys()");

	if (crypto_kx_keypair(publicKey, privateKey) != 0)
	{
		Serial.println("crypto_kx_keypair() Failed!");
	}

	if (crypto_sign_keypair(signPublicKey, signPrivateKey) != 0)
	{
		Serial.println("crypto_sign_keypair() Failed!");
	}

	customRandBytes(dataReceiverPublicKey, crypto_sign_PUBLICKEYBYTES);

	Serial.print("dataReceiver Key: ");
	hexPrint(dataReceiverPublicKey, crypto_kx_PUBLICKEYBYTES);
	Serial.println("");

	Serial.print("      Public Key: ");
	hexPrint(publicKey, crypto_kx_PUBLICKEYBYTES);
	Serial.println("");

	Serial.print("     Private Key: ");
	hexPrint(privateKey, crypto_kx_SECRETKEYBYTES);
	Serial.println("");

	Serial.print(" Sign Public Key: ");
	hexPrint(signPublicKey, crypto_sign_PUBLICKEYBYTES);
	Serial.println("");

	Serial.print("Sign Private Key: ");
	hexPrint(signPrivateKey, crypto_sign_SECRETKEYBYTES);
	Serial.println("");
}



bool
generateSharedKeys(void)
{
	Serial.println("generateSharedKeys()");
	if (crypto_kx_client_session_keys(rxSharedKey, txSharedKey, publicKey, privateKey, dataReceiverPublicKey) != 0)
	{
		Serial.println("generateSharedKeys() FAILED");
		return false;
	}
	Serial.print("txKey: ");
	hexPrint(txSharedKey, crypto_kx_SESSIONKEYBYTES);
	Serial.print("rxKey: ");
	hexPrint(rxSharedKey, crypto_kx_SESSIONKEYBYTES);
	return true;
}



void
customRandSeed(void)
{
	Serial.println("customRandSeed()");
	randomSeed(analogRead(0)); // TODO: Verify this pin is floating!!! Make #define for it
}



uint32_t
customRand(void)
{
	//Serial.println("customRand()");
	return (uint32_t)random(UINT32_MAX);
}



const char*
customRandName(void)
{
	Serial.println("customRandName()");
	return "m0";
}



void
customRandBytes(void* buf, const size_t size)
{
	Serial.println("customRandBytes()");
	// for (uint16_t i = 0; i < size; i += 4u) {
	// 	if (i + 3u < size) {
	// 		((uint32_t*) buf)[i] = customRand();
	// 	} else {
	// 		for (uint8_t j = i; j < size; j++) {
	// 			((uint8_t*) buf)[j] = (uint8_t) customRand();
	// 		}
	// 	}
	// }
	uint32_t r = 0;
	uint8_t* byte = &((uint8_t*)(&r))[0];
	uint8_t* arr = (uint8_t*)buf;
	for (uint8_t j = 0; j < size; j++)
	{
		r = random(0, 255);
		arr[j] = *byte;
		//	Serial.print(arr[j], HEX);
	}
	//Serial.println("");
}