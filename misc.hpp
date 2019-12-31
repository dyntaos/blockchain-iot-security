#ifndef __MISC_H
#define __MISC_H


namespace blockchainSec {

// https://stackoverflow.com/questions/116038/how-do-i-read-an-entire-file-into-a-stdstring-in-c
std::string readFile2(const std::string &fileName);

// http://www.cplusplus.com/forum/beginner/251052/
std::string trim(const std::string& line);

}

#endif //__MISC_H
