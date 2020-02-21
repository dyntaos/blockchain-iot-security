#include <iostream>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <string>
#include <vector>

#include <blockchainsec.hpp>
#include <blockchainsec_except.hpp>
#include <console.hpp>
#include <misc.hpp>

using namespace std;



namespace blockchainSec {



BlockchainSecConsole::BlockchainSecConsole(void) {
	boost::assign::insert(cmd_map)
		("is_admin",                  cmd_is_admin)
		("is_authd",                  cmd_is_authd)
		("is_device",                 cmd_is_device)
		("is_gateway",                cmd_is_gateway)
		("get_my_device_id",          cmd_get_my_device_id)
		("get_data_receiver",         cmd_get_data_receiver)
		("get_default_data_receiver", cmd_get_default_data_receiver)
		("get_key",                   cmd_get_key)
		("get_addr_type",             cmd_get_addr_type)
		("get_addr",                  cmd_get_addr)
		("get_name",                  cmd_get_name)
		("get_mac",                   cmd_get_mac)
		("get_data",                  cmd_get_data)
		("get_data_timestamp",        cmd_get_data_timestamp)
		("get_creation_timestamp",    cmd_get_creation_timestamp)
		("get_num_admin",             cmd_get_num_admin)
		("get_num_devices",           cmd_get_num_devices)
		("get_num_gateways",          cmd_get_num_gateways)
		("get_active_admins",         cmd_get_active_admins)
		("get_authorized_devices",    cmd_get_authorized_devices)
		("get_authorized_gateways",   cmd_get_authorized_gateways)
		("add_device",                cmd_add_device)
		("add_gateway",               cmd_add_gateway)
		("remove_device",             cmd_remove_device)
		("remove_gateway",            cmd_remove_gateway)
		("update_data_receiver",      cmd_update_data_receiver)
		("set_default_data_receiver", cmd_set_default_data_receiver)
		("update_addr",               cmd_update_addr)
		// ("push_data",                 cmd_push_data)
		("authorize_admin",           cmd_authorize_admin)
		("deauthorize_admin",         cmd_deauthorize_admin)
		("encrypt_and_push_data",     cmd_encrypt_and_push_data)
		("get_data_and_decrypt",      cmd_get_data_and_decrypt)
		("get_received_devices",      cmd_get_received_devices)
		("help",                      cmd_help)
	;
}



vector<string> BlockchainSecConsole::tokenize(string input) {
	vector<string> v;
	boost::escaped_list_separator<char> escapedListSep("\\", " ", "\"");
	boost::tokenizer<boost::escaped_list_separator<char>> tok(input, escapedListSep);

	for (boost::tokenizer<boost::escaped_list_separator<char>>::iterator it = tok.begin(); it != tok.end(); ++it) {
		v.push_back(*it);
	}
	return v;
}



void BlockchainSecConsole::startThread(blockchainSec::BlockchainSecLib & blockchainSec) {
	if (initialized)
		throw blockchainSec::ThreadExistsException(
			"An instance of the console thread already exists for this object."
		);

	haltThread = false;
	initialized = true;

	consoleThread = new thread(
		this->console,
		std::ref(blockchainSec),
		std::ref(*this)
	);
}



void BlockchainSecConsole::stopThread(void) {
	haltThread = true;
}



void BlockchainSecConsole::console(blockchainSec::BlockchainSecLib & blockchainSec, BlockchainSecConsole & self) {
	vector<string> tokens;
	string input;

	for (; !self.haltThread;) {
		if (blockchainSec.is_admin(blockchainSec.getClientAddress().substr(2))) {
			cout << " ## ";
		} else {
			cout << " $$ ";
		}
		getline(cin, input);

		if (self.haltThread) break;

		boost::trim(input);

		if (input == "") continue;

		tokens = self.tokenize(input);

		self.processCommand(tokens, blockchainSec);

	}
	self.initialized = false;
	self.haltThread = true;
}



void BlockchainSecConsole::processCommand(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	string cmd;

	if (cmds.size() == 0) return;

	cmd = cmds[0];

	if (cmd_map.find(cmds[0]) == cmd_map.end()) {
		cerr << "Unknown command: \"" << cmd << "\"" << endl;
		return;
	}
	cmd_map[cmds[0]](cmds, blockchainSec);
}



void BlockchainSecConsole::cmd_is_admin(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout << "Usage: is_admin address" << endl;
		return;
	}
	if (!isEthereumAddress(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid Ethereum address" << endl;
		return;
	}
	try {
		if (blockchainSec.is_admin(cmds[1])) {
			cout << "TRUE" << endl;
		} else {
			cout << "FALSE" << endl;
		}
	} catch (BlockchainSecLibException & e) {
		cerr << "Invalid argument provided" << endl;
	}
}



void BlockchainSecConsole::cmd_is_authd(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout << "Usage: is_authd deviceID" << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		if (blockchainSec.is_authd(strtoul(cmds[1].c_str(), nullptr, 10))) {
			cout << "TRUE" << endl;
		} else {
			cout << "FALSE" << endl;
		}
	} catch (BlockchainSecLibException & e) {
		cerr << "Invalid argument provided" << endl;
	}
}



