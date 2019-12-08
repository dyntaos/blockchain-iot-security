#ifndef __BLOCKCHAIN_SEC_LIB_H
#define __BLOCKCHAIN_SEC_LIB_H


namespace blockchain_sec {

class Blockchain_Sec_Lib {
	public:
		Blockchain_Sec_Lib(std::string const ipc_path, std::string eth_my_addr, std::string const eth_sec_contract_addr);
		~Blockchain_Sec_Lib();
	private:
		std::string ipc_path;
		std::string eth_my_addr;
		std::string eth_sec_contract_addr;

		std::string ethabi(std::string args);
		std::string eth_ipc_request(std::string json_request);
		std::string eth_call(std::string abi_data);
		std::string eth_sendTransaction(std::string abi_data);
};

}

#endif //__BLOCKCHAIN_SEC_LIB_H
