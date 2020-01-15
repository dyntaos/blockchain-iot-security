#include <string>
#include <fstream>
#include <vector>
#include <boost/algorithm/string/replace.hpp>

#include <misc.hpp>


using namespace std;

namespace blockchainSec {



// https://stackoverflow.com/questions/116038/how-do-i-read-an-entire-file-into-a-stdstring-in-c
string readFile2(string const& fileName) {
	ifstream ifs(fileName.c_str(), ios::in | ios::binary | ios::ate);

	ifstream::pos_type fileSize = ifs.tellg();
	ifs.seekg(0, ios::beg);

	vector<char> bytes(fileSize);
	ifs.read(bytes.data(), fileSize);

	return string(bytes.data(), fileSize);
}



bool isHex(string const& str) {
	for (char *s = (char*) str.c_str(); *s != 0; s++) {
		if ((*s < 48) || (*s > 70 && *s < 97) || (*s > 102)) return false;
	}
	return true;
}



bool isEthereumAddress(string const& str) {
	return isHex(str) && str.length() > 0 && str.length() <= 40;
}



string escapeSingleQuotes(string const& str) {
	return boost::replace_all_copy(str, "'", "'\\''");
}



// https://stackoverflow.com/questions/17261798/converting-a-hex-string-to-a-byte-array
vector<char> hexToBytes(string const& hex) {
	vector<char> bytes;

	for (unsigned int i = 0; i < hex.length(); i += 2) {
		string byteString = hex.substr(i, 2);
		char byte = (char) strtol(byteString.c_str(), NULL, 16);
		bytes.push_back(byte);
	}
	return bytes;
}



}
