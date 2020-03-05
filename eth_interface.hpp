#ifndef __ETH_INTERFACE_HPP
#define __ETH_INTERFACE_HPP

#include "json/include/nlohmann/json.hpp"
#include <libconfig.h++>
#include <mutex>
#include <sodium.h>
#include <thread>

#include <eth_interface_except.hpp>
#include <subscription_server.hpp>


#define IPC_BUFFER_LENGTH 128

#define ETH_DEFAULT_GAS "0x7A120"


using Json = nlohmann::json;

namespace eth_interface
{



class EthInterface
{
	public:
	void initialize(
		std::string ipcPath,
		std::string clientAddress,
		std::string contractAddress,
		std::vector<std::pair<std::string, std::string>> contractEventSignatures);

	std::string getClientAddress(void);
	std::string getContractAddress(void);

	void joinThreads(void);

	protected:
	std::string ipcPath;
	std::string clientAddress;
	std::string contractAddress;

	EventLogWaitManager* eventLogWaitManager;

	std::string getFrom(std::string const& funcName, std::string const& ethabiEncodeArgs);
	uint64_t getIntFromContract(std::string const& funcName);
	std::string getArrayFromContract(std::string const& funcName);
	Json call_helper(std::string const& data);
	std::unique_ptr<std::unordered_map<std::string, std::string>> contract_helper(std::string const& data);
	bool callMutatorContract(
		std::string const& funcName,
		std::string const& ethabiEncodeArgs,
		std::unique_ptr<std::unordered_map<std::string, std::string>>& eventLog);

	std::string create_contract(void);
	std::string getTransactionReceipt(std::string const& transactionHash);

	std::string eth_ipc_request(std::string const& jsonRequest);
	std::string eth_call(std::string const& abiData);
	std::string eth_sendTransaction(std::string const& abiData);
	std::string eth_createContract(std::string const& data);
	std::string eth_getTransactionReceipt(std::string const& transactionHash);
};


} //namespace


#endif //__ETH_INTERFACE_HPP