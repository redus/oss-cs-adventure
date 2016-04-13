#include "json/json.h"
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <sstream>
#include <regex>

using string_vec = std::vector<std::string>;
const std::string RARE = "_RARE_";

// REQ: root is a tree of single sentence
// EFF: helper for replace_tree.
//		recursively read until it reaches the word, and
//		replace rare(< 5 occurrences) words with RARE
void walk_tree(Json::Value &root, std::unordered_map<std::string, int> 
	&count){
	if (root.size() == 2) {
		std::string word = root[1].asString();
		if (count[word] < 5){
			root[1] = RARE;
		}
		return;	
	} else if (root.size() == 3){
		walk_tree(root[1], count);
		walk_tree(root[2], count);
	} else {
		std::cerr << "json: tree with 1 node found" << std::endl;
		return;
	}
}

// EFF: replace rare(< 5 occurrences)words with RARE
void replace_rare(){
	// hash word->count from "cfg.counts"
	std::unordered_map<std::string, int> count;
	std::ifstream count_in;
	count_in.open("cfg.counts");

	std::string num, rule, word, dump;
	while (count_in >> num >> rule){
		if (rule == "UNARYRULE"){
			// consume POS (irrelevant) and read in word
			count_in >> dump >> word;
			count[word] += std::stoi(num);
		} else {
			std::getline(count_in, dump);
		}
	}
	count_in.close();

	// for (auto it = count.cbegin(); it != count.cend(); ++it){
	// 	std::cout << it->first << ": " << it->second << std::endl;
	// }
	std::ifstream fin;
	std::ofstream fout;
	fin.open("parse_train.dat");
	fout.open("parse_rare_train.dat");
	std::string line;

	while(std::getline(fin, line)){
		std::stringstream sin;
		sin << line;

		Json::Value root;
		Json::CharReaderBuilder rbuilder;
		rbuilder["collectComments"] = false;
		std::string errs;
		Json::parseFromStream(rbuilder, sin, &root, &errs);

		walk_tree(root, count);
		std::stringstream sout;
		sout << root;
		std::string s = sout.str();

		// regex is not working at all
		// fix in sublime after output ugh
		// std::regex word_front(R"(,\n\t+)");
		// std::regex blanks(R"(\n\t+)");
		// std::cout << std::regex_search(s, blanks) << std::endl;
		// s = std::regex_replace(s, word_front, std::string(", "));
		// s = std::regex_replace(s, blanks, std::string(""));
		// std::cout << s << std::endl;
		fout << s << '\n';
	}
	
	fin.close();
	fout.close();
	
}

