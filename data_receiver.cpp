//#include <boost/algorithm/string/trim.hpp>
#include <iostream>
//#include <ethabi.hpp>
#include <data_receiver.hpp>


using namespace std;

namespace blockchainSec
{



DataReceiverManager::DataReceiverManager(
	BlockchainSecLib *blockchain,
	string const& clientAddress,
	string const& contractAddress,
	string const& ipcPath)
{
	this->blockchain = blockchain;
	this->clientAddress = clientAddress;
	this->contractAddress = contractAddress;
	this->ipcPath = ipcPath;

	receivedUpdatesCVLock = std::unique_lock<std::mutex>(receivedUpdatesLockMtx);

	receiverThread = new thread(&DataReceiverManager::dataReceiverThread, this);
}



void
DataReceiverManager::joinThread(void)
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
	uint16_t i;


restart: // TODO: Get rid of this

	try
	{
		socket.connect(ep);
	}
	catch (...)
	{
		throw ResourceRequestFailedException(
			"Failed to open Unix Domain Socket with Geth Ethereum client via \""
			+ ipcPath + "\"");
	}


	data = "{"
				"\"id\":1,"
				"\"method\":\"eth_subscribe\","
				"\"params\":["
					"\"logs\",{"
						"\"address\":\"0x" + contractAddress + "\","
						"\"topics\":[\"" + "0x" + contractLogSignatures[i].second + "\","
									"\"0x000000000000000000000000" + clientAddress + "\""
						"]"
					"}"
				"]"
			"}";


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


subParse: // TODO: Get rid of this

	message = subscribeParse.substr(0, subscribeParse.find_first_of('\n', 0));
	subscribeParse = subscribeParse.substr(subscribeParse.find_first_of('\n', 0) + 1);

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
			"Signature: "
			+ contractLogSignatures[i].second + "\n"
												"Request: "
			+ data + "\n"
						"Responce: "
			+ message + "\n");
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
	string result = jsonResponce["result"];

	subscriptionToEventName[result] = contractLogSignatures[i].first;


	boost::trim(subscribeParse);
	if (subscribeParse.length() > 0)
		goto subParse;


	receivedUpdatesDataMtx.lock();
	receivedUpdates.clear();
	receivedUpdatesDataMtx.unlock();

	try
	{
		subscribedReceiverDeviceID = blockchain->get_my_device_id();
	}
	catch (DeviceNotAssignedException& e)
	{
		subscribedReceiverDeviceID = 0;
	}
}



void
DataReceiverManager::dataReceiverThread(void)
{
	boost::asio::io_service io_service;
	boost::asio::local::stream_protocol::endpoint ep(ipcPath);
	boost::asio::local::stream_protocol::socket socket(io_service);
	char receiveBuffer[IPC_BUFFER_LENGTH];
	int receiveLength;
	Json jsonData, resultJsonObject;
	string data, message, method, subscription;
	vector<string> topics;

	dataReceiverThreadSetup(socket, ep);


	for (;;)
	{
		//receivedUpdatesDataMtx.lock();
		if (subscribedReceiverDeviceID != blockchain->get_my_device_id())
		{
			// Cancel existing subscription and recreate with new device ID
			// TODO
			socket.close();
			dataReceiverThreadSetup(socket, ep);
			// TODO: Clear JSON String buffers
		}

		if (wantSubscribedReceiverUpdates && !subscribedReceiverUpdates)
		{
			// Subscribe to Device_Data_Updated events

			try
			{
				subscribedReceiverDeviceID = blockchain->get_my_device_id();
			}
			catch (DeviceNotAssignedException& e)
			{
				subscribedReceiverDeviceID = 0;
			}

			if (subscribedReceiverDeviceID != 0)
			{
				receivedUpdates.clear();

				stringstream deviceIdSS;
				deviceIdSS << std::hex << subscribedReceiverDeviceID;
				string deviceIdHex = "00000000" + deviceIdSS.str();
				deviceIdHex = deviceIdHex.substr(deviceIdHex.length() - 8);

				data = "{"
						"\"id\":2,"
						"\"method\":\"eth_subscribe\","
						"\"params\":["
							"\"logs\",{"
								"\"address\":\"0x" + contractAddress + "\","
								"\"topics\":[\"" + "0x" SIGNATURE_DEVICE_DATA_UPDATED "\","
											"\"0x00000000000000000000000000000000000000000000000000000000" + deviceIdHex + "\""
								"]"
							"}"
						"]"
					"}";

				socket.send(boost::asio::buffer(data.c_str(), data.length()));
				subscribedReceiverUpdates = true;
			}
			else
			{
				wantSubscribedReceiverUpdates = false; // Don't spam GETH with requests and abort
				receivedUpdatesCV.notify_all();
			}
		}
		receivedUpdatesDataMtx.unlock();



		while (receiveParse.find_first_of('\n', 0) == string::npos)
		{
			receiveLength = socket.receive(boost::asio::buffer(receiveBuffer, IPC_BUFFER_LENGTH - 1));

			if (receiveLength == 0)
			{
				// Socket was closed by other end
				socket.close(); // TODO: What happens if this socket is already closed?
				dataReceiverThreadSetup(socket, ep);
			}
			receiveBuffer[receiveLength] = 0;
			receiveParse += receiveBuffer;
		}


		message = receiveParse.substr(0, receiveParse.find_first_of('\n', 0));
		receiveParse = receiveParse.substr(receiveParse.find_first_of('\n', 0) + 1);



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

		cout << "Got RECEIVED_DATA_UPDATED NOTIFICATION:" << endl;

		for (auto it = event.begin(); it != event.end(); ++it)
		{
			cout << "event[\"" << it->first << "\"] = \"" << it->second << "\"" << endl;
		}

		receivedUpdatesDataMtx.lock();
		receivedUpdates.insert(strtoul(event["device_id"], nullptr, 16));
		receivedUpdatesDataMtx.unlock();

	}
	socket.close();
}


} //namespace
