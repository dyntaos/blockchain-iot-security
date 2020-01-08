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
		BlockchainSecLib(bool compile);
		BlockchainSecLib(void);
		~BlockchainSecLib();

		std::string getClientAddress(void);
		std::string getContractAddress(void);

		bool is_admin(std::string clientAddress);
		bool is_authd(uint32_t deviceID);
		bool get_my_device_id(void);
		std::string get_key(uint32_t deviceID);

		bool add_device(std::string deviceAddress, std::string name, std::string mac, std::string publicKey, bool gatewayManaged);
		bool add_gateway(std::string gatewayAddress, std::string name, std::string mac, std::string publicKey);

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

		Json call_helper(std::string data);
		std::unique_ptr<std::unordered_map<std::string, std::string>> contract_helper(std::string data);

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
