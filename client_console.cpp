#include <iostream>

#include <boost/algorithm/string.hpp> // TODO: Temp for boost::to_upper()
#include <ctime> // TODO: TEMP
#include <stdlib.h>
#include <unistd.h>

#include <cxxopts.hpp>

#include <blockchainsec.hpp>
#include <data_receiver.hpp>
#include <client.hpp>
#include <console.hpp>

#ifdef LORA_GATEWAY
#include <lora_trx.hpp>
#endif

using namespace std;
using namespace blockchainSec;


bool compileFlag = false;
bool gatewayFlag = false;
bool consoleFlag = false;
bool receiverFlag = false;



cxxopts::ParseResult
parseFlags(int argc, char* argv[])
{
	bool helpFlag;
	try
	{
		cxxopts::Options options(argv[0], "");
		options
			.positional_help("[optional args]")
			.show_positional_help();

		options
			.allow_unrecognised_options()
			.add_options()
				("h,help",
					"Display help",
					cxxopts::value<bool>(helpFlag))
				("C,compile",
					"Compile & upload the solidity contract to the blockchain, and save the "
					"contract address, overwriting the old address in the config file",
					cxxopts::value<bool>(compileFlag))
				("g,gateway",
					"Start this client in gateway mode. TODO MORE",
					cxxopts::value<bool>(gatewayFlag))
				("c,console",
					"Start this client in with the console enabled.",
					cxxopts::value<bool>(consoleFlag))
				("r,receiver",
					"Start this client as a data receiver.",
					cxxopts::value<bool>(receiverFlag));

		auto result = options.parse(argc, argv);

		if (helpFlag)
		{
			cout << options.help({ "", "Group" }) << endl;
			exit(EXIT_SUCCESS);
		}

		return result;
	}
	catch (const cxxopts::OptionException& e)
	{
		cout << "Error parsing flags: " << e.what() << endl;
		exit(EXIT_FAILURE);
	}
}



void
printIntVector(vector<uint32_t> v)
{
	for (vector<uint32_t>::iterator it = v.begin(); it != v.end(); ++it)
	{
		cout << "\t" << unsigned(*it) << endl;
	}
}



int
main(int argc, char* argv[])
{
	BlockchainSecLib* sec;
	BlockchainSecConsole* blockchainSecConsole;

#ifdef LORA_GATEWAY
	LoraTrx* trx;
#endif //LORA_GATEWAY

	cout << ".:Blockchain Security Framework Client:." << endl;

	auto flags = parseFlags(argc, argv);
	(void)argc;
	(void)argv;


	if (compileFlag)
	{
		cout << "Compiling smart contract..." << endl;
	}


	// #ifndef LORA_GATEWAY
	sec = new BlockchainSecLib(compileFlag);
	// #endif

	if (gatewayFlag)
	{
#ifdef LORA_GATEWAY
		cout << "Started in gateway mode..." << endl;
		trx = new LoraTrx(sec);
		trx->server_init();
#else
		cout << "This architecture does not support running as a LoRa gateway!" << endl;
		exit(EXIT_FAILURE);
#endif //LORA_GATEWAY
	}


	if (consoleFlag)
	{
		cout << "Console enabled..." << endl;
		blockchainSecConsole = new BlockchainSecConsole();
		blockchainSecConsole->startThread(*sec);
	}

	if (receiverFlag)
	{
		blockchainSec::DataReceiverManager dataRecv(sec);
		sec->joinThreads();
		dataRecv.joinThreads();
	}

	sec->joinThreads();


	return EXIT_SUCCESS;
}
