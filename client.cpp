#include <iostream>
#include <thread>

#include <stdlib.h>
#include <unistd.h>

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

	auto flags = parse_flags(argc, argv);;
	(void) argc;
	(void) argv;


	if (compile_flag) {
		cout << "Compiling smart contract..." << endl;
	}

	if (gateway_flag) {
#ifdef LORA_GATEWAY
		cout << "Started in gateway mode..." << endl;
		trx = new LoraTrx();
		trx->server_init();
#else
		cout << "This architecture does not support running as a LoRa gateway!" << endl;
		exit(EXIT_FAILURE);
#endif //LORA_GATEWAY
	}

	sec = new BlockchainSecLib(compile_flag);

#ifdef _DEBUG
	sec->test();
#endif //_DEBUG

#ifdef LORA_GATEWAY
	if (gateway_flag) {
		thread send_thread(senderThread, std::ref(*trx));
		string msg;

		for (;;) {
			msg = trx->readMessage();
			cout << "readMessage(): " << msg << endl;
		}

		trx->close_server();
	}
#endif //LORA_GATEWAY

	return EXIT_SUCCESS;
}

