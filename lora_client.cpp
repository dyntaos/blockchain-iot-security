#include <iostream>
#include <string>
#include <array>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include <libconfig.h++>
#include <cpp-base64/base64.h>
#include <misc.hpp>
#include <sodium.h>
#include <eth_interface_except.hpp>
#include <lora_client.hpp>


using namespace std;
using namespace eth_interface;



RH_RF95 rf95(RF_CS_PIN, RF_IRQ_PIN);

libconfig::Config cfg;
libconfig::Setting* cfgRoot;

uint32_t loraDeviceId;

uint16_t packetnum = 0U;
uint8_t flags = 255U;
uint8_t fragment = 0U;
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



bool
setup(void)
{
	string msg;

	if (!bcm2835_init())
	{
		cerr << __BASE_FILE__ << " bcm2835_init() Failed" << endl
			 << endl;
		return 1;
	}

	msg = "RF95 CS=GPIO" + to_string(RF_CS_PIN);

#ifdef RF_IRQ_PIN
	msg += ", IRQ=GPIO" + to_string(RF_IRQ_PIN);
	// IRQ Pin input/pull down
	pinMode(RF_IRQ_PIN, INPUT);
	bcm2835_gpio_set_pud(RF_IRQ_PIN, BCM2835_GPIO_PUD_DOWN);
	// Now we can enable Rising edge detection
	bcm2835_gpio_ren(RF_IRQ_PIN);
#endif

#ifdef RF_RST_PIN
	msg += ", RST=GPIO" + to_string(RF_RST_PIN);
	// Pulse a reset on module
	pinMode(RF_RST_PIN, OUTPUT);
	digitalWrite(RF_RST_PIN, LOW);
	bcm2835_delay(150);
	digitalWrite(RF_RST_PIN, HIGH);
	bcm2835_delay(100);
#endif

	if (!rf95.init())
	{
		cerr << endl
			 << "RF95 module init failed, Please verify wiring/module" << endl;
	}
	else
	{
		rf95.setTxPower(14, false);

		// You can optionally require this module to wait until Channel Activity
		// Detection shows no activity on the channel before transmitting by setting
		// the CAD timeout to non-zero:
		//rf95.setCADTimeout(10000);

		rf95.setFrequency(RF_FREQUENCY);
		rf95.setPromiscuous(true);
		rf95.setModeRx();

		rf95.setThisAddress(loraDeviceId);
		rf95.setHeaderFrom(loraDeviceId);

		cout << msg << " Radio Initialized @ " << RF_FREQUENCY << "MHz" << endl;
		return true;
	}

	return false;
}



