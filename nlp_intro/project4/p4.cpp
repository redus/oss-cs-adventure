#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <cmath>
#include "StringHelper.hpp"


/***
 *	ADT: Global linear model
 *	
 *  perform Viterbi algorithm on given sentence
 **/
class GLM{
public:
	typedef std::vector<std::string> string_vec;
	GLM(){}

	// ctor
	GLM(std::string weight_file){
		read_weight(weight_file);
	}

	void update_weight(const string_vec &sentence, const string_vec &tags, 
		const string_vec &gold_tags){

		for (unsigned i = 0; i <= sentence.size(); ++i){
			std::string t_tag2 = i < 2 ? START : tags[i-2];
			std::string t_tag1 = i < 1 ? START : tags[i-1];
			std::string t_tag = i < sentence.size() ? tags[i] : STOP;
			History t_history{t_tag2, t_tag1, sentence, i};
			// std::cout << '-' << std::endl; 
			double sum = 0;
			for (std::string key : gen_feature_keys(t_history, t_tag)){
				sum += 1;
			}
			for (std::string key : gen_feature_keys(t_history, t_tag)){
				weight[key] -= sum;
				// std::cout << key << ' ' << weight[key] << std::endl; 
			}

			std::string g_tag2 = i < 2 ? START : gold_tags[i-2];
			std::string g_tag1 = i < 1 ? START : gold_tags[i-1];
			std::string g_tag = i < sentence.size() ? gold_tags[i] : STOP;
			History g_history{g_tag2, g_tag1, sentence, i};
			sum = 0;
			for (std::string key : gen_feature_keys(g_history, g_tag)){
				sum += 1;
			}
			for (std::string key : gen_feature_keys(g_history, g_tag)){
				weight[key] += sum;
				// std::cout << key << ' ' << weight[key] << std::endl; 
			}
		}
	}

	void print_weight(std::ostream &out){
		// print with DELIM changed to space
		for (auto i = weight.begin(); i != weight.end(); ++i)
		{
			StringHelper::string_vec tokens = StringHelper::split(i->first, DELIM);
			tokens[0] = feature_to_string(tokens[0]);
			std::string key = StringHelper::join(tokens, DELIM );
			out << key << DELIM << i->second << std::endl; 
		}
	}

	// EFF: perform viterbi to calculate global linear score on sentence, and
	//		return tag sequence with the maximum score
	string_vec viterbi(const string_vec &sentence){
		std::vector<str_double_map> pi (sentence.size() + 1);
		std::vector<std::unordered_map<std::string, std::string>> bp (sentence.size() + 1);

		// base case: 0
		pi[0][START + DELIM + START] = 0;


		// populate DP space
		// tag order: t, u, v 
		for (unsigned i = 1; i < pi.size(); ++i){
			for (auto u : TAGS){
				if (i == 1) {
					u = START;
				}
				for (auto v : TAGS){
					double max = -INFINITY;
					std::string max_t = "";

					for (auto t : TAGS){
						if (i < 3){
							t = START;
						}
						History history{t, u, sentence, i - 1};
						std::string key = t + DELIM + u;
						double result = pi[i - 1][key] + local_inner_product(
							history, v);

						if (max < result){
							max = result;
							max_t = t;
						}
						// std::cout << t << " " << u << " " << v << " " << result << '\n';
					}

					std::string key = u + DELIM + v;
					pi[i][key] = max;
					bp[i][key] = max_t;
					// std::cout << max << ' ' << max_t << std::endl;
				}
			}
		}


		// finding maximum key and backtracking
		double max = -INFINITY;
		std::string max_key = "";

		for (auto &u: TAGS){
			for (auto &v: TAGS){
				History history{u, v, sentence, sentence.size()};
				std::string key = u + DELIM + v;
				double result = pi[sentence.size()][key] * local_inner_product(
					history, STOP);

				if (max < result){
					max = result;
					max_key = key;
				}
			}
		}

		// maximum tag sequence
		string_vec result (sentence.size());
		int position = max_key.find(DELIM);
		result[result.size() - 1] = max_key.substr(position + 1);
		if (result.size() > 1){
			result[result.size() - 2] = max_key.substr(0, position);
		}
		
		for (int i = result.size() - 3; i >= 0; --i){
			std::string key= result[i + 1] + DELIM + result[i + 2];
			result[i] = bp[i + 3][key];
		}
		return result;
	}

private:
	std::unordered_map<std::string, int> global_features;
	typedef std::unordered_map<std::string, double> str_double_map;
	str_double_map weight;

