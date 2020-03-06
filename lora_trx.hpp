#ifndef __LORA_TRX_HPP
#define __LORA_TRX_HPP


#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include <bcm2835.h>
#include <unistd.h>

#include <blockchainsec.hpp>
#include <packet.hpp>

#define BOARD_DRAGINO_PIHAT
#include <RasPiBoards.h>
#include <RH_RF95.h>


#define RF_FREQUENCY 915.10 // TODO: Move to config file


namespace blockchainSec
{



class LoraTrx
{

	private:
	BlockchainSecLib* blockchainSec;
	std::queue<struct packet*> rx_queue, tx_queue;
	std::mutex rx_queue_mutex, tx_queue_mutex;
	std::mutex rx_ulock_mutex;
	std::condition_variable rx_queue_condvar;
	std::thread *server_thread, *forwarder_thread;
	bool halt_server = true;
	bool hardwareInitialized = false;
	uint32_t gatewayDeviceId;

	static void serverThread(
		std::queue<struct packet*>& rx_queue,
		std::queue<struct packet*>& tx_queue,
		std::mutex& rx_queue_mutex,
		std::mutex& tx_queue_mutex,
		std::condition_variable& rx_queue_condvar,
		bool& halt_server,
		LoraTrx& trx);

	static void forwarderThread(
		bool& halt_server,
		LoraTrx& trx);

	bool setup(void);

	struct packet* readMessage(void);
	void processPacket(struct packet* p);
	bool verifySignature(struct packet* p);

	public:
	//	RH_RF95 rf95(RF_CS_PIN, RF_IRQ_PIN);

	LoraTrx(uint32_t gatewayDeviceId, BlockchainSecLib* blockchainSec);

	bool sendMessage(std::string msg_str, uint32_t toDeviceId);
	void server_init(void);
	void close_server(void);
};


} //namespace


#endif //__LORA_TRX_HPP
