#ifndef __BLOCKCHAINSEC_HPP
#define __BLOCKCHAINSEC_HPP

#include <thread>
#include <mutex>
#include <libconfig.h++>
#include <sodium.h>
#include "json/include/nlohmann/json.hpp"

#include <subscription_server.hpp>
#include <blockchainsec_except.hpp>

#define BLOCKCHAINSEC_CONFIG_F						"blockchainsec.conf"
#define BLOCKCHAINSEC_MAX_DEV_NAME					32
#define BLOCKCHAINSEC_GETTRANSRECEIPT_MAXRETRIES	25
#define BLOCKCHAINSEC_GETTRANSRECEIPT_RETRY_DELAY	1


#define ETH_CONTRACT_SOL							"dev_mgmt_contract.sol"
#define ETH_CONTRACT_BIN							"dev_mgmt_contract.bin"
#define ETH_CONTRACT_ABI							"dev_mgmt_contract.abi"

#define IPC_BUFFER_LENGTH							128

#define ETH_DEFAULT_GAS								"0x7A120"


using Json = nlohmann::json;

namespace blockchainSec {



class BlockchainSecLib {
	public:
		enum AddrType_e {UNSET, IPV4, IPV6, LORA, ZIGBEE, BLUETOOTH, OTHER};
		typedef AddrType_e AddrType;

		BlockchainSecLib(bool compile);
		BlockchainSecLib(void);
		~BlockchainSecLib();

		std::string getClientAddress(void);
		std::string getContractAddress(void);

		// Blockchain boolean methods
		bool is_admin(std::string const& isAdminAddress);
		bool is_authd(uint32_t deviceID);

		// Blockchain accessor methods
		bool is_device(uint32_t deviceID);
		bool is_gateway(uint32_t deviceID);
		uint32_t get_my_device_id(void);
		uint32_t get_datareceiver(uint32_t deviceID);
		uint32_t get_default_datareceiver();
		std::string get_key(uint32_t deviceID);
		AddrType get_addrtype(uint32_t deviceID);
		std::string get_addr(uint32_t deviceID);
		std::string get_name(uint32_t deviceID);
		std::string get_mac(uint32_t deviceID);
		std::vector<std::string> get_data(uint32_t deviceID);
		time_t get_dataTimestamp(uint32_t deviceID);
		time_t get_creationTimestamp(uint32_t deviceID);
		uint32_t get_num_admin(void);
		uint32_t get_num_devices(void);
		uint32_t get_num_gateways(void);
		std::vector<std::string> get_active_admins(void);
		std::vector<std::string> get_authorized_devices(void);
		std::vector<std::string> get_authorized_gateways(void);

		// Blockchain mutator methods
		uint32_t add_device(std::string const& deviceAddress, std::string const& name, std::string const& mac, bool gatewayManaged);
		uint32_t add_gateway(std::string const& gatewayAddress, std::string const& name, std::string const& mac);
		bool remove_device(uint32_t deviceID);
		bool remove_gateway(uint32_t deviceID);
		bool update_datareceiver(uint32_t deviceID, uint32_t dataReceiverID);
		bool set_default_datareceiver(uint32_t dataReceiverID);
		bool update_addr(uint32_t deviceID, AddrType addrType, std::string const& addr);
		bool push_data(uint32_t deviceID, std::string & data, uint16_t dataLen, std::string & nonce);
		bool authorize_admin(std::string const& adminAddr);
		bool deauthorize_admin(std::string const& adminAddr);

		bool updateLocalKeys(void);
		void loadLocalDeviceParameters(void);
		bool loadDataReceiverPublicKey(uint32_t deviceID);
		bool encryptAndPushData(std::string const& data);
		std::string getDataAndDecrypt(uint32_t const deviceID);

		void joinThreads(void);

#ifdef _DEBUG
		void test(void);
#endif //_DEBUG

	private:
		std::string ipcPath;
		std::string clientAddress;
		std::string contractAddress;
		uint32_t localDeviceID;

		libconfig::Config cfg;
		libconfig::Setting *cfgRoot;

		bool derriveSharedSecret = true; // Only recalculate the shared key if the keys have changed
		unsigned char dataReceiver_pk[crypto_kx_PUBLICKEYBYTES + 1];
		unsigned char client_pk[crypto_kx_PUBLICKEYBYTES + 1], client_sk[crypto_kx_SECRETKEYBYTES + 1];
		unsigned char rxSharedKey[crypto_kx_SESSIONKEYBYTES + 1], txSharedKey[crypto_kx_SESSIONKEYBYTES + 1];

		std::thread *subscriptionListener;
		EventLogWaitManager *eventLogWaitManager;

		std::string getFrom(std::string const& funcName, std::string const& ethabiEncodeArgs);
		std::string getFromDeviceID(std::string const& funcName, uint32_t deviceID);
		uint64_t getIntFromDeviceID(std::string const& funcName, uint32_t deviceID);
		uint64_t getIntFromContract(std::string const& funcName);
		std::string getStringFromDeviceID(std::string const& funcName, uint32_t deviceID);
		std::vector<std::string> getStringsFromDeviceID(std::string const& funcName, uint32_t deviceID);
		std::string getArrayFromContract(std::string const& funcName);
		Json call_helper(std::string const& data);
		std::unique_ptr<std::unordered_map<std::string, std::string>> contract_helper(std::string const& data);
		bool callMutatorContract(std::string const& funcName, std::string const& ethabiEncodeArgs, std::unique_ptr<std::unordered_map<std::string, std::string>> & eventLog);

		void create_contract(void);
		std::string getTransactionReceipt(std::string const& transactionHash);

		std::string eth_ipc_request(std::string const& jsonRequest);
		std::string eth_call(std::string const& abiData);
		std::string eth_sendTransaction(std::string const& abiData);
		std::string eth_createContract(std::string const& data);
		std::string eth_getTransactionReceipt(std::string const& transactionHash);

		bool update_publickey(uint32_t deviceID, std::string const& publicKey);


};

}

#endif //__BLOCKCHAINSEC_HPP
