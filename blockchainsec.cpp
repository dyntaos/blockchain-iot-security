#include <array>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <sstream>
#include <string>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <blockchainsec.hpp>
#include <cpp-base64/base64.h>
#include <ethabi.hpp>
#include <misc.hpp>


//TODO: Make a function to verify ethereum address formatting! (Apply to configuration file validation)


using namespace std;
using namespace libconfig;

namespace blockchainSec
{



BlockchainSecLib::BlockchainSecLib(void)
	: BlockchainSecLib(false)
{
}



BlockchainSecLib::BlockchainSecLib(bool compile)
	: EthInterface(ETH_CONTRACT_SOL, ETH_CONTRACT_ABI, ETH_CONTRACT_BIN)
{
	std::string ipcPath;
	std::string clientAddress;
	std::string contractAddress;

	if (sodium_init() < 0)
	{
		throw CryptographicFailureException("Could not initialize libSodium!");
	}

	cfg.setOptions(Config::OptionFsync
		| Config::OptionSemicolonSeparators
		| Config::OptionColonAssignmentForGroups
		| Config::OptionOpenBraceOnSeparateLine);

	cout << "Reading config file..." << endl;

	try
	{
		cfg.readFile(BLOCKCHAINSEC_CONFIG_F);
	}
	catch (const FileIOException& e)
	{
		cerr << "IO error reading config file!" << endl;
		exit(EXIT_FAILURE);
	}
	catch (const ParseException& e)
	{
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

	if (!cfg.exists("ipcPath"))
	{
		cerr << "Configuration file does not contain 'ipcPath'!" << endl;
		exit(EXIT_FAILURE);
	}

	cfg.lookupValue("ipcPath", ipcPath); // TODO Check return value

	if (!cfg.exists("clientAddress"))
	{
		cerr << "Configuration file does not contain 'clientAddress'!" << endl;
		exit(EXIT_FAILURE);
	}

	cfg.lookupValue("clientAddress", clientAddress); // TODO Check return value

	if (!compile && !cfg.exists("contractAddress"))
	{
		cerr << "Config doesnt have contractAddress"
			 << endl
			 << "Either add a contract address to the conf file or "
				"create a new contract using the `--compile` flag"
			 << endl;

		exit(EXIT_FAILURE);
	}

	cout << "Config contains contractAddress" << endl;
	cfg.lookupValue("contractAddress", contractAddress); // TODO Check return value

	initialize(ipcPath, clientAddress, contractAddress, contractEventSignatures());

	if (compile)
	{
		/* We will compile the contract with solc, upload it
		 * to the chain and save the address to the config file */
		cout << "Compiling and uploading contract..."
			 << endl
			 << endl;

		try
		{
			contractAddress = this->create_contract();

			if (cfgRoot->exists("contractAddress"))
				cfgRoot->remove("contractAddress");
			cfgRoot->add("contractAddress", Setting::TypeString) = contractAddress;
			cfg.writeFile(BLOCKCHAINSEC_CONFIG_F);

			cout << "Successfully compiled and uploaded contract"
				 << endl
				 << "The new contract address has been "
					"written to the configuration file"
				 << endl;

			exit(EXIT_SUCCESS);
		}
		catch (const TransactionFailedException& e)
		{
			throw e; // TODO
			cerr << "Failed to create contract!" << endl;
			exit(EXIT_FAILURE);
		}
	}

	loadLocalDeviceParameters();
}



vector<pair<string, string>>
BlockchainSecLib::contractEventSignatures(void)
{
	vector<pair<string, string>> vecLogSigs;
	vecLogSigs.push_back(pair<string, string>("Add_Device", "9ff928f5be58747f4db3b34d53225d1e1cf1562a3158bd45c2934fca5907f919"));
	vecLogSigs.push_back(pair<string, string>("Remove_Device", "c3d811754f31d6181381ab5fbf732898911891abe7d32e97de73a1ea84c2e363"));
	vecLogSigs.push_back(pair<string, string>("Remove_Gateway", "0d014d0489a2ad2061dbf1dffe20d304792998e0635b29eda36a724992b6e5c9"));
	vecLogSigs.push_back(pair<string, string>("Push_Data", "bba4d289b156cad6df20a164dc91021ab64d1c7d594ddd9128fca71d6366b3c9"));
	vecLogSigs.push_back(pair<string, string>("Update_DataReceiver", "e21f6cd2771fa3b4f5641e2fd1a3d52156a9a8cc10da311d5de41a5755ca6acf"));
	vecLogSigs.push_back(pair<string, string>("Set_Default_DataReceiver", "adf201dc3ee5a3915c67bf861b4c0ec432dded7b6a82938956e1f411c5636287"));
	vecLogSigs.push_back(pair<string, string>("Update_Addr", "f873df4dfb480f3a05c2bde3cae4779f61d6b60c3f6a0f1ab57aac0fe021a686"));
	vecLogSigs.push_back(pair<string, string>("Update_PublicKey", "9f99e7c31d775c4f75816a8e1a0655e1e5f5bab88311d820d261ebab2ae8d91f"));
	vecLogSigs.push_back(pair<string, string>("Update_SignKey", "7abf29af9412d237982695e733aed1af1c9ba2c4df30a4a18b76f478c7ace921"));
	vecLogSigs.push_back(pair<string, string>("Authorize_Admin", "134c4a950d896d7c32faa850baf4e3bccf293ae2538943709726e9596ce9ebaf"));
	vecLogSigs.push_back(pair<string, string>("Deauthorize_Admin", "e96008d87980c624fca6a2c0ecc59bcef2ef54659e80a1333aff845ea113f160"));
	return vecLogSigs;
}



void
BlockchainSecLib::loadLocalDeviceParameters(void)
{
	try
	{
		localDeviceID = get_my_device_id();
	}
	catch (DeviceNotAssignedException& e)
	{
		localDeviceID = 0; // DeviceID of 0 means DeviceID is no assigned
	}

	if (localDeviceID != 0)
	{
		// This client has a device ID -- verify it has proper keys
		string publicKey;
		string signPublicKey;

		try
		{
			publicKey = boost::trim_copy(get_key(localDeviceID)); // TODO: What makes a valid public key?
		}
		catch (...)
		{ // TODO
			cerr << "This device does not have a public key on the blockchain..." << endl;
			publicKey = "";
		}

		try
		{
			signPublicKey = boost::trim_copy(get_signkey(localDeviceID)); // TODO: What makes a valid sign public key?
		}
		catch (...)
		{ // TODO
			cerr << "This device does not have a signature public key on the blockchain..." << endl;
			signPublicKey = "";
		}


		if (cfgRoot->exists("privateKey") && publicKey == "")
		{
			cerr << "Client has private key, but no public "
					"key exists on the blockchain for device ID "
				 << localDeviceID << endl;
			// TODO: Throw?
		}

		if (!cfgRoot->exists("privateKey") && publicKey != "")
		{
			cerr << "Client has public key on the blockchain, "
					"but no private key exists locally!"
				 << endl;
			// TODO: Throw?
		}


		if (cfgRoot->exists("signPrivateKey") && signPublicKey == "")
		{
			cerr << "Client has signature private key, but no signature "
					"public key exists on the blockchain for device ID "
				 << localDeviceID << endl;
			// TODO: Throw?
		}

		if (!cfgRoot->exists("signPrivateKey") && signPublicKey != "")
		{
			cerr << "Client has signature public key on the blockchain, "
					"but no signature private key exists locally!"
				 << endl;
			// TODO: Throw?
		}


		if (
			!cfgRoot->exists("privateKey") && publicKey == "" &&
			!cfgRoot->exists("signPrivateKey") && signPublicKey == "")
		{
			if (updateLocalKeys())
			{
				cout << "Successfully generated keypairs for local client (device ID "
					 << localDeviceID << ")..." << endl;
			}
			else
			{
				cerr << "Failed to create keys for local clients (device ID "
					 << localDeviceID << ")..." << endl;
			}
		}
		else
		{
			if (cfgRoot->exists("privateKey") && publicKey != "")
			{
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

			if (cfgRoot->exists("signPrivateKey") && signPublicKey != "")
			{
				string hexSk;
				vector<char> byteVector;

				cfg.lookupValue("signPrivateKey", hexSk); // TODO Check return value
				byteVector = hexToBytes(hexSk);
				memcpy(this->signPrivateKey, byteVector.data(), crypto_sign_SECRETKEYBYTES);
				signPrivateKey[crypto_sign_SECRETKEYBYTES] = 0;
				cout << "Loaded signature private key..." << endl;

				byteVector = hexToBytes(signPublicKey);
				memcpy(this->signPublicKey, byteVector.data(), crypto_sign_PUBLICKEYBYTES);
				signPublicKey[crypto_sign_PUBLICKEYBYTES] = 0;
				cout << "Loaded signature public key..." << endl;
			}
		}
	}
	loadDataReceiverPublicKey(localDeviceID);
}



bool
BlockchainSecLib::loadDataReceiverPublicKey(uint32_t deviceID)
{
	uint32_t dataReceiver;
	string dataReceiverPublicKey;
	vector<char> byteVector;

	try
	{
		dataReceiver = get_datareceiver(deviceID);
		if (dataReceiver == 0)
		{
			return false;
		}
		dataReceiverPublicKey = get_key(dataReceiver);
	}
	catch (EthException& e)
	{
		return false;
	}
	if (boost::trim_copy(dataReceiverPublicKey) == "")
		return false;

	byteVector = hexToBytes(dataReceiverPublicKey);
	memcpy(dataReceiver_pk, byteVector.data(), crypto_kx_PUBLICKEYBYTES);
	dataReceiver_pk[crypto_kx_PUBLICKEYBYTES] = 0;

	return true;
}



// Throws ResourceRequestFailedException from ethabi()
string
BlockchainSecLib::getFromDeviceID(string const& funcName, uint32_t deviceID)
{
	return getFrom(funcName, " -p '" + boost::lexical_cast<string>(deviceID) + "'");
}



// Throws ResourceRequestFailedException from ethabi()
uint64_t
BlockchainSecLib::getIntFromDeviceID(string const& funcName, uint32_t deviceID)
{
	string value;

	value = getFromDeviceID(funcName, deviceID);
	return stoul(value, nullptr, 16);
}



// Throws ResourceRequestFailedException from ethabi()
string
BlockchainSecLib::getStringFromDeviceID(string const& funcName, uint32_t deviceID)
{
	return ethabi_decode_result(
		getEthContractABI(),
		funcName,
		getFromDeviceID(funcName, deviceID));
}



// Throws ResourceRequestFailedException from ethabi()
vector<string>
BlockchainSecLib::getStringsFromDeviceID(string const& funcName, uint32_t deviceID)
{
	return ethabi_decode_results(
		getEthContractABI(),
		funcName,
		getFromDeviceID(funcName, deviceID));
}



// Throws ResourceRequestFailedException from ethabi()
// Throws InvalidArgumentException
bool
BlockchainSecLib::is_admin(string const& isAdminAddress)
{
	if (!isEthereumAddress(isAdminAddress))
	{
		throw InvalidArgumentException("Address is an invalid format!");
	}
	return stoi(getFrom("is_admin", " -p '" + isAdminAddress + "'")) == 1;
}



// Throws ResourceRequestFailedException from ethabi()
bool
BlockchainSecLib::is_authd(uint32_t deviceID)
{
	return getIntFromDeviceID("is_authd", deviceID) != 0;
}



// Throws ResourceRequestFailedException from ethabi()
bool
BlockchainSecLib::is_device(uint32_t deviceID)
{
	return getIntFromDeviceID("is_device", deviceID) != 0;
}



// Throws ResourceRequestFailedException from ethabi()
bool
BlockchainSecLib::is_gateway(uint32_t deviceID)
{
	return getIntFromDeviceID("is_gateway", deviceID) != 0;
}



// Throws ResourceRequestFailedException from ethabi()
bool
BlockchainSecLib::is_gateway_managed(uint32_t deviceID)
{
	return getIntFromDeviceID("is_gateway_managed", deviceID) != 0;
}



// Throws ResourceRequestFailedException from ethabi()
uint32_t
BlockchainSecLib::get_my_device_id(void)
{
	uint32_t deviceId;
	try
	{
		deviceId = getIntFromContract("get_my_device_id");
	}
	catch (CallFailedException & e)
	{
		deviceId = 0;
	}
	if (deviceId == 0)
	{
		throw DeviceNotAssignedException(
			"This client has no device ID assigned");
	}
	return getIntFromContract("get_my_device_id");
}



// Throws ResourceRequestFailedException from ethabi()
uint32_t
BlockchainSecLib::get_datareceiver(uint32_t deviceID)
{
	return getIntFromDeviceID("get_datareceiver", deviceID);
}



// Throws ResourceRequestFailedException from ethabi()
uint32_t
BlockchainSecLib::get_default_datareceiver()
{
	return getIntFromContract("get_default_datareceiver");
}



// Throws ResourceRequestFailedException from ethabi()
string
BlockchainSecLib::get_key(uint32_t deviceID)
{
	return getStringFromDeviceID("get_key", deviceID);
}



// Throws ResourceRequestFailedException from ethabi()
string
BlockchainSecLib::get_signkey(uint32_t deviceID)
{
	return getStringFromDeviceID("get_signkey", deviceID);
}



// Throws ResourceRequestFailedException from ethabi()
BlockchainSecLib::AddrType
BlockchainSecLib::get_addrtype(uint32_t deviceID)
{
	return BlockchainSecLib::AddrType(getIntFromDeviceID("get_addrType", deviceID));
}



// Throws ResourceRequestFailedException from ethabi()
string
BlockchainSecLib::get_addr(uint32_t deviceID)
{
	return getStringFromDeviceID("get_addr", deviceID);
}



// Throws ResourceRequestFailedException from ethabi()
string
BlockchainSecLib::get_name(uint32_t deviceID)
{
	return getStringFromDeviceID("get_name", deviceID);
}



// Throws ResourceRequestFailedException from ethabi()
vector<string>
BlockchainSecLib::get_data(uint32_t deviceID)
{
	vector<string> result = getStringsFromDeviceID("get_data", deviceID);
	result[1] = to_string(strtol(result[1].c_str(), nullptr, 16));

	return result;
}



// Throws ResourceRequestFailedException from ethabi()
time_t
BlockchainSecLib::get_dataTimestamp(uint32_t deviceID)
{
	return getIntFromDeviceID("get_dataTimestamp", deviceID);
}



// Throws ResourceRequestFailedException from ethabi()
time_t
BlockchainSecLib::get_creationTimestamp(uint32_t deviceID)
{
	return getIntFromDeviceID("get_creationTimestamp", deviceID);
}



// Throws ResourceRequestFailedException from ethabi()
uint32_t
BlockchainSecLib::get_num_admin(void)
{
	return getIntFromContract("get_num_admin");
}



// Throws ResourceRequestFailedException from ethabi()
uint32_t
BlockchainSecLib::get_num_devices(void)
{
	return getIntFromContract("get_num_devices");
}



// Throws ResourceRequestFailedException from ethabi()
uint32_t
BlockchainSecLib::get_num_gateways(void)
{
	return getIntFromContract("get_num_gateways");
}



// Throws ResourceRequestFailedException from ethabi()
vector<string>
BlockchainSecLib::get_active_admins(void)
{
	return ethabi_decode_results(
		getEthContractABI(),
		"get_active_admins",
		getArrayFromContract("get_active_admins"));
}



// Throws ResourceRequestFailedException from ethabi()
vector<uint32_t>
BlockchainSecLib::get_authorized_devices(void)
{
	return ethabi_decode_uint32_array(
		getEthContractABI(),
		"get_authorized_devices",
		getArrayFromContract("get_authorized_devices"));
}



// Throws ResourceRequestFailedException from ethabi()
vector<uint32_t>
BlockchainSecLib::get_authorized_gateways(void)
{
	return ethabi_decode_uint32_array(
		getEthContractABI(),
		"get_authorized_gateways",
		getArrayFromContract("get_authorized_gateways"));
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
uint32_t
BlockchainSecLib::add_device(
	string const& deviceAddress,
	string const& name,
	bool gatewayManaged)
{
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	if (!isEthereumAddress(deviceAddress))
	{
		throw InvalidArgumentException("Address is an invalid format!");
	}

	if (name.length() > BLOCKCHAINSEC_MAX_DEV_NAME)
	{
		throw InvalidArgumentException(
			"Device name exceeds maximum length of BLOCKCHAINSEC_MAX_DEV_NAME.");
	}

	ethabiEncodeArgs = " -p '" + deviceAddress + "' -p '" + escapeSingleQuotes(name) + "'  -p " + (gatewayManaged ? "true" : "false") + " -p false";

	if (callMutatorContract("add_device", ethabiEncodeArgs, eventLog))
	{
		uint32_t deviceId = stoul((*eventLog.get())["device_id"], NULL, 16); // TODO: What id [device_id] doesn't exist? try catch
		if (deviceId == get_my_device_id())
		{
			loadLocalDeviceParameters();
		}
		return deviceId;
	}
	else
	{
		return 0;
	}
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
uint32_t
BlockchainSecLib::add_gateway(string const& gatewayAddress, string const& name)
{
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	if (name.length() > BLOCKCHAINSEC_MAX_DEV_NAME)
	{
		throw InvalidArgumentException(
			"Device name exceeds maximum length of BLOCKCHAINSEC_MAX_DEV_NAME.");
	}

	ethabiEncodeArgs = " -p '" + gatewayAddress + "' -p '" + escapeSingleQuotes(name) + "'  -p false -p true";

	if (callMutatorContract("add_device", ethabiEncodeArgs, eventLog))
	{
		uint32_t deviceId = stoul((*eventLog.get())["device_id"], NULL, 16); // TODO: What id [device_id] doesn't exist? try catch
		if (deviceId == get_my_device_id())
		{
			loadLocalDeviceParameters();
		}
		return deviceId;
	}
	else
	{
		return 0;
	}
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool
BlockchainSecLib::remove_device(uint32_t deviceID)
{
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(deviceID) + "'";

	return callMutatorContract("remove_device", ethabiEncodeArgs, eventLog);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool
BlockchainSecLib::remove_gateway(uint32_t deviceID)
{
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(deviceID) + "'";

	return callMutatorContract("remove_gateway", ethabiEncodeArgs, eventLog);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool
BlockchainSecLib::update_datareceiver(uint32_t deviceID, uint32_t dataReceiverID)
{
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(deviceID) + "' -p '" + boost::lexical_cast<string>(dataReceiverID) + "'";

	return callMutatorContract("update_datareceiver", ethabiEncodeArgs, eventLog);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool
BlockchainSecLib::set_default_datareceiver(uint32_t dataReceiverID)
{
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(dataReceiverID) + "'";

	return callMutatorContract("set_default_datareceiver", ethabiEncodeArgs, eventLog);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool
BlockchainSecLib::update_addr(
	uint32_t deviceID,
	BlockchainSecLib::AddrType addrType,
	string const& addr)
{
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(deviceID) + "' -p '" + boost::lexical_cast<string>(addrType) + "' -p '" + escapeSingleQuotes(addr) + "'";

	return callMutatorContract("update_addr", ethabiEncodeArgs, eventLog);
}



bool
BlockchainSecLib::update_publickey(uint32_t deviceID, string const& publicKey)
{
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(deviceID) + "' -p '" + escapeSingleQuotes(publicKey) + "'";

	return callMutatorContract("update_publickey", ethabiEncodeArgs, eventLog);
}



bool
BlockchainSecLib::update_signkey(uint32_t deviceID, string const& signPublicKey)
{
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(deviceID) + "' -p '" + escapeSingleQuotes(signPublicKey) + "'";

	return callMutatorContract("update_signkey", ethabiEncodeArgs, eventLog);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool
BlockchainSecLib::push_data(
	uint32_t deviceID,
	string& data,
	uint16_t dataLen,
	string& nonce)
{
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	string dataAscii = base64_encode((unsigned char*)data.c_str(), data.length());
	string nonceAscii = base64_encode((unsigned char*)nonce.c_str(), nonce.length());

	ethabiEncodeArgs = " -p '" + boost::lexical_cast<string>(deviceID) + "' -p '";
	ethabiEncodeArgs += dataAscii;
	ethabiEncodeArgs += "' -p '" + boost::lexical_cast<string>(dataLen) + "' -p '";
	ethabiEncodeArgs += nonceAscii;
	ethabiEncodeArgs += "'";

	return callMutatorContract("push_data", ethabiEncodeArgs, eventLog);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool
BlockchainSecLib::authorize_admin(string const& adminAddress)
{
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	if (!isEthereumAddress(adminAddress))
	{
		throw InvalidArgumentException("Address is an invalid format!");
	}
	ethabiEncodeArgs = " -p '" + adminAddress + "'";

	return callMutatorContract("authorize_admin", ethabiEncodeArgs, eventLog);
}



// Throws ResourceRequestFailedException from ethabi()
// Throws TransactionFailedException from eth_sendTransaction()
bool
BlockchainSecLib::deauthorize_admin(string const& adminAddress)
{
	string ethabiEncodeArgs;
	unique_ptr<unordered_map<string, string>> eventLog;

	if (!isEthereumAddress(adminAddress))
	{
		throw InvalidArgumentException("Address is an invalid format!");
	}
	ethabiEncodeArgs = " -p '" + adminAddress + "'";

	return callMutatorContract("deauthorize_admin", ethabiEncodeArgs, eventLog);
}



bool
BlockchainSecLib::updateLocalKeys(void) // TODO: Generate and update signature keys
{
	unsigned char pk[crypto_kx_PUBLICKEYBYTES + 1],
		sk[crypto_kx_SECRETKEYBYTES + 1];
	unsigned char signPublicKey[crypto_sign_PUBLICKEYBYTES + 1],
		signPrivateKey[crypto_sign_SECRETKEYBYTES + 1];

	string pkHex, skHex;
	string signPublicKeyHex, signPrivateKeyHex;

	bool updatePublicResult, updateSignResult;

	derriveSharedSecret = true;
	crypto_kx_keypair(pk, sk);
	crypto_sign_keypair(signPublicKey, signPrivateKey);

	pk[crypto_kx_PUBLICKEYBYTES] = sk[crypto_kx_SECRETKEYBYTES] = 0;
	signPublicKey[crypto_sign_PUBLICKEYBYTES] = signPrivateKey[crypto_sign_SECRETKEYBYTES] = 0;

	pkHex = hexStr(pk, crypto_kx_PUBLICKEYBYTES);
	skHex = hexStr(sk, crypto_kx_SECRETKEYBYTES);
	signPublicKeyHex = hexStr(signPublicKey, crypto_sign_PUBLICKEYBYTES);
	signPrivateKeyHex = hexStr(signPrivateKey, crypto_sign_SECRETKEYBYTES);


#ifdef _DEBUG
	cout << "updateLocalKeys(): public key = " << pkHex << endl;
	cout << "updateLocalKeys(): private key = " << skHex << endl;
	cout << "updateLocalKeys(): signature public key = " << signPublicKeyHex << endl;
	cout << "updateLocalKeys(): signature private key = " << signPrivateKeyHex << endl;
#endif //_DEBUG

	updatePublicResult = update_publickey(get_my_device_id(), pkHex); // TODO: TRY CATCH!!!!!
	updateSignResult = update_signkey(get_my_device_id(), signPublicKeyHex);

	if (updatePublicResult)
	{

#ifdef _DEBUG
		cout << "updateLocalKeys(): update_publickey() returned TRUE" << endl;
#endif //_DEBUG

		memcpy(client_pk, pk, crypto_kx_PUBLICKEYBYTES + 1);
		memcpy(client_sk, sk, crypto_kx_SECRETKEYBYTES + 1);

		if (cfgRoot->exists("publicKey"))
			cfgRoot->remove("publicKey");
		cfgRoot->add("publicKey", Setting::TypeString) = pkHex;

		if (cfgRoot->exists("privateKey"))
			cfgRoot->remove("privateKey");
		cfgRoot->add("privateKey", Setting::TypeString) = skHex;

		cfg.writeFile(BLOCKCHAINSEC_CONFIG_F);
	}
#ifdef _DEBUG
	else
	{
		cout << "updateLocalKeys(): update_publicslkey() returned FALSE" << endl;
	}
#endif //_DEBUG

	if (updateSignResult)
	{

#ifdef _DEBUG
		cout << "updateLocalKeys(): update_signkey() returned TRUE" << endl;
#endif //_DEBUG

		memcpy(this->signPublicKey, signPublicKey, crypto_sign_PUBLICKEYBYTES + 1);
		memcpy(this->signPrivateKey, signPrivateKey, crypto_sign_SECRETKEYBYTES + 1);

		if (cfgRoot->exists("signPublicKey"))
			cfgRoot->remove("signPublicKey");
		cfgRoot->add("signPublicKey", Setting::TypeString) = signPublicKeyHex;

		if (cfgRoot->exists("signPrivateKey"))
			cfgRoot->remove("signPrivateKey");
		cfgRoot->add("signPrivateKey", Setting::TypeString) = signPrivateKeyHex;

		cfg.writeFile(BLOCKCHAINSEC_CONFIG_F);
	}
#ifdef _DEBUG
	else
	{
		cout << "updateLocalKeys(): update_signkey() returned FALSE" << endl;
	}
#endif //_DEBUG

	if (updatePublicResult && updateSignResult)
	{
		return true;
	}

#ifdef _DEBUG
	cout << "updateLocalKeys(): update_publickey() returned FALSE" << endl;
#endif //_DEBUG

	return false;
}



bool
BlockchainSecLib::encryptAndPushData(string const& data)
{
	const uint16_t cipherLen = data.length();
	unsigned char nonce[crypto_secretbox_NONCEBYTES];
	unsigned char* cipher{ new unsigned char[cipherLen]{} };
	string cipherStr, nonceStr;

	if (derriveSharedSecret)
	{
		if (crypto_kx_client_session_keys(rxSharedKey, txSharedKey, client_pk, client_sk, dataReceiver_pk) != 0)
		{
			// dataReceiver public key is suspicious
			throw CryptographicFailureException("Suspicious cryptographic key");
		}
		derriveSharedSecret = false;
	}

	randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);

	crypto_stream_xchacha20_xor( // TODO: Check return value?
		cipher,
		(unsigned char*)data.c_str(),
		data.length(),
		nonce,
		txSharedKey);

	cipherStr = string((char*)cipher, cipherLen);
	nonceStr = string((char*)nonce, crypto_secretbox_NONCEBYTES);

	push_data(localDeviceID, cipherStr, data.length(), nonceStr);

	delete[] cipher;
	return true;
}



string
BlockchainSecLib::getDataAndDecrypt(uint32_t const deviceID)
{
	unsigned char rxKey[crypto_kx_SESSIONKEYBYTES + 1],
		txKey[crypto_kx_SESSIONKEYBYTES + 1];
	string nodePublicKeyStr;
	vector<string> chainData;
	uint16_t msgLen;

	if (get_my_device_id() != get_datareceiver(deviceID))
	{
		throw CryptographicKeyMissmatchException(
			"Cannot decrypt the requested data with current keys");
	}

	nodePublicKeyStr = get_key(deviceID);

	unsigned char nodePublicKey[crypto_kx_PUBLICKEYBYTES + 1];
	vector<char> byteVector = hexToBytes(nodePublicKeyStr);

	memcpy(nodePublicKey, byteVector.data(), crypto_kx_PUBLICKEYBYTES);
	nodePublicKey[crypto_kx_PUBLICKEYBYTES] = 0;

	if (crypto_kx_server_session_keys(rxKey, txKey, client_pk, client_sk, nodePublicKey) != 0)
	{
		throw CryptographicFailureException("Suspicious cryptographic key");
	}

	chainData = get_data(deviceID);
	msgLen = stoi(chainData[1]);

	unsigned char* message{ new unsigned char[msgLen]{} };

	string cipherStr = base64_decode(chainData[0]);
	string nonceStr = base64_decode(chainData[2]);

	crypto_stream_xchacha20_xor( // TODO: Check return value
		message,
		(unsigned char*)cipherStr.c_str(),
		msgLen,
		(unsigned char*)nonceStr.c_str(),
		rxKey);

	string result = string((char*)message, msgLen);
	delete[] message;

	return result;
}



vector<uint32_t>
BlockchainSecLib::getReceivedDevices(uint32_t deviceID)
{
	vector<uint32_t> authorized, result;

	authorized = get_authorized_devices(); // TODO

	for (vector<uint32_t>::iterator it = authorized.begin(); it != authorized.end(); ++it)
	{
		if (get_datareceiver(*it) == deviceID)
			result.push_back(*it);
	}
	return result;
}



} //namespace
