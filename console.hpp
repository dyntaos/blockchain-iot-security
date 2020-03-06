#ifndef __CONSOLE_HPP
#define __CONSOLE_HPP

#include <blockchainsec.hpp>
#include <thread>
#include <unordered_map>
#include <vector>



namespace blockchainSec
{


class BlockchainSecConsole
{
	private:
	std::unordered_map<std::string, void (*)(std::vector<std::string>&, BlockchainSecLib&)> cmd_map;
	std::thread* consoleThread;
	bool initialized = false;
	bool haltThread = true;

	std::vector<std::string> tokenize(std::string input);
	static void console(
		blockchainSec::BlockchainSecLib& blockchainSec,
		BlockchainSecConsole& self);

	void processCommand(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);

	static void cmd_is_admin(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_is_authd(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_is_device(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_is_gateway(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_is_gateway_managed(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_my_device_id(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_data_receiver(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_default_data_receiver(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_key(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_sign_key(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_addr_type(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_addr(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_name(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_data(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_data_timestamp(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_creation_timestamp(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_num_admin(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_num_devices(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_num_gateways(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_active_admins(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_authorized_devices(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_authorized_gateways(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_add_device(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_add_gateway(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_remove_device(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_remove_gateway(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_update_data_receiver(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_set_default_data_receiver(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_update_addr(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_authorize_admin(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_deauthorize_admin(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_encrypt_and_push_data(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_data_and_decrypt(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_get_received_devices(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_update_local_keys(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_update_public_key(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_update_sign_key(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);
	static void cmd_help(std::vector<std::string>& cmds, BlockchainSecLib& blockchainSec);


	public:
	BlockchainSecConsole(void);

	void startThread(blockchainSec::BlockchainSecLib& blockchainSec);
	void stopThread(void);
};


} //namespace


#endif //__CONSOLE_HPP