void BlockchainSecConsole::cmd_is_device(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout << "Usage: is_device deviceID" << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		if (blockchainSec.is_device(strtoul(cmds[1].c_str(), nullptr, 10))) {
			cout << "TRUE" << endl;
		} else {
			cout << "FALSE" << endl;
		}
	} catch (BlockchainSecLibException & e) {
		cerr << "Invalid argument provided" << endl;
	}
}



void BlockchainSecConsole::cmd_is_gateway(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout << "Usage: is_device deviceID" << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		if (blockchainSec.is_gateway(strtoul(cmds[1].c_str(), nullptr, 10))) {
			cout << "TRUE" << endl;
		} else {
			cout << "FALSE" << endl;
		}
	} catch (BlockchainSecLibException & e) {
		cerr << "Invalid argument provided" << endl;
	}
}



void BlockchainSecConsole::cmd_get_my_device_id(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 1) {
		cout << "Usage: get_my_device_id" << endl;
		return;
	}
	try {
		cout << blockchainSec.get_my_device_id() << endl;
	} catch (BlockchainSecLibException & e) {
		cerr
			<< "No device ID associated with address \""
			<< blockchainSec.getClientAddress()
			<< "\""
			<< endl;
	}
}



void BlockchainSecConsole::cmd_get_data_receiver(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout << "Usage: get_data_receiver deviceID" << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		cout << blockchainSec.get_datareceiver(strtoul(cmds[1].c_str(), nullptr, 10)) << endl;
	} catch (BlockchainSecLibException & e) {
		cerr
			<< "Device ID "
			<< cmds[1]
			<< " does not exist"
			<< endl;
	}
}



void BlockchainSecConsole::cmd_get_default_data_receiver(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 1) {
		cout << "Usage: get_default_data_receiver" << endl;
		return;
	}
	try {
		cout << blockchainSec.get_default_datareceiver() << endl;
	} catch (BlockchainSecLibException & e) {
		cerr
			<< "Unable to retreive default data receiver"
			<< endl;
	}
}



void BlockchainSecConsole::cmd_get_key(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout << "Usage: get_key deviceID" << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		cout << blockchainSec.get_key(strtoul(cmds[1].c_str(), nullptr, 10)) << endl;
	} catch (BlockchainSecLibException & e) {
		cerr
			<< "Device ID "
			<< cmds[1]
			<< " does not exist"
			<< endl;
	}
}



