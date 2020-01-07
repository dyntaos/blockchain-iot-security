#include <string>
#include <array>
#include <iostream>
#include <boost/algorithm/string/trim.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <blockchainsec.hpp>
#include <ethabi.hpp>
#include <misc.hpp>


//TODO: Make a function to verify ethereum address formatting! (Apply to configuration file validation)
//TODO: Make function to see if "0x" needs to be prepended to function arguments
//TODO: Make all stored addresses NOT store "0x"

using namespace std;
using namespace libconfig;
namespace blockchainSec {



string BlockchainSecLib::getClientAddress(void) {
	return this->eth_my_addr;
}



string BlockchainSecLib::getContractAddress(void) {
	return this->eth_sec_contract_addr;
}



BlockchainSecLib::BlockchainSecLib(bool compile) {

	this->eth_sec_contract_addr = eth_sec_contract_addr; //TODO Check for reasonableness
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
		cerr << "Config doesnt have contract_addr"
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

	cout << "Config contains contract_addr" << endl;
	cfg.lookupValue("contract_addr", this->eth_sec_contract_addr);

	eventLogWaitManager = new EventLogWaitManager(getClientAddress().substr(2), getContractAddress().substr(2));

	subscriptionListener = new thread(&EventLogWaitManager::ipc_subscription_listener_thread, eventLogWaitManager);
}



BlockchainSecLib::BlockchainSecLib(void) : BlockchainSecLib(false) {}



BlockchainSecLib::~BlockchainSecLib() {}



void BlockchainSecLib::add_device(string client_addr, string name, string mac, string public_key, bool gateway_managed) {
	string data, transaction_hash, transaction_receipt;
	Json transactionJsonData;

	if (name.length() > BLOCKCHAINSEC_MAX_DEV_NAME) {
		throw InvalidArgumentException("Device name exceeds maximum length of BLOCKCHAINSEC_MAX_DEV_NAME.");
	}
	//TODO: If !gateway_managed make sure clientAddr is valid

	data = ethabi("encode -l function " ETH_CONTRACT_ABI " add_device -p '" + client_addr + "' -p '" + name + "' -p '" + mac + "' -p '" + public_key + "' -p " + (gateway_managed ? "true" : "false"));

	transaction_hash = this->eth_sendTransaction(data);

	transactionJsonData = Json::parse(transaction_hash);
	auto findResult = transactionJsonData.find("result");

	if (findResult == transactionJsonData.end()) {
		throw TransactionFailedException("add_device(): Transaction hash was not present in responce to eth_sendTransaction!");
	}
	transaction_hash = findResult.value();

	try {
		transaction_receipt = this->getTransactionReceipt(transaction_hash);
	} catch(const TransactionFailedException &e) {
		//TODO: How to best handle this? Allow it to be passed up -- Append to what()?
		throw e;
	}

	auto eventLog = eventLogWaitManager->getEventLog(transaction_hash); // TODO URGENT: If the transaction fails this will hang!
	string logStr = "{ ";
	for (std::pair<std::string, std::string> kv : *eventLog.get()) {
		logStr += "\"" + kv.first + "\":\"" + kv.second + "\", ";
	}
	logStr = logStr.substr(0, logStr.length() - 2);
	logStr += " }";
	cout << "Device added successfully!"
		<< endl
		<< logStr
		<< endl;
}



string BlockchainSecLib::add_gateway(string client_addr, string name, string mac, string public_key) {
	string data;

	if (name.length() > BLOCKCHAINSEC_MAX_DEV_NAME) {
		throw InvalidArgumentException("Gateway name exceeds maximum length of BLOCKCHAINSEC_MAX_DEV_NAME.");
	}
	//TODO: If !gateway_managed make sure clientAddr is valid

	data = ethabi("encode -l function " ETH_CONTRACT_ABI " add_gateway -p '" + client_addr + "' -p '" + name + "' -p '" + mac + "' -p '" + public_key + "'");
	return this->eth_sendTransaction(data);
}



#ifdef _DEBUG
void BlockchainSecLib::test(void) {

}
#endif //_DEBUG



void BlockchainSecLib::create_contract(void) {
	string contract_bin,
			transactionJsonStr,
			transaction_hash,
			transaction_receipt,
			contract_addr;
	Json transactionJsonData, receiptJsonData;

	// TODO: When project is complete we dont need to recompile this everytime
	system("solc --bin '" ETH_CONTRACT_SOL "' | tail -n +4 > '" ETH_CONTRACT_BIN "'");
	system("solc --abi '" ETH_CONTRACT_SOL "' | tail -n +4 > '" ETH_CONTRACT_ABI "'");
	//TODO: Check for success

	contract_bin = boost::trim_copy(readFile2(ETH_CONTRACT_BIN));

	transactionJsonStr = this->eth_createContract(contract_bin);

	transactionJsonData = Json::parse(transactionJsonStr);
	auto jsonFindResult = transactionJsonData.find("result");

	if (jsonFindResult == transactionJsonData.end()) {
		// "result" not in JSON responce
		// TODO: What if "result" is not a string
		throw TransactionFailedException("create_contract(): Transaction hash was not present in responce to eth_sendTransaction!");
	}

	transaction_hash = jsonFindResult.value();

	cout << "Parsed contract creation transaction hash: " <<
		transaction_hash << endl;

	try {
		transaction_receipt = this->getTransactionReceipt(transaction_hash);
	} catch(const TransactionFailedException &e) {
		//TODO: How to best handle this?
		throw e;
	}

	receiptJsonData = Json::parse(transaction_receipt);
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

	contract_addr = subJsonFindResult.value();

	if (contract_addr.compare("null") == 0) {
		throw TransactionFailedException("create_contract(): \"contractAddress\" was null!");
	}

	cout << "Contract Address: " << contract_addr << endl;

	//TODO: Save contract to config
	if (cfg_root->exists("contract_addr")) cfg_root->remove("contract_addr");
	cfg_root->add("contract_addr", Setting::TypeString) = contract_addr;
	this->eth_sec_contract_addr = contract_addr;
	cfg.writeFile(BLOCKCHAINSEC_CONFIG_F);
}



string BlockchainSecLib::getTransactionReceipt(string transaction_hash) {
	int retries = 0;
	string transaction_receipt, result;
	Json jsonData;

	while(retries <= BLOCKCHAINSEC_GETTRANSRECEIPT_MAXRETRIES) {
		transaction_receipt = this->eth_getTransactionReceipt(transaction_hash);
		retries++;

#ifdef _DEBUG
		cout << "Try #" << retries << endl;
		cout << transaction_receipt << endl << endl;
#endif //_DEBUG

		jsonData = Json::parse(transaction_receipt);
		auto jsonFindResult = jsonData.find("result");
		if (jsonFindResult == jsonData.end()) {
			sleep(BLOCKCHAINSEC_GETTRANSRECEIPT_RETRY_DELAY);
			continue;
		} else if (jsonFindResult.value().is_null()) {
			sleep(BLOCKCHAINSEC_GETTRANSRECEIPT_RETRY_DELAY);
			continue;
		}

		return transaction_receipt;
	}

	throw TransactionFailedException(
			"Failed to obtain transaction result in getTransactionReceipt() "
			"for transaction hash \"" + transaction_hash + "\"; "
			"transaction may or may not have been mined!"
	);
}



// TODO: Should this be ported to Unix Domain Sockets?
// TODO: At the very least, replace cerr/return with exceptions
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
		throw ResourceRequestFailedException("eth_ipc_request(): Failed to popen() unix domain socket for IPC with geth! Is geth running?");
	}

	ipc_fd = fileno(ipc);

	if (fgets(ipc_buffer.data(), PIPE_BUFFER_LENGTH, ipc) == NULL) {
		throw ResourceRequestFailedException("eth_ipc_request(): Error: Failed to read from IPC!");
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
		throw ResourceRequestFailedException("eth_ipc_request(): Failed to pclose() unix domain socket for IPC with geth!");
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
									"\"gas\":\"0x14F46B\"," //TODO: WHERE THE THE BLOCK GAS LIMIT SET? Choose this value more intentionally
									"\"gasPrice\":\"0x0\","
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



void BlockchainSecLib::joinThreads(void) {
	subscriptionListener->join();
}



} //namespace blockchain-sec
