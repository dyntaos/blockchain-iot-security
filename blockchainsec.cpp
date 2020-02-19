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
#include <cpp-base64/base64.h>


//TODO: Make a function to verify ethereum address formatting! (Apply to configuration file validation)
//TODO: Make function to see if "0x" needs to be prepended to function arguments
//TODO: Make all stored addresses NOT store "0x"


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

	if (sodium_init() < 0) {
		throw CryptographicFailureException("Could not initialize libSodium!");
	}

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

	cfg.lookupValue("ipcPath", ipcPath); // TODO Check return value

	if (!cfg.exists("clientAddress")) {
		cerr << "Configuration file does not contain 'clientAddress'!" << endl;
		exit(EXIT_FAILURE);
	}

	cfg.lookupValue("clientAddress", clientAddress); // TODO Check return value

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
	cfg.lookupValue("contractAddress", contractAddress); // TODO Check return value

	eventLogWaitManager = new EventLogWaitManager(getClientAddress().substr(2), getContractAddress().substr(2), ipcPath);
	subscriptionListener = new thread(&EventLogWaitManager::ipc_subscription_listener_thread, eventLogWaitManager);

	loadLocalDeviceParameters();
}



void BlockchainSecLib::loadLocalDeviceParameters(void) {
	// TODO: try catch ?
	localDeviceID = get_my_device_id();

	if (localDeviceID != 0) {
		// This client has a device ID -- verify it has proper keys
		string publicKey = boost::trim_copy(get_key(localDeviceID)); // TODO: What makes a valid public key?

		if (cfgRoot->exists("privateKey") && publicKey == "") {
			cerr << "Client has private key, but no public key exists on the blockchain for device ID " << localDeviceID << endl;
			// TODO: Throw?
			// TODO: CLI Flag to force generate new keys?
			exit(EXIT_FAILURE);
		}
		if (!cfgRoot->exists("privateKey") && publicKey != "") {
			cerr << "Client has public key on the blockchain, but no private key exists locally!" << endl;
			// TODO: Throw?
			// TODO: CLI Flag to force generate new keys?
			exit(EXIT_FAILURE);
		}

		if (!cfgRoot->exists("privateKey") && publicKey == "") {
			if (updateLocalKeys()) {
				cout << "Successfully generated keypair for local client (device ID " << localDeviceID << ")..." << endl;
			} else {
				cerr << "Failed to create keys for local client (device ID " << localDeviceID << ")..." << endl;
			}

		} else {
			string hexSk;
			vector<char> byteVector;

			cfg.lookupValue("privateKey", hexSk); // TODO Check return value
			byteVector = hexToBytes(hexSk);
			memcpy(client_sk, byteVector.data(), crypto_kx_SECRETKEYBYTES);
			client_sk[crypto_kx_SECRETKEYBYTES] = 0;
			cout << "Loaded private key..." << endl;

			byteVector = hexToBytes(publicKey);
			memcpy(client_pk, byteVector.data(), crypto_kx_PUBLICKEYBYTES);
			client_pk[crypto_kx_PUBLICKEYBYTES] = 0;
			cout << "Loaded public key..." << endl;
		}
	}
	loadDataReceiverPublicKey(localDeviceID);

}



bool BlockchainSecLib::loadDataReceiverPublicKey(uint32_t deviceID) {
	uint32_t dataReceiver;
	string dataReceiverPublicKey;
	vector<char> byteVector;

	try {
		dataReceiver = get_datareceiver(deviceID);
		if (dataReceiver == 0) {
			return false;
		}
		dataReceiverPublicKey = get_key(dataReceiver);
	} catch (BlockchainSecLibException &e) {
		return false;
	}
	if (boost::trim_copy(dataReceiverPublicKey) == "") return false;

	byteVector = hexToBytes(dataReceiverPublicKey);
	memcpy(dataReceiver_pk, byteVector.data(), crypto_kx_PUBLICKEYBYTES);
	dataReceiver_pk[crypto_kx_PUBLICKEYBYTES] = 0;

	return true;
}



// Throws ResourceRequestFailedException from ethabi()
string BlockchainSecLib::getFrom(string const& funcName, string const& ethabiEncodeArgs) {
	string data, result;

	data = ethabi(
		"encode -l function " ETH_CONTRACT_ABI " " +
		funcName +
		ethabiEncodeArgs
	);

#ifdef _DEBUG
	cout << "getFrom(): " << funcName << "  " << ethabiEncodeArgs << endl;
#endif //_DEBUG

	Json callJson = call_helper(data);
	result = callJson["result"];

	return result.substr(2);
}



