#include <iostream>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <string>
#include <vector>

#include <blockchainsec.hpp>
#include <blockchainsec_except.hpp>
#include <console.hpp>

using namespace std;



namespace blockchainSec {


vector<string> BlockchainSecConsole::tokenize(string input) {
	vector<string> v;
	boost::escaped_list_separator<char> escapedListSep("\\", " ", "\"\'");
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
		cin >> input;

		if (self.haltThread) break;

		boost::trim(input);

		if (input == "") continue;

		tokens = self.tokenize(input);

		for (vector<string>::iterator it = tokens.begin(); it != tokens.end(); ++it) {
			cout << "Token :: " << *it << endl;
		}
	}
	self.initialized = false;
	self.haltThread = true;
}



}