#ifndef __BLOCKCHAINSEC_EXCEPTIONS_HPP
#define __BLOCKCHAINSEC_EXCEPTIONS_HPP

#include <string>
#include <stdexcept>


namespace blockchainSec {



class TransactionException : public std::runtime_error {
	public:
		TransactionException(const char* msg) : std::runtime_error(msg) {}
		TransactionException(const std::string& msg) : std::runtime_error(msg) {}
};

class CallFailedException : public TransactionException {
	public:
		CallFailedException(const char* msg) : TransactionException(msg) {}
		CallFailedException(const std::string& msg) : TransactionException(msg) {}
};

class TransactionFailedException : public TransactionException {
	public:
		TransactionFailedException(const char* msg) : TransactionException(msg) {}
		TransactionFailedException(const std::string& msg) : TransactionException(msg) {}
};




class InvalidArgumentException : public std::runtime_error {
	public:
		InvalidArgumentException (const char* msg) : std::runtime_error(msg) {}
		InvalidArgumentException (const std::string& msg) : std::runtime_error(msg) {}
};



class ResourceRequestFailedException : public std::runtime_error {
	public:
		ResourceRequestFailedException (const char* msg) : std::runtime_error(msg) {}
		ResourceRequestFailedException (const std::string& msg) : std::runtime_error(msg) {}
};



}

#endif //__BLOCKCHAINSEC_EXCEPTIONS_HPP
