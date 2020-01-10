#include <string>
#include <fstream>
#include <vector>

#include <misc.hpp>


using namespace std;

namespace blockchainSec {



// https://stackoverflow.com/questions/116038/how-do-i-read-an-entire-file-into-a-stdstring-in-c
string readFile2(const string &fileName) {
	ifstream ifs(fileName.c_str(), ios::in | ios::binary | ios::ate);

	ifstream::pos_type fileSize = ifs.tellg();
	ifs.seekg(0, ios::beg);

	vector<char> bytes(fileSize);
	ifs.read(bytes.data(), fileSize);

	return string(bytes.data(), fileSize);
}



bool isHex(string str) {
	for (char *s = (char*) str.c_str(); *s != 0; s++) {
		if ((*s < 48) || (*s > 70 && *s < 97) || (*s > 102)) return false;
	}
	return true;
}



bool isEthereumAddress(string str) {
	return isHex(str) && str.length() > 0 && str.length() <= 40;
}



}
