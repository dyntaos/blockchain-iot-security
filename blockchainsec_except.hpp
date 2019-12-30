#ifndef __BLOCKCHAINSEC_EXCEPTIONS_HPP
#define __BLOCKCHAINSEC_EXCEPTIONS_HPP

#include <string>
#include <stdexcept>


namespace blockchainSec {


class jsonTypeException : public std::runtime_error {
	public:
		jsonTypeException(const char* msg) : std::runtime_error(msg) {}
		jsonTypeException(const std::string& msg) : std::runtime_error(msg) {}
};


class jsonNullException : public virtual jsonTypeException {
	public:
		jsonNullException(const char* msg) : jsonTypeException(msg) {}
		jsonNullException(const std::string& msg) : jsonTypeException(msg) {}
};


}

#endif //__BLOCKCHAINSEC_EXCEPTIONS_HPP