	// feature identifiers
	const std::string F_TAG = "_TAG_";
	const std::string F_TRIGRAM = "_TRIGRAM_";
	const std::string F_SUFF = "_SUFF_";
	const std::string F_PREF = "_PREF_";
	const string_vec TAGS = {"O", "I-GENE"};

	const std::string START = "*";
	const std::string STOP = "STOP";
	const char DELIM = '|';

	struct History{
		std::string tag_2; // 2nd previous tag
		std::string tag_1; // previous tag
		const string_vec &words; // sentence  
		unsigned index;
	};

	// REQ: file is seperated by space
	// EFF: populate weight from filename
	void read_weight(std::string filename){
		std::ifstream fin;
		fin.open(filename);

		
		std::string line;
		while(std::getline(fin, line)){
			StringHelper::string_vec tokens = StringHelper::split(line, DELIM);
			std::string key = "";
			std::string type = tokens[0];

			if (type == "TAG"){
				key = F_TAG + DELIM + tokens[1] + DELIM + tokens[2];
			} else if (type == "TRIGRAM"){
				key = F_TRIGRAM + DELIM + tokens[1] + DELIM + tokens[2] + DELIM + 
					tokens[3];
			} else if (type == "SUFF"){
				key = F_SUFF + DELIM + tokens[1] + DELIM + tokens[2];
			}else if (type == "PREF"){
				key = F_PREF + DELIM + tokens[1] + DELIM + tokens[2];
			}

			double value = std::stod(tokens[tokens.size() - 1]);
			weight[key] = value;
			// std::cout << key << ' ' << value << std::endl;
		}
		fin.close();
	}

	std::string feature_to_string(std::string feature){
		if (feature == F_TAG){
			return "TAG";
		} else if (feature == F_TRIGRAM){
			return "TRIGRAM";
		} else if (feature == F_SUFF){
			return "SUFF";
		} else if (feature == F_PREF){
			return "PREF";
		} else {
			return "";
		}
	}
	// EFF: return vector of feature keys
	string_vec gen_feature_keys(const History &history, const std::string tag){
		string_vec result;
		
		if (history.index < history.words.size()){
			std::string word = history.words[history.index];
			// tag feature
			result.push_back(F_TAG + DELIM + tag + DELIM + word);

			// prefix, suffix feature
			for (int i = 1; word.size() >= i && i <= 3; ++i){
				std::string prefix = word.substr(0, i);
				std::string suffix = word.substr(word.size() - i);
				result.push_back(F_PREF + DELIM + tag + DELIM + prefix);
				result.push_back(F_SUFF + DELIM + tag + DELIM + suffix);
			}
		} 
		// trigram feature
		result.push_back(F_TRIGRAM + DELIM + history.tag_2 + DELIM + history.tag_1
				+ DELIM + tag);
		
		return result;
	}


	double local_inner_product(const History &history, const std::string tag){
		double sum = 0;
		// make feature key array
		for (std::string key : gen_feature_keys(history, tag)){
			
			global_features[key] = 1;
			sum += weight[key] * global_features[key];
			// std::cout << key <<  ' ' << weight[key] << ' ' << std::endl;
		}
		
		
		return sum;
	}
};

int main(int argc, char** argv){

	if (argc < 2){
		std::cerr << "usage: p4 [train|model] <model_file>" << std::endl;
		exit(1);
	}

	GLM glm = (argv[1] == std::string("train")) ? GLM() : GLM(argv[2]);
	std::ifstream fin;
	std::ofstream fout;
	
	
	std::string line;
	GLM::string_vec sentence; 
	if (argv[1] == std::string("train")){
		int times = 6;
		GLM::string_vec gold_tags;
		fin.open("gene.train");
		fout.open("suffix_tagger.model");

		for (int i = 0; i < times; ++i) {
			while(std::getline(fin, line)){
				if (line.empty()){
					GLM::string_vec tags = glm.viterbi(sentence);
					glm.update_weight(sentence, tags, gold_tags);
					sentence.clear();
					gold_tags.clear();
				} else {
					std::string word, tag;
					std::istringstream iss(line);
					iss >> word >> tag;
					sentence.push_back(word);
					gold_tags.push_back(tag);
				}
			}
			fin.clear();
			fin.seekg(0);
		}

		glm.print_weight(fout);

	} else {
		fin.open("gene.dev");
		fout.open("gene.dev.p4.out");
		while(std::getline(fin, line)){
			if (line.empty()){
				GLM::string_vec tags = glm.viterbi(sentence);
				for (int i = 0; i < sentence.size(); ++i){
					fout << sentence[i] << ' ' << tags[i] << '\n';
				}
				fout << '\n';
				sentence.clear();
			} else {
				sentence.push_back(line);
			}
		}
	}

	fin.close();
	fout.close();
	
	return 0;
}
