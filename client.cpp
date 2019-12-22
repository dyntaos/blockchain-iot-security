#include <iostream>
#include <cxxopts.hpp>
#include <blockchainsec.h>
#include <lora_trx.h>
#include <client.h>

using namespace std;
using namespace blockchainSec;

//TODO: Globals are bad
bool compile_flag = false;
bool gateway_flag = false;

BlockchainSecLib *sec;

cxxopts::ParseResult parse_flags(int argc, char* argv[]) {
	bool help_flag;
	try {
		cxxopts::Options options(argv[0], "");
		options
			.positional_help("[optional args]")
			.show_positional_help();

		options
			.allow_unrecognised_options()
			.add_options()
			("h,help", "Display help", cxxopts::value<bool>(help_flag))
			("c,compile", "Compile & upload the solidity contract to the blockchain, and save the contract address, overwriting the old address in the config file", cxxopts::value<bool>(compile_flag))
			("g,gateway", "Start this client in gateway mode. TODO MORE", cxxopts::value<bool>(gateway_flag));

		auto result = options.parse(argc, argv);

		if (help_flag) {
			cout << options.help({"", "Group"}) << endl;
			exit(EXIT_SUCCESS);
		}

		return result;

	} catch (const cxxopts::OptionException& e) {
		cout << "Error parsing flags: " << e.what() << endl;
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char *argv[]) {
	LoraTrx *trx;
	cout << ".:Blockchain Security Framework Client:." << endl;

	auto flags = parse_flags(argc, argv);;
	(void) argc;
	(void) argv;


	if (compile_flag) {
		cout << "Got compile flag" << endl;
	}

	if (gateway_flag) {
		cout << "Got gateway flag" << endl;
		trx = new LoraTrx();
		trx->server_init();
	}

	sec = new BlockchainSecLib(compile_flag);

#ifdef _DEBUG
	sec->test();
#endif //_DEBUG

	if (gateway_flag) {
		for (;;) {
			cout << "LoraTrx::readMessage() returned: " << trx->readMessage() << endl;
		}

		trx->close_server();
	}

	return EXIT_SUCCESS;
}
