#ifndef __SUBSCRIPTION_SERVER_HPP
#define __SUBSCRIPTION_SERVER_HPP

#include <boost/asio.hpp>
#include <condition_variable>
#include <set>
#include <mutex>
#include <string>
#include <thread>
#include <blockchainsec.hpp>


#define EVENT_LOG_NAME                "Device_Data_Updated"
#define SIGNATURE_DEVICE_DATA_UPDATED "98e87e8e01e32169ab3efa3535263beca76318be8ede6a321bcbfcbf1e499d26"
#define NO_DEVICE_RETRY_INTERVAL      30

namespace blockchainSec
{


class DataReceiverManager
{
	public:

	DataReceiverManager(
		BlockchainSecLib *blockchain,
		std::string const& clientAddress,
		std::string const& contractAddress,
		std::string const& ipcPath);

	void joinThread(void);

	// Thread main function for the monitor thread
	void dataReceiverThread(void);

	private:
	std::thread* receiverThread = NULL;

	blockchainSec::BlockchainSecLib *blockchain = NULL;
	std::string contractAddress;
	std::string clientAddress;
	std::string ipcPath;

	std::string subscriptionHash;

	bool subscribedReceiverUpdates = false;
	bool wantSubscribedReceiverUpdates = false;
	uint32_t subscribedReceiverDeviceID = 0;
	std::set<uint32_t> receivedUpdates;
	std::mutex receivedUpdatesDataMtx;
	std::mutex receivedUpdatesLockMtx;
	std::unique_lock<std::mutex> receivedUpdatesCVLock;
	std::condition_variable receivedUpdatesCV;

	void dataReceiverThreadSetup(
		boost::asio::local::stream_protocol::socket& socket,
		boost::asio::local::stream_protocol::endpoint& ep);
};


} //namespace


#endif //__DATA_RECEIVER_HPP