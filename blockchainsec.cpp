#include <string>
#include <array>
#include <iostream>

#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <blockchainsec.h>
#include <misc.h>
//#include <gason.h>

//TODO: Make a function to verify ethereum address formatting! (Apply to configuration file validation)
//TODO: Make function to see if "0x" needs to be prepended to function arguments

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

	if (!compile && !cfg.exists("contract_addr")) {
		cout << "Config doesnt have contract_addr" << endl
			<< "Either add a contract address to the conf file or create a "
			"new contract using the `--compile` flag" << endl;

		exit(EXIT_FAILURE);
	}

	if (compile) {
		/* We will compile the contract with solc, upload it
		 * to the chain and save the address to the config file */
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
	return this->eth_sendTransaction(data);
}



string BlockchainSecLib::add_gateway(string client_addr, string name, string mac, string public_key) {
	string data;

	if (name.length() > BLOCKCHAINSEC_MAX_DEV_NAME) return ""; //Device name too long -- returning "" is an error
	//TODO: If !gateway_managed make sure clientAddr is valid

	data = this->ethabi("encode -l function " ETH_CONTRACT_ABI " add_gateway -p '0x" + client_addr + "' -p '" + name + "' -p '" + mac + "' -p '" + public_key + "'");
	return this->eth_sendTransaction(data);
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
	string contract_bin,
			result,
			transaction_hash,
			transaction_receipt,
			contract_addr;
	char *source, *endptr;
	bool json_error = true;
	int status;
	JsonValue value;
	JsonAllocator allocator;

	// TODO: When project is complete we dont need to recompile this everytime
	system("solc --bin '" ETH_CONTRACT_SOL "' | tail -n +4 > '" ETH_CONTRACT_BIN "'");
	system("solc --abi '" ETH_CONTRACT_SOL "' | tail -n +4 > '" ETH_CONTRACT_ABI "'");
	//TODO: Check for success

	contract_bin = trim(readFile2(ETH_CONTRACT_BIN));

	result = this->eth_createContract(contract_bin);
	source = (char*) result.c_str(); //TODO: Is there a better solution than this cast?

	cout << result << endl;

	status = jsonParse(source, &endptr, &value, allocator);
	if (status != JSON_OK) {
		cerr << "Error parsing responce JSON data from eth_sendTransaction() during contract creation: " << jsonStrError(status) <<
			" at " << endptr - source << endl;
		exit(EXIT_FAILURE);
	}

	if (value.getTag() == JSON_OBJECT) {
		for (auto i: value) {
			if (strcmp(i->key, "result") == 0) {
				cout << i->value.toString() << endl;
				transaction_hash = i->value.toString();
				json_error = false;
				break;
			}
		}
	}

	if (json_error) {
		cerr << "Corrupt JSON responce to eth_sendTransaction during contract creation: " <<
		 	result << endl;
		exit(EXIT_FAILURE);
	}

	cout << "Parsed contract creation transaction hash: " <<
		transaction_hash << endl;

	//TODO: eth.getTransactionReceipt -> json(contractAddress)
	transaction_receipt = this->getTransactionReceipt(transaction_hash);

	cout << "Transaction Receipt: " <<
		transaction_receipt << endl;

	contract_addr = getJSONstring(transaction_receipt, "contractAddr");

	if (contract_addr.compare("null") == 0) {
		cerr << "Failed to obtain contract address..." << endl;
		exit(EXIT_FAILURE);
	}

	cout << "Contract Address: " <<
		contract_addr << endl;

	//TODO: Save contract to config
	cfg_root->add("contract_addr", Setting::TypeString) = contract_addr;
	cfg.writeFile(BLOCKCHAINSEC_CONFIG_F);

	return true;
}



