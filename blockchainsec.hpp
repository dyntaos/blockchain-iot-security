#ifndef __BLOCKCHAINSEC_HPP
#define __BLOCKCHAINSEC_HPP

#include <thread>
#include <mutex>
#include <libconfig.h++>
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

#define PIPE_BUFFER_LENGTH							128

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
		bool is_admin(std::string clientAddress);
		bool is_authd(uint32_t deviceID);

		// Blockchain accessor methods
		bool is_device(uint32_t deviceID);
		bool is_gateway(uint32_t deviceID);
		uint32_t get_my_device_id(void);
		std::string get_key(uint32_t deviceID);
		AddrType get_addrtype(uint32_t deviceID);
		std::string get_addr(uint32_t deviceID);
		std::string get_name(uint32_t deviceID);
		std::string get_mac(uint32_t deviceID);
		std::string get_data(uint32_t deviceID);
		time_t get_dataTimestamp(uint32_t deviceID);
		time_t get_creationTimestamp(uint32_t deviceID);
		uint32_t get_num_admin(void);
		uint32_t get_num_devices(void);
		uint32_t get_num_gateways(void);
		std::vector<std::string> get_active_admins(void);
		std::vector<std::string> get_authorized_devices(void);
		std::vector<std::string> get_authorized_gateways(void);

		// Blockchain mutator methods
		bool add_device(std::string deviceAddress, std::string name, std::string mac, std::string publicKey, bool gatewayManaged);
		bool add_gateway(std::string gatewayAddress, std::string name, std::string mac, std::string publicKey);
		bool remove_device(uint32_t deviceID);
		bool remove_gateway(uint32_t deviceID);
		bool update_addr(uint32_t deviceID, AddrType addrType, std::string addr);
		bool push_data(uint32_t deviceID, std::string data);
		bool authorize_admin(std::string adminAddr);
		bool deauthorize_admin(std::string adminAddr);

		void joinThreads(void);

#ifdef _DEBUG
		void test(void);
#endif //_DEBUG

	private:
		std::string ipcPath;
		std::string clientAddress;
		std::string contractAddress;
		libconfig::Config cfg;
		libconfig::Setting *cfgRoot;

		std::thread *subscriptionListener;
		EventLogWaitManager *eventLogWaitManager;

		std::string getFrom(std::string funcName, std::string ethabiEncodeArgs);
		std::string getFromDeviceID(std::string funcName, uint32_t deviceID);
		uint64_t getIntFromDeviceID(std::string funcName, uint32_t deviceID);
		uint64_t getIntFromContract(std::string funcName);
		std::string getStringFromDeviceID(std::string funcName, uint32_t deviceID);
		std::string getArrayFromContract(std::string funcName);
		Json call_helper(std::string data);
		std::unique_ptr<std::unordered_map<std::string, std::string>> contract_helper(std::string data);
		bool callMutatorContract(std::string funcName, std::string ethabiEncodeArgs);

		void create_contract(void);
		std::string getTransactionReceipt(std::string transactionHash);

		std::string eth_ipc_request(std::string jsonRequest);
		std::string eth_call(std::string abiData);
		std::string eth_sendTransaction(std::string abiData);
		std::string eth_createContract(std::string data);
		std::string eth_getTransactionReceipt(std::string transactionHash);

};

}

#endif //__BLOCKCHAINSEC_HPP