string_vec split(const std::string &s, char delim) {
	string_vec elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

class CKY{
	template <typename T>
	using string_T_map = std::unordered_map<std::string, T>;
	using pi_table = std::vector<std::vector<string_T_map<double>>>;
	using bp_pair = std::pair<std::string, unsigned>;
	using bp_table = std::vector<std::vector<string_T_map<bp_pair>>>;
	
	// flatten several keys to one using DELIMITER
	// ex. X, Y1, Y2 (3 keys) -> X_Y1_Y2 (1 key)
	const char DELIMITER = '_';

	// key: string of pos/word combination with DELIMITER
	// value: count of occurences
	std::unordered_map<std::string, unsigned> nonterminal, unary, binary, word_count;

public:
	CKY(std::string train_data){
		read_counts(train_data);
	}

	// REQ: train_data is the name of file containing following template only.
	//		1. <count> NONTERMINAL <POS>
	//		2. <count> UNARYRULE <POS> <word>
	//		3. <count> BINARYRULE <POS1> <POS2> <POS3>
	// EFF: populate counts of nonterminal, unary, and binary rule.
	void read_counts(std::string train_data){
		std::ifstream fin;
		fin.open(train_data);
		unsigned count;
		std::string rule, word, pos1, pos2, pos3;

		while(fin >> count >> rule){
			if (rule == "NONTERMINAL"){
				fin >> pos1;
				nonterminal[pos1] = count;
			} else if (rule == "UNARYRULE"){
				fin >> pos1 >> word;
				std::string key = pos1 + DELIMITER + word;
				unary[key] = count;
				word_count[word] = count;
			} else if (rule == "BINARYRULE") {
				fin >> pos1 >> pos2 >> pos3;
				// std::string key = pos2 + DELIMITER + pos3;
				// binary[pos1][key] = count;
				std::string key = pos1 + DELIMITER + pos2 + DELIMITER + pos3;
				binary[key] = count;
			} else {
				// dump line and continue
				std::cerr << "Ignoring line that doesn't follow the rule" << std::endl;
				std::getline(fin, rule);
			}
		}
		fin.close();
	}


	// EFF: return count(POS -> word) / count(POS)
	double unary_ML(const std::string &pos, const std::string &word){
		std::string key = "";
		if (word_count.count(word) > 0){
			key = pos + DELIMITER + word;
		} else {
			key = pos + DELIMITER + RARE;
		}
		return unary[key] / (double) nonterminal[pos];
		
	}

	// EFF: return count(pos1 -> pos2 pos3) / count(pos1)
	double binary_ML(const std::string &pos1, const std::string &pos2, 
		const std::string &pos3){
		// return binary[pos1][pos2 + DELIMITER + pos3] / 
		// 	(double) nonterminal[pos1];
		return binary[pos1 + DELIMITER + pos2 + DELIMITER + pos3] / 
			(double) nonterminal[pos1];
	}


	// EFF: initialize DP tables
	void init_tables(pi_table &pi, bp_table &bp, const string_vec &words){
		for (unsigned i = 0; i < words.size(); ++i){
			pi[i].resize(words.size());
			for (auto it = nonterminal.cbegin(); it != nonterminal.cend(); ++it){
				pi[i][i][it->first] = unary_ML(it->first, words[i]);
			}
		}

		for (unsigned i = 0; i < words.size(); ++i){
			bp[i].resize(words.size());
		}
		// for (unsigned i = 0; i < pi.size(); i++){
		// 	for (unsigned j = 0; j < pi[i].size(); ++j){
		// 		std::cout << pi[i][j]["NOUN"] << ' ';
		// 	}
		// 	std::cout << std::endl;
		// }
		// std::cout << std::endl;
	}

	// EFF: backtrack using bp
	void backtrack(bp_table &bp, unsigned i, unsigned j, std::string &pos, 
		string_vec &words, Json::Value &result){
		if (i == j){
			result.append(pos);
			result.append(words[i]);
			// std::cout << result << std::endl;
			return;
		} else {
			string_vec y_z = split(bp[i][j][pos].first, DELIMITER);
			unsigned s_point = bp[i][j][pos].second;

			result.append(pos);
			result.append(Json::Value());
			result.append(Json::Value());
			backtrack(bp, i, s_point, y_z[0], words, result[1]);
			backtrack(bp, s_point + 1, j, y_z[1], words, result[2]);
		}
	}

	// REQ: test_data is name of the file containing plain sentences
	//	 separated by ' '
	// EFF: read in test data,
	//		perform CKY algorithm, and
	//		output in JSON format.
	void do_cky(const std::string &test_data, const std::string &output){
		std::ifstream fin(test_data);
		std::ofstream fout(output);
		std::string word;

		while(std::getline(fin, word)){
			string_vec words = split(word, ' ');

			// CKY
			pi_table pi(words.size());
			bp_table bp(words.size());
			init_tables(pi, bp, words);

			for (unsigned length = 1; length < words.size(); ++length){
				for (unsigned i = 0; i < words.size() - length; ++i){
					unsigned j = i + length;

					// std::cout << "i j: " << i << ' ' << j << std::endl;
					for (auto jt = binary.cbegin(); jt != binary.cend(); ++jt){
						std::string x_y_z = jt->first;
						size_t delim_left = x_y_z.find(DELIMITER);
						size_t delim_right = x_y_z.rfind(DELIMITER);
						std::string x = x_y_z.substr(0, delim_left);
						std::string y = x_y_z.substr(delim_left + 1, delim_right - delim_left - 1);
						std::string z = x_y_z.substr(delim_right + 1);
						// std::cout << x_y_z << ": " << x << ' ' << y << ' ' << z << std::endl;

						double max = pi[i][j][x];
						unsigned max_s_point = 0;
						std::string max_y_z = "end";

						for (unsigned s_point = i; s_point < j; ++s_point){
							double current = binary_ML(x, y, z) * 
								pi[i][s_point][y] * pi[s_point + 1][j][z];

							if (max < current){
								max = current;
								max_s_point = s_point;
								max_y_z = y + DELIMITER + z;
							}
						}

						if (pi[i][j][x] < max){
							pi[i][j][x] = max;
							bp[i][j][x] = std::make_pair(max_y_z, max_s_point);
						}
					}
				}
			}

			// given test data is all 'SBARQ' 
			std::string start_tag = "SBARQ";
			Json::Value parse_result(Json::arrayValue);
			backtrack(bp, 0, words.size() - 1, start_tag, words, parse_result);
			// end CKY
			
			fout << parse_result << '\n';
		}
		fout.close();
		fin.close();
	}
};

int main(int argc, char** argv){

	if (argc < 3){
		std::cout << "usage: p2 COUNT_DATA TEST_DATA (output_file)" << std::endl;
		exit(1);
	}

	CKY cky(argv[1]);
	std::string filename = argc >= 4 ? argv[3] : "parse_dev.out";
	cky.do_cky(argv[2], filename);
	
	return 0;
}