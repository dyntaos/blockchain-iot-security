#ifndef __LORA_TRX_HPP
#define __LORA_TRX_HPP


#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

#include <bcm2835.h>
#include <unistd.h>
//#include <stdio.h>

//#include <RH_RF69.h>
#include <RH_RF95.h>


#define BOARD_DRAGINO_PIHAT

#include <RasPiBoards.h>


#define RF_FREQUENCY		915.10
#define RF_NODE_ID			1


namespace blockchainSec {





typedef struct lora_msg_t {
	uint8_t id;
	uint32_t from;
	uint32_t to;
	uint8_t flags;
	int8_t  rssi;
	uint8_t len;
	uint8_t *data;
} lora_msg;



class LoraTrx {

	private:

		std::queue<lora_msg*> rx_queue, tx_queue;
		std::mutex rx_queue_mutex, tx_queue_mutex;
		std::mutex rx_ulock_mutex;
		std::condition_variable rx_queue_condvar;
		std::thread *server_thread;
		bool halt_server = true;

		static void server(
				std::queue<lora_msg*> &rx_queue,
				std::queue<lora_msg*> &tx_queue,
				std::mutex &rx_queue_mutex,
				std::mutex &tx_queue_mutex,
				std::condition_variable &rx_queue_condvar,
				bool &halt_server,
				LoraTrx &trx
		);


	public:
	//	RH_RF95 rf95(RF_CS_PIN, RF_IRQ_PIN);
		bool _hardwareInitialized = false;

		LoraTrx(void);

		std::string readMessage(void);
		bool sendMessage(std::string msg_str);
		void server_init(void);
		void close_server(void);
		bool _setup(void);
};


}


#endif // __LORA_TRX_HPP
