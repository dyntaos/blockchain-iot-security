#include <iostream>
#include <ethabi.hpp>
#include <data_receiver.hpp>


using namespace std;

namespace blockchainSec
{



DataReceiverManager::DataReceiverManager(BlockchainSecLib *blockchain)
{
	this->blockchain = blockchain;

	receivedUpdatesCVLock = std::unique_lock<std::mutex>(receivedUpdatesLockMtx);

	receiverThread = new thread(&DataReceiverManager::dataReceiverThread, this);
}



void
DataReceiverManager::joinThreads(void)
{
	if (receiverThread != NULL)
	{
		receiverThread->join();
	}
}



void
DataReceiverManager::dataReceiverThreadSetup(
	boost::asio::local::stream_protocol::socket& socket,
	boost::asio::local::stream_protocol::endpoint& ep)
{
	char receiveBuffer[IPC_BUFFER_LENGTH];
	string subscribeParse, data, message;
	Json jsonResponce;
	int receiveLength;


restart: // TODO: Get rid of this

	try
	{
		subscribedReceiverDeviceID = blockchain->get_my_device_id();
	}
	catch (DeviceNotAssignedException& e)
	{
		subscribedReceiverDeviceID = 0;
	}

	if (subscribedReceiverDeviceID == 0)
	{
		sleep(NO_DEVICE_RETRY_INTERVAL);
		goto restart;
	}

	stringstream deviceIdSS;
	deviceIdSS << std::hex << subscribedReceiverDeviceID;
	string deviceIdHex = "00000000" + deviceIdSS.str();
	deviceIdHex = deviceIdHex.substr(deviceIdHex.length() - 8);

	data = "{"
				"\"id\":1,"
				"\"method\":\"eth_subscribe\","
				"\"params\":["
					"\"logs\",{"
						"\"address\":\"0x" + blockchain->getContractAddress() + "\","
						"\"topics\":[\"" + "0x" SIGNATURE_DEVICE_DATA_UPDATED "\","
									"\"0x00000000000000000000000000000000000000000000000000000000" + deviceIdHex + "\""
						"]"
					"}"
				"]"
			"}";

	try
	{
		socket.connect(ep);
	}
	catch (...)
	{
		throw ResourceRequestFailedException(
			"Failed to open Unix Domain Socket with Geth Ethereum client via \""
			+ blockchain->getIPCPath() + "\"");
	}

#ifdef _DEBUG
	cout << "dataReceiverThreadSetup(): Sending subscription request: "
		<< endl
		<< data
		<< endl;
#endif //_DEBUG

	socket.send(boost::asio::buffer(data.c_str(), data.length()));


subscribeReceive: // TODO: Get rid of this

	while (subscribeParse.find_first_of('\n', 0) == string::npos)
	{
		receiveLength = socket.receive(boost::asio::buffer(receiveBuffer, IPC_BUFFER_LENGTH - 1));

		if (receiveLength == 0)
		{
			// Socket was closed by other end
			goto restart;
		}
		receiveBuffer[receiveLength] = 0;
		subscribeParse += receiveBuffer;
	}


	message = subscribeParse.substr(0, subscribeParse.find_first_of('\n', 0));
	subscribeParse = subscribeParse.substr(subscribeParse.find_first_of('\n', 0) + 1);

#ifdef _DEBUG
	cout << "dataReceiverThreadSetup(): Got reply:"
		<< endl
		<< message
		<< endl;
#endif //_DEBUG

	// TODO: Should this be in a try catch? What to do if fails?
	try
	{
		jsonResponce = Json::parse(message);
	}
	catch (const Json::exception& e)
	{
		cerr << "dataReceiverThread(): JSON responce error in responce while subscribing:"
				<< endl
				<< "\t"
				<< message
				<< endl
				<< e.what()
				<< endl;
		goto restart; // TODO: Remove this
	}

	if (jsonResponce.count("error") > 0)
	{
		throw ResourceRequestFailedException(
			"dataReceiverThread(): Got an error responce to eth_subscribe!\n"
			"Request: " + data +
			"\nResponce: " + message + "\n");
	}

	if (jsonResponce.contains("method") > 0)
	{
		receiveParse += message + "\n";
		goto subscribeReceive;
	}

	if (jsonResponce.count("result") == 0 || !jsonResponce["result"].is_string())
	{
		throw ResourceRequestFailedException(
			"dataReceiverThread(): Unexpected responce to eth_subscribe received!");
	}
	subscriptionHash = jsonResponce["result"];

	receivedUpdatesDataMtx.lock();
	receivedUpdates.clear();
	receivedUpdatesDataMtx.unlock();
}



void
DataReceiverManager::dataReceiverThread(void)
{
	boost::asio::io_service io_service;
	boost::asio::local::stream_protocol::endpoint ep(blockchain->getIPCPath().c_str());
	boost::asio::local::stream_protocol::socket socket(io_service);
	char receiveBuffer[IPC_BUFFER_LENGTH];
	int receiveLength;
	Json jsonData, resultJsonObject;
	string data, message, method, subscription;
	vector<string> topics;

	dataReceiverThreadSetup(socket, ep);

	for (;;)
	{
		if (subscribedReceiverDeviceID != blockchain->get_my_device_id())
		{
			// Cancel existing subscription and recreate with the new device ID
			socket.close();
			receiveParse = "";
			dataReceiverThreadSetup(socket, ep);
		}

		while (receiveParse.find_first_of('\n', 0) == string::npos)
		{
			receiveLength = socket.receive(boost::asio::buffer(receiveBuffer, IPC_BUFFER_LENGTH - 1));

			if (receiveLength == 0)
			{
				// Socket was closed by other end
				socket.close();
				receiveParse = "";
				dataReceiverThreadSetup(socket, ep);
				continue;
			}
			receiveBuffer[receiveLength] = 0;
			receiveParse += receiveBuffer;
		}

		message = receiveParse.substr(0, receiveParse.find_first_of('\n', 0));
		receiveParse = receiveParse.substr(receiveParse.find_first_of('\n', 0) + 1);

#ifdef _DEBUG
		cout << "dataReceiverThread(): Got reply:"
		<< endl
		<< message
		<< endl;
#endif //_DEBUG

		try
		{
			jsonData = Json::parse(message);

			method = jsonData["method"];
			subscription = jsonData["params"]["subscription"];
			resultJsonObject = jsonData["params"]["result"];

			topics.clear();

			for (Json::iterator iter = resultJsonObject["topics"].begin(); iter != resultJsonObject["topics"].end(); ++iter)
			{
				string iterStr = *iter;
				iterStr = iterStr.substr(2);
				topics.push_back(iterStr);
			}

			data = resultJsonObject["data"];
		}
		catch (const Json::exception& e)
		{
			cerr << "dataReceiverThread(): JSON responce error in responce:"
				 << endl
				 << "\t"
				 << message
				 << endl
				 << e.what()
				 << endl;
			continue;
		}

		if (method.compare("eth_subscription") != 0)
		{
			// "method" field of the JSON data is not "eth_subscription"
			cerr << "dataReceiverThread(): \"method\" field of JSON message is \"eth_subscription\"!"
				 << endl
				 << "\t"
				 << message
				 << endl
				 << endl;
			continue;
		}

		unordered_map<string, string> event = ethabi_decode_log(
												blockchain->getEthContractABI(),
												EVENT_LOG_NAME,
												topics,
												data.substr(2));

		receivedUpdatesDataMtx.lock();
		receivedUpdates.insert(strtoul(event["device_id"].c_str(), nullptr, 16)); // TODO: Try/Catch?
		receivedUpdatesDataMtx.unlock();

		receivedUpdatesCV.notify_one();
	}
	socket.close();
}



set<uint32_t>
DataReceiverManager::getReceivedChanges(void)
{
	receivedUpdatesDataMtx.lock();

	if (receivedUpdates.size() == 0)
	{
		receivedUpdatesDataMtx.unlock();
		while (receivedUpdates.size() == 0)
		{
			receivedUpdatesCV.wait(receivedUpdatesCVLock);
		}
		receivedUpdatesDataMtx.lock();
	}
	set<uint32_t> result = receivedUpdates;
	receivedUpdates.clear();

	receivedUpdatesDataMtx.unlock();

	return result;
}



} //namespace
