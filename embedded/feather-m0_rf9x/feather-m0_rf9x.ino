#include "feather-m0.h"
#include "libsodium_include/sodium.h"
#include "packet.hpp"


//#define SERIAL_ON

#define ERROR_RF95_INIT					11
#define ERROR_SET_FREQUENCY				12
#define ERROR_SODIUM_INIT				21
#define ERROR_LOAD_DATA_RECV_KEY		22
#define ERROR_LOAD_ENC_PUBLIC_KEY		23
#define ERROR_LOAD_ENC_PRIVATE_KEY		24
#define ERROR_LOAD_SIGN_PUBLIC_KEY		25
#define ERROR_LOAD_SIGN_PRIVATE_KEY		26



int16_t packetnum = 0; // packet counter, we increment per transmission
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


void
flash_error_code(uint8_t code)
{
	uint8_t ones = code % 10;
	uint8_t tens = ((code % 100) - ones) / 10;

	while (1)
	{
		for (uint8_t i = 0; i < tens; i++) {
			digitalWrite(LED_BUILTIN, HIGH);
			delay(500);
			digitalWrite(LED_BUILTIN, LOW);
			delay(300);
		}
		delay(600);
		for (uint8_t i = 0; i < ones; i++) {
			digitalWrite(LED_BUILTIN, HIGH);
			delay(200);
			digitalWrite(LED_BUILTIN, LOW);
			delay(300);
		}
		delay(3000);
	}
}


// From: https://forum.arduino.cc/index.php?topic=8774.0
int
uptime(char *s, size_t n)
{
	long days  = 0;
	long hours = 0;
	long mins  = 0;
	long secs  = 0;

	secs  = millis() / 1000;
	mins  = secs / 60;
	hours = mins / 60;
	days  = hours / 24;
	secs  = secs - (mins * 60);
	mins  = mins - (hours * 60);
	hours = hours - (days * 24);

	return snprintf(s, n, "%ldd %ld:%ld:%ld", days, hours, mins, secs);
}


void
setup(void)
{
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(RFM95_RST, OUTPUT);
	digitalWrite(RFM95_RST, HIGH);

#ifdef SERIAL_ON
	Serial.begin(115200);
	while (!Serial)
	{ // TODO: Change to boot without Serial
		delay(50);
	}
#endif

	delay(100);

	// Reset RF95 Radio
	digitalWrite(RFM95_RST, LOW);
	delay(10);
	digitalWrite(RFM95_RST, HIGH);
	delay(10);

	while (!rf95.init())
	{
#ifdef SERIAL_ON
		Serial.println("LoRa radio init failed");
#endif

		flash_error_code(ERROR_RF95_INIT);
	}

#ifdef SERIAL_ON
	Serial.println("LoRa radio initialized");
#endif

	if (!rf95.setFrequency(RF95_FREQ))
	{
#ifdef SERIAL_ON
		Serial.println("setFrequency failed");
#endif

		flash_error_code(ERROR_SET_FREQUENCY);
	}

#ifdef SERIAL_ON
	Serial.print("Set Freq to: ");
	Serial.println(RF95_FREQ);
#endif

	// The default transmitter power is 13dBm, using PA_BOOST.
	// If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
	// you can set transmitter powers from 5 to 23 dBm:
	rf95.setTxPower(23, false);

	rf95.setThisAddress(DEVICE_ID);

	randombytes_set_implementation(&randImplM0);

	if (sodium_init() == -1)
	{
		flash_error_code(ERROR_SODIUM_INIT);
	}


	//generateKeys();

	if (!hexToBin(dataReceiverPublicKey, DATA_RECEIVER_PUBLIC_KEY, crypto_sign_PUBLICKEYBYTES * 2))
	{
#ifdef SERIAL_ON
		Serial.println("Error loading data receiver public key!");
#endif
		flash_error_code(ERROR_LOAD_DATA_RECV_KEY);
	}

	if (!hexToBin(publicKey, PUBLIC_KEY, crypto_kx_PUBLICKEYBYTES * 2))
	{
#ifdef SERIAL_ON
		Serial.println("Error loading existing public key!");
#endif
		flash_error_code(ERROR_LOAD_ENC_PUBLIC_KEY);
	}

	if (!hexToBin(privateKey, PRIVATE_KEY, crypto_kx_SECRETKEYBYTES * 2))
	{
#ifdef SERIAL_ON
		Serial.println("Error loading existing private key!");
#endif
		flash_error_code(ERROR_LOAD_ENC_PRIVATE_KEY);
	}

	if (!hexToBin(signPublicKey, SIGN_PUBLIC_KEY, crypto_sign_PUBLICKEYBYTES * 2))
	{
#ifdef SERIAL_ON
		Serial.println("Error loading existing signing public key!");
#endif
		flash_error_code(ERROR_LOAD_SIGN_PUBLIC_KEY);
	}

	if (!hexToBin(signPrivateKey, SIGN_PRIVATE_KEY, crypto_sign_SECRETKEYBYTES * 2))
	{
#ifdef SERIAL_ON
		Serial.println("Error loading existing signing private key!");
#endif
		flash_error_code(ERROR_LOAD_SIGN_PRIVATE_KEY);
	}

	generateSharedKeys(); // TODO: Check return value

#ifdef SERIAL_ON
	Serial.println("\n\rDone setup()");
#endif

	digitalWrite(LED_BUILTIN, LOW);
}


