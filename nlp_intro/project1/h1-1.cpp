#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <cctype>


const std::string TAGS[] = {"O", "I-GENE",};
const int TAGS_SIZE = 2;
const std::string NUM = "_NUM_";
const std::string ALL_CAPS = "_ALL_CAPS_";
const std::string LAST_CAPS = "_LAST_CAPS_";
const std::string RARE = "_RARE_";

/***
 *	ADT: Hidden markov model tagger
 *	populate word-tag count and tag seq count, and
 *  give q(TAG_i|TAG_{i-2}, TAG_{i-1}) and e(word|TAG) using maximum likelihood probability
 **/

class HMM{
	// words: count of tag associated with words
	//		input: <tag>|<word>
	//		output: count of word corresponding to tag
	// gram: n-gram sequence count. 
	// 		input: tag sequence concated by _
	// 		output: count of that sequence
	std::unordered_map<std::string, int> words, gram;

public:
	// ctor
	HMM(std::string filename){
		read_file(filename);
	}

	// EFF: populate words and gram from filename
	void read_file(std::string filename){
		std::ifstream fin;
		fin.open(filename);

		int num;
		std::string type;
		while(fin >> num >> type){
			std::string tag, word;

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
		// for (auto i = words.cbegin(); i != words.cend(); ++i){
		// 	if (i->second < 5){
		// 		std::cout << i->first << " " << i->second << std::endl;
		// 	}
			
		// }
		// for (auto j = gram.cbegin(); j != gram.cend(); ++j){
		// 	std::cout << j->first << " " << j->second << std::endl;
		// }
		fin.close();
	}

	int count_words(std::string x){
		int result = 0;
		for (auto &y: TAGS){
			std::string key = y + "|" + x;
			if (words.count(key) > 0){
				result += words[key];
			}
		}
		return result;
	}

	std::string rare_type(std::string x){
		bool caps = true;
		for (unsigned i = 0; i < x.size(); ++i){
			if (isdigit(x[i])){
				return NUM;
			} else if (!isupper(x[i])){
				caps = false;
				break;
			}
		}

		if (caps){
			return ALL_CAPS;
		} else if (isupper(x[x.size() - 1])){
			return LAST_CAPS;
		} else {
			return RARE;
		}
	}

	// REQ: x is the word, y is TAG
	// EFF: e(x|y) = count(y -> x) / count(x)
	double e(std::string x, std::string y){
		std::string key = y + "|" + x;
		double result = 0;

		if (count_words(x) > 0){
			result = (double) words[key] / gram[y];
		} else {
			result = (double) words[y + "|" + rare_type(x)] / gram[y];
		}
		return result;
	}

	// REQ: order or arguments is y_i, y_i-2, y_i-1
	// EFF: return q(y_i | y_i-2, y_i-1)
	double q(std::string y0, std::string y2, std::string y1){
		std::string tri_key = y2 + "_" + y1 + "_" + y0;
		std::string bi_key = y2 + "_" + y1;
		return (double) gram[tri_key] / gram[bi_key];
	}
	
	// EFF: return tag that maximizes e(word|tag)
	std::string best_y(const std::string x){
		std::string result = "";
		double max_e = 0;
		for (int i = 0; i < TAGS_SIZE; ++i){
			std::string key = TAGS[i] + '|' + x;
			double current = e(x, TAGS[i]);
			if (max_e < current){
				max_e = current;
				result = TAGS[i];
			}
		}

		
		return result;


	}

	std::vector<std::string> viterbi(const std::vector<std::string> &sq){
		std::vector<std::unordered_map<std::string, double>> pi (sq.size() + 1);
		std::vector<std::unordered_map<std::string, std::string>> bp (sq.size() + 1);

		// base case: 0
		pi[0]["*_*"] = 1;


		// populate DP space
		// order: w, u, v 
		for (unsigned i = 1; i < pi.size(); ++i){
			for (int j = 0; j < TAGS_SIZE; ++j){
				std::string u = i < 2 ? "*" : TAGS[j];

				for (auto &v : TAGS){
					double max = 0;
					std::string max_w = "";

					for (int k = 0; k < TAGS_SIZE; ++k){
						std::string w = i < 3 ? "*" : TAGS[k];
						std::string key = w + "_" + u;

						double result = pi[i - 1][key] * q(v, w, u) 
							* e(sq[i - 1], v);
						if (max < result){
							max = result;
							max_w = w;
						}
						// std::cout << w << " " << u << " " << v << " " << result << '\n';
					}

					std::string key = u + "_" + v;
					pi[i][key] = max;
					bp[i][key] = max_w;
				}
			}
		}

		// finding maximum key and backtracking
		double max = 0;
		std::string max_key = "";
		for (auto &u: TAGS){
			for (auto &v: TAGS){
				std::string key = u + "_" + v;
				double result = pi[pi.size() - 1][key] * q("STOP", u, v);
				if (max < result){
					max = result;
					max_key = key;
				}
			}
		}
		// std::cout << max_key << std::endl;

		std::vector<std::string> result (sq.size());
		int position = max_key.find("_");
		result[result.size() - 1] = max_key.substr(position + 1);
		result[result.size() - 2] = max_key.substr(0, position);

		for (int i = result.size() - 3; i >= 0; --i){
			std::string key= result[i + 1] + '_' + result[i + 2];
			result[i] = bp[i + 3][key];
		}
		
		// for (unsigned i = 0; i < pi.size(); ++i){
		// 	for (auto j = pi[i].cbegin(); j != pi[i].cend(); ++j){
		// 		std::cout << i << ": " << j->first << " " << j->second << std::endl;
		// 	}
		// }

		// for (unsigned i = 0; i < bp.size(); ++i){
		// 	for (auto j = bp[i].cbegin(); j != bp[i].cend(); ++j){
		// 		std::cout << i << ": " << j->first << " " << j->second << std::endl;
		// 	}
		// }

		return result;
	}
};


int main(int argc, char** argv){
	// if (argc < 3){
	// 	std::cout << "USAGE: hmm word TAG" << std::endl;
	// 	exit(1);
	// }

	
	HMM hmm("gene.rare.counts");
	// std::vector<std::string> x;

	// for (int i = 1; i < argc; ++i){
	// 	// puts(argv[i]);
	// 	x.push_back(argv[i]);
	// }
	// for (auto & i: hmm.viterbi(x)){
	// 	std::cout << i << " ";
	// } 
	// std::cout << std::endl;

	// std::string x = argv[1];
	// std::string y1 = argv[2];
	// std::string y0 = argv[3];
	// std::cout << "e(" << x << ", " << y << ") = " << wc.e(x, y) << std::endl;
	// std::cout << "best y: " << hmm.best_y(x) << std::endl;
	// std::cout << "q(" << y0 << " | " << y2 << ", " << y1 << ") = " 
	// << hmm.q(y0, y2, y1) << std::endl; 

	
	std::ifstream fin;
	std::ofstream fout;
	std::string line;
	fin.open("gene.dev");
	fout.open("gene_dev.p3.out");



	// while(std::getline(fin, line)){
	// 	if(line == ""){
	// 		fout << '\n';
	// 	} else {
	// 		fout << line << ' ' << hmm.best_y(line) << '\n';
	// 	}
	// }

	std::vector<std::string> sentence;
	while(std::getline(fin, line)){
		if (line == ""){
			std::vector<std::string> tags = hmm.viterbi(sentence);
			for (unsigned i = 0; i < tags.size(); ++i){
				fout << sentence[i] << ' ' << tags[i] << '\n';
			}
			fout << '\n';
			sentence.clear();
		} else {
			sentence.push_back(line);
		}
	}

	fin.close();
	fout.close();
	
	return 0;
}
