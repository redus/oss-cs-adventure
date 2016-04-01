#include "json/json.h"
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>



// EFF: replace rare(< 5 occurrences)
void replace_rare(){
	// construct json from raw json data
	Json::Value root;
	std::ifstream json, original;
	std::ofstream copy;
	json.open("parse_train.json");
	original.open("parse_train.dat")

	Json::CharReaderBuilder rbuilder;
	rbuilder["collectComments"] = false;
	rbuilder["strictRoot"] = false;
	std::string errs;
	Json::parseFromStream(rbuilder, fin, &root, &errs);
	
	fin.close();
	// count all the word ocurrences into hash (key: word, val: count)
	// filter the hash for less than 5 occurrences
	// if there's a match in parse_train.dat


	
}

int main(){
	replace_rare();
	return 0;
}