void BlockchainSecConsole::cmd_get_addr_type(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout << "Usage: get_addrtype deviceID" << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		BlockchainSecLib::AddrType t = blockchainSec.get_addrtype(strtoul(cmds[1].c_str(), nullptr, 10));
		switch (t) {
			case BlockchainSecLib::AddrType::UNSET:
				cout << "Unset" << endl;
				break;

			case BlockchainSecLib::AddrType::IPV4:
				cout << "IPv4" << endl;
				break;

			case BlockchainSecLib::AddrType::IPV6:
				cout << "IPv6" << endl;
				break;

			case BlockchainSecLib::AddrType::LORA:
				cout << "LoRa" << endl;
				break;

			case BlockchainSecLib::AddrType::OTHER:
				cout << "Other" << endl;
				break;

			default:
				cerr << "Error retreiving address type" << endl;
				break;
		}
	} catch (BlockchainSecLibException & e) {
		cerr
			<< "Device ID "
			<< cmds[1]
			<< " does not exist"
			<< endl;
	}
}



void BlockchainSecConsole::cmd_get_addr(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout << "Usage: get_addr deviceID" << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		cout << blockchainSec.get_addr(strtoul(cmds[1].c_str(), nullptr, 10)) << endl;
	} catch (BlockchainSecLibException & e) {
		cerr
			<< "Device ID "
			<< cmds[1]
			<< " does not exist"
			<< endl;
	}
}



void BlockchainSecConsole::cmd_get_name(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout << "Usage: get_name deviceID" << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		cout << blockchainSec.get_name(strtoul(cmds[1].c_str(), nullptr, 10)) << endl;
	} catch (BlockchainSecLibException & e) {
		cerr
			<< "Device ID "
			<< cmds[1]
			<< " does not exist"
			<< endl;
	}
}



void BlockchainSecConsole::cmd_get_mac(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout << "Usage: get_mac deviceID" << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		cout << blockchainSec.get_mac(strtoul(cmds[1].c_str(), nullptr, 10)) << endl;
	} catch (BlockchainSecLibException & e) {
		cerr
			<< "Device ID "
			<< cmds[1]
			<< " does not exist"
			<< endl;
	}
}



void BlockchainSecConsole::cmd_get_data(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout << "Usage: get_data deviceID" << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		vector<string> v = blockchainSec.get_data(strtoul(cmds[1].c_str(), nullptr, 10));
		for (vector<string>::iterator it = v.begin(); it != v.end(); ++it) {
			cout << *it << endl;
		}
	} catch (BlockchainSecLibException & e) {
		cerr
			<< "Device ID "
			<< cmds[1]
			<< " does not exist"
			<< endl;
	}
}



void BlockchainSecConsole::cmd_get_data_timestamp(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout << "Usage: get_data_timestamp deviceID" << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		time_t t = blockchainSec.get_dataTimestamp(strtoul(cmds[1].c_str(), nullptr, 10));
		cout
			<< unsigned(t) << endl
			<< ctime(&t) << endl; // TODO: Does the return value of ctime() need to be free'd?
	} catch (BlockchainSecLibException & e) {
		cerr
			<< "Device ID "
			<< cmds[1]
			<< " does not exist"
			<< endl;
	}
}



void BlockchainSecConsole::cmd_get_creation_timestamp(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout << "Usage: get_creation_timestamp deviceID" << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		time_t t = blockchainSec.get_creationTimestamp(strtoul(cmds[1].c_str(), nullptr, 10));
		cout
			<< unsigned(t) << endl
			<< ctime(&t) << endl; // TODO: Does the return value of ctime() need to be free'd?
	} catch (BlockchainSecLibException & e) {
		cerr
			<< "Device ID "
			<< cmds[1]
			<< " does not exist"
			<< endl;
	}
}



void BlockchainSecConsole::cmd_get_num_admin(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 1) {
		cout << "Usage: get_num_admin" << endl;
		return;
	}
	try {
		cout << blockchainSec.get_num_admin() << endl;
	} catch (BlockchainSecLibException & e) {
		cerr
			<< "Unable to retreive number of authorized administrators"
			<< endl;
	}
}



