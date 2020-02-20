#include <iostream>

#include <stdlib.h>
#include <unistd.h>
#include <ctime> // TODO: TEMP
#include <boost/algorithm/string.hpp> // TODO: Temp for boost::to_upper()

#include <cxxopts.hpp>

#include <blockchainsec.hpp>
#include <console.hpp>
#include <client.hpp>

#ifdef LORA_GATEWAY
#include <lora_trx.hpp>
#endif

using namespace std;
using namespace blockchainSec;

//TODO: Globals are bad
bool compileFlag = false;
bool gatewayFlag = false;
bool consoleFlag = false;



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
			(
				"h,help",
				"Display help",
				cxxopts::value<bool>(helpFlag)
			)
			(
				"C,compile",
				"Compile & upload the solidity contract to the blockchain, and save the "
				"contract address, overwriting the old address in the config file",
				cxxopts::value<bool>(compileFlag)
			)
			(
				"g,gateway",
				"Start this client in gateway mode. TODO MORE",
				cxxopts::value<bool>(gatewayFlag)
			)
			(
				"c,console",
				"Start this client in with the console enabled.",
				cxxopts::value<bool>(consoleFlag)
			);

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
	BlockchainSecLib *sec;
	BlockchainSecConsole *blockchainSecConsole;

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


	if (consoleFlag) {
		cout << "Console enabled..." << endl;
		blockchainSecConsole = new BlockchainSecConsole();
		blockchainSecConsole->startThread(*sec);
	}

#ifdef LORA_GATEWAY
	if (gatewayFlag) {
		struct packet *msg;
		string msgStr;

		for (;;) {
			msg = trx->readMessage();
			msgStr = string((char*) msg->payload.bytes, msg->len);
			cout << "Receive[" << unsigned(msg->len) << "]: " << msg << endl;

			if (trx->sendMessage(boost::to_upper_copy("Hello from the server: " + string((char*) msg->payload.bytes, msg->len)), msg->from)) {
				cout << "Sent reply to LoRa node" << endl << endl;
			} else {
				cout << "Error sending reply to LoRa node..." << endl << endl;
			}

			delete msg;
			msg = NULL;
		}

		trx->close_server();
		exit(EXIT_SUCCESS);
	}
#endif //LORA_GATEWAY


	sec->joinThreads();


	return EXIT_SUCCESS;
}
