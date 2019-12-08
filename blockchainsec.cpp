#include <string>
#include <array>
#include <iostream>
#include <blockchainsec.h>
#include "gason.h"

#define PIPE_BUFFER_LENGTH			64
#define ETH_DEFAULT_GAS				"0x7A120"

using namespace std;

namespace blockchainSec {

BlockchainSecLib::BlockchainSecLib(std::string const ipc_path, std::string eth_my_addr, std::string const eth_sec_contract_addr) {
	this->ipc_path = ipc_path;
	this->eth_my_addr = eth_my_addr; //TODO Check for reasonableness
	this->eth_sec_contract_addr = eth_sec_contract_addr; //TODO Check for reasonableness
}

BlockchainSecLib::~BlockchainSecLib() {

}

std::string BlockchainSecLib::trim(const std::string& line) {
	const char* WhiteSpace = " \t\v\r\n";
	std::size_t start = line.find_first_not_of(WhiteSpace);
	std::size_t end = line.find_last_not_of(WhiteSpace);
	return start == end ? std::string() : line.substr(start, end - start + 1);
}

#ifdef _DEBUG
void BlockchainSecLib::test(void) {
	bool testsPassed = true;
	std::string result;
	result = trim(this->ethabi("encode params -v bool 1"));
	if (result != "0000000000000000000000000000000000000000000000000000000000000001") {
		cout << "ethabi(): Test 1 failed! Got return value of '" << result << "'\n";
		testsPassed = false;
	}

	if (testsPassed) {
		cout << "All tests passed successfully!\n";
	} else {
		cout << "Tests failed!\n";
	}
}
#endif //_DEBUG

std::string BlockchainSecLib::ethabi(std::string args) {
	std::string result;
	std::array<char, PIPE_BUFFER_LENGTH> pipe_buffer;

#ifdef _DEBUG
	cout << "ethabi()\n";
#endif //_DEBUG

	FILE *ethabi_pipe = popen(("ethabi " + args).c_str(), "r");
	if (ethabi_pipe == NULL) {
		// Failed to open pipe to ethabi -- is the binary installed and in $PATH?
		cerr << "ethabi(): Failed to popen() pipe to ethabi binary. Is the binary installed and in the $PATH environment variable?\n";
		return ""; //TODO
	}
	while (fgets(pipe_buffer.data(), PIPE_BUFFER_LENGTH, ethabi_pipe) != NULL) {
		result += pipe_buffer.data();
	}
	if (pclose(ethabi_pipe) < 0) {
		cerr << "ethabi(): Failed to pclose() pipe to ethabi binary!";
		return result; //TODO: We may still have valid data to return, despite the error
	}

#ifdef _DEBUG
	cout << "ethabi(): Call successful!\n";
#endif //_DEBUG

	return result;
}

std::string BlockchainSecLib::eth_ipc_request(std::string json_request) {
	std::string json;
	std::array<char, PIPE_BUFFER_LENGTH> ipc_buffer;

#ifdef _DEBUG
	cout << "eth_ipc_request()\n";
#endif //_DEBUG

	FILE *ipc = popen(("echo '" + json_request + "' | nc -U '" + this->ipc_path + "'").c_str(), "r");
	if (ipc == NULL) {
		// Failed to open Unix domain socket for IPC -- Perhaps geth is not running?
		cerr << "eth_ipc_request(): Failed to popen() unix domain socket for IPC with geth! Is geth running?\n";
		return ""; //TODO
	}
	while (fgets(ipc_buffer.data(), PIPE_BUFFER_LENGTH, ipc) != NULL) {
		json += ipc_buffer.data();
	}
	if (pclose(ipc) < 0) {
		cerr << "eth_ipc_request(): Failed to pclose() unix domain socket for IPC with geth!";
		return json; //TODO: We may still have valid data to return, despite the error
	}

#ifdef _DEBUG
	cout << "eth_ipc_request(): Successfully relayed request\n";
#endif //_DEBUG

	return json;
}

std::string BlockchainSecLib::eth_call(std::string abi_data) {
	std::string json_request = "{""jsonrpc"":""2.0"","
								"""method"":""eth_call"","
								"""params"":[{"
									"""to"":""" + this->eth_sec_contract_addr + ""","
									"""gasPrice"":0,"
									"""data"":""" + abi_data +
								"""}],""id"":1}";

#ifdef _DEBUG
	cout << "eth_call()\n";
#endif //_DEBUG

	return this->eth_ipc_request(json_request);
}

std::string BlockchainSecLib::eth_sendTransaction(std::string abi_data) {
	std::string json_request = "{""jsonrpc"":""2.0"","
								"""method"":""eth_sendTransaction"""
								",""params"":[{"
									"""from"":""" + this->eth_my_addr + ""","
									"""to"":""" + this->eth_sec_contract_addr + ""","
									"""gas"":0,"
									"""gasPrice"":""" + ETH_DEFAULT_GAS + ""","
									"""data"":""" + abi_data +
								"""}],"
								"""id"":1}";

#ifdef _DEBUG
	cout << "eth_sendTransaction()\n";
#endif //_DEBUG

	return this->eth_ipc_request(json_request);
}

} //namespace blockchain-sec
