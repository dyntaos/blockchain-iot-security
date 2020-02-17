#include <iostream>

#include <stdlib.h>
#include <unistd.h>
#include <time.h> // TODO: TEMP
#include <boost/algorithm/string.hpp> // TODO: Temp for boost::to_upper()

#include <cxxopts.hpp>

#include <blockchainsec.hpp>
#include <client.hpp>

#ifdef LORA_GATEWAY
#include <lora_trx.hpp>
#endif

using namespace std;
using namespace blockchainSec;

//TODO: Globals are bad
bool compileFlag = false;
bool gatewayFlag = false;

BlockchainSecLib *sec;



cxxopts::ParseResult parseFlags(int argc, char* argv[]) {
	bool helpFlag;
	try {
		cxxopts::Options options(argv[0], "");
		options
			.positional_help("[optional args]")
			.show_positional_help();

		options
			.allow_unrecognised_options()
			.add_options()
			("h,help", "Display help", cxxopts::value<bool>(helpFlag))
			("c,compile", "Compile & upload the solidity contract to the blockchain, and save the contract address, overwriting the old address in the config file", cxxopts::value<bool>(compileFlag))
			("g,gateway", "Start this client in gateway mode. TODO MORE", cxxopts::value<bool>(gatewayFlag));

		auto result = options.parse(argc, argv);

		if (helpFlag) {
			cout << options.help({"", "Group"}) << endl;
			exit(EXIT_SUCCESS);
		}

		return result;

	} catch (const cxxopts::OptionException& e) {
		cout << "Error parsing flags: " << e.what() << endl;
		exit(EXIT_FAILURE);
	}
}



void printVector(vector<uint32_t> v) {
	for (vector<uint32_t>::iterator it = v.begin(); it != v.end(); ++it) {
		cout << "\t" << *it << endl;
	}
}


