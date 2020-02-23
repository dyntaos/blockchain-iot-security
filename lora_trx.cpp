#include <iostream>
#include <string>

#include <sys/types.h>
#include <string.h>
#include <fcntl.h>

#include <misc.hpp>
#include <lora_trx.hpp>


using namespace std;

namespace blockchainSec {


RH_RF95 rf95(RF_CS_PIN, RF_IRQ_PIN);


LoraTrx::LoraTrx(uint32_t gatewayDeviceId, BlockchainSecLib *blockchainSec) {
	if (blockchainSec == NULL) {
		throw InvalidArgumentException(
			"blockchainSec argument to LoraTrx"
			" constructor is a null pointer"
		);
	}
	this->gatewayDeviceId = gatewayDeviceId;
	this->blockchainSec = blockchainSec;

	if (!blockchainSec->is_gateway(blockchainSec->get_my_device_id())) {
		cerr
			<< "WARNING: This device was started as a LoRa gateway"
			<< " but is not registered as a gateway on Ethereum!"
			<< endl;
	}
}


bool LoraTrx::setup(void) {

	if (!bcm2835_init()) {
		cerr << __BASE_FILE__ << " bcm2835_init() Failed" << endl << endl;
		return 1;
	}

	printf( "RF95 CS=GPIO%d", RF_CS_PIN);

#ifdef RF_IRQ_PIN
	printf( ", IRQ=GPIO%d", RF_IRQ_PIN );
	// IRQ Pin input/pull down
	pinMode(RF_IRQ_PIN, INPUT);
	bcm2835_gpio_set_pud(RF_IRQ_PIN, BCM2835_GPIO_PUD_DOWN);
	// Now we can enable Rising edge detection
	bcm2835_gpio_ren(RF_IRQ_PIN);
#endif

#ifdef RF_RST_PIN
	printf( ", RST=GPIO%d", RF_RST_PIN );
	// Pulse a reset on module
	pinMode(RF_RST_PIN, OUTPUT);
	digitalWrite(RF_RST_PIN, LOW );
	bcm2835_delay(150);
	digitalWrite(RF_RST_PIN, HIGH );
	bcm2835_delay(100);
#endif

	if (!rf95.init()) {
		cerr << endl << "RF95 module init failed, Please verify wiring/module" << endl;
	} else {
		rf95.setTxPower(14, false);

		// You can optionally require this module to wait until Channel Activity
		// Detection shows no activity on the channel before transmitting by setting
		// the CAD timeout to non-zero:
		//rf95.setCADTimeout(10000);

		rf95.setFrequency(RF_FREQUENCY);

		// If we need to send something
		rf95.setThisAddress(gatewayDeviceId);
		rf95.setHeaderFrom(gatewayDeviceId);

		// Be sure to grab all node packet
		// we're sniffing to display, it's a demo
		rf95.setPromiscuous(true);

		// We're ready to listen for incoming message
		rf95.setModeRx();

		cout << " Radio Initialized @ " << RF_FREQUENCY << "MHz" << endl;
		hardwareInitialized = true;
		return true;
	}

	return false;
}



void LoraTrx::server_init(void) {

	halt_server = false;

	server_thread = new thread(
		this->serverThread,
		std::ref(rx_queue),
		std::ref(tx_queue),
		std::ref(rx_queue_mutex),
		std::ref(tx_queue_mutex),
		std::ref(rx_queue_condvar),
		std::ref(halt_server),
		std::ref(*this)
	);

	forwarder_thread = new thread(
		this->forwarderThread,
		std::ref(halt_server),
		std::ref(*this)
	);
}



void LoraTrx::close_server(void) {
	halt_server = true;
	rx_queue_condvar.notify_all();

	if (server_thread != NULL) server_thread->join();
	if (forwarder_thread != NULL) forwarder_thread->join();
}



void LoraTrx::serverThread(queue<struct packet*> &rx_queue, queue<struct packet*> &tx_queue, mutex &rx_queue_mutex, mutex &tx_queue_mutex, condition_variable &rx_queue_condvar, bool &halt_server, LoraTrx &trx) {
	string msg;
	struct packet *msg_buffer = NULL, *tx_buffer = NULL;
	bool tx_mode = false;

	if (!trx.hardwareInitialized) {
		if (!trx.setup()) {
			halt_server = true;
			cerr << "The gateway server was unable to initialize hardware!" << endl;
		}
	}

	while (!halt_server) {

		if (msg_buffer == NULL) {
			msg_buffer = new struct packet;
		}

		tx_queue_mutex.lock();
		if (!tx_queue.empty()) {
			tx_buffer = tx_queue.front();
			tx_queue.pop();
			tx_queue_mutex.unlock();

			if (!tx_mode) {
				rf95.setModeTx();
				tx_mode = true;
			}

			rf95.setHeaderTo(tx_buffer->to);

			if (!rf95.send((uint8_t*) &tx_buffer->payload.bytes, tx_buffer->len)) {
				cerr << "Error transmitting packet..." << endl;
				// TODO: Discard or retry packet? Keep track of attempts of packet and try X times?
			} else {
				rf95.waitPacketSent();
			}

			delete tx_buffer;
			tx_buffer = NULL;
		} else tx_queue_mutex.unlock();

		if (tx_mode) {
			rf95.setModeRx();
			tx_mode = false;
		}

		if (rf95.available()) {

			uint8_t buf[RH_RF95_MAX_MESSAGE_LEN + 1];
			msg_buffer->len   = RH_RF95_MAX_MESSAGE_LEN;

			msg_buffer->from     = rf95.headerFrom();
			msg_buffer->to       = rf95.headerTo();
			msg_buffer->id       = rf95.headerId();
			msg_buffer->fragment = rf95.headerFragment();
			msg_buffer->flags    = rf95.headerFlags();
			msg_buffer->rssi     = rf95.lastRssi();

			if (rf95.recv(buf, &msg_buffer->len)) {

				memcpy(msg_buffer->payload.bytes, buf, msg_buffer->len);

#ifdef _DEBUG
				string printbuffer = hexStr(buf, msg_buffer->len);
				cout << "*Packet* " << endl
					<< "\tLength: " << unsigned(msg_buffer->len) << endl
					<< "\tFrom: " << unsigned(msg_buffer->from) << endl
					<< "\tTo: " << unsigned(msg_buffer->to) << endl
					<< "\tID: " << unsigned(msg_buffer->id) << endl
					<< "\tFragment: " << unsigned(msg_buffer->fragment) << endl
					<< "\tFlags: " << unsigned(msg_buffer->flags) << endl
					<< "\tRSSI: " << unsigned(msg_buffer->rssi) << endl
					<< "\tMessage Hex: " << printbuffer << endl
					<< "\tMessage ASCII: " << string((char*) msg_buffer->payload.bytes, msg_buffer->len)
					<< endl << endl
					<< "\t   Good received packet count: " << unsigned(rf95.rxGood()) << endl
					<< "\t    Bad received packet count: " << unsigned(rf95.rxBad()) << endl
					<< "\tGood transmitted packet count: " << unsigned(rf95.txGood())
					<< endl << endl;
#endif // _DEBUG

			} else {
				cerr << "Error receiving packet..." << endl;
				continue;
			}

			rx_queue_mutex.lock();
			rx_queue.push(msg_buffer);
			rx_queue_mutex.unlock();

			rx_queue_condvar.notify_all();

			msg_buffer = NULL;
		}

		bcm2835_delay(1); // TODO: Should we wait at all? Watch CPU loads...
	}

	halt_server = true;
}



void LoraTrx::forwarderThread(bool &halt_server, LoraTrx &trx) {
	struct packet *p;

// #ifdef _DEBUG // TODO: Uncomment
	cout
		<< "LoRa forwarder thread initialized"
		<< endl;
// #endif // _DEBUG

	while (!halt_server) {
		p = trx.readMessage();
		trx.processPacket(p);
		delete p;
	}
}



struct packet *LoraTrx::readMessage(void) {
	struct packet *msg;
	unique_lock<mutex> ulock(rx_ulock_mutex);

start:
	while (rx_queue.empty()) rx_queue_condvar.wait(ulock);

