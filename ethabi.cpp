#include <array>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <blockchainsec.hpp>


using namespace std;

namespace blockchainSec {



string ethabi(string args) {
	string result;
	array<char, PIPE_BUFFER_LENGTH> pipe_buffer;

#ifdef _DEBUG
	cout << "ethabi(): Requested '" << args << "'" << endl;
#endif //_DEBUG

	FILE *ethabi_pipe = popen(("ethabi " + args).c_str(), "r");
	if (ethabi_pipe == NULL) {
		// Failed to open pipe to ethabi -- is the binary installed and in $PATH?
		throw ResourceRequestFailedException("ethabi(): Failed to popen() pipe to ethabi binary. Is the binary installed and in the $PATH environment variable?");
	}
	while (fgets(pipe_buffer.data(), PIPE_BUFFER_LENGTH, ethabi_pipe) != NULL) {
		result += pipe_buffer.data();
	}
	if (pclose(ethabi_pipe) < 0) {
		throw ResourceRequestFailedException("ethabi(): Failed to pclose() pipe to ethabi binary!");
	}

#ifdef _DEBUG
	cout << "ethabi(): Call successful!" << endl;
	cout << "ethabi(): Returning: " << trim(result) << endl;
#endif //_DEBUG

	return boost::trim_copy(result);;
}



map<string, string> ethabi_decode_log(string abi_file, string event_name, string topics, string data) {
	string query, responce;
	vector<string> lines;
	map<string, string> parsedLog;

	query = "decode log '" + abi_file + "' " + event_name + " -l " + topics + " " + data;
	responce = ethabi(query);

	// TODO: Devise a way to detect success or failiure, as the exit status is always 0
	boost::split(lines, responce, boost::is_any_of("\n"));

	for (vector<string>::iterator iter = lines.begin(); iter != lines.end(); ++iter) {
		if (boost::trim_copy(*iter).compare("")) {
			continue;
		}
		//TODO: What if there is no space to sepatate key and value?
		string key = (*iter).substr(0, (*iter).find_first_of(" ") - 1); //TODO: is minus 1 correct?
		string value = (*iter).substr((*iter).find_first_of(" ") + 1);
		parsedLog[key] = value;
	}
	return parsedLog;
}



}