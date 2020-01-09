#ifndef __ETHABI_HPP
#define __ETHABI_HPP

#include <string>
#include <map>


namespace blockchainSec {



std::string ethabi(std::string args);
std::unordered_map<std::string, std::string> ethabi_decode_log(std::string abiFile, std::string eventName, std::vector<std::string> topics, std::string data);
std::string ethabi_decode_result(std::string abiFile, std::string eventName, std::string data);
std::vector<std::string> ethabi_decode_array(std::string abiFile, std::string eventName, std::string data);


}

#endif //__ETHABI_HPP