void BlockchainSecConsole::cmd_get_num_devices(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 1) {
		cout << "Usage: get_num_devices" << endl;
		return;
	}
	try {
		cout << blockchainSec.get_num_devices() << endl;
	} catch (BlockchainSecLibException & e) {
		cerr
			<< "Unable to retreive number of authorized devices"
			<< endl;
	}
}



void BlockchainSecConsole::cmd_get_num_gateways(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 1) {
		cout << "Usage: get_num_gateways" << endl;
		return;
	}
	try {
		cout << blockchainSec.get_num_gateways() << endl;
	} catch (BlockchainSecLibException & e) {
		cerr
			<< "Unable to retreive number of authorized gateways"
			<< endl;
	}
}



void BlockchainSecConsole::cmd_get_active_admins(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 1) {
		cout << "Usage: get_active_admins" << endl;
		return;
	}
	try {
		vector<string> v = blockchainSec.get_active_admins();

		for (vector<string>::iterator it = v.begin(); it != v.end(); ++it) {
			cout << *it << endl;
		}
	} catch (BlockchainSecLibException & e) {
		cerr
			<< "Unable to retreive default data receiver"
			<< endl;
	}
}



void BlockchainSecConsole::cmd_get_authorized_devices(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 1) {
		cout << "Usage: get_authorized_devices" << endl;
		return;
	}
	try {
		vector<uint32_t> v = blockchainSec.get_authorized_devices();
		bool first = true;
		cout << "[";
		for (vector<uint32_t>::iterator it = v.begin(); it != v.end(); ++it) {
			if (!first) {
				cout << ", ";
			}
			first = false;
			cout << unsigned(*it);
		}
		cout << "]" << endl;
	} catch (BlockchainSecLibException & e) {
		cerr
			<< "Unable to retreive default data receiver"
			<< endl;
	}
}



void BlockchainSecConsole::cmd_get_authorized_gateways(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 1) {
		cout << "Usage: get_authorized_gateways" << endl;
		return;
	}
	try {
		vector<uint32_t> v = blockchainSec.get_authorized_gateways();
		bool first = true;
		cout << "[";
		for (vector<uint32_t>::iterator it = v.begin(); it != v.end(); ++it) {
			if (!first) {
				cout << ", ";
			}
			first = false;
			cout << unsigned(*it);
		}
		cout << "]" << endl;
	} catch (BlockchainSecLibException & e) {
		cerr
			<< "Unable to retreive default data receiver"
			<< endl;
	}
}



void BlockchainSecConsole::cmd_add_device(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 4) {
		cout
			<< "Usage: add_device address name gatewayManaged" << endl
			<< "\t       address - Ethereum address of authorized client for this device." << endl
			<< "\t          name - An arbitrary name to identify this device by. Does not have to be unique." << endl
			<< "\tgatewayManaged - [true/false] May gateways act on the behalf of this device?" << endl;
		return;
	}
	if (!isEthereumAddress(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid Ethereum address" << endl;
		return;
	}
	bool gatewayManaged;
	if (boost::algorithm::to_lower_copy(cmds[3]) == "true") {
		gatewayManaged = true;
	} else if (boost::algorithm::to_lower_copy(cmds[3]) == "false") {
		gatewayManaged = false;
	} else {
		cerr << "gatewayManaged must be \"true\" or \"false\"" << endl;
		return;
	}
	try {
		auto deviceId = blockchainSec.add_device(cmds[1], cmds[2], "", gatewayManaged); // TODO: Eliminate all MAC stuff?
		cout << "Device ID: " << unsigned(deviceId) << endl;
	} catch (BlockchainSecLibException & e) {
		cerr << "Failed to add new device" << endl;
	}
}



