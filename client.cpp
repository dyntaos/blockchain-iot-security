#include <iostream>
#include <blockchainsec.h>
#include <client.h>

using namespace std;
using namespace blockchainSec;

BlockchainSecLib *sec;

int main(int argc, char *argv[]) {
	sec = new BlockchainSecLib(IPC_PATH, ETH_MY_ADDR, ETH_CONTRACT_ADDR); //TODO Load these from config file

	cout << ".:Blockchain Security Framework Client:." << endl;

#ifdef _DEBUG
	sec->test();
#endif //_DEBUG

	cout << "Double 7: " << sec->contract_double(7) << endl;
	cout << "Double 11: " << sec->contract_double(11) << endl;
	cout << "Double 13: " << sec->contract_double(13) << endl;

	return 0;
}