// Throws ResourceRequestFailedException from ethabi()
string BlockchainSecLib::getFromDeviceID(string const& funcName, uint32_t deviceID) {
	return getFrom(funcName, " -p '" + boost::lexical_cast<string>(deviceID) + "'");
}



// Throws ResourceRequestFailedException from ethabi()
uint64_t BlockchainSecLib::getIntFromContract(string const& funcName) {
	stringstream ss;
	uint64_t result;

	ss << getFrom(funcName, "");
	ss >> result;

	return result;
}



// Throws ResourceRequestFailedException from ethabi()
uint64_t BlockchainSecLib::getIntFromDeviceID(string const& funcName, uint32_t deviceID) {
	string value;

	value = getFromDeviceID(funcName, deviceID);
	return stoul(value, nullptr, 16);
}



// Throws ResourceRequestFailedException from ethabi()
string BlockchainSecLib::getStringFromDeviceID(string const& funcName, uint32_t deviceID) {
	return ethabi_decode_result(
		ETH_CONTRACT_ABI,
		funcName,
		getFromDeviceID(funcName, deviceID)
	);
}



// Throws ResourceRequestFailedException from ethabi()
vector<string> BlockchainSecLib::getStringsFromDeviceID(string const& funcName, uint32_t deviceID) {
	return ethabi_decode_results(
		ETH_CONTRACT_ABI,
		funcName,
		getFromDeviceID(funcName, deviceID)
	);
}



// Throws ResourceRequestFailedException from ethabi()
string BlockchainSecLib::getArrayFromContract(string const& funcName) {
	string arrayStr;
	arrayStr = getFrom(funcName, "");
	return arrayStr;
}



Json BlockchainSecLib::call_helper(string const& data) {
	string callStr = eth_call(data);
	Json callJson = Json::parse(callStr);
	string callResult = callJson["result"];
	if (callResult.compare("0x") == 0) {
		// The contract failed to execute (a require statement failed)
		throw CallFailedException("eth_call did not execute successfully!");
	}
	return callJson;
}



