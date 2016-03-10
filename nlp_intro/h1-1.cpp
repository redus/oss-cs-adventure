#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>


/***
 *	ADT: word counts
 *	populate word-tag count and tag seq count, and
 *  give e(word|TAG) using maximum likelihood probability
 **/

class word_count{
	// words: count of tag associated with words
	//		input: <tag>|<word>
	//		output: count of word corresponding to tag
	// gram: n-gram sequence count. 
	// 		input: tag sequence concated by _
	// 		output: count of that sequence
	std::unordered_map<std::string, int> words, gram;

public:
	// ctor
	word_count(std::string filename){
		read_file(filename);
	}

	// EFF: populate words and gram from filename
	void read_file(std::string filename){
		std::ifstream fin;
		fin.open(filename);

		while(fin.good()){
			int num;
			std::string type, tag, word;
			fin >> num >> type;

			if (type == "WORDTAG"){
				fin >> tag >> word;
				std::string key = tag + "|" + word;
				words[key] = num;
			} else {
				int n = type[0] - '0';

				fin >> tag;
				std::string key = tag;

				for (int i = 1; i < n; ++i){
					fin >> tag;
					key = key + "_" +tag;
				}

				gram[key] = num;
			}
		}
		// for (auto i = gram.cbegin(); i != gram.cend(); ++i){
		// 	std::cout << i->first << " " << i->second << std::endl;
		// }
		fin.close();
	}

	// REQ: x is the word, y is TAG
	// EFF: e(x|y) = count(y -> x) / count(x)
	double e(std::string x, std::string y){
		std::string key = y + "|" + x;
		return (double) words[key] / gram[y];
	}

};



int main(int argc, char** argv){
	if (argc < 3){
		std::cout << "USAGE: word_count word TAG" << std::endl;
		exit(1);
	}

	std::string x = argv[1];
	std::string y = argv[2];
	word_count wc("gene.counts");
	std::cout << "e(" << x << ", " << y << ") = " << wc.e(x, y) << std::endl;
	return 0;
}