void
loop(void)
{
	float battVoltage;
	int pos = 0;
	char sensorData[156]; // TODO: Remove magic number

#ifdef SERIAL_ON
	Serial.println("Begin main loop...");
#endif

	// Read battery voltage: https://learn.adafruit.com/adafruit-feather-m0-basic-proto/power-management
	battVoltage = analogRead(BATT_VOLTAGE_PIN);
	battVoltage *= 2;    // we divided by 2, so multiply back
	battVoltage *= 3.3;  // Multiply by 3.3V, our reference voltage
	battVoltage /= 1024; // convert to voltage

#ifdef SERIAL_ON
	Serial.print("Battery voltage: ");
	Serial.println(battVoltage);
#endif

	pos = sprintf(
		sensorData,
		"%d.%dV\n",
		(int) battVoltage,
		((int) (battVoltage * (100))) - (((int) battVoltage) * (100)));

	pos += uptime(&sensorData[pos], ((size_t) sizeof(sensorData) - pos));

#ifdef SERIAL_ON
	Serial.println("Data:");
	Serial.println(sensorData);
#endif

	sendData(sensorData, pos);

#ifdef SERIAL_ON
	Serial.println("Returned to main loop and waiting for next iteration...");
#endif

	delay(DATA_SEND_INTERVAL);
}



void
encryptData(struct packet_data* pd, char* data, uint8_t dataLen)
{
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

	crypto_sign_init(&state);
	//crypto_sign_update(&state, &p->to, sizeof(p->to)); // TODO: To field is ignored so any gateway can process this
	crypto_sign_update(&state, (unsigned char*)&p->from, sizeof(p->from)); // TODO: Convert these to network byte order
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

#ifdef SERIAL_ON
	Serial.print("Signature: ");
	hexPrint(p->payload.data.signature, crypto_sign_BYTES);
	Serial.println();
#endif
}



void
transmitData(struct packet* p)
{
	struct packet recvP;

	rf95.setHeaderTo(p->to);
	rf95.setHeaderFrom(p->from);
	rf95.setHeaderId(p->id);
	rf95.setHeaderFlags(p->flags);
	rf95.setHeaderFragment(p->fragment);

	// rf95.send((uint8_t *) p->payload.bytes, 13 + p->len); // TODO: Magic numbers
	rf95.send((uint8_t*)p->payload.bytes, p->len);
	rf95.waitPacketSent();

#ifdef SERIAL_ON
	Serial.println("transmitData(): Packet Sent!");
#endif

	if (rf95.waitAvailableTimeout(REPLY_TIMEOUT))
	{

		if (rf95.recv((uint8_t*)recvP.payload.bytes, &recvP.len))
		{
#ifdef SERIAL_ON
			Serial.println("Received reply");
#endif

			recvP.from = rf95.headerFrom();
			recvP.to = rf95.headerTo();
			recvP.id = rf95.headerId();
			recvP.fragment = rf95.headerFragment();
			recvP.flags = rf95.headerFlags();
			recvP.rssi = rf95.lastRssi();

			processReply(&recvP);
		}
#ifdef SERIAL_ON
		else
		{
			Serial.println("Receive failed");
		}
#endif
	}
#ifdef SERIAL_ON
	else
	{
		Serial.println("No reply received within timeout");
	}
#endif

	rf95.sleep();
}



void
processReply(struct packet* p)
{
#ifdef SERIAL_ON
	Serial.println("processReply()");
#endif
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

	customRandBytes(p.payload.data.crypto_nonce, crypto_secretbox_NONCEBYTES);
	encryptData(&p.payload.data, data, dataLen);
	signPacket(&p);
	transmitData(&p);

	return true;
}



void
generateKeys(void)
{
	if (crypto_kx_keypair(publicKey, privateKey) != 0)
	{
#ifdef SERIAL_ON
		Serial.println("crypto_kx_keypair() Failed!");
#endif
	}

	if (crypto_sign_keypair(signPublicKey, signPrivateKey) != 0)
	{
#ifdef SERIAL_ON
		Serial.println("crypto_sign_keypair() Failed!");
#endif
	}

	customRandBytes(dataReceiverPublicKey, crypto_sign_PUBLICKEYBYTES);

#ifdef SERIAL_ON
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
#endif
}



bool
generateSharedKeys(void)
{
	if (crypto_kx_client_session_keys(rxSharedKey, txSharedKey, publicKey, privateKey, dataReceiverPublicKey) != 0)
	{
#ifdef SERIAL_ON
		Serial.println("generateSharedKeys() FAILED");
#endif
		return false;
	}
#ifdef SERIAL_ON
	Serial.print("txKey: ");
	hexPrint(txSharedKey, crypto_kx_SESSIONKEYBYTES);
	Serial.print("\n\rrxKey: ");
	hexPrint(rxSharedKey, crypto_kx_SESSIONKEYBYTES);
#endif
	return true;
}



void
customRandSeed(void)
{
	randomSeed(analogRead(0)); // This pin must be floating!!!
}



uint32_t
customRand(void)
{
	return (uint32_t)random(UINT32_MAX);
}



const char*
customRandName(void)
{
	return "m0";
}



void
customRandBytes(void* buf, const size_t size)
{
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
	}
}