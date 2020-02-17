
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


LoraTrx::LoraTrx(uint32_t gatewayDeviceId) {
	this->gatewayDeviceId = gatewayDeviceId;
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
			this->server,
			std::ref(rx_queue),
			std::ref(tx_queue),
			std::ref(rx_queue_mutex),
			std::ref(tx_queue_mutex),
			std::ref(rx_queue_condvar),
			std::ref(halt_server),
			std::ref(*this)
	);
}



void LoraTrx::close_server(void) {
	server_thread->join();
	halt_server = true;
}



void LoraTrx::server(queue<lora_msg*> &rx_queue, queue<lora_msg*> &tx_queue, mutex &rx_queue_mutex, mutex &tx_queue_mutex, condition_variable &rx_queue_condvar, bool &halt_server, LoraTrx &trx) {
	string msg;
	lora_msg *msg_buffer = NULL, *tx_buffer = NULL;
	bool tx_mode = false;

	if (!trx.hardwareInitialized) {
		if (!trx.setup()) {
			halt_server = true;
			cerr << "The gateway server was unable to initialize hardware!" << endl;
		}
	}

	while (!halt_server) {

		if (msg_buffer == NULL) {
			msg_buffer = new lora_msg;
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

			if (!rf95.send(tx_buffer->data, tx_buffer->len)) {
				cerr << "Error transmitting packet..." << endl;
				// TODO: Discard or retry packet? Keep track of attempts of packet and try X times?
			} else {
				rf95.waitPacketSent();
			}

			delete tx_buffer->data;
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

				msg_buffer->data = new uint8_t[msg_buffer->len];
				memcpy(msg_buffer->data, buf, msg_buffer->len);

//#ifdef _DEBUG
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
					<< "\tMessage ASCII: " << string((char*) msg_buffer->data, msg_buffer->len)
					<< endl << endl;
//#endif // _DEBUG

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



lora_msg_t LoraTrx::readMessage(void) {
	//string result;
	lora_msg *msg;
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

	//result = string((char*) msg->data, msg->len);
	//delete msg->data;
	//delete msg;
	//return result;
	return msg;
}



bool LoraTrx::sendMessage(string msg_str, uint32_t toDeviceId) {
	lora_msg *msg;

	if (msg_str.length() > RH_RF95_MAX_MESSAGE_LEN) {
		return false;
	}

	msg = new lora_msg;

	msg->len = msg_str.length();
	msg->data = new uint8_t[msg->len];
	msg->to = toDeviceId;

	memcpy(msg->data, msg_str.c_str(), msg->len);

	tx_queue_mutex.lock();
	tx_queue.push(msg);
	tx_queue_mutex.unlock();

	return true;
}



}
