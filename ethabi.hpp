#ifndef __ETHABI_HPP
#define __ETHABI_HPP

#include <string>
#include <map>


namespace blockchainSec {



std::string ethabi(std::string args);
std::unordered_map<std::string, std::string> ethabi_decode_log(std::string abi_file, std::string event_name, std::vector<std::string> topics, std::string data);
std::string ethabi_decode_result(std::string abi_file, std::string event_name, std::string data);



}

#endif //__ETHABI_HPP
