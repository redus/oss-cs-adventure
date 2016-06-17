#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <cmath>
#include "StringHelper.hpp"


typedef std::unordered_map<std::string, double> str_double_map;
typedef std::vector<std::string> string_vec;

/***
 *	ADT: Global linear model
 *	
 *  perform Viterbi algorithm on given sentence
 **/
class GLM{
	std::unordered_map<std::string, int>  global_features;
	str_double_map weight;

	// feature identifiers
	const std::string F_TAG = "_TAG_";
	const std::string F_TRIGRAM = "_TRIGRAM_";
	const std::string TAGS[2] = {"O", "I-GENE"};
	const int TAGS_SIZE = 2;

	const std::string START = "*";
	const std::string STOP = "STOP";
	const std::string DELIM = "+";

	struct History{
		std::string tag_2; // 2nd previous tag
		std::string tag_1; // previous tag
		const string_vec &words; // sentence  
		unsigned index;
	};

public:
	// ctor
	GLM(std::string weight_file){
		read_weight(weight_file);
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
		result[result.size() - 2] = max_key.substr(0, position);

		for (int i = result.size() - 3; i >= 0; --i){
			std::string key= result[i + 1] + DELIM + result[i + 2];
			result[i] = bp[i + 3][key];
		}
		return result;
	}

private:
	// REQ: file is seperated by space
	// EFF: populate weight from filename
	void read_weight(std::string filename){
		std::ifstream fin;
		fin.open(filename);

		double value;
		std::string type;
		while(fin >> type){
			std::string key = "";

			if (type == "TAG"){
				std::string word, tag;
				fin >> word >> tag;
				key = F_TAG + DELIM + tag + DELIM + word;
			} else if (type == "TRIGRAM"){
				std::string tag1, tag2, tag3;
				fin >> tag1 >> tag2 >> tag3;
				key = F_TRIGRAM + DELIM + tag1 + DELIM + tag2 + DELIM + tag3;
			} else {
				// dump line 
				std::string line;
				std::getline(fin, line);
			}

			fin >> value;
			weight[key] = value;
			// std::cout << key << ' ' << value << std::endl;
		}
		fin.close();
	}

	// EFF: return vector of feature keys
	string_vec gen_feature_keys(const History &history, std::string tag){
		string_vec result;
		// tag feature
		if (history.index < history.words.size()){
			result.push_back(F_TAG + DELIM + tag + DELIM + history.words[history.index]);
		} 
		// trigram feature
		result.push_back(F_TRIGRAM + DELIM + history.tag_2 + DELIM + history.tag_1
				+ DELIM + tag);
		return result;

	}


	double local_inner_product(const History &history, std::string tag){
		double sum = 0;
		// make feature key array
		for (std::string key : gen_feature_keys(history, tag)){
			global_features[key] = 1;
			sum += weight[key] * global_features[key];
		}
		
		// std::cout << weight[w_key]  << ' ' << global_features[w_key] << std::endl;
		return sum;
	}
};


class Perceptron
{
	str_double_map weights;
	int max_suffix_length;
public:

	Perceptron() : max_suffix_length(3) {};
 	Perceptron(int suffix) : max_suffix_length(suffix) {};
	// REQ
	void update(const string_vec &sentence, string_vec &tags, string_vec &gold_tags){
		for (int i = 0; i != sentence.size(); ++i){
			suffix_feature(sentence[i], tags[i], false);
			suffix_feature(sentence[i], gold_tags[i], true);
		}
	}

	// suffix feature

	void suffix_feature(std::string word, std::string tag, bool is_gold_tag){
		for (int i = 1; i <= max_suffix_length; ++i)
		{
			std::string suffix = word.substr(word.size() - i);
			int value = is_gold_tag ? 1 : -1;
			weights[suffix + ' ' + tag] += value; 
		}
	}
	void print_weights(){
		for (auto i = weights.begin(); i != weights.end(); ++i){
			std::cout << "SUFFIX " << i->first << ' ' << i->second << std::endl; 
		}
	}
	
};

int main(int argc, char** argv){
	GLM glm("tag.model");
	Perceptron perceptron;

	std::ifstream fin;
	std::ofstream fout;
	std::string line;
	fin.open("gene.train");
	fout.open("gene.dev.p4.out");


	string_vec sentence, gold_tags;
	while(std::getline(fin, line)){
		if (line == ""){
			string_vec tags = glm.viterbi(sentence);
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
