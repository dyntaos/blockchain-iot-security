#ifndef __CONSOLE_HPP
#define __CONSOLE_HPP

#include <thread>
#include <vector>
#include <blockchainsec.hpp>



namespace blockchainSec {


class BlockchainSecConsole {
	private:
		std::thread *consoleThread;
		bool initialized = false;
		bool haltThread = true;

		std::vector<std::string> tokenize(std::string input);
		static void console(
			blockchainSec::BlockchainSecLib & blockchainSec,
			BlockchainSecConsole & self
		);

		void processCommand(std::vector<std::string> & cmds, BlockchainSecLib & blockchainSec);

	public:
		BlockchainSecConsole() {};

		void startThread(blockchainSec::BlockchainSecLib & blockchainSec);
		void stopThread(void);

};


}


#endif // __CONSOLE_HPP