void BlockchainSecConsole::cmd_add_gateway(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 3) {
		cout
			<< "Usage: add_gateway address name" << endl
			<< "\t       address - Ethereum address of authorized client for this device." << endl
			<< "\t          name - An arbitrary name to identify this device by. Does not have to be unique." << endl;
		return;
	}
	if (!isEthereumAddress(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid Ethereum address" << endl;
		return;
	}
	try {
		auto deviceId = blockchainSec.add_gateway(cmds[1], cmds[2], ""); // TODO: Eliminate all MAC stuff?
		cout << "Device ID: " << unsigned(deviceId) << endl;
	} catch (BlockchainSecLibException & e) {
		cerr << "Failed to add new gateway" << endl;
	}
}



void BlockchainSecConsole::cmd_remove_device(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout << "Usage: remove_device deviceID" << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		if (blockchainSec.remove_device(strtoul(cmds[1].c_str(), nullptr, 10))) {
			cout << "TRUE" << endl;
		} else {
			cout << "FALSE" << endl;
		}
	} catch (BlockchainSecLibException & e) {
		cerr << "Error completing command" << endl;
	}
}



void BlockchainSecConsole::cmd_remove_gateway(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout << "Usage: remove_gatway deviceID" << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		if (blockchainSec.remove_gateway(strtoul(cmds[1].c_str(), nullptr, 10))) {
			cout << "TRUE" << endl;
		} else {
			cout << "FALSE" << endl;
		}
	} catch (BlockchainSecLibException & e) {
		cerr << "Error completing command" << endl;
	}
}



void BlockchainSecConsole::cmd_update_data_receiver(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 3) {
		cout << "Usage: update_data_receiver deviceID dataReceiverID" << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	if (!isInt(cmds[2])) {
		cerr << "\"" << cmds[2] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		if (blockchainSec.update_datareceiver(
				strtoul(cmds[1].c_str(), nullptr, 10),
				strtoul(cmds[2].c_str(), nullptr, 10)
			)
		) {
			cout << "TRUE" << endl;
		} else {
			cout << "FALSE" << endl;
		}
	} catch (BlockchainSecLibException & e) {
		cerr << "Error completing command" << endl;
	}
}



void BlockchainSecConsole::cmd_set_default_data_receiver(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout << "Usage: set_default_data_receiver dataReceiverID" << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		if (blockchainSec.set_default_datareceiver(
				strtoul(cmds[1].c_str(), nullptr, 10)
			)
		) {
			cout << "TRUE" << endl;
		} else {
			cout << "FALSE" << endl;
		}
	} catch (BlockchainSecLibException & e) {
		cerr << "Error completing command" << endl;
	}
}



void BlockchainSecConsole::cmd_update_addr(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 4) {
		cout
			<< "Usage: update_addr deviceID addrType addr" << endl
			<< "\tdeviceID - Device ID of the device to modify" << endl
			<< "\taddrType - [Unset, IPv4, IPv6, LoRa, Other] Protocol of the address" << endl
			<< "\t    addr - The new address to store" << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		if (blockchainSec.set_default_datareceiver(
				strtoul(cmds[1].c_str(), nullptr, 10)
			)
		) {
			cout << "TRUE" << endl;
		} else {
			cout << "FALSE" << endl;
		}
	} catch (BlockchainSecLibException & e) {
		cerr << "Error completing command" << endl;
	}
}



void BlockchainSecConsole::cmd_authorize_admin(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout
			<< "Usage: authorize_admin address" << endl;
		return;
	}
	if (!isEthereumAddress(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid Ethereum address" << endl;
		return;
	}
	try {
		if (blockchainSec.authorize_admin(cmds[1])) {
			cout << "TRUE" << endl;
		} else {
			cout << "FALSE" << endl;
		}
	} catch (BlockchainSecLibException & e) {
		cerr << "Error completing command" << endl;
	}
}



