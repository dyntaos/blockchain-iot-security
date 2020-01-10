#ifndef __SUBSCRIPTION_SERVER_HPP
#define __SUBSCRIPTION_SERVER_HPP

#include <string>
#include <map>
#include <condition_variable>
#include <mutex>


namespace blockchainSec {


class EventLogWaitManager {
	public:
		EventLogWaitManager(std::string const& clientAddress, std::string const& contractAddress, std::string const& ipcPath);
		std::unique_ptr<std::unordered_map<std::string, std::string>> getEventLog(std::string const& logID);
		void setEventLog(std::string const& logID, std::unordered_map<std::string, std::string> const& eventLog);

		// Thread main function for geth log monitor thread
		void ipc_subscription_listener_thread(void);

	private:
		std::string contractAddress;
		std::string clientAddress;
		std::string ipcPath;

		struct EventLogWaitElement {
			std::mutex cvLockMtx;
			std::unique_lock<std::mutex> cvLock;
			std::condition_variable cv;
			bool hasWaitingThread = false;
			bool hasEventLog = false;
			std::unique_ptr<std::unordered_map<std::string, std::string>> eventLog; //TODO: Should this be a pointer?

			EventLogWaitElement() {
				cvLock = std::unique_lock<std::mutex>(cvLockMtx);
			}

			std::string toString(void) {
				std::string str = "{ ";
				for (std::pair<std::string, std::string> kv : *eventLog.get()) {
					str += "\"" + kv.first + "\":\"" + kv.second + "\", ";
				}
				str = str.substr(0, str.length() - 2);
				str += " }";
				return str;
			}
		};

		std::mutex mtx;
		std::unordered_map<std::string, std::unique_ptr<EventLogWaitElement>> eventLogMap;
};



}

#endif //__SUBSCRIPTION_SERVER_HPP