#include <iostream>

#include <stdlib.h>
#include <unistd.h>

#include <cxxopts.hpp>

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
		trx = new LoraTrx();
		trx->server_init();
#else
		cout << "This architecture does not support running as a LoRa gateway!" << endl;
		exit(EXIT_FAILURE);
#endif //LORA_GATEWAY
	}

	sec = new BlockchainSecLib(compileFlag);

#ifdef _DEBUG
	sec->test();
#endif //_DEBUG

#ifdef LORA_GATEWAY
	if (gatewayFlag) {
		thread send_thread(senderThread, std::ref(*trx));
		string msg;

		for (;;) {
			msg = trx->readMessage();
			cout << "readMessage(): " << msg << endl;
		}

		trx->close_server();
	}
#endif //LORA_GATEWAY

	sleep(4);
	cout << "Adding device..." << endl;
	if (!sec->add_device("0000000000000000000000000000000000000000", "TestDevice 1", "TEST MAC 1", "TEST   PUBKEY1", true)) {
		cout << "Failed to add device 1!" << endl;
	} else {
		cout << "Successfully added device 1!" << endl;
	}


	cout << "Adding device2..." << endl;
	if (!sec->add_device("0000000000000000000000000000000000000001", "Test    Device 2", "TEST     MAC 2", "TEST   PUBKEY    2", false)) {
		cout << "Failed to add device 2!" << endl;
	} else {
		cout << "Successfully added device 2!" << endl;
	}
	sec->joinThreads();
	return EXIT_SUCCESS;
}
