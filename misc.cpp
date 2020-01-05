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



}
