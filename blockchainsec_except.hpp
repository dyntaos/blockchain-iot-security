#ifndef __BLOCKCHAINSEC_EXCEPTIONS_HPP
#define __BLOCKCHAINSEC_EXCEPTIONS_HPP

#include <string>
#include <stdexcept>


namespace blockchainSec {


class JsonException : public std::runtime_error {
	public:
		JsonException(const char* msg) : std::runtime_error(msg) {}
		JsonException(const std::string& msg) : std::runtime_error(msg) {}
};


class JsonElementNotFoundException : public JsonException {
	public:
		JsonElementNotFoundException(const char* msg) : JsonException(msg) {}
		JsonElementNotFoundException(const std::string& msg) : JsonException(msg) {}
};


class JsonDataException : public JsonException {
	public:
		JsonDataException(const char* msg) : JsonException(msg) {}
		JsonDataException(const std::string& msg) : JsonException(msg) {}
};


class JsonTypeException : public JsonException {
	public:
		JsonTypeException(const char* msg) : JsonException(msg) {}
		JsonTypeException(const std::string& msg) : JsonException(msg) {}
};


class JsonNullException : public JsonTypeException {
	public:
		JsonNullException(const char* msg) : JsonTypeException(msg) {}
		JsonNullException(const std::string& msg) : JsonTypeException(msg) {}
};




class TransactionException : public std::runtime_error {
	public:
		TransactionException(const char* msg) : std::runtime_error(msg) {}
		TransactionException(const std::string& msg) : std::runtime_error(msg) {}
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




}

#endif //__BLOCKCHAINSEC_EXCEPTIONS_HPP
