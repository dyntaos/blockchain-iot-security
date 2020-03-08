#include <iostream>

#include <boost/algorithm/string.hpp> // TODO: Temp for boost::to_upper()
#include <ctime> // TODO: TEMP
#include <stdlib.h>
#include <unistd.h>

#include <cxxopts.hpp>

#include <blockchainsec.hpp>
#include <client.hpp>
#include <console.hpp>
#include <data_receiver.hpp>

#ifdef LORA_GATEWAY
#include <lora_trx.hpp>
#endif

using namespace std;
using namespace blockchainSec;


bool compileFlag = false;
bool gatewayFlag = false;
bool consoleFlag = false;
bool receiverFlag = false;
bool sensorFlag = false;



cxxopts::ParseResult
parseFlags(int argc, char *argv[])
{
	bool helpFlag;
	try
	{
		cxxopts::Options options(argv[0], "");
		options
			.positional_help("[optional args]")
			.custom_help("If no flags are provided, data will be read from "
				"STDIN and pushed to the blockchain before exiting.")
			.show_positional_help();

		options
			.allow_unrecognised_options()
			.add_options()("h,help",
				"Display help",
				cxxopts::value<bool>(helpFlag))("C,compile",
				"Compile & upload the solidity contract to the blockchain, and save the "
				"contract address, overwriting the old address in the config file",
				cxxopts::value<bool>(compileFlag))("g,gateway",
				"Start this client in gateway mode. TODO MORE",
				cxxopts::value<bool>(gatewayFlag))("c,console",
				"Start this client in with the administrator console enabled.",
				cxxopts::value<bool>(consoleFlag))("r,receiver",
				"Start this client as a data receiver.",
				cxxopts::value<bool>(receiverFlag))("s,sensor",
				"Start this client in sensor mode (collect and push sensor data to the blockchain).",
				cxxopts::value<bool>(sensorFlag));

		auto result = options.parse(argc, argv);

		if (helpFlag)
		{
			cout << options.help({ "", "Group" }) << endl;
			exit(EXIT_SUCCESS);
		}

		return result;
	}
	catch (const cxxopts::OptionException &e)
	{
		cout << "Error parsing flags: " << e.what() << endl;
		exit(EXIT_FAILURE);
	}
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

	data = "uname -a\n";

	try
	{
		data += runBinary("uname -a");
	}
	catch (ResourceRequestFailedException &e)
	{
		data += e.what();
	}

	data += "\nnproc\n";

	try
	{
		data += runBinary("nproc");
	}
	catch (ResourceRequestFailedException &e)
	{
		data += e.what();
	}

	data += "\nuptime\n";

	try
	{
		data += runBinary("uptime");
	}
	catch (ResourceRequestFailedException &e)
	{
		data += e.what();
	}

	data += "\nfree -h\n";

	try
	{
		data += runBinary("free -h");
	}
	catch (ResourceRequestFailedException &e)
	{
		data += e.what();
	}
	return data;
}



void
dataReceiver(BlockchainSecLib& blockchain)
{
	DataReceiverManager dataRecv(&blockchain);

	for (;;)
	{
		set<uint32_t> receivedUpdates = dataRecv.getReceivedChanges();

		if (receivedUpdates.size() == 0)
		{
			cerr << "ERROR: Received updates yielded 0 device IDs!" << endl; // TODO: Eliminate this message after testing?
			continue;
		}

		for (auto it = receivedUpdates.begin(); it != receivedUpdates.end(); ++it)
		{
			// TODO: Make into a single string then cout
			cout << "Device ID "
				 << *it
				 << " new data:"
				 << endl
				 << "\""
				 << blockchain.getDataAndDecrypt(*it)
				 << "\""
				 << endl
				 << endl;
		}
	}
	dataRecv.joinThreads();
}



int
main(int argc, char *argv[])
{
	BlockchainSecLib *sec;
	BlockchainSecConsole *blockchainSecConsole;
	thread *dataReceiverThread;

#ifdef LORA_GATEWAY
	LoraTrx *trx;
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
		dataReceiverThread = new thread(
			dataReceiver,
			std::ref(*sec));
	}

	if (!(compileFlag || gatewayFlag || consoleFlag || receiverFlag || sensorFlag))
	{
		// If no flags are provided, read data from stdin and push to blockchain and exit
		// TODO

	}
	else
	{
		if (receiverFlag) dataReceiverThread->join();
		sec->joinThreads();
	}

	return EXIT_SUCCESS;
}