void
loadConfig(void)
{
	vector<char> byteVector;
	string temp;

	cfg.setOptions(
		  libconfig::Config::OptionFsync
		| libconfig::Config::OptionSemicolonSeparators
		| libconfig::Config::OptionColonAssignmentForGroups
		| libconfig::Config::OptionOpenBraceOnSeparateLine);

	cout << "Reading config file..." << endl;

	try
	{
		cfg.readFile(BLOCKCHAINSEC_CONFIG_F);
	}
	catch (const libconfig::FileIOException& e)
	{
		cerr << "IO error reading config file!" << endl;
		exit(EXIT_FAILURE);
	}
	catch (const libconfig::ParseException& e)
	{
		cerr << "Config file error "
			 << e.getFile()
			 << ":"
			 << e.getLine()
			 << " - "
			 << e.getError()
			 << endl;
		exit(EXIT_FAILURE);
	}

	cfgRoot = &cfg.getRoot();

	if (!cfg.exists("loraDeviceId"))
	{
		cerr << "Configuration file does not contain 'loraDeviceId'!" << endl;
		exit(EXIT_FAILURE);
	}
	cfg.lookupValue("loraDeviceId", temp);
	if (!isInt(temp))
	{
		cerr << "Configuration file contains an error for 'loraDeviceId': Must be an unsigned integer!" << endl;
		exit(EXIT_FAILURE);
	}
	loraDeviceId = strtoul(temp.c_str(), nullptr, 10);


	if (!cfg.exists("publicKey"))
	{
		cerr << "Configuration file does not contain 'publicKey'!" << endl;
		exit(EXIT_FAILURE);
	}
	cfg.lookupValue("publicKey", temp);
	if (!isHex(temp))
	{
		cerr << "Configuration file contains an error for 'publicKey': Value is not valid hexidecimal!" << endl;
		exit(EXIT_FAILURE);
	}
	if (temp.length() != crypto_kx_PUBLICKEYBYTES * 2)
	{
		cerr << "Configuration file contains an error for 'publicKey': Length must be "
			<< crypto_kx_PUBLICKEYBYTES * 2
			<< " characters and not include \"0x\" as a prefix!" << endl;
		exit(EXIT_FAILURE);
	}
	byteVector = hexToBytes(temp);
	memcpy(publicKey, byteVector.data(), crypto_kx_PUBLICKEYBYTES);


	if (!cfg.exists("privateKey"))
	{
		cerr << "Configuration file does not contain 'privateKey'!" << endl;
		exit(EXIT_FAILURE);
	}
	cfg.lookupValue("privateKey", temp);
	if (!isHex(temp))
	{
		cerr << "Configuration file contains an error for 'privateKey': Value is not valid hexidecimal!" << endl;
		exit(EXIT_FAILURE);
	}
	if (temp.length() != crypto_kx_SECRETKEYBYTES * 2)
	{
		cerr << "Configuration file contains an error for 'privateKey': Length must be "
			<< crypto_kx_SECRETKEYBYTES * 2
			<< " characters and not include \"0x\" as a prefix!" << endl;
		exit(EXIT_FAILURE);
	}
	byteVector = hexToBytes(temp);
	memcpy(privateKey, byteVector.data(), crypto_kx_SECRETKEYBYTES);


	if (!cfg.exists("signPublicKey"))
	{
		cerr << "Configuration file does not contain 'signPublicKey'!" << endl;
		exit(EXIT_FAILURE);
	}
	cfg.lookupValue("signPublicKey", temp);
	if (!isHex(temp))
	{
		cerr << "Configuration file contains an error for 'signPublicKey': Value is not valid hexidecimal!" << endl;
		exit(EXIT_FAILURE);
	}
	if (temp.length() != crypto_sign_PUBLICKEYBYTES * 2)
	{
		cerr << "Configuration file contains an error for 'signPublicKey': Length must be "
			<< crypto_sign_PUBLICKEYBYTES * 2
			<< " characters and not include \"0x\" as a prefix!" << endl;
		exit(EXIT_FAILURE);
	}
	byteVector = hexToBytes(temp);
	memcpy(signPublicKey, byteVector.data(), crypto_sign_PUBLICKEYBYTES);


	if (!cfg.exists("signPrivateKey"))
	{
		cerr << "Configuration file does not contain 'signPrivateKey'!" << endl;
		exit(EXIT_FAILURE);
	}
	cfg.lookupValue("signPrivateKey", temp);
	if (!isHex(temp))
	{
		cerr << "Configuration file contains an error for 'signPrivateKey': Value is not valid hexidecimal!" << endl;
		exit(EXIT_FAILURE);
	}
	if (temp.length() != crypto_sign_SECRETKEYBYTES * 2)
	{
		cerr << "Configuration file contains an error for 'signPrivateKey': Length must be "
			<< crypto_sign_SECRETKEYBYTES * 2
			<< " characters and not include \"0x\" as a prefix!" << endl;
		exit(EXIT_FAILURE);
	}
	byteVector = hexToBytes(temp);
	memcpy(signPrivateKey, byteVector.data(), crypto_sign_SECRETKEYBYTES);


	if (!cfg.exists("dataReceiverPublicKey"))
	{
		cerr << "Configuration file does not contain 'dataReceiverPublicKey'!" << endl;
		exit(EXIT_FAILURE);
	}
	cfg.lookupValue("dataReceiverPublicKey", temp);
	if (!isHex(temp))
	{
		cerr << "Configuration file contains an error for 'dataReceiverPublicKey': Value is not valid hexidecimal!" << endl;
		exit(EXIT_FAILURE);
	}
	if (temp.length() != crypto_kx_PUBLICKEYBYTES * 2)
	{
		cerr << "Configuration file contains an error for 'dataReceiverPublicKey': Length must be "
			<< crypto_kx_PUBLICKEYBYTES * 2
			<< " characters and not include \"0x\" as a prefix!" << endl;
		exit(EXIT_FAILURE);
	}
	byteVector = hexToBytes(temp);
	memcpy(dataReceiverPublicKey, byteVector.data(), crypto_kx_PUBLICKEYBYTES);

	cout << "Configuration file successfully loaded" << endl;
}



