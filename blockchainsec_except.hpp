#ifndef __BLOCKCHAINSEC_EXCEPTIONS_HPP
#define __BLOCKCHAINSEC_EXCEPTIONS_HPP

#include <string>
#include <stdexcept>


namespace blockchainSec {


class BlockchainSecLibException : public std::runtime_error {
	public:
		BlockchainSecLibException(const char* msg) : std::runtime_error(msg) {}
		BlockchainSecLibException(const std::string& msg) : std::runtime_error(msg) {}
};



class TransactionException : public BlockchainSecLibException {
	public:
		TransactionException(const char* msg) : BlockchainSecLibException(msg) {}
		TransactionException(const std::string& msg) : BlockchainSecLibException(msg) {}
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



class InvalidArgumentException : public BlockchainSecLibException {
	public:
		InvalidArgumentException (const char* msg) : BlockchainSecLibException(msg) {}
		InvalidArgumentException (const std::string& msg) : BlockchainSecLibException(msg) {}
};



class ResourceRequestFailedException : public BlockchainSecLibException {
	public:
		ResourceRequestFailedException (const char* msg) : BlockchainSecLibException(msg) {}
		ResourceRequestFailedException (const std::string& msg) : BlockchainSecLibException(msg) {}
};

class ThreadExistsException : public BlockchainSecLibException {
	public:
		ThreadExistsException (const char* msg) : BlockchainSecLibException(msg) {}
		ThreadExistsException (const std::string& msg) : BlockchainSecLibException(msg) {}
};



class CryptographicException : public BlockchainSecLibException {
	public:
		CryptographicException (const char* msg) : BlockchainSecLibException(msg) {}
		CryptographicException (const std::string& msg) : BlockchainSecLibException(msg) {}
};

class CryptographicFailureException : public CryptographicException {
	public:
		CryptographicFailureException (const char* msg) : CryptographicException(msg) {}
		CryptographicFailureException (const std::string& msg) : CryptographicException(msg) {}
};

class CryptographicKeyMissmatchException : public CryptographicException {
	public:
		CryptographicKeyMissmatchException (const char* msg) : CryptographicException(msg) {}
		CryptographicKeyMissmatchException (const std::string& msg) : CryptographicException(msg) {}
};



}

#endif //__BLOCKCHAINSEC_EXCEPTIONS_HPP
