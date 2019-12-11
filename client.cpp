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

	/*cout << "Double 7: " << sec->contract_double(7) << endl;
	cout << "Double 11: " << sec->contract_double(11) << endl;
	cout << "Double 13: " << sec->contract_double(13) << endl;*/

	cout << "getvar: " << sec->contract_getvar() << endl;
	cout << "setvar: " << sec->contract_setvar(5) << endl;
	cout << "getvar: " << sec->contract_getvar() << endl;
	cout << "setvar: " << sec->contract_setvar(92) << endl;
	cout << "getvar: " << sec->contract_getvar() << endl;

	cout << "setmap(33, 75): " << sec->contract_setmap(33, 75) << endl;
	cout << "getmap(33): " << sec->contract_getmap(33) << endl;
	cout << "setmap(41, 69): " << sec->contract_setmap(41, 69) << endl;
	cout << "getmap(41): " << sec->contract_getmap(41) << endl;
	cout << "setmap(887, 990): " << sec->contract_setmap(887, 990) << endl;
	cout << "getmap(887): " << sec->contract_getmap(887) << endl;


	return 0;
}
