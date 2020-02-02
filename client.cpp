#include <iostream>
#include <ctime>
#include <boost/algorithm/string/trim.hpp>

#include <stdlib.h>
#include <unistd.h>

#include <cxxopts.hpp>
#include <sodium.h>

#include <blockchainsec.hpp>
#include <lora_trx.hpp>
#include <client.hpp>

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



#ifdef LORA_GATEWAY

void senderThread(LoraTrx &trx) {
	int r;
	char output[256];
	char d[] = "recruitrecruiterrefereerehaverelativereporterrepresentativerestaurantreverendrguniotrichriderritzyroarsfulipwparkrrollerroofroomroommaterosessagesailorsalesmansaloonsargeantsarkscaffoldsceneschoolseargeantsecondsecretarysellerseniorsequencesergeantservantserverservingsevenseventeenseveralsexualitysheik/villainshepherdsheriffshipshopshowsidekicksingersingingsirensistersixsixteenskatesslaveslickersmallsmugglersosocialsoldiersolidersonsongsongstresssossoyspeakerspokenspysquawsquirestaffstagestallstationstatuesteedstepfatherstepmotherstewardessstorestorekeeperstorystorytellerstrangerstreetstripperstudentstudiostutterersuitsuitorssuperintendentsupermarketsupervisorsurgeonsweethearttailortakertastertaverntaxiteachertechnicianteentelegramtellertenthalthothetheatretheirtherthiefthirty-fivethisthreethroughthrowertickettimetknittotossedtouchtouristtouriststowntownsmantradetradertraintrainertravelertribetriptroopertroubledtrucktrusteetrustytubtwelvetwenty-fivetwintyuncleupstairsurchinsv.vaETERevaletvampirevanvendorvicarviceroyvictimvillagevisitorvocalsvonwaitingwaitresswalkerwarwardenwaswasherwomanwatchingwatchmanweaverwelwerewesswherewhichwhitewhowhosewifewinnerwithwittiestwomanworkerwriterxxxyyellowyoungyoungeryoungestyouthyszealot";

	for (;;) {
		sleep((rand() % 9) + 1);
		r = rand() % 255;
		strncpy(output, d, r);
		cout << "Send[" << r << "]: " << output << endl;
		trx.sendMessage(output);
	}
}

#endif //LORA_GATEWAY



void printVector(vector<string> v) {
	for (vector<string>::iterator it = v.begin(); it != v.end(); ++it) {
		cout << "\t" << *it << endl;
	}
}



string runBinary(string cmd) {
	string result;
	FILE *inFd;
	array<char, SENSOR_PIPE_BUFFER_LEN> pipeBuffer;

	inFd = popen(cmd.c_str(), "r");

	if (fgets(pipeBuffer.data(), SENSOR_PIPE_BUFFER_LEN, inFd) == NULL) {
		throw ResourceRequestFailedException("runBinary(\"" + cmd + "\"): Error: Failed to read from pipe!");
	}

	result += pipeBuffer.data();

	while (fgets(pipeBuffer.data(), SENSOR_PIPE_BUFFER_LEN, inFd) != NULL) {
		result += pipeBuffer.data();
	}

	if (pclose(inFd) < 0) {
		throw ResourceRequestFailedException("runBinary(\"" + cmd + "\"): Failed to pclose()!");
	}
	return result;
}


string querySensor(void) {
	string data;

	data = "uname -a\n";

	try {
		data += runBinary("uname -a");
	} catch (ResourceRequestFailedException &e) {
		data += e.what();
	}

	data += "\nnproc\n";

	try {
		data += runBinary("nproc");
	} catch (ResourceRequestFailedException &e) {
		data += e.what();
	}

	data += "\nuptime\n";

	try {
		data += runBinary("uptime");
	} catch (ResourceRequestFailedException &e) {
		data += e.what();
	}

	data += "\nfree -h\n";

	try {
		data += runBinary("free -h");
	} catch (ResourceRequestFailedException &e) {
		data += e.what();
	}
	return data;
}


int main(int argc, char *argv[]) {

	auto flags = parseFlags(argc, argv);

	cout << endl << "\t.:Blockchain Security Framework Client:." << endl << endl;

	if (compileFlag) {
		cout << "Compiling smart contract..." << endl;
	}
	sec = new BlockchainSecLib(compileFlag);

	if (gatewayFlag) {
		cout << "This binary does not support running as a LoRa gateway!" << endl;
		exit(EXIT_FAILURE);
	}

	for (;;) {

		while (sec->get_my_device_id() == 0) {
			uint32_t dev;

			cerr << "This device does not have an associated device ID..." << endl << "Creating device..." << endl;

			cout << endl << "Adding this address as a device..." << endl;
			dev = sec->add_device(sec->getClientAddress(), "Local Device 1", "LOCAL MAC 1", false);
			if (dev == 0) {
				cout << "Failed to add local device!" << endl << endl;
			} else {
				cout << "Successfully added local device as deviceID " << dev << "!" << endl << endl;
			}

			sec->update_datareceiver(dev, dev);

			sec->updateLocalKeys();

			//cerr << "This device does not have an associated device ID... Retrying in " << INVALID_DEVICE_TRY_INTERVAL << " seconds..." << endl;
			//sleep(INVALID_DEVICE_TRY_INTERVAL);
		}
		sec->loadLocalDeviceParameters();

		cout << "Found device ID #" << sec->get_my_device_id() << endl << endl;


		for (;;) {
			string data = querySensor();
			uint32_t myDeviceID = sec->get_my_device_id();

			if (myDeviceID == 0) {
				cerr << "This device does not have a registered device ID! Retrying in " << INVALID_DEVICE_TRY_INTERVAL << " seconds...\n";
				sleep(INVALID_DEVICE_TRY_INTERVAL);
				break;
			}

			cout << "Attempting to push data to the blockchain..." << endl;

			if (!sec->encryptAndPushData(data)) {
				cerr << "Failed to push data to the blockchain! Retrying in " << INVALID_DEVICE_TRY_INTERVAL << " seconds...\n";
				sleep(INVALID_DEVICE_TRY_INTERVAL);
				break;
			}

			cout << "Successfully pushed data to the blockchain..." << endl << endl;

			string chainData;
			time_t dataTimestamp;

			try {
				chainData = sec->getDataAndDecrypt(myDeviceID);
				dataTimestamp = sec->get_dataTimestamp(myDeviceID);

			} catch (BlockchainSecLibException &e) {
				cerr << "Error while retrieving data from the blockchain!" << endl << e.what() << endl;
				sleep(INVALID_DEVICE_TRY_INTERVAL);
				break;
			}

			string timestamp = asctime(localtime(&dataTimestamp));
			boost::trim(timestamp);

			cout << "Got data timestamped as " << timestamp << ":" << endl << endl << chainData << endl;

			sleep(DATA_PUSH_INTERVAL);
		}

	}


	sec->joinThreads();
	return EXIT_SUCCESS;
}
