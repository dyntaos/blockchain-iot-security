#ifndef __ETHABI_HPP
#define __ETHABI_HPP

#include <string>
#include <map>


namespace blockchainSec {



std::string ethabi(std::string const& args);
std::unordered_map<std::string, std::string> ethabi_decode_log(std::string const& abiFile, std::string const& eventName, std::vector<std::string> & topics, std::string const& data);
std::string ethabi_decode_result(std::string const& abiFile, std::string const& eventName, std::string const& data);
std::vector<std::string> ethabi_decode_array(std::string const& abiFile, std::string const& eventName, std::string const& data);


}

#endif //__ETHABI_HPP
