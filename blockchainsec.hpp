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

		void add_device(std::string client_addr, std::string name, std::string mac, std::string public_key, bool gateway_managed);
		std::string add_gateway(std::string client_addr, std::string name, std::string mac, std::string public_key);

		void joinThreads(void);
#ifdef _DEBUG
		void test(void);
#endif //_DEBUG

	private:
		std::string ipc_path;
		std::string eth_my_addr;
		std::string eth_sec_contract_addr;
		libconfig::Config cfg;
		libconfig::Setting *cfg_root;

		std::thread *subscriptionListener;
		EventLogWaitManager *eventLogWaitManager;

		void create_contract(void);
		std::string getTransactionReceipt(std::string transaction_hash);

		std::string eth_ipc_request(std::string json_request);
		std::string eth_call(std::string abi_data);
		std::string eth_sendTransaction(std::string abi_data);
		std::string eth_createContract(std::string data);
		std::string eth_getTransactionReceipt(std::string transaction_hash);
};

}

#endif //__BLOCKCHAINSEC_HPP
