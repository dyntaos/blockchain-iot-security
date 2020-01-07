#include <string>
#include <array>
#include <iostream>
#include <boost/asio.hpp>
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
}



BlockchainSecLib::BlockchainSecLib(void) : BlockchainSecLib(false) {}



BlockchainSecLib::~BlockchainSecLib() {}



string BlockchainSecLib::add_device(string client_addr, string name, string mac, string public_key, bool gateway_managed) {
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

	//TODO: Confirm success from logs
	return "";
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



void BlockchainSecLib::ipc_subscription_listener_thread(BlockchainSecLib &lib) {
//void BlockchainSecLib::ipc_subscription_listener_thread(void) {
	const string events[] = {
		"Add_Device",
		"Add_Gateway",
		"Remove_Device",
		"Remove_Gateway",
		"Push_Data",
		"Update_Addr",
		"Authorize_Admin",
		"Deauthorize_Admin"
	};
	const string signatures[] = {
		"6d8b1348ac9490868dcf5de10f66764c3fd6abf3274c6d36fbf877fc9f6e798f",
		"3288eeac8a88a024bc145e835dbfecbe5095e00c717c48986f19943367e9fa20",
		"c3d811754f31d6181381ab5fbf732898911891abe7d32e97de73a1ea84c2e363",
		"0d014d0489a2ad2061dbf1dffe20d304792998e0635b29eda36a724992b6e5c9",
		"0924baadbe7a09acb87f9108bb215dea5664035966d186b4fa71905d11fe1b51",
		"8489be1d551a279fae5e4ed28b2a0aab728d48550f6a64375f627ac809ac2a80",
		"134c4a950d896d7c32faa850baf4e3bccf293ae2538943709726e9596ce9ebaf",
		"e96008d87980c624fca6a2c0ecc59bcef2ef54659e80a1333aff845ea113f160"
	};
	char recvbuff[4097];
	int recvlen;

	Json jsonData;
	string data;

	map<string, string> subscriptionToEventName;

	boost::asio::io_service io_service;
	boost::asio::local::stream_protocol::endpoint ep("/home/kale/.ethereum/geth.ipc");
	boost::asio::local::stream_protocol::socket socket(io_service);

#ifdef _DEBUG
	cout << "ipc_subscription_listener_thread()" << endl;
#endif //_DEBUG

	//TODO: Setup separate subscriptions for each log and store subscription/event names
restart:
	socket.connect(ep);

	for (uint16_t i = 0; i < 8; i++) {
		data = "{\"id\":1,"
					"\"method\":\"eth_subscribe\","
					"\"params\":["
						"\"logs\",{"
							"\"address\":\"" + lib.getContractAddress() + "\","
							"\"topics\":[\"" +
								"0x" + signatures[i] + "\"," +
								"\"0x000000000000000000000000" + lib.getClientAddress().substr(2) +
							"\"]"
						"}"
					"]"
				"}";

		socket.send(boost::asio::buffer(data.c_str(), data.length()));

		recvlen = socket.receive(boost::asio::buffer(recvbuff, 4096)); // TODO: What if the entire message is not received...
		recvbuff[recvlen] = 0;

		// TODO URGENT: What if a subscription comes before this loop completes?!
		// TODO: Should this be in a try catch? What to do if fails?
		Json responce = Json::parse(recvbuff);
		if (responce.count("error") > 0) {
			throw ResourceRequestFailedException("ipc_subscription_listener_thread(): Got an error responce to eth_subscribe!");
		}
		if (!responce["result"].is_string()) {
			throw ResourceRequestFailedException("ipc_subscription_listener_thread(): Unexpected responce to eth_subscribe received!");
		}
		string result = responce["result"];

		subscriptionToEventName[result] = events[i];
	}

	// TODO: What if the connection is closed by the other end?
	for (;;) {
		recvlen = socket.receive(boost::asio::buffer(recvbuff, 4096)); // TODO: What if the entire message is not received...
		recvbuff[recvlen] = 0;

		string method, subscription, data, logIndex;
		vector<string> topics;
		Json resultJsonObject;

		try {
			jsonData = Json::parse(recvbuff); //TODO: Check for error? Try Catch?

			method = jsonData["method"];
			subscription = jsonData["params"]["subscription"];
			resultJsonObject = jsonData["params"]["result"];
			for (Json::iterator iter = resultJsonObject["topics"].begin(); iter != resultJsonObject["topics"].end(); ++iter) {
				string iterStr = *iter;
				iterStr = iterStr.substr(2);
				topics.push_back(iterStr);
			}
			data = resultJsonObject["data"];
			logIndex = resultJsonObject["logIndex"];

		} catch (const Json::exception &e) {
			//TODO: More granularity?
			cerr << "ipc_subscription_listener_thread(): JSON responce error in responce:"
				<< endl
				<< "\t"
				<< recvbuff
				<< endl
				<< e.what()
				<< endl;
			continue;
		}

		if (method.compare("eth_subscription") != 0) {
			// "method" field of the JSON data is not "eth_subscription"
			cerr << "ipc_subscription_listener_thread(): \"method\" field of JSON message is \"eth_subscription\"!"
				<< endl
				<< "\t"
				<< recvbuff
				<< endl
				<< endl;
			continue;
		}

		if (subscriptionToEventName.count(subscription) <= 0) {
			//TODO: The subscription does not exist! Something is out of sync! -- Unsubscribe and start again?
			cerr << "ipc_subscription_listener_thread(): Received a subscription hash that does not exist internally!"
				<< endl
				<< "\t"
				<< recvbuff
				<< endl
				<< endl;

			socket.close();
			subscriptionToEventName.clear();
			goto restart; // TODO: Refactor to remove this
		}

		map<string, string> log = ethabi_decode_log(ETH_CONTRACT_ABI, subscriptionToEventName[subscription], topics, data.substr(2));

		eventLogMapMutex.lock();

		eventLogMap[subscription] = log;

		eventLogMapMutex.unlock();

		cout << "\t"
			<< "Event log received:"
			<< endl
			<< "[\"" << subscription
			<< "\" (AKA \""
			<< subscriptionToEventName[subscription]
			<< "\")] = "
			<< eventLogMap[subscription].dump()
			<< endl << endl;

	}
	socket.close();
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



} //namespace blockchain-sec
