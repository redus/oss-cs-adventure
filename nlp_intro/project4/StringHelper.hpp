#pragma once
#include <vector>
#include <sstream>

class StringHelper{
public:
	// vector of string
	typedef std::vector<std::string> string_vec;


	static string_vec split(const std::string &s, char delim) {
		string_vec elems;
	    std::stringstream ss(s);
	    std::string item;
	    while (std::getline(ss, item, delim)) {
	        elems.push_back(item);
	    }
	    return elems;
	}

	static std::string join(const string_vec &tokens, char delim) {
	    std::string result = tokens.size() > 0 ? tokens[0] : "";
	    for (int i = 1; i < tokens.size(); ++i)
	    {
	    	result += delim + tokens[i];
	    }
	    return result;
	}

};