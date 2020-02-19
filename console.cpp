#include <iostream>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <string>
#include <vector>

#include <blockchainsec.hpp>
#include <blockchainsec_except.hpp>
#include <console.hpp>
#include <misc.hpp>

using namespace std;



namespace blockchainSec {


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

	if (cmd == "is_admin") {
		if (cmds.size() != 2) {
			cerr << "Usage: is_admin address" << endl;
			return;
		}
		if (!isEthereumAddress(cmds[1])) {
			cerr << "\"" << cmds[1] << "\" is not a valid address" << endl;
			return;
		}
		if (blockchainSec.is_admin(cmds[1])) {
			cout << "TRUE" << endl;
		} else {
			cout << "FALSE";
		}

	} else if (cmd == "is_authd") {

	} else if (cmd == "is_device") {

	} else if (cmd == "is_gateway") {

	} else if (cmd == "get_my_device_id") {

	} else if (cmd == "get_datareceiver") {

	} else if (cmd == "get_default_datareceiver") {

	} else if (cmd == "get_key") {

	} else if (cmd == "get_addrtype") {

	} else if (cmd == "get_addr") {

	} else if (cmd == "get_name") {

	} else if (cmd == "get_mac") {

	} else if (cmd == "get_data") {

	} else if (cmd == "get_datatimestamp") {

	} else if (cmd == "get_creationtimestamp") {

	} else if (cmd == "get_num_admin") {

	} else if (cmd == "get_num_devices") {

	} else if (cmd == "get_num_gateways") {

	} else if (cmd == "get_active_admins") {

	} else if (cmd == "get_authorized_devices") {

	} else if (cmd == "get_authorized_gateways") {

	} else if (cmd == "add_device") {

	} else if (cmd == "add_gateway") {

	} else if (cmd == "remove_device") {

	} else if (cmd == "remove_gateway") {

	} else if (cmd == "update_datareceiver") {

	} else if (cmd == "set_default_datareceiver") {

	} else if (cmd == "update_addr") {

	} else if (cmd == "push_data") {

	} else if (cmd == "authorize_admin") {

	} else if (cmd == "deauthorize_admin") {

	} else if (cmd == "encryptandpushdata") {

	} else if (cmd == "getdataanddecrypt") {

	} else if (cmd == "getreceiveddevices") {

	} else {
		cerr << "Unknown command: \"" << cmd << "\"" << endl;
	}
}



}