	rx_queue_mutex.lock();
	if (rx_queue.empty()) {
		rx_queue_mutex.unlock();
		goto start;
	}

	msg = rx_queue.front();
	rx_queue.pop();

	rx_queue_mutex.unlock();

	return msg;
}



bool LoraTrx::sendMessage(string msg_str, uint32_t toDeviceId) {
	struct packet *msg;

	if (msg_str.length() > RH_RF95_MAX_MESSAGE_LEN) {
		return false;
	}

	msg = new struct packet;

	msg->len = msg_str.length();
	msg->to = toDeviceId;

	memcpy(msg->payload.bytes, msg_str.c_str(), msg->len);

	tx_queue_mutex.lock();
	tx_queue.push(msg);
	tx_queue_mutex.unlock();

	return true;
}



void LoraTrx::processPacket(struct packet *p) {
	uint8_t maskedFlags;

	if (p == NULL) {
		throw InvalidArgumentException("Packet struct is a null pointer");
	}

	maskedFlags = p->flags & 0xF;
	switch (maskedFlags) {
		case PACKET_TYPE_ACK:
			cerr
				<< "Received currently unsupported message "
				"type PACKET_TYPE_ACK"
				<< endl;
			break;

		case PACKET_TYPE_DATA_FIRST:
			if (!verifySignature(p)) {
				cerr << "Packet from device ID "
					<< unsigned(p->from)
					<< " was discarded due to invalid signature"
					<< endl;
				return;
			}
#ifdef _DEBUG
			cout
				<< "Received LoRa PACKET_TYPE_DATA_FIRST packet "
				"with valid signature from device ID "
				<< unsigned(p->from)
				<< endl;
#endif // _DEBUG

			// Ensure this device is a gateway before even trying to push data
			if (
				blockchainSec->is_gateway(blockchainSec->get_my_device_id()) // TODO: New contract function for is_gateway_managed() and check here
			) {
				try {
					string data = string((char*) p->payload.data.data, p->len);
					string nonce = string((char*) p->payload.data.crypto_nonce, crypto_secretbox_NONCEBYTES);
					blockchainSec->push_data(
						p->from,
						data,
						p->len,
						nonce
					);
				} catch (ResourceRequestFailedException & e) {
					cerr
						<< "Failed to encode transaction request when attempting"
						" to forward LoRa packet data for device ID "
						<< unsigned(p->from)
						<< " to the blockchain"
						<< endl;
					return;
				} catch (TransactionFailedException & e) {
					cerr
						<< "Failed to forward LoRa packet data to blockchain;"
						<< endl
						<< "\tIs this gateway authorized and is device ID "
						<< unsigned(p->from)
						<< " gateway managed?"
						<< endl;
					return;
				} catch (BlockchainSecLibException & e) {
					// Catch any exceptions not caught above
					cerr
						<< "Caught unknown exception when forwarding "
						"LoRa packet data from device ID "
						<< unsigned(p->from)
						<< " to the blockchain"
						<< endl;
					return;
				}
//#ifdef _DEBUG  // TODO: Uncomment
			cout
				<< "Successfully forwarded LoRa packet data "
				"from device ID "
				<< unsigned(p->from)
				<< " to the blockchain"
				<< endl;
//#endif // _DEBUG

			}

			break;

		case PACKET_TYPE_DATA_INTERMEDIARY:
			cerr
				<< "Received currently unsupported message "
				"type PACKET_TYPE_DATA_INTERMEDIARY"
				<< endl;
			break;

		case PACKET_TYPE_DATA_LAST:
			cerr
				<< "Received currently unsupported message "
				"type PACKET_TYPE_DATA_LAST"
				<< endl;
			break;

		case PACKET_TYPE_GET_RECEIVER_KEY:
			cerr
				<< "Received currently unsupported message "
				"type PACKET_TYPE_GET_RECEIVER_KEY"
				<< endl;
			break;

		case PACKET_TYPE_UPDATE_KEY:
			cerr
				<< "Received currently unsupported message "
				"type PACKET_TYPE_UPDATE_KEY"
				<< endl;
			break;

		default:
			cerr
				<< "Unknown message LoRa message type "
				<< unsigned(maskedFlags)
				<< endl;
			return; // TODO: Can this stay as a return or can we just fall through?
	}
}



bool LoraTrx::verifySignature(struct packet *p) {
	string senderPubKey;
	crypto_sign_state state;
	uint8_t maskedFlags;

	if (p == NULL) return false;

	try {
		senderPubKey = blockchainSec->get_key(p->from);
	} catch (BlockchainSecLibException & e) {
		return false;
	}

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

	if (crypto_sign_final_verify(
			&state,
			p->payload.data.signature,
			(unsigned char*) senderPubKey.c_str()
		) != 0
	) {
		return false;
	}
	return true;
}


}
