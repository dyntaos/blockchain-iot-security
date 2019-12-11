#ifndef __BLOCKCHAINSEC_H
#define __BLOCKCHAINSEC_H


namespace blockchainSec {

class BlockchainSecLib {
	public:
		BlockchainSecLib(std::string const ipc_path, std::string eth_my_addr, std::string const eth_sec_contract_addr);
		~BlockchainSecLib();
		std::string contract_double(int n);
		std::string contract_getvar(void);
		std::string contract_setvar(int n);
		std::string contract_getmap(int n);
		std::string contract_setmap(int n, int v);

#ifdef _DEBUG
		void test(void);
#endif //_DEBUG

	private:
		std::string ipc_path;
		std::string eth_my_addr;
		std::string eth_sec_contract_addr;

		std::string trim(const std::string& line); // http://www.cplusplus.com/forum/beginner/251052/
		std::string ethabi(std::string args);
		std::string eth_ipc_request(std::string json_request);
		std::string eth_call(std::string abi_data);
		std::string eth_sendTransaction(std::string abi_data);
};

}

#endif //__BLOCKCHAINSEC_H
