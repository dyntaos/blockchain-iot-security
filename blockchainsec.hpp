#ifndef __BLOCKCHAINSEC_HPP
#define __BLOCKCHAINSEC_HPP

#include "json/include/nlohmann/json.hpp"
#include <libconfig.h++>
#include <mutex>
#include <sodium.h>
#include <thread>

#include <eth_interface.hpp>
#include <eth_interface_except.hpp>
#include <subscription_server.hpp>

#define BLOCKCHAINSEC_CONFIG_F "blockchainsec.conf"
#define BLOCKCHAINSEC_MAX_DEV_NAME 32

#define ETH_CONTRACT_SOL "dev_mgmt_contract.sol"
#define ETH_CONTRACT_BIN "dev_mgmt_contract.bin"
#define ETH_CONTRACT_ABI "dev_mgmt_contract.abi"


using namespace eth_interface;

namespace blockchainSec
{



class BlockchainSecLib : public eth_interface::EthInterface
{
	public:
	enum AddrType_e
	{
		UNSET,
		IPV4,
		IPV6,
		LORA,
		OTHER
	};
	typedef AddrType_e AddrType;

	BlockchainSecLib(void);
	BlockchainSecLib(bool compile);

	// Blockchain boolean methods
	bool is_admin(std::string const& isAdminAddress);
	bool is_authd(uint32_t deviceID);

	// Blockchain accessor methods
	bool is_device(uint32_t deviceID);
	bool is_gateway(uint32_t deviceID);
	bool is_gateway_managed(uint32_t deviceID);
	uint32_t get_my_device_id(void);
	uint32_t get_datareceiver(uint32_t deviceID);
	uint32_t get_default_datareceiver(void);
	std::string get_key(uint32_t deviceID);
	std::string get_signKey(uint32_t deviceID);
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
	std::vector<uint32_t> get_authorized_devices(void);
	std::vector<uint32_t> get_authorized_gateways(void);

	// Blockchain mutator methods
	uint32_t add_device(std::string const& deviceAddress, std::string const& name, std::string const& mac, bool gatewayManaged);
	uint32_t add_gateway(std::string const& gatewayAddress, std::string const& name, std::string const& mac);
	bool remove_device(uint32_t deviceID);
	bool remove_gateway(uint32_t deviceID);
	bool update_datareceiver(uint32_t deviceID, uint32_t dataReceiverID);
	bool set_default_datareceiver(uint32_t dataReceiverID);
	bool update_addr(uint32_t deviceID, AddrType addrType, std::string const& addr);
	bool update_publickey(uint32_t deviceID, std::string const& publicKey);
	bool update_signPublickey(uint32_t deviceID, std::string const& signPublicKey);
	bool push_data(uint32_t deviceID, std::string& data, uint16_t dataLen, std::string& nonce);
	bool authorize_admin(std::string const& adminAddr);
	bool deauthorize_admin(std::string const& adminAddr);

	bool updateLocalKeys(void);
	void loadLocalDeviceParameters(void);
	bool loadDataReceiverPublicKey(uint32_t deviceID);

	bool encryptAndPushData(std::string const& data);
	std::string getDataAndDecrypt(uint32_t const deviceID);
	std::vector<uint32_t> getReceivedDevices(uint32_t deviceID);

	private:
	uint32_t localDeviceID;

	libconfig::Config cfg;
	libconfig::Setting* cfgRoot;

	bool derriveSharedSecret = true; // Only recalculate the shared key if the keys have changed
	unsigned char dataReceiver_pk[crypto_kx_PUBLICKEYBYTES + 1];
	unsigned char client_pk[crypto_kx_PUBLICKEYBYTES + 1], client_sk[crypto_kx_SECRETKEYBYTES + 1];
	unsigned char rxSharedKey[crypto_kx_SESSIONKEYBYTES + 1], txSharedKey[crypto_kx_SESSIONKEYBYTES + 1];

	std::vector<std::pair<std::string, std::string>> contractEventSignatures(void);

	std::string getFromDeviceID(std::string const& funcName, uint32_t deviceID);
	uint64_t getIntFromDeviceID(std::string const& funcName, uint32_t deviceID);
	std::string getStringFromDeviceID(std::string const& funcName, uint32_t deviceID);
	std::vector<std::string> getStringsFromDeviceID(std::string const& funcName, uint32_t deviceID);
};


} //namespace


#endif //__BLOCKCHAINSEC_HPP
