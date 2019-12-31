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


// http://www.cplusplus.com/forum/beginner/251052/
string trim(const string& line) {
	const char* WhiteSpace = " \t\v\r\n";
	size_t start = line.find_first_not_of(WhiteSpace);
	size_t end = line.find_last_not_of(WhiteSpace);
	return start == end ? string() : line.substr(start, end - start + 1);
}

}
