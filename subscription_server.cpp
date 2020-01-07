#include <iostream>
#include <string>
#include <condition_variable>
#include <mutex>
#include <boost/asio.hpp>

#include <blockchainsec.hpp>
#include <ethabi.hpp>
#include <subscription_server.hpp>


using namespace std;
namespace blockchainSec {



EventLogWaitManager::EventLogWaitManager(string clientAddress, string contractAddress) {
	this->clientAddress = clientAddress;
	this->contractAddress = contractAddress;
}


unique_ptr<unordered_map<string, string>> EventLogWaitManager::getEventLog(string logID) {
	unordered_map<string, string> *element;
	mtx.lock();

	if (eventLogMap.count(logID) == 0) {
		eventLogMap.emplace(logID, make_unique<EventLogWaitElement>());
	}

	if (eventLogMap[logID]->hasWaitingThread) {
		mtx.unlock();
		throw ResourceRequestFailedException("Cannot request log which already has a thread waiting on it.");
	}
	eventLogMap[logID].get()->hasWaitingThread = true;

	if (!eventLogMap[logID].get()->hasEventLog) {
		mtx.unlock();
		while (!eventLogMap[logID].get()->hasEventLog) eventLogMap[logID].get()->cv.wait(eventLogMap[logID].get()->cvLock);
		mtx.lock();
	}

	element = new unordered_map<string, string>(*eventLogMap[logID].get()->eventLog.get());
	eventLogMap.erase(logID);
	mtx.unlock();

	return unique_ptr<unordered_map<string, string>>(element);
}



void EventLogWaitManager::setEventLog(string logID, unordered_map<string, string> eventLog) {
	mtx.lock();

	if (eventLogMap.count(logID) == 0) {
		eventLogMap.emplace(logID, make_unique<EventLogWaitElement>());
	}

	if (eventLogMap[logID].get()->hasEventLog) {
		mtx.unlock();
		throw ResourceRequestFailedException("logID \"" + logID + "\" already has an associated event log.");
	}

	eventLogMap[logID].get()->eventLog = unique_ptr<unordered_map<string, string>>(new unordered_map<string, string>(eventLog));
	eventLogMap[logID].get()->hasEventLog = true;

	if (eventLogMap[logID].get()->hasWaitingThread) {
		eventLogMap[logID].get()->cv.notify_all();
	}

	mtx.unlock();
}



void EventLogWaitManager::ipc_subscription_listener_thread(void) {
	const string events[] = {
		"Add_Device",
		"Add_Gateway",
		"Remove_Device",
		"Remove_Gateway",
		"Push_Data",
		"Update_Addr",
		"Authorize_Admin",
		"Deauthorize_Admin"
	};
	const string signatures[] = {
		"6d8b1348ac9490868dcf5de10f66764c3fd6abf3274c6d36fbf877fc9f6e798f",
		"3288eeac8a88a024bc145e835dbfecbe5095e00c717c48986f19943367e9fa20",
		"c3d811754f31d6181381ab5fbf732898911891abe7d32e97de73a1ea84c2e363",
		"0d014d0489a2ad2061dbf1dffe20d304792998e0635b29eda36a724992b6e5c9",
		"0924baadbe7a09acb87f9108bb215dea5664035966d186b4fa71905d11fe1b51",
		"8489be1d551a279fae5e4ed28b2a0aab728d48550f6a64375f627ac809ac2a80",
		"134c4a950d896d7c32faa850baf4e3bccf293ae2538943709726e9596ce9ebaf",
		"e96008d87980c624fca6a2c0ecc59bcef2ef54659e80a1333aff845ea113f160"
	};
	char recvbuff[4097];
	int recvlen;

	Json jsonData;
	string data;

	map<string, string> subscriptionToEventName;

	boost::asio::io_service io_service;
	boost::asio::local::stream_protocol::endpoint ep("/home/kale/.ethereum/geth.ipc"); //TODO: Remove hardcoded value
	boost::asio::local::stream_protocol::socket socket(io_service);

#ifdef _DEBUG
	cout << "ipc_subscription_listener_thread()" << endl;
#endif //_DEBUG

	//TODO: Setup separate subscriptions for each log and store subscription/event names
restart:
	socket.connect(ep);

	for (uint16_t i = 0; i < 8; i++) {
		data = "{\"id\":1,"
					"\"method\":\"eth_subscribe\","
					"\"params\":["
						"\"logs\",{"
							"\"address\":\"0x" + contractAddress + "\","
							"\"topics\":[\"" +
								"0x" + signatures[i] + "\"," +
								"\"0x000000000000000000000000" + clientAddress +
							"\"]"
						"}"
					"]"
				"}";

		socket.send(boost::asio::buffer(data.c_str(), data.length()));

		recvlen = socket.receive(boost::asio::buffer(recvbuff, 4096)); // TODO: What if the entire message is not received...
		recvbuff[recvlen] = 0;

		// TODO URGENT: What if a subscription comes before this loop completes?!
		// TODO: Should this be in a try catch? What to do if fails?
		Json responce = Json::parse(recvbuff);
		if (responce.count("error") > 0) {
			throw ResourceRequestFailedException("ipc_subscription_listener_thread(): Got an error responce to eth_subscribe!");
		}
		if (!responce["result"].is_string()) {
			throw ResourceRequestFailedException("ipc_subscription_listener_thread(): Unexpected responce to eth_subscribe received!");
		}
		string result = responce["result"];

		subscriptionToEventName[result] = events[i];
	}

	// TODO: What if the connection is closed by the other end?
	for (;;) {
		recvlen = socket.receive(boost::asio::buffer(recvbuff, 4096)); // TODO: What if the entire message is not received...
		recvbuff[recvlen] = 0;

		string method, subscription, data, transactionHash;
		vector<string> topics;
		Json resultJsonObject;

		try {
			jsonData = Json::parse(recvbuff); //TODO: Check for error? Try Catch?

			method = jsonData["method"];
			subscription = jsonData["params"]["subscription"];
			resultJsonObject = jsonData["params"]["result"];
			for (Json::iterator iter = resultJsonObject["topics"].begin(); iter != resultJsonObject["topics"].end(); ++iter) {
				string iterStr = *iter;
				iterStr = iterStr.substr(2);
				topics.push_back(iterStr);
			}
			data = resultJsonObject["data"];
			transactionHash = resultJsonObject["transactionHash"];

		} catch (const Json::exception &e) {
			//TODO: More granularity?
			cerr << "ipc_subscription_listener_thread(): JSON responce error in responce:"
				<< endl
				<< "\t"
				<< recvbuff
				<< endl
				<< e.what()
				<< endl;
			continue;
		}

		if (method.compare("eth_subscription") != 0) {
			// "method" field of the JSON data is not "eth_subscription"
			cerr << "ipc_subscription_listener_thread(): \"method\" field of JSON message is \"eth_subscription\"!"
				<< endl
				<< "\t"
				<< recvbuff
				<< endl
				<< endl;
			continue;
		}

		if (subscriptionToEventName.count(subscription) <= 0) {
			//TODO: The subscription does not exist! Something is out of sync! -- Unsubscribe and start again?
			cerr << "ipc_subscription_listener_thread(): Received a subscription hash that does not exist internally!"
				<< endl
				<< "\t"
				<< recvbuff
				<< endl
				<< endl;

			socket.close();
			subscriptionToEventName.clear();
			goto restart; // TODO: Refactor to remove this
		}

		unordered_map<string, string> log = ethabi_decode_log(ETH_CONTRACT_ABI, subscriptionToEventName[subscription], topics, data.substr(2));

		setEventLog(transactionHash, log);

		mtx.lock();
		cout << "\t"
			<< "Event log received:"
			<< endl
			<< "[\"" << transactionHash
			<< "\" (\""
			<< subscriptionToEventName[subscription]
			<< "\")] = "
			<< eventLogMap[transactionHash].get()->toString()
			<< endl << endl;
		mtx.unlock();


	}
	socket.close();
}



}
