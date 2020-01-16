#include <iostream>
#include <string>
#include <condition_variable>
#include <mutex>
#include <boost/asio.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <blockchainsec.hpp>
#include <ethabi.hpp>
#include <subscription_server.hpp>


using namespace std;
namespace blockchainSec {



EventLogWaitManager::EventLogWaitManager(string const& clientAddress, string const& contractAddress, string const& ipcPath) {
	this->clientAddress = clientAddress;
	this->contractAddress = contractAddress;
	this->ipcPath = ipcPath;
}


unique_ptr<unordered_map<string, string>> EventLogWaitManager::getEventLog(string const& logID) {
	unordered_map<string, string> *element;

#ifdef _DEBUG
	cout << "EventLogWaitManager::getEventLog(): Acquiring mutex..." << endl << endl;
#endif //_DEBUG

	mtx.lock();

#ifdef _DEBUG
	cout << "EventLogWaitManager::getEventLog(): Mutex acquired..." << endl << endl;
#endif //_DEBUG

	if (eventLogMap.count(logID) == 0) {
		eventLogMap.emplace(logID, make_unique<EventLogWaitElement>());
	}

	if (eventLogMap[logID]->hasWaitingThread) {
		mtx.unlock();
		throw ResourceRequestFailedException("Cannot request log which already has a thread waiting on it.");
	}
	eventLogMap[logID].get()->hasWaitingThread = true;

	if (!eventLogMap[logID].get()->hasEventLog) {
#ifdef _DEBUG
	cout << "EventLogWaitManager::getEventLog(): Releasing mutex and waiting on condition variable \""
		<< logID << "\"..." << endl << endl;
#endif //_DEBUG
		mtx.unlock();
		while (!eventLogMap[logID].get()->hasEventLog) eventLogMap[logID].get()->cv.wait(eventLogMap[logID].get()->cvLock);
		mtx.lock();
	}

	element = new unordered_map<string, string>(*eventLogMap[logID].get()->eventLog.get());
	eventLogMap.erase(logID);
	mtx.unlock();

#ifdef _DEBUG
	cout << "EventLogWaitManager::getEventLog(): Mutex released and returning..." << endl << endl;
#endif //_DEBUG

	return unique_ptr<unordered_map<string, string>>(element);
}



void EventLogWaitManager::setEventLog(string const& logID, unordered_map<string, string> const& eventLog) {
	mtx.lock();

	if (eventLogMap.count(logID) == 0) {

#ifdef _DEBUG
	cout << "EventLogWaitManager::setEventLog(): Create EventLogWaitElement \""
		<< logID << "\"..." << endl << endl;
#endif //_DEBUG

		eventLogMap.emplace(logID, make_unique<EventLogWaitElement>());
	}
#ifdef _DEBUG
	 else {
		 cout << "EventLogWaitManager::setEventLog(): EventLogWaitElement \""
			<< logID << "\" already exists..." << endl << endl;
	 }
#endif //_DEBUG


	if (eventLogMap[logID].get()->hasEventLog) {
		mtx.unlock();
		throw ResourceRequestFailedException("logID \"" + logID + "\" already has an associated event log.");
	}

	eventLogMap[logID].get()->eventLog = unique_ptr<unordered_map<string, string>>(new unordered_map<string, string>(eventLog));
	eventLogMap[logID].get()->hasEventLog = true;

	if (eventLogMap[logID].get()->hasWaitingThread) {

#ifdef _DEBUG
	cout << "EventLogWaitManager::setEventLog(): Notfiy condition variable \""
		<< logID << "\"..." << endl << endl;
#endif //_DEBUG

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
		"Update_DataReceiver",
		"Set_Default_DataReceiver",
		"Update_Addr",
		"Update_PublicKey",
		"Authorize_Admin",
		"Deauthorize_Admin"
	};
	const string signatures[] = {
		"91f9cfa89e92f74404a9e92923329b12ef1b50b3d6d57acd9167d5b9e5e4fe01",
		"ee7c8e0cb00212a30df0bb395130707e3e320b32bae1c79b3ee3c61cbf3c7671",
		"c3d811754f31d6181381ab5fbf732898911891abe7d32e97de73a1ea84c2e363",
		"0d014d0489a2ad2061dbf1dffe20d304792998e0635b29eda36a724992b6e5c9",
		"0924baadbe7a09acb87f9108bb215dea5664035966d186b4fa71905d11fe1b51",
		"e21f6cd2771fa3b4f5641e2fd1a3d52156a9a8cc10da311d5de41a5755ca6acf",
		"adf201dc3ee5a3915c67bf861b4c0ec432dded7b6a82938956e1f411c5636287",
		"8489be1d551a279fae5e4ed28b2a0aab728d48550f6a64375f627ac809ac2a80",
		"9f99e7c31d775c4f75816a8e1a0655e1e5f5bab88311d820d261ebab2ae8d91f",
		"134c4a950d896d7c32faa850baf4e3bccf293ae2538943709726e9596ce9ebaf",
		"e96008d87980c624fca6a2c0ecc59bcef2ef54659e80a1333aff845ea113f160"
	};
	char receiveBuffer[IPC_BUFFER_LENGTH];
	int receiveLength;
	uint16_t i;

	Json jsonData, resultJsonObject, jsonResponce;
	string subscribeParse, receiveParse, data, message, method, subscription, transactionHash;
	vector<string> topics;
	map<string, string> subscriptionToEventName;

	boost::asio::io_service io_service;
	boost::asio::local::stream_protocol::endpoint ep(ipcPath);
	boost::asio::local::stream_protocol::socket socket(io_service);

#ifdef _DEBUG
	cout << "ipc_subscription_listener_thread()" << endl;
#endif //_DEBUG

restart: // TODO: Get rid of this

	socket.connect(ep);

	for (i = 0; i < 10; i++) { // TODO URGENT: Dynamically set upper bound****
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

subscribeReceive: // TODO: Get rid of this

		while (subscribeParse.find_first_of('\n', 0) == string::npos) {
			receiveLength = socket.receive(boost::asio::buffer(receiveBuffer, IPC_BUFFER_LENGTH - 1));
			if (receiveLength == 0) {
				// Socket was closed by other end
				goto restart;
			}
			receiveBuffer[receiveLength] = 0;
			subscribeParse += receiveBuffer;
		}

subParse:  // TODO: Get rid of this

		message = subscribeParse.substr(0, subscribeParse.find_first_of('\n', 0));
		subscribeParse = subscribeParse.substr(subscribeParse.find_first_of('\n', 0) + 1);

		// TODO: Should this be in a try catch? What to do if fails?
		try {
			jsonResponce = Json::parse(message);
		} catch (const Json::exception &e) {
			cerr << "ipc_subscription_listener_thread(): JSON responce error in responce while subscribing:"
				<< endl
				<< "\t"
				<< message
				<< endl
				<< e.what()
				<< endl;
			goto restart; // TODO: Remove this
		}

		if (jsonResponce.count("error") > 0) {
			cerr << "\ti = " << i << endl << endl; //TODO: REMOVE!
			throw ResourceRequestFailedException(
				"ipc_subscription_listener_thread(): Got an error responce to eth_subscribe!\n"
				"Signature: " + signatures[i] + "\n"
 				"Request: " + data + "\n"
				"Responce: " + message + "\n"
			);
		}

		if (jsonResponce.contains("method") > 0) {
			receiveParse += message + "\n";
			goto subscribeReceive;
		}

		if (jsonResponce.count("result") == 0 || !jsonResponce["result"].is_string()) {
			throw ResourceRequestFailedException("ipc_subscription_listener_thread(): Unexpected responce to eth_subscribe received!");
		}
		string result = jsonResponce["result"];

		subscriptionToEventName[result] = events[i];
	}

	boost::trim(subscribeParse);
	if (subscribeParse.length() > 0) goto subParse;

	for (;;) {
		while (receiveParse.find_first_of('\n', 0) == string::npos) {
			receiveLength = socket.receive(boost::asio::buffer(receiveBuffer, IPC_BUFFER_LENGTH - 1));
			if (receiveLength == 0) {
				// Socket was closed by other end
				socket.close(); // TODO: What happens if this socket is already closed?
				subscriptionToEventName.clear();
				goto restart;
			}
			receiveBuffer[receiveLength] = 0;
			receiveParse += receiveBuffer;
		}

		message = receiveParse.substr(0, receiveParse.find_first_of('\n', 0));
		receiveParse = receiveParse.substr(receiveParse.find_first_of('\n', 0) + 1);

		try {
			jsonData = Json::parse(message);

			method = jsonData["method"];
			subscription = jsonData["params"]["subscription"];
			resultJsonObject = jsonData["params"]["result"];

			topics.clear();

			for (Json::iterator iter = resultJsonObject["topics"].begin(); iter != resultJsonObject["topics"].end(); ++iter) {
				string iterStr = *iter;
				iterStr = iterStr.substr(2);
				topics.push_back(iterStr);
			}

			data = resultJsonObject["data"];
			transactionHash = resultJsonObject["transactionHash"];

		} catch (const Json::exception &e) {
			cerr << "ipc_subscription_listener_thread(): JSON responce error in responce:"
				<< endl
				<< "\t"
				<< message
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
				<< message
				<< endl
				<< endl;
			continue;
		}

		if (subscriptionToEventName.count(subscription) <= 0) {cout << "Count topics2: " << topics.size() << endl;
			//TODO: The subscription does not exist! Something is out of sync! -- Unsubscribe and start again?
			cerr << "ipc_subscription_listener_thread(): Received a subscription hash that does not exist internally!"
				<< endl
				<< "\t"
				<< message
				<< endl
				<< endl;

			socket.close();
			subscriptionToEventName.clear();
			goto restart; // TODO: Refactor to remove this
		}

		unordered_map<string, string> log = ethabi_decode_log(ETH_CONTRACT_ABI, subscriptionToEventName[subscription], topics, data.substr(2));
		log["EventName"] = subscriptionToEventName[subscription];
		setEventLog(transactionHash, log);

#ifdef _DEBUG
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
#endif //_DEBUG

	}
	socket.close();
}



}
