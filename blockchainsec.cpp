#include <string>
#include <array>
#include <iostream>

#include <unistd.h>
#include <fcntl.h>

#include <blockchainsec.h>
#include <misc.h>
#include <gason.h>

//TODO: Make a function to verify ethereum address formatting! (Apply to configuration file validation)

using namespace std;
using namespace libconfig;


namespace blockchainSec {



BlockchainSecLib::BlockchainSecLib(bool compile) {


	this->eth_sec_contract_addr = eth_sec_contract_addr; //TODO Check for reasonableness
	cfg.setOptions(Config::OptionFsync
				 | Config::OptionSemicolonSeparators
				 | Config::OptionColonAssignmentForGroups
				 | Config::OptionOpenBraceOnSeparateLine);

	cout << "Read config file..." << endl;

	try {
		cfg.readFile(BLOCKCHAINSEC_CONFIG_F);

	} catch(const FileIOException &e) {
		cerr << "IO error reading config file!" << endl;
		exit(EXIT_FAILURE);

	} catch(const ParseException &e) {
		cerr << "BlockchainSec: Config file error " << e.getFile() << ":" << e.getLine() << " - " << e.getError() << endl;
		exit(EXIT_FAILURE);
	}

	cfg_root = &cfg.getRoot();

	if (!cfg.exists("ipc_path")) {
		cerr << "Configuration file does not contain 'ipc_path'!" << endl;
		exit(EXIT_FAILURE);
	}

	cfg.lookupValue("ipc_path", this->ipc_path); //TODO Check for reasonableness

	if (!cfg.exists("client_eth_addr")) {
		cerr << "Configuration file does not contain 'client_eth_addr'!" << endl;
		exit(EXIT_FAILURE);
	}

	cfg.lookupValue("client_eth_addr", this->eth_my_addr); //TODO Check for reasonableness

	if (!cfg.exists("contract_addr")) {
		/* A contract address is not saved in the config file.
		 * We will compile the contract with solc, upload it
		 * to the chain and save the address to the config file */
		if (compile) {
			cout << "Compiling and uploading contract..." << endl << endl;
			if (this->create_contract()) {
				cout << "Successfully compiled and uploaded contract" << endl
					<< "The new contract address has been written to the "
					"configuration file" << endl;
				exit(EXIT_SUCCESS);
			} else {
				cerr << "Failed to create contract!" << endl;
				exit(EXIT_FAILURE);
			}

		} else {
			cout << "Config doesnt have contract_addr" << endl
				<< "Either import a config or compile and upload"
				" a contract using the `--compile` flag" << endl;

			exit(EXIT_FAILURE);
		}

	}

	cout << "Config had contract_addr" << endl;
	cfg.lookupValue("contract_addr", this->eth_sec_contract_addr);
}



BlockchainSecLib::BlockchainSecLib(void) : BlockchainSecLib(false) {}



BlockchainSecLib::~BlockchainSecLib() {

}



string BlockchainSecLib::add_device(string client_addr, string name, string mac, string public_key, bool gateway_managed) {
	string data;

	if (name.length() > BLOCKCHAINSEC_MAX_DEV_NAME) return ""; //Device name too long -- returning "" is an error
	//TODO: If !gateway_managed make sure clientAddr is valid

	data = this->ethabi("encode -l function " ETH_CONTRACT_ABI " add_device -p '0x" + client_addr + "' -p '" + name + "' -p '" + mac + "' -p '" + public_key + "' -p " + (gateway_managed ? "true" : "false"));
	return this->eth_sendTransaction("0x" + data);
}



string BlockchainSecLib::add_gateway(string client_addr, string name, string mac, string public_key) {
	string data;

	if (name.length() > BLOCKCHAINSEC_MAX_DEV_NAME) return ""; //Device name too long -- returning "" is an error
	//TODO: If !gateway_managed make sure clientAddr is valid

	data = this->ethabi("encode -l function " ETH_CONTRACT_ABI " add_gateway -p '0x" + client_addr + "' -p '" + name + "' -p '" + mac + "' -p '" + public_key + "'");
	return this->eth_sendTransaction("0x" + data);
}



#ifdef _DEBUG
void BlockchainSecLib::test(void) {
	bool testsPassed = true;
	string result;

	result = this->ethabi("encode params -v bool 1");
	if (result != "0000000000000000000000000000000000000000000000000000000000000001") {
		cout << "ethabi(): Test 1 failed! Got return value of '" << result << "'" << endl;
		testsPassed = false;
	}

	result = this->ethabi("encode params -v bool 1 -v string gavofyork -v bool 0");
	if (result != "00000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000060000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000096761766f66796f726b0000000000000000000000000000000000000000000000") {
		cout << "ethabi(): Test 2 failed! Got return value of '" << result << "'" << endl;
		testsPassed = false;
	}

	result = this->ethabi("encode params -v bool[] [1,0,false]");
	if (result != "00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000003000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000") {
		cout << "ethabi(): Test 3 failed! Got return value of '" << result << "'" << endl;
		testsPassed = false;
	}

	result = this->ethabi("decode params -t bool[] 00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000003000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
	if (result != "bool[] [true,false,false]") {
		cout << "ethabi(): Test 4 failed! Got return value of '" << result << "'" << endl;
		testsPassed = false;
	}

	if (testsPassed) {
		cout << "All tests passed successfully!" << endl;
	} else {
		cout << "Tests failed!" << endl;
	}
}
#endif //_DEBUG



bool BlockchainSecLib::create_contract(void) {
	string contract_bin, contract_abi;

	// TODO: When project is complete we dont need to recompile this everytime
	system("solc --bin '" ETH_CONTRACT_SOL "' | tail -n +4 > '" ETH_CONTRACT_BIN "'");
	system("solc --abi '" ETH_CONTRACT_SOL "' | tail -n +4 > '" ETH_CONTRACT_ABI "'");
	//TODO: Check for success

	contract_bin = readFile2(ETH_CONTRACT_BIN);
	contract_abi = readFile2(ETH_CONTRACT_ABI);



	//TODO: Save contract to config
	cfg_root->add("contract_addr", Setting::TypeString) = "test";
	cfg.writeFile(BLOCKCHAINSEC_CONFIG_F);

	return true;
}



string BlockchainSecLib::ethabi(string args) {
	string result;
	array<char, PIPE_BUFFER_LENGTH> pipe_buffer;

#ifdef _DEBUG
	cout << "ethabi(): Requested '" << args << "'" << endl;
#endif //_DEBUG

	FILE *ethabi_pipe = popen(("ethabi " + args).c_str(), "r");
	if (ethabi_pipe == NULL) {
		// Failed to open pipe to ethabi -- is the binary installed and in $PATH?
		cerr << "ethabi(): Failed to popen() pipe to ethabi binary. Is the binary installed and in the $PATH environment variable?" << endl;
		return ""; //TODO
	}
	while (fgets(pipe_buffer.data(), PIPE_BUFFER_LENGTH, ethabi_pipe) != NULL) {
		result += pipe_buffer.data();
	}
	if (pclose(ethabi_pipe) < 0) {
		cerr << "ethabi(): Failed to pclose() pipe to ethabi binary!";
		return result; //TODO: We may still have valid data to return, despite the error
	}

#ifdef _DEBUG
	cout << "ethabi(): Call successful!" << endl;
	cout << "ethabi(): Returning: " << trim(result) << endl;
#endif //_DEBUG

	return trim(result);
}



string BlockchainSecLib::eth_ipc_request(string json_request) {
	int ipc_fd_flags, ipc_fd;
	string json;
	array<char, PIPE_BUFFER_LENGTH> ipc_buffer;

#ifdef _DEBUG
	cout << "eth_ipc_request(): " << json_request << endl;
#endif //_DEBUG

	FILE *ipc = popen(("echo '" + json_request + "' | nc -U '" + this->ipc_path + "'").c_str(), "r");
	if (ipc == NULL) {
		// Failed to open Unix domain socket for IPC -- Perhaps geth is not running?
		cerr << "eth_ipc_request(): Failed to popen() unix domain socket for IPC with geth! Is geth running?" << endl;
		return ""; //TODO
	}

	ipc_fd = fileno(ipc);

	if (fgets(ipc_buffer.data(), PIPE_BUFFER_LENGTH, ipc) == NULL) {
		cout << "eth_ipc_request(): Error: Failed to read from IPC!" << endl;
	}

	json += ipc_buffer.data();

	ipc_fd_flags = fcntl(ipc_fd, F_GETFL, 0);
	ipc_fd_flags |= O_NONBLOCK;
	fcntl(ipc_fd, F_SETFL, ipc_fd_flags);

	while (fgets(ipc_buffer.data(), PIPE_BUFFER_LENGTH, ipc) != NULL) {
#ifdef _DEBUG
		cout << "eth_ipc_request(): Read: ''" << ipc_buffer.data() << "'" << endl;
#endif //_DEBUG
		json += ipc_buffer.data();
	}

	if (pclose(ipc) < 0) {
		cerr << "eth_ipc_request(): Failed to pclose() unix domain socket for IPC with geth!";
		return json; //TODO: We may still have valid data to return, despite the error
	}

#ifdef _DEBUG
	cout << "eth_ipc_request(): Successfully relayed request" << endl;
#endif //_DEBUG

	return json;
}



string BlockchainSecLib::eth_call(string abi_data) {
	string json_request = "{\"jsonrpc\":\"2.0\","
								"\"method\":\"eth_call\","
								"\"params\":[{"
									//"\"from\":\"" + this->eth_my_addr + "\","
									"\"to\":\"" + this->eth_sec_contract_addr + "\","
									"\"data\":\"" + abi_data +
								"\"},\"latest\"],\"id\":1}";

#ifdef _DEBUG
	cout << "eth_call()" << endl;
#endif //_DEBUG

	return this->eth_ipc_request(json_request);
}



string BlockchainSecLib::eth_sendTransaction(string abi_data) {
	string json_request = "{\"jsonrpc\":\"2.0\","
								"\"method\":\"eth_sendTransaction\""
								",\"params\":[{"
									"\"from\":\"" + this->eth_my_addr + "\","
									"\"to\":\"" + this->eth_sec_contract_addr + "\","
									//"\"gas\":0,"
									//"\"gasPrice\":\"" + ETH_DEFAULT_GAS + "\","
									"\"data\":\"" + abi_data +
								"\"}],"
								"\"id\":1}";

#ifdef _DEBUG
	cout << "eth_sendTransaction()" << endl;
#endif //_DEBUG

	return this->eth_ipc_request(json_request);
}



} //namespace blockchain-sec
