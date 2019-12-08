#include "gason.h"

#define IPC_READ_BUFFER_LENGTH		64
#define ETH_DEFAULT_GAS				"0x7A120"

using namespace std;
using namespace blockchain-sec;

namespace blockchain-sec {

class blockchain-sec::Blockchain-Sec-Lib {

	public:
		Blockchain-Sec-Lib(std::string const ipc_path, std::string eth_my_addr, std::string const eth_sec_contract_addr) {
			this->ipc_path = ipc_path;
			this->eth_my_addr = eth_my_addr; //TODO Check for reasonableness
			this->eth_sec_contract_addr = eth_sec_contract_addr; //TODO Check for reasonableness
		}

		~Blockchain-Sec-Lib() {

		}



	private:
		std::string ipc_path;
		std::string eth_my_addr;
		std::string eth_sec_contract_addr;

		std::string eth_ipc_request(std::string json_request) {
			std::string json;
			std::array<char, IPC_READ_BUFFER_LENGTH> ipc_buffer;

			cout << "eth_ipc_request()\n";
			FILE *ipc = popen("echo '" + json_request.c_str() + "' | nc -U '" + this->ipc_path.c_str() + "'", "r");
			if (ipc == NULL) {
				// Failed to open Unix domain socket for IPC -- Perhaps geth is not running?
				cerr << "eth_ipc_request(): Failed to popen() unix domain socket for IPC with geth! Is geth running?\n";
				return ""; //TODO
			}
			while (fgets(ipc_buffer.data(), IPC_READ_BUFFER_LENGTH, ipc) != NULL) {
				json += ipc_buffer.data();
			}
			if (pclose(ipc) < 0) {
				cerr << "eth_ipc_request(): Failed to pclose() unix domain socket for IPC with geth!";
				return json; //TODO: We may still have valid data to return, despite the error
			}
			cout << "eth_ipc_request(): Successfully relayed request\n";
			return json;
		}

		std::string eth_call(std::string abi_data) {
			std::string json_request = "{""jsonrpc"":""2.0"","
										"""method"":""eth_call"","
										"""params"":[{"
											"""to"":""" + this->eth_sec_contract_addr + ""","
											"""gasPrice"":0,"
											"""data"":""" + abi_data +
										"""}],""id"":1}";
			cout << "eth_call()\n";
			return this->eth_ipc_request(json_request);
		}

		std::string eth_sendTransaction(std::string abi_data) {
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
			cout << "eth_sendTransaction()\n";
			return this->eth_ipc_request(json_request);
		}

}

} //namespace blockchain-sec