unique_ptr<unordered_map<string, string>> BlockchainSecLib::contract_helper(string const& data) {
	string transactionHash, transactionReceipt;
	Json transactionJsonData;

#ifdef _DEBUG
	cout << "contract_helper(): Before call_helper()" << endl;
#endif //_DEBUG

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
#ifdef _DEBUG
	cout << "contract_helper(): Calling eventLogWaitManager->getEventLog()..." << endl << endl;
#endif //_DEBUG
	return eventLogWaitManager->getEventLog(transactionHash);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool BlockchainSecLib::callMutatorContract(string const& funcName, string const& ethabiEncodeArgs, unique_ptr<unordered_map<string, string>> & eventLog) {
	//unique_ptr<unordered_map<string, string>> eventLog;  // TODO
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



// Throws ResourceRequestFailedException from ethabi()
// Throws InvalidArgumentException
bool BlockchainSecLib::is_admin(string const& isAdminAddress) {
	if (!isEthereumAddress(isAdminAddress)) {
		throw InvalidArgumentException("Address is an invalid format!");
	}
	return stoi(getFrom("is_admin", " -p '" + isAdminAddress + "'")) == 1;
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
uint32_t BlockchainSecLib::get_datareceiver(uint32_t deviceID) {
	return getIntFromDeviceID("get_datareceiver", deviceID);
}



// Throws ResourceRequestFailedException from ethabi()
uint32_t BlockchainSecLib::get_default_datareceiver() {
	return getIntFromContract("get_default_datareceiver");
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
vector<string> BlockchainSecLib::get_data(uint32_t deviceID) {
	vector <string> result = getStringsFromDeviceID("get_data", deviceID);
	//result[0] = result[0].substr(1, result[0].length() - 2);
	result[1] = to_string(strtol(result[1].c_str(), nullptr, 16));
	//result[2] = result[2].substr(1, result[2].length() - 2);
	return result;
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
uint32_t BlockchainSecLib::get_num_admin(void) {
	return getIntFromContract("get_num_admin");
}



// Throws ResourceRequestFailedException from ethabi()
uint32_t BlockchainSecLib::get_num_devices(void) {
	return getIntFromContract("get_num_devices");
}



// Throws ResourceRequestFailedException from ethabi()
uint32_t BlockchainSecLib::get_num_gateways(void) {
	return getIntFromContract("get_num_gateways");
}



// Throws ResourceRequestFailedException from ethabi()
vector<uint32_t> BlockchainSecLib::get_active_admins(void) {
	return ethabi_decode_uint32_array(
		ETH_CONTRACT_ABI,
		"get_active_admins",
		getArrayFromContract("get_active_admins")
	);
}



// Throws ResourceRequestFailedException from ethabi()
vector<uint32_t> BlockchainSecLib::get_authorized_devices(void) {
	return ethabi_decode_uint32_array(
		ETH_CONTRACT_ABI,
		"get_authorized_devices",
		getArrayFromContract("get_authorized_devices")
	);
}



// Throws ResourceRequestFailedException from ethabi()
vector<uint32_t> BlockchainSecLib::get_authorized_gateways(void) {
	return ethabi_decode_uint32_array(
		ETH_CONTRACT_ABI,
		"get_authorized_gateways",
		getArrayFromContract("get_authorized_gateways")
	);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
uint32_t BlockchainSecLib::add_device(string const& deviceAddress, string const& name, string const& mac, bool gatewayManaged) {
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	if (!isEthereumAddress(deviceAddress)) {
		throw InvalidArgumentException("Address is an invalid format!");
	}

	if (name.length() > BLOCKCHAINSEC_MAX_DEV_NAME) {
		throw InvalidArgumentException(
			"Device name exceeds maximum length of BLOCKCHAINSEC_MAX_DEV_NAME."
		);
	}

	ethabiEncodeArgs = " -p '" + deviceAddress +
						"' -p '" + escapeSingleQuotes(name) +
						"' -p '" + escapeSingleQuotes(mac) +
						"' -p " + (gatewayManaged ? "true" : "false");

	if (callMutatorContract("add_device", ethabiEncodeArgs, eventLog)) {
		return stoul((*eventLog.get())["device_id"], NULL, 16); // TODO: What id [device_id] doesn't exist? try catch
	} else {
		return 0;
	}
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
uint32_t BlockchainSecLib::add_gateway(string const& gatewayAddress, string const& name, string const& mac) {
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	if (name.length() > BLOCKCHAINSEC_MAX_DEV_NAME) {
		throw InvalidArgumentException(
			"Device name exceeds maximum length of BLOCKCHAINSEC_MAX_DEV_NAME."
		);
	}

	ethabiEncodeArgs = " -p '" + gatewayAddress +
						"' -p '" + escapeSingleQuotes(name) +
						"' -p '" + escapeSingleQuotes(mac) + "'";

	if (callMutatorContract("add_gateway", ethabiEncodeArgs, eventLog)) {
		return stoul((*eventLog.get())["device_id"], NULL, 16); // TODO: What id [device_id] doesn't exist? try catch
	} else {
		return 0;
	}
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool BlockchainSecLib::remove_device(uint32_t deviceID) {
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(deviceID) + "'";

	return callMutatorContract("remove_device", ethabiEncodeArgs, eventLog);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool BlockchainSecLib::remove_gateway(uint32_t deviceID) {
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(deviceID) + "'";

	return callMutatorContract("remove_gateway", ethabiEncodeArgs, eventLog);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool BlockchainSecLib::update_datareceiver(uint32_t deviceID, uint32_t dataReceiverID) {
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(deviceID) +
						"' -p '" + boost::lexical_cast<string>(dataReceiverID) + "'";

	return callMutatorContract("update_datareceiver", ethabiEncodeArgs, eventLog);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool BlockchainSecLib::set_default_datareceiver(uint32_t dataReceiverID) {
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(dataReceiverID) + "'";

	return callMutatorContract("set_default_datareceiver", ethabiEncodeArgs, eventLog);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool BlockchainSecLib::update_addr(uint32_t deviceID, BlockchainSecLib::AddrType addrType, string const& addr) {
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(deviceID) +
						"' -p '" + boost::lexical_cast<string>(addrType) +
						"' -p '" + escapeSingleQuotes(addr) + "'";

	return callMutatorContract("update_addr", ethabiEncodeArgs, eventLog);
}



bool BlockchainSecLib::update_publickey(uint32_t deviceID, string const& publicKey) {
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(deviceID) +
						"' -p '" + escapeSingleQuotes(publicKey) + "'";

	return callMutatorContract("update_publickey", ethabiEncodeArgs, eventLog);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool BlockchainSecLib::push_data(uint32_t deviceID, string & data, uint16_t dataLen, string & nonce) {
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	string dataAscii = base64_encode((unsigned char*) data.c_str(), data.length());
	string nonceAscii = base64_encode((unsigned char*) nonce.c_str(), nonce.length());

	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(deviceID) + "' -p '";
	ethabiEncodeArgs += dataAscii;
	ethabiEncodeArgs +=  "' -p '" + boost::lexical_cast<string>(dataLen) + "' -p '";
	ethabiEncodeArgs +=  nonceAscii;
	ethabiEncodeArgs += "'";

	return callMutatorContract("push_data", ethabiEncodeArgs, eventLog);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool BlockchainSecLib::authorize_admin(string const& adminAddress) {
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	if (!isEthereumAddress(adminAddress)) {
		throw InvalidArgumentException("Address is an invalid format!");
	}
	ethabiEncodeArgs = " -p '" + adminAddress + "'";

	return callMutatorContract("authorize_admin", ethabiEncodeArgs, eventLog);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool BlockchainSecLib::deauthorize_admin(string const& adminAddress) {
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	if (!isEthereumAddress(adminAddress)) {
		throw InvalidArgumentException("Address is an invalid format!");
	}
	ethabiEncodeArgs = " -p '" + adminAddress + "'";

	return callMutatorContract("deauthorize_admin", ethabiEncodeArgs, eventLog);
}




#ifdef _DEBUG
void BlockchainSecLib::test(void) {

}
#endif //_DEBUG



// Throws TransactionFailedException from eth_sendTransaction() and locally
// Throws ResourceRequestFailedException
void BlockchainSecLib::create_contract(void) {
	string contractBin,
			transactionJsonStr,
			transactionHash,
			transactionReceipt,
			contractAddress;
	Json transactionJsonData, receiptJsonData;

	if (system("solc --bin '" ETH_CONTRACT_SOL "' | tail -n +4 > '" ETH_CONTRACT_BIN "'") != 0) {
		throw ResourceRequestFailedException("solc failed to compile contract to binary format!");
	}
	if (system("solc --abi '" ETH_CONTRACT_SOL "' | tail -n +4 > '" ETH_CONTRACT_ABI "'") != 0) {
		throw ResourceRequestFailedException("solc failed to compile contract to abi format!");
	}

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

	transactionReceipt = this->getTransactionReceipt(transactionHash);

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



string BlockchainSecLib::getTransactionReceipt(string const& transactionHash) {
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

#ifdef _DEBUG
		cout << "getTransactionReceipt(): returning..." << endl << endl;
#endif //_DEBUG

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
string BlockchainSecLib::eth_ipc_request(string const& jsonRequest) {
	int ipcFdFlags, ipcFd;
	string json;
	array<char, IPC_BUFFER_LENGTH> ipcBuffer;

#ifdef _DEBUG
	cout << "eth_ipc_request(): " << jsonRequest << endl;
#endif //_DEBUG

	FILE *ipc = popen(("echo '" + jsonRequest + "' | nc -U '" + ipcPath + "'").c_str(), "r");
	if (ipc == NULL) {
		// Failed to open Unix domain socket for IPC -- Perhaps geth is not running?
		throw ResourceRequestFailedException("eth_ipc_request(): Failed to popen() unix domain socket for IPC with geth! Is geth running?");
	}

	ipcFd = fileno(ipc);

	if (fgets(ipcBuffer.data(), IPC_BUFFER_LENGTH, ipc) == NULL) {
		throw ResourceRequestFailedException("eth_ipc_request(): Error: Failed to read from IPC!");
	}

	json += ipcBuffer.data();

	ipcFdFlags = fcntl(ipcFd, F_GETFL, 0);
	ipcFdFlags |= O_NONBLOCK;
	fcntl(ipcFd, F_SETFL, ipcFdFlags);

	while (fgets(ipcBuffer.data(), IPC_BUFFER_LENGTH, ipc) != NULL) {
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



string BlockchainSecLib::eth_call(string const& abiData) {
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



string BlockchainSecLib::eth_sendTransaction(string const& abiData) {
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



string BlockchainSecLib::eth_createContract(string const& data) {
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



string BlockchainSecLib::eth_getTransactionReceipt(string const& transactionHash) {
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



bool BlockchainSecLib::updateLocalKeys(void) {
	unsigned char pk[crypto_kx_PUBLICKEYBYTES + 1], sk[crypto_kx_SECRETKEYBYTES + 1];
	//unsigned char rx[crypto_kx_SESSIONKEYBYTES + 1], tx[crypto_kx_SESSIONKEYBYTES + 1];
	string pkHex, skHex;

	crypto_kx_keypair(pk, sk);

	pk[crypto_kx_PUBLICKEYBYTES] =
		sk[crypto_kx_SECRETKEYBYTES] = 0;
	//	rx[crypto_kx_SESSIONKEYBYTES] =
	//	tx[crypto_kx_SESSIONKEYBYTES] = 0;

	pkHex = hexStr(pk, crypto_kx_PUBLICKEYBYTES);
	skHex = hexStr(sk, crypto_kx_SECRETKEYBYTES);

#ifdef _DEBUG
	cout << "updateLocalKeys(): public key = " << pkHex << endl;
	cout << "updateLocalKeys(): private key = " << skHex << endl;
#endif //_DEBUG

	if (update_publickey(get_my_device_id(), pkHex)) {

#ifdef _DEBUG
	cout << "updateLocalKeys(): update_publickey() returned TRUE." << endl;
#endif //_DEBUG

		memcpy(client_pk, pk, crypto_kx_PUBLICKEYBYTES + 1);
		memcpy(client_sk, sk, crypto_kx_SECRETKEYBYTES + 1);

		if (cfgRoot->exists("privateKey")) cfgRoot->remove("privateKey");
		cfgRoot->add("privateKey", Setting::TypeString) = skHex;

		cfg.writeFile(BLOCKCHAINSEC_CONFIG_F);

		return true;
	}
#ifdef _DEBUG
	cout << "updateLocalKeys(): update_publickey() returned FALSE." << endl;
#endif //_DEBUG
	return false;
}



bool BlockchainSecLib::encryptAndPushData(string const& data) {
	const uint16_t cipherLen = data.length()  + crypto_secretbox_MACBYTES;
	unsigned char nonce[crypto_secretbox_NONCEBYTES + 1];
	unsigned char *cipher{new unsigned char[cipherLen]{}};
	string cipherStr, nonceStr;

	if (derriveSharedSecret) {
		if (crypto_kx_client_session_keys(rxSharedKey, txSharedKey, client_pk, client_sk, dataReceiver_pk) != 0) {
			// dataReceiver public key is suspicious
			throw CryptographicFailureException("Suspicious cryptographic key");
		}
		derriveSharedSecret = false;
	}

	randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);
	nonce[crypto_secretbox_NONCEBYTES] = 0;
	crypto_secretbox_easy(cipher, (unsigned char*) data.c_str(), data.length(), nonce, txSharedKey);

	cipherStr = string((char*) cipher, cipherLen);
	nonceStr = string((char*) nonce, crypto_secretbox_NONCEBYTES);

	push_data(localDeviceID, cipherStr, data.length(), nonceStr);

	delete[] cipher;
	return true;
}



string BlockchainSecLib::getDataAndDecrypt(uint32_t const deviceID) {
	unsigned char rxKey[crypto_kx_SESSIONKEYBYTES + 1], txKey[crypto_kx_SESSIONKEYBYTES + 1];
	string nodePublicKeyStr;
	vector<string> chainData;
	uint16_t msgLen;

	if (get_my_device_id() != get_datareceiver(deviceID)) {
		throw CryptographicKeyMissmatchException("Cannot decrypt the requested data with current keys");
	}

	nodePublicKeyStr = get_key(deviceID);

	unsigned char nodePublicKey[crypto_kx_PUBLICKEYBYTES + 1];
	vector<char> byteVector = hexToBytes(nodePublicKeyStr);
	memcpy(nodePublicKey, byteVector.data(), crypto_kx_PUBLICKEYBYTES);
	nodePublicKey[crypto_kx_PUBLICKEYBYTES] = 0;

	if (crypto_kx_server_session_keys(rxKey, txKey, client_pk, client_sk, nodePublicKey) != 0) {
		throw CryptographicFailureException("Suspicious cryptographic key");
	}

	chainData = get_data(deviceID);
	msgLen = stoi(chainData[1]);

	unsigned char *message{new unsigned char[msgLen + 1]{}};
	message[msgLen] = 0;

	string cipherStr = base64_decode(chainData[0]);
	string nonceStr = base64_decode(chainData[2]);

	if (crypto_secretbox_open_easy(
			message,
			(unsigned char*) cipherStr.c_str(),
			cipherStr.length(),
			(unsigned char*) nonceStr.c_str(),
			rxKey
	) != 0) {
		// Message was forged
		throw CryptographicFailureException("Message forged");
	}

	string result = string((char*) message, msgLen);
	delete[] message;

	return result;
}



vector<uint32_t> BlockchainSecLib::getReceivedDevices(uint32_t deviceID) {
	vector<uint32_t> authorized, result;

	authorized = get_authorized_devices();

	for (vector<uint32_t>::iterator it = authorized.begin(); it != authorized.end(); ++it) {
		if (get_datareceiver(*it) == deviceID) result.push_back(*it);
	}
	return result;
}



} //namespace blockchain-sec