string BlockchainSecLib::getTransactionReceipt(string transaction_hash) {
	int retries = 0;
	string transaction_receipt, result;

	while(retries <= BLOCKCHAINSEC_GETTRANSRECEIPT_MAXRETRIES) {
		transaction_receipt = this->eth_getTransactionReceipt(transaction_hash);
		retries++;
		cout << "Try #" << retries << endl;
		cout << transaction_receipt << endl << endl;

		try {
			result = this->getJSONstring(transaction_receipt, "result");
			return result;
		} catch(jsonNullException& e) {
			sleep(BLOCKCHAINSEC_GETTRANSRECEIPT_RETRY_DELAY);
			continue;
		} catch(jsonTypeException& e) {
			throw e; //TODO
		}

		sleep(BLOCKCHAINSEC_GETTRANSRECEIPT_RETRY_DELAY);
	}
	throw runtime_error("Failed to obtain transaction result in getTransactionReceipt() for transaction hash \"" + transaction_hash + "\"; transaction may or may not have been mined!");
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
									"\"data\":\"0x" + abi_data +
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
									"\"data\":\"0x" + abi_data +
								"\"}],"
								"\"id\":1}";

#ifdef _DEBUG
	cout << "eth_sendTransaction()" << endl;
#endif //_DEBUG

	return this->eth_ipc_request(json_request);
}



string BlockchainSecLib::eth_createContract(string data) {
	string json_request = "{\"jsonrpc\":\"2.0\","
								"\"method\":\"eth_sendTransaction\""
								",\"params\":[{"
									"\"from\":\"" + this->eth_my_addr + "\","
									//"\"gas\":0,"
									//"\"gasPrice\":\"" + ETH_DEFAULT_GAS + "\","
									"\"data\":\"0x" + data +
								"\"}],"
								"\"id\":1}";

#ifdef _DEBUG
	cout << "eth_createContract()" << endl;
#endif //_DEBUG

	return this->eth_ipc_request(json_request);
}



string BlockchainSecLib::eth_getTransactionReceipt(string transaction_hash) {
	string json_request = "{\"jsonrpc\":\"2.0\","
								"\"method\":\"eth_getTransactionReceipt\","
								"\"params\":["
									"\"" + transaction_hash + "\""
								"],\"id\":1}";

#ifdef _DEBUG
	cout << "eth_getTransactionReceipt()" << endl;
#endif //_DEBUG

	return this->eth_ipc_request(json_request);
}



string BlockchainSecLib::getJSONstring(string json, string element) {
	char *source, *endptr;
	int status;
	JsonValue value;
	JsonAllocator allocator;

	source = (char*) json.c_str();

	status = jsonParse(source, &endptr, &value, allocator);
	if (status != JSON_OK) {
		cerr << "Error parsing responce JSON data in getJSONstring(): " << jsonStrError(status) <<
			" at " << endptr - source << endl;
		string error = "Error parsing JSON data in getJSONstring(): ";
		error += jsonStrError(status);
		error += " at ";
		error += (endptr - source);
		throw runtime_error(error);
	}
	return this->getJSONstring(value, element);
}



string BlockchainSecLib::getJSONstring(JsonValue value, string element) {
	string result;
	bool json_error = true;

	if (value.getTag() == JSON_OBJECT || value.getTag() == JSON_ARRAY) {
		for (auto i: value) {
			if (i->value.getTag() == JSON_OBJECT) {
				try {
					result = this->getJSONstring(i->value, element);
					return result;
				} catch(runtime_error& e) {
					
				}
			} else if ((strcmp(i->key, element.c_str()) == 0) && (i->value.getTag() == JSON_STRING)) {
				cout << "getJSONstring(): key = " << i->key << "; value = " << i->value.toString() << endl;
				result = i->value.toString();
				json_error = false;
				break;
			} else if (strcmp(i->key, element.c_str()) == 0) {
				if (i->value.getTag() == JSON_NULL) {
					throw new jsonNullException("Requested JSON element is null.");
				} else {
					throw new jsonTypeException("Requested JSON element was a type other than JSON_STRING.");
				}
			}
		}
	}

	if (json_error) {
		cerr << "getJSONstring() did not find the requested element \"" <<
			element << "\"" << endl;
		string error = "getJSONstring() did not find the requested element \"";
		error += element;
		error += "\"";
		throw runtime_error(error);
	}
	return result;
}



} //namespace blockchain-sec
