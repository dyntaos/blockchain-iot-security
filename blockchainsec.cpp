#include <string>
#include <array>
#include <iostream>
#include <sstream>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <blockchainsec.hpp>
#include <ethabi.hpp>
#include <misc.hpp>


//TODO: Make a function to verify ethereum address formatting! (Apply to configuration file validation)
//TODO: Make function to see if "0x" needs to be prepended to function arguments
//TODO: Make all stored addresses NOT store "0x"
//TODO: Escape quotes in strings to ethabi

using namespace std;
using namespace libconfig;
namespace blockchainSec {



string BlockchainSecLib::getClientAddress(void) {
	return clientAddress;
}



string BlockchainSecLib::getContractAddress(void) {
	return contractAddress;
}



BlockchainSecLib::BlockchainSecLib(void) : BlockchainSecLib(false) {}



BlockchainSecLib::~BlockchainSecLib() {}



BlockchainSecLib::BlockchainSecLib(bool compile) {

	cfg.setOptions(Config::OptionFsync
				 | Config::OptionSemicolonSeparators
				 | Config::OptionColonAssignmentForGroups
				 | Config::OptionOpenBraceOnSeparateLine);

	cout << "Reading config file..." << endl;

	try {
		cfg.readFile(BLOCKCHAINSEC_CONFIG_F);

	} catch(const FileIOException &e) {
		cerr << "IO error reading config file!" << endl;
		exit(EXIT_FAILURE);

	} catch(const ParseException &e) {
		cerr << "BlockchainSec: Config file error "
			<< e.getFile()
			<< ":"
			<< e.getLine()
			<< " - "
			<< e.getError()
			<< endl;
		exit(EXIT_FAILURE);
	}

	cfgRoot = &cfg.getRoot();

	if (!cfg.exists("ipcPath")) {
		cerr << "Configuration file does not contain 'ipcPath'!" << endl;
		exit(EXIT_FAILURE);
	}

	cfg.lookupValue("ipcPath", ipcPath); //TODO Check for reasonableness

	if (!cfg.exists("clientAddress")) {
		cerr << "Configuration file does not contain 'clientAddress'!" << endl;
		exit(EXIT_FAILURE);
	}

	cfg.lookupValue("clientAddress", clientAddress); //TODO Check for reasonableness

	if (!compile && !cfg.exists("contractAddress")) {
		cerr << "Config doesnt have contractAddress"
			<< endl
			<< "Either add a contract address to the conf file or "
			"create a new contract using the `--compile` flag"
			<< endl;

		exit(EXIT_FAILURE);
	}

	if (compile) {
		/* We will compile the contract with solc, upload it
		 * to the chain and save the address to the config file */
		cout << "Compiling and uploading contract..."
			<< endl
			<< endl;

		try {
			this->create_contract();
			cout << "Successfully compiled and uploaded contract"
				<< endl
				<< "The new contract address has been "
				"written to the configuration file"
				<< endl;

			exit(EXIT_SUCCESS);

		} catch(const TransactionFailedException &e) {
			throw e;
			cerr << "Failed to create contract!" << endl;
			exit(EXIT_FAILURE);
		}
	}

	cout << "Config contains contractAddress" << endl;
	cfg.lookupValue("contractAddress", contractAddress);

	eventLogWaitManager = new EventLogWaitManager(getClientAddress().substr(2), getContractAddress().substr(2), ipcPath);

	subscriptionListener = new thread(&EventLogWaitManager::ipc_subscription_listener_thread, eventLogWaitManager);
}



// Throws ResourceRequestFailedException from ethabi()
string BlockchainSecLib::getFrom(string funcName, string ethabiEncodeArgs) {
	string data, result;

	data = ethabi(
		"encode -l function " ETH_CONTRACT_ABI " " +
		funcName +
		ethabiEncodeArgs
	);

	Json callJson = call_helper(data);
	result = callJson["result"];
	return result.substr(2);
}



// Throws ResourceRequestFailedException from ethabi()
string BlockchainSecLib::getFromDeviceID(string funcName, uint32_t deviceID) {
	return getFrom(funcName, " -p '" + boost::lexical_cast<string>(deviceID) + "'");
}



// Throws ResourceRequestFailedException from ethabi()
uint64_t BlockchainSecLib::getIntFromContract(string funcName) {
	stringstream ss;
	uint64_t result;

	ss << getFrom(funcName, "");
	ss >> result;

	return result;
}



// Throws ResourceRequestFailedException from ethabi()
uint64_t BlockchainSecLib::getIntFromDeviceID(string funcName, uint32_t deviceID) {
	stringstream ss;
	uint64_t result;

	ss << getFromDeviceID(funcName, deviceID);
	ss >> result;

	return result;
}



// Throws ResourceRequestFailedException from ethabi()
string BlockchainSecLib::getStringFromDeviceID(string funcName, uint32_t deviceID) {
	return ethabi_decode_result(
		ETH_CONTRACT_ABI,
		funcName,
		getFromDeviceID(funcName, deviceID)
	);
}



// Throws ResourceRequestFailedException from ethabi()
string BlockchainSecLib::getArrayFromContract(string funcName) {
	string arrayStr;
	arrayStr = getFrom(funcName, "");
	//cout << funcName << ":" << endl << arrayStr << endl;
	return arrayStr;
}



Json BlockchainSecLib::call_helper(string data) {
	string callStr = eth_call(data);
	Json callJson = Json::parse(callStr);
	string callResult = callJson["result"];
	if (callResult.compare("0x") == 0) {
		// The contract failed to execute (a require statement failed)
		throw new CallFailedException("eth_call did not execute successfully!");
	}
	return callJson;
}



unique_ptr<unordered_map<string, string>> BlockchainSecLib::contract_helper(string data) {
	string transactionHash, transactionReceipt;
	Json transactionJsonData;

	// Make an eth_call with the parameters first to check the contract will not fail
	call_helper(data);

	transactionHash = eth_sendTransaction(data);
	transactionJsonData = Json::parse(transactionHash);
	auto findResult = transactionJsonData.find("result");

	if (findResult == transactionJsonData.end()) {
		throw TransactionFailedException(
			"Transaction hash was not present in responce to eth_sendTransaction!"
		);
	}

	transactionHash = findResult.value();
	transactionReceipt = getTransactionReceipt(transactionHash);
	return eventLogWaitManager->getEventLog(transactionHash);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool BlockchainSecLib::callMutatorContract(string funcName, string ethabiEncodeArgs) {
	unique_ptr<unordered_map<string, string>> eventLog;
	string data;

	data = ethabi(
		"encode -l function " ETH_CONTRACT_ABI " " + funcName + ethabiEncodeArgs
	);

	try {
		eventLog = contract_helper(data);
	} catch (const CallFailedException &e) {
		return false;
	} catch (const TransactionFailedException &e) {
		return false;
	}

#ifdef _DEBUG
	string logStr = "{ ";
	for (std::pair<std::string, std::string> kv : *eventLog.get()) {
		logStr += "\"" + kv.first + "\":\"" + kv.second + "\", ";
	}
	logStr = logStr.substr(0, logStr.length() - 2);
	logStr += " }";
	cout << funcName
		<< "() successful!"
		<< endl
		<< logStr
		<< endl;
#endif //_DEBUG

	return true;
}



// TODO: Change "clientAddress" to something more suitable
// Throws ResourceRequestFailedException from ethabi()
bool BlockchainSecLib::is_admin(string clientAddress) {
	return stoi(getFrom("is_admin", " -p '" + clientAddress + "'")) == 1;
}



// Throws ResourceRequestFailedException from ethabi()
bool BlockchainSecLib::is_authd(uint32_t deviceID) {
	return getIntFromDeviceID("is_authd", deviceID) != 0;
}



// Throws ResourceRequestFailedException from ethabi()
bool BlockchainSecLib::is_device(uint32_t deviceID) {
	return getIntFromDeviceID("is_device", deviceID) != 0;
}



// Throws ResourceRequestFailedException from ethabi()
bool BlockchainSecLib::is_gateway(uint32_t deviceID) {
	return getIntFromDeviceID("is_gateway", deviceID) != 0;
}



// Throws ResourceRequestFailedException from ethabi()
uint32_t BlockchainSecLib::get_my_device_id(void) {
	return getIntFromContract("get_my_device_id");
}



// Throws ResourceRequestFailedException from ethabi()
string BlockchainSecLib::get_key(uint32_t deviceID) {
	return getStringFromDeviceID("get_key", deviceID);
}



// Throws ResourceRequestFailedException from ethabi()
BlockchainSecLib::AddrType BlockchainSecLib::get_addrtype(uint32_t deviceID) {
	return BlockchainSecLib::AddrType(getIntFromDeviceID("get_addrType", deviceID));
}



// Throws ResourceRequestFailedException from ethabi()
string BlockchainSecLib::get_addr(uint32_t deviceID) {
	return getStringFromDeviceID("get_addr", deviceID);
}



// Throws ResourceRequestFailedException from ethabi()
string BlockchainSecLib::get_name(uint32_t deviceID) {
	return getStringFromDeviceID("get_name", deviceID);
}



// Throws ResourceRequestFailedException from ethabi()
string BlockchainSecLib::get_mac(uint32_t deviceID) {
	return getStringFromDeviceID("get_mac", deviceID);
}



// Throws ResourceRequestFailedException from ethabi()
string BlockchainSecLib::get_data(uint32_t deviceID) {
	return getStringFromDeviceID("get_data", deviceID);
}



// Throws ResourceRequestFailedException from ethabi()
time_t BlockchainSecLib::get_dataTimestamp(uint32_t deviceID) {
	return getIntFromDeviceID("get_dataTimestamp", deviceID);
}



// Throws ResourceRequestFailedException from ethabi()
time_t BlockchainSecLib::get_creationTimestamp(uint32_t deviceID) {
	return getIntFromDeviceID("get_creationTimestamp", deviceID);
}



// Throws ResourceRequestFailedException from ethabi()
uint16_t BlockchainSecLib::get_num_admin(void) {
	return getIntFromContract("get_num_admin");
}



// Throws ResourceRequestFailedException from ethabi()
vector<string> BlockchainSecLib::get_active_admins(void) {
	return ethabi_decode_array(
		ETH_CONTRACT_ABI,
		"get_active_admins",
		getArrayFromContract("get_active_admins")
	);
}



// Throws ResourceRequestFailedException from ethabi()
vector<string> BlockchainSecLib::get_authorized_devices(void) {
	return ethabi_decode_array(
		ETH_CONTRACT_ABI,
		"get_authorized_devices",
		getArrayFromContract("get_authorized_devices")
	);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool BlockchainSecLib::add_device(string deviceAddress, string name, string mac, string publicKey, bool gatewayManaged) {
	string ethabiEncodeArgs;

	if (name.length() > BLOCKCHAINSEC_MAX_DEV_NAME) {
		throw InvalidArgumentException(
			"Device name exceeds maximum length of BLOCKCHAINSEC_MAX_DEV_NAME."
		);
	}

	ethabiEncodeArgs = " -p '" + deviceAddress +
						"' -p '" + name +
						"' -p '" + mac +
						"' -p '" + publicKey +
						"' -p " + (gatewayManaged ? "true" : "false");

	return callMutatorContract("add_device", ethabiEncodeArgs);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool BlockchainSecLib::add_gateway(string gatewayAddress, string name, string mac, string publicKey) {
	string ethabiEncodeArgs;

	if (name.length() > BLOCKCHAINSEC_MAX_DEV_NAME) {
		throw InvalidArgumentException(
			"Device name exceeds maximum length of BLOCKCHAINSEC_MAX_DEV_NAME."
		);
	}

	ethabiEncodeArgs = " -p '" + gatewayAddress +
						"' -p '" + name +
						"' -p '" + mac +
						"' -p '" + publicKey + "'";

	return callMutatorContract("add_gateway", ethabiEncodeArgs);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool BlockchainSecLib::remove_device(uint32_t deviceID) {
	string ethabiEncodeArgs;

	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(deviceID) + "'";

	return callMutatorContract("remove_device", ethabiEncodeArgs);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool BlockchainSecLib::remove_gateway(uint32_t deviceID) {
	string ethabiEncodeArgs;

	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(deviceID) + "'";

	return callMutatorContract("remove_gateway", ethabiEncodeArgs);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool BlockchainSecLib::update_addr(uint32_t deviceID, BlockchainSecLib::AddrType addrType, string addr) {
	string ethabiEncodeArgs;

	//TODO: Escape quotes in strings? Or will the return value of ethabi handle invalid input?
	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(deviceID) +
						"' -p '" + boost::lexical_cast<string>(addrType) +
						"' -p '" + addr + "'";

	return callMutatorContract("update_addr", ethabiEncodeArgs);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool BlockchainSecLib::push_data(uint32_t deviceID, string data) {
	string ethabiEncodeArgs;

	//TODO URGENT: Escape quotes in strings!!
	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(deviceID) +
						"' -p '" + data + "'";

	return callMutatorContract("push_data", ethabiEncodeArgs);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool BlockchainSecLib::authorize_admin(string adminAddr) {
	string ethabiEncodeArgs;

	//TODO URGENT: Escape quotes in strings!!
	ethabiEncodeArgs = " -p '" + adminAddr + "'";

	return callMutatorContract("authorize_admin", ethabiEncodeArgs);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool BlockchainSecLib::deauthorize_admin(string adminAddr) {
	string ethabiEncodeArgs;

	//TODO URGENT: Escape quotes in strings!!
	ethabiEncodeArgs = " -p '" + adminAddr + "'";

	return callMutatorContract("deauthorize_admin", ethabiEncodeArgs);
}




#ifdef _DEBUG
void BlockchainSecLib::test(void) {

}
#endif //_DEBUG



void BlockchainSecLib::create_contract(void) {
	string contractBin,
			transactionJsonStr,
			transactionHash,
			transactionReceipt,
			contractAddress;
	Json transactionJsonData, receiptJsonData;

	// TODO: When project is complete we dont need to recompile this everytime
	system("solc --bin '" ETH_CONTRACT_SOL "' | tail -n +4 > '" ETH_CONTRACT_BIN "'");
	system("solc --abi '" ETH_CONTRACT_SOL "' | tail -n +4 > '" ETH_CONTRACT_ABI "'");
	//TODO: Check for success

	contractBin = boost::trim_copy(readFile2(ETH_CONTRACT_BIN));

	transactionJsonStr = this->eth_createContract(contractBin);

	transactionJsonData = Json::parse(transactionJsonStr);
	auto jsonFindResult = transactionJsonData.find("result");

	if (jsonFindResult == transactionJsonData.end()) {
		// "result" not in JSON responce
		// TODO: What if "result" is not a string
		throw TransactionFailedException("create_contract(): Transaction hash was not present in responce to eth_sendTransaction!");
	}

	transactionHash = jsonFindResult.value();

	cout << "Parsed contract creation transaction hash: " <<
		transactionHash << endl;

	try {
		transactionReceipt = this->getTransactionReceipt(transactionHash);
	} catch(const TransactionFailedException &e) {
		//TODO: How to best handle this?
		throw e;
	}

	receiptJsonData = Json::parse(transactionReceipt);
	jsonFindResult = receiptJsonData.find("result");

	if (!jsonFindResult.value().is_object()) {
		throw TransactionFailedException("create_contract(): \"result\" was not a JSON object!");
	}
	auto subJsonFindResult = jsonFindResult.value().find("contractAddress");

	if (subJsonFindResult == jsonFindResult.value().end()) {
		// "contractAddress" not in JSON responce
		// TODO: What if "contractAddress" is not a string
		throw TransactionFailedException("create_contract(): \"contractAddress\" was not present in responce!");
	}

	contractAddress = subJsonFindResult.value();

	if (contractAddress.compare("null") == 0) {
		throw TransactionFailedException("create_contract(): \"contractAddress\" was null!");
	}

	cout << "Contract Address: " << contractAddress << endl;

	if (cfgRoot->exists("contractAddress")) cfgRoot->remove("contractAddress");
	cfgRoot->add("contractAddress", Setting::TypeString) = contractAddress;
	this->contractAddress = contractAddress;
	cfg.writeFile(BLOCKCHAINSEC_CONFIG_F);
}



string BlockchainSecLib::getTransactionReceipt(string transactionHash) {
	int retries = 0;
	string transactionReceipt, result;
	Json jsonData;

	while(retries <= BLOCKCHAINSEC_GETTRANSRECEIPT_MAXRETRIES) {
		transactionReceipt = this->eth_getTransactionReceipt(transactionHash);
		retries++;

#ifdef _DEBUG
		cout << "Try #" << retries << endl;
		cout << transactionReceipt << endl << endl;
#endif //_DEBUG

		jsonData = Json::parse(transactionReceipt);
		auto jsonFindResult = jsonData.find("result");
		if (jsonFindResult == jsonData.end()) {
			sleep(BLOCKCHAINSEC_GETTRANSRECEIPT_RETRY_DELAY);
			continue;
		} else if (jsonFindResult.value().is_null()) {
			sleep(BLOCKCHAINSEC_GETTRANSRECEIPT_RETRY_DELAY);
			continue;
		}

		return transactionReceipt;
	}

	throw TransactionFailedException(
			"Failed to obtain transaction result in getTransactionReceipt() "
			"for transaction hash \"" + transactionHash + "\"; "
			"transaction may or may not have been mined!"
	);
}



// TODO: Should this be ported to Unix Domain Sockets?
// TODO: At the very least, replace cerr/return with exceptions
string BlockchainSecLib::eth_ipc_request(string jsonRequest) {
	int ipcFdFlags, ipcFd;
	string json;
	array<char, PIPE_BUFFER_LENGTH> ipcBuffer;

#ifdef _DEBUG
	cout << "eth_ipc_request(): " << jsonRequest << endl;
#endif //_DEBUG

	FILE *ipc = popen(("echo '" + jsonRequest + "' | nc -U '" + ipcPath + "'").c_str(), "r");
	if (ipc == NULL) {
		// Failed to open Unix domain socket for IPC -- Perhaps geth is not running?
		throw ResourceRequestFailedException("eth_ipc_request(): Failed to popen() unix domain socket for IPC with geth! Is geth running?");
	}

	ipcFd = fileno(ipc);

	if (fgets(ipcBuffer.data(), PIPE_BUFFER_LENGTH, ipc) == NULL) {
		throw ResourceRequestFailedException("eth_ipc_request(): Error: Failed to read from IPC!");
	}

	json += ipcBuffer.data();

	ipcFdFlags = fcntl(ipcFd, F_GETFL, 0);
	ipcFdFlags |= O_NONBLOCK;
	fcntl(ipcFd, F_SETFL, ipcFdFlags);

	while (fgets(ipcBuffer.data(), PIPE_BUFFER_LENGTH, ipc) != NULL) {
#ifdef _DEBUG
		cout << "eth_ipc_request(): Read: ''" << ipcBuffer.data() << "'" << endl;
#endif //_DEBUG
		json += ipcBuffer.data();
	}

	if (pclose(ipc) < 0) {
		throw ResourceRequestFailedException("eth_ipc_request(): Failed to pclose() unix domain socket for IPC with geth!");
	}

#ifdef _DEBUG
	cout << "eth_ipc_request(): Successfully relayed request" << endl;
#endif //_DEBUG

	return json;
}



string BlockchainSecLib::eth_call(string abiData) {
	string jsonRequest = "{\"jsonrpc\":\"2.0\","
								"\"method\":\"eth_call\","
								"\"params\":[{"
									//"\"from\":\"" + clientAddress + "\","
									"\"to\":\"" + contractAddress + "\","
									"\"data\":\"0x" + abiData +
								"\"},\"latest\"],\"id\":1}";

#ifdef _DEBUG
	cout << "eth_call()" << endl;
#endif //_DEBUG

	return this->eth_ipc_request(jsonRequest);
}



string BlockchainSecLib::eth_sendTransaction(string abiData) {
	string jsonRequest = "{\"jsonrpc\":\"2.0\","
								"\"method\":\"eth_sendTransaction\""
								",\"params\":[{"
									"\"from\":\"" + clientAddress + "\","
									"\"to\":\"" + contractAddress + "\","
									"\"gas\":\"0x14F46B\"," //TODO: WHERE THE THE BLOCK GAS LIMIT SET? Choose this value more intentionally
									"\"gasPrice\":\"0x0\","
									"\"data\":\"0x" + abiData +
								"\"}],"
								"\"id\":1}";

#ifdef _DEBUG
	cout << "eth_sendTransaction()" << endl;
#endif //_DEBUG

	return this->eth_ipc_request(jsonRequest);
}



string BlockchainSecLib::eth_createContract(string data) {
	string jsonRequest = "{\"jsonrpc\":\"2.0\","
								"\"method\":\"eth_sendTransaction\""
								",\"params\":[{"
									"\"from\":\"" + clientAddress + "\","
									//"\"gas\":0,"
									//"\"gasPrice\":\"" + ETH_DEFAULT_GAS + "\","
									"\"data\":\"0x" + data +
								"\"}],"
								"\"id\":1}";

#ifdef _DEBUG
	cout << "eth_createContract()" << endl;
#endif //_DEBUG

	return this->eth_ipc_request(jsonRequest);
}



string BlockchainSecLib::eth_getTransactionReceipt(string transactionHash) {
	string jsonRequest = "{\"jsonrpc\":\"2.0\","
								"\"method\":\"eth_getTransactionReceipt\","
								"\"params\":["
									"\"" + transactionHash + "\""
								"],\"id\":1}";

#ifdef _DEBUG
	cout << "eth_getTransactionReceipt()" << endl;
#endif //_DEBUG

	return this->eth_ipc_request(jsonRequest);
}



void BlockchainSecLib::joinThreads(void) {
	subscriptionListener->join();
}



} //namespace blockchain-sec