void BlockchainSecConsole::cmd_deauthorize_admin(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout
			<< "Usage: deauthorize_admin address" << endl;
		return;
	}
	if (!isEthereumAddress(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid Ethereum address" << endl;
		return;
	}
	try {
		if (blockchainSec.deauthorize_admin(cmds[1])) {
			cout << "TRUE" << endl;
		} else {
			cout << "FALSE" << endl;
		}
	} catch (BlockchainSecLibException & e) {
		cerr << "Error completing command" << endl;
	}
}



void BlockchainSecConsole::cmd_encrypt_and_push_data(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout
			<< "Usage: encrypt_and_push_data data" << endl
			<< "\tEncrypt and store the data on the blockchain. "
			"Data will be stored for this device ID." << endl;
		return;
	}
	try {
		if (blockchainSec.encryptAndPushData(cmds[1])) {
			cout << "TRUE" << endl;
		} else {
			cout << "FALSE" << endl;
		}
	} catch (BlockchainSecLibException & e) {
		cerr << "Error completing command" << endl;
	}
}



void BlockchainSecConsole::cmd_get_data_and_decrypt(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout
			<< "Usage: get_data_and_decrypt deviceID" << endl
			<< "\tRetreive and decrypt the data of a given device ID. "
			"This can only be done if this device is the data receiver "
			"of the given device ID." << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		string d = blockchainSec.getDataAndDecrypt(strtoul(cmds[1].c_str(), nullptr, 10));
		cout << "Data: \"" << d << "\"" << endl;
	} catch (BlockchainSecLibException & e) {
		cerr << "Error completing command" << endl;
	}
}



void BlockchainSecConsole::cmd_get_received_devices(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	if (cmds.size() != 2) {
		cout << "Usage: get_received_devices dataReceiverID" << endl;
		return;
	}
	if (!isInt(cmds[1])) {
		cerr << "\"" << cmds[1] << "\" is not a valid device ID" << endl;
		return;
	}
	try {
		vector<uint32_t> v = blockchainSec.getReceivedDevices(strtoul(cmds[1].c_str(), nullptr, 10));
		bool first = true;
		cout << "[";
		for (vector<uint32_t>::iterator it = v.begin(); it != v.end(); ++it) {
			if (!first) {
				cout << ", ";
			}
			first = false;
			cout << unsigned(*it);
		}
		cout << "]";
	} catch (BlockchainSecLibException & e) {
		cerr
			<< "Unable to retreive list of received devices"
			<< endl;
	}
}



void BlockchainSecConsole::cmd_help(vector<string> & cmds, BlockchainSecLib & blockchainSec) {
	(void) cmds;
	(void) blockchainSec;
	cout
		<< "Commands:\n"
		"\tis_admin\n"
		"\tis_authd\n"
		"\tis_device\n"
		"\tis_gateway\n"
		"\tget_my_device_id\n"
		"\tget_data_receiver\n"
		"\tget_default_data_receiver\n"
		"\tget_key\n"
		"\tget_addr_type\n"
		"\tget_addr\n"
		"\tget_name\n"
		"\tget_mac\n"
		"\tget_data\n"
		"\tget_data_timestamp\n"
		"\tget_creation_timestamp\n"
		"\tget_num_admin\n"
		"\tget_num_devices\n"
		"\tget_num_gateways\n"
		"\tget_active_admins\n"
		"\tget_authorized_devices\n"
		"\tget_authorized_gateways\n"
		"\tadd_device\n"
		"\tadd_gateway\n"
		"\tremove_device\n"
		"\tremove_gateway\n"
		"\tupdate_data_receiver\n"
		"\tset_default_data_receiver\n"
		"\tupdate_addr\n"
		"\tauthorize_admin\n"
		"\tdeauthorize_admin\n"
		"\tencrypt_and_push_data\n"
		"\tget_data_and_decrypt\n"
		"\tget_received_devices\n"
		<< endl;
}


}