string
runBinary(string cmd)
{
	string result;
	FILE *inFd;
	array<char, SENSOR_PIPE_BUFFER_LEN> pipeBuffer;

	inFd = popen(cmd.c_str(), "r");

	if (fgets(pipeBuffer.data(), SENSOR_PIPE_BUFFER_LEN, inFd) == NULL)
	{
		throw ResourceRequestFailedException("runBinary(\"" + cmd + "\"): Error: Failed to read from pipe!");
	}

	result += pipeBuffer.data();

	while (fgets(pipeBuffer.data(), SENSOR_PIPE_BUFFER_LEN, inFd) != NULL)
	{
		result += pipeBuffer.data();
	}

	if (pclose(inFd) < 0)
	{
		throw ResourceRequestFailedException("runBinary(\"" + cmd + "\"): Failed to pclose()!");
	}
	return result;
}



string
querySensor(void)
{
	string data;

	try
	{
		data += runBinary("uptime");
	}
	catch (ResourceRequestFailedException &e)
	{
		data += e.what();
	}

	return data;
}



void
sendSensorData(void)
{
	string data;

	for (;;)
	{
		data = querySensor();
		sendData(data.c_str(), data.length());

		bcm2835_delay(DATA_SEND_INTERVAL);
	}
}



void
encryptData(struct packet_data* pd, const char* data, uint8_t dataLen)
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
			p->len - 13 - sizeof(p->payload.data.crypto_nonce) - sizeof(p->payload.data.signature)
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

#ifdef VERBOSE
	Serial.println("transmitData(): Packet Sent!");
#endif

	if (rf95.waitAvailableTimeout(REPLY_TIMEOUT))
	{

		if (rf95.recv((uint8_t*)recvP.payload.bytes, &recvP.len))
		{
#ifdef VERBOSE
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
#ifdef VERBOSE
		else
		{
			Serial.println("Receive failed");
		}
#endif
	}
#ifdef VERBOSE
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
	(void) p;
#ifdef VERBOSE
	Serial.println("processReply()");
#endif
	return;
}



bool
sendData(const char* data, uint8_t dataLen)
{
	struct packet p;

	//Currently, only a max of 156 bytes can be sent
	if (dataLen > 156)
		return false;


	p.to = 0;
	p.from = DEVICE_ID;
	p.flags = PACKET_TYPE_DATA_FIRST;
	p.id = 0;
	p.fragment = 0;
	// TODO: Magic number
	p.len = 13 + dataLen + sizeof(p.payload.data.crypto_nonce) + sizeof(p.payload.data.signature); // TODO: This is only valid for PACKET_TYPE_DATA_FIRST

	randombytes_buf(p.payload.data.crypto_nonce, crypto_secretbox_NONCEBYTES);
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
#ifdef VERBOSE
		Serial.println("crypto_kx_keypair() Failed!");
#endif
	}

	if (crypto_sign_keypair(signPublicKey, signPrivateKey) != 0)
	{
#ifdef VERBOSE
		Serial.println("crypto_sign_keypair() Failed!");
#endif
	}

	randombytes_buf(dataReceiverPublicKey, crypto_sign_PUBLICKEYBYTES);

#ifdef VERBOSE
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
#ifdef VERBOSE
		Serial.println("generateSharedKeys() FAILED");
#endif
		return false;
	}
#ifdef VERBOSE
	Serial.print("txKey: ");
	hexPrint(txSharedKey, crypto_kx_SESSIONKEYBYTES);
	Serial.print("\n\rrxKey: ");
	hexPrint(rxSharedKey, crypto_kx_SESSIONKEYBYTES);
#endif
	return true;
}



int
main(int argc, char *argv[])
{
	// Check if root
	if (geteuid() != 0)
	{
		cerr << "This must be run as root (sudo)"
			<< endl;
		exit(EXIT_FAILURE);
	}

	if (argc == 2 && strcmp("-s", argv[1]) != 0)
	{
		cerr << "Unknown flag: " << argv[1]
			 << endl;
	}

	loadConfig();

	if (!setup())
	{
		return EXIT_FAILURE;
	}

	if (argc == 2 && strcmp("-s", argv[1]) != 0)
	{
		for (;;)
		{
			sendSensorData();
		}
	}
	else
	{

	}

	return EXIT_SUCCESS;
}