int main(int argc, char *argv[]) {

#ifdef LORA_GATEWAY
	LoraTrx *trx;
#endif //LORA_GATEWAY

	cout << ".:Blockchain Security Framework Client:." << endl;

	auto flags = parseFlags(argc, argv);
	(void) argc;
	(void) argv;


	if (compileFlag) {
		cout << "Compiling smart contract..." << endl;
	}

	if (gatewayFlag) {
#ifdef LORA_GATEWAY
		cout << "Started in gateway mode..." << endl;
		trx = new LoraTrx(1); // TODO: Gateway ID
		trx->server_init();
#else
		cout << "This architecture does not support running as a LoRa gateway!" << endl;
		exit(EXIT_FAILURE);
#endif //LORA_GATEWAY
	}

#ifndef LORA_GATEWAY
	sec = new BlockchainSecLib(compileFlag);
#endif

#ifdef _DEBUG
	sec->test();
#endif //_DEBUG

#ifdef LORA_GATEWAY
	if (gatewayFlag) {
		lora_msg_t msg;
		string msgStr;

		for (;;) {
			msg = trx->readMessage();
			msgStr = string(msg->data, msg->len);
			cout << "Receive[" << msg.length() << "]: " << msg << endl << endl;

			if (trx->sendMessage(boost::to_upper_copy("Hello from the server: " + msg))) {
				cout << "Sent reply to LoRa node" << endl;
			} else {
				cout << "Error sending reply to LoRa node..." << endl;
			}
		}

		trx->close_server();
		exit(EXIT_SUCCESS);
	}
#endif //LORA_GATEWAY

#ifndef LORA_GATEWAY

	sleep(3);

	uint32_t dev;

	cout << endl << "Adding device..." << endl;
	dev = sec->add_device("0000000000000000000000000000000000000000", "TestDevice 1", "TEST MAC 1", true);
	if (dev == 0) {
		cout << "Failed to add device!" << endl << endl;
	} else {
		cout << "Successfully added device as deviceID " << dev << "!" << endl << endl;
	}


	cout << endl << "Adding device..." << endl;
	dev = sec->add_device("0000000000000000000000000000000000000001", "Test    Device 2", "TEST     MAC 2", false);
	if (dev == 0) {
		cout << "Failed to add device!" << endl << endl;
	} else {
		cout << "Successfully added device as deviceID " << dev << "!" << endl << endl;
	}


	cout << "is_admin(a4528ce8f47845b3bbf842da92bae9359e23fa3b) = ";
	if (sec->is_admin("a4528ce8f47845b3bbf842da92bae9359e23fa3b")) {
		cout << "TRUE" << endl << endl;
	} else {
		cout << "FALSE" << endl << endl;
	}


	cout << "is_admin(0000000000000000000000000000000000000000) = ";
	if (sec->is_admin("0000000000000000000000000000000000000000")) {
		cout << "TRUE" << endl << endl;
	} else {
		cout << "FALSE" << endl << endl;
	}


	cout << "is_authd(1) = " << sec->is_authd(1) << endl;
	cout << "is_authd(2) = " << sec->is_authd(2) << endl;
	cout << "is_authd(3) = " << sec->is_authd(3) << endl;
	cout << "is_authd(4) = " << sec->is_authd(4) << endl << endl;


	cout << "get_my_device_id() = " << sec->get_my_device_id() << endl << endl;


	cout << "get_key(1) = " << sec->get_key(1) << endl;
	cout << "get_key(2) = " << sec->get_key(2) << endl << endl;

	cout << "authorize_admin(deadbeeff47845b3bbf842da92bae9359e23fa3b)" << endl << endl;
	sec->authorize_admin("deadbeeff47845b3bbf842da92bae9359e23fa3b");

	cout << "get_num_admin() = " << sec->get_num_admin() << endl << endl;

	cout << "get admins..."<< endl;
	printVector(sec->get_active_admins());

	cout << endl << "authorize_admin(deadcafebabe51b3bbf842da92bae9359e23fa3b)" << endl << endl;
	sec->authorize_admin("deadcafebabe51b3bbf842da92bae9359e23fa3b");

	cout << "get_num_admin() = " << sec->get_num_admin() << endl << endl;

	cout << "get admins..."<< endl;
	printVector(sec->get_active_admins());

	cout << endl << "get authorized devices..."<< endl;
	printVector(sec->get_authorized_devices());

	cout << "remove_device(1)..." << endl;
	if (sec->remove_device(1)) {
		cout << "\tSuccess" << endl << endl;
	} else {
		cout << "\tFailure" << endl << endl;
	}

	cout << "get authorized devices..."<< endl;
	printVector(sec->get_authorized_devices());


	cout << endl << "Adding this address as a device..." << endl;
	dev = sec->add_device("a4528ce8f47845b3bbf842da92bae9359e23fa3b", "Local Device 1", "LOCAL MAC 1", false);
	if (dev == 0) {
		cout << "Failed to add local device!" << endl << endl;
	} else {
		cout << "Successfully added local device as deviceID " << dev << "!" << endl << endl;
	}

	sec->updateLocalKeys();

	uint32_t my_dev = sec->get_my_device_id();
	cout << "My device_id = " << my_dev << endl;
	cout << "My public key: " << sec->get_key(my_dev) << endl;

	cout << "Default data receiver: " << sec->get_default_datareceiver() << endl;

	cout << "Set default data receiver to 2..." << endl;
	sec->set_default_datareceiver(2);

	cout << "Default data receiver: " << sec->get_default_datareceiver() << endl;

	cout << "get_datareceiver(1): " << sec->get_datareceiver(1) << endl;
	cout << "update_datareceiver(1, 2)..." << endl;

	if (sec->update_datareceiver(1, 2)) {
		cout << "\tSuccess..." << endl;
	} else {
		cout << "\tFailed!" << endl;
	}

	cout << "get_datareceiver(1): " << sec->get_datareceiver(1) << endl;

	cout << endl << "DONE!" << endl;

	sec->joinThreads();

#endif


	return EXIT_SUCCESS;
}
