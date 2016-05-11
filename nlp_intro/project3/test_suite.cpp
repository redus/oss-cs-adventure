#include <unordered_map>
#include <iostream>
#include <fstream>
#include <string>
#include "StringHelper.hpp"

// Model t(f|e) by hash[original][foreign]
typedef std::unordered_map<std::string, std::unordered_map<std::string, 
	double>> TScores;

void init_t(TScores &t_scores, std::string t_fname){
	std::ifstream t_stream;
	t_stream.open(t_fname);
	std::string o, f;
	double value;

	while (t_stream >> o >> f >> value){
		t_scores[o][f] = value;
	}
	t_stream.close();
}

// MOD: o_in, f_in
// EFF: find best alignment according to the t_score, and
// 		print according to dev.out style
void align(std::string o_fname, std::string f_fname, TScores &t_scores){
	std::ifstream o_in, f_in;
	o_in.open(o_fname);
	f_in.open(f_fname);

	std::string o_line, f_line;
	int line = 1;
	while (std::getline(o_in, o_line) && std::getline(f_in, f_line)){
		StringHelper::string_vec o_words = StringHelper::split(o_line, ' ');
		StringHelper::string_vec f_words = StringHelper::split(f_line, ' ');

		
		for (int i = 0; i < f_words.size(); ++i){
			double max = 0;
			int match_o = 1;
			for (int j = 0; j < o_words.size(); ++j){
				if (max < t_scores[o_words[j]][f_words[i]]){
					max = t_scores[o_words[j]][f_words[i]];
					match_o = j;
				}
			}
			
			// 1 based
			std::cout << line << ' ' << match_o + 1 << ' ' << i + 1 << std::endl;
		}
		line++;
	}

	o_in.close();
	f_in.close();
}

int main(int argc, char** argv){
	if (argc < 4){
		printf("USAGE: test_suite [original file] [foreign file] [t_score_file] \n");
		exit(1);
	}

	TScores t_scores;
	init_t(t_scores, argv[3]);
	// for (auto it = t_scores.cbegin(); it != t_scores.cend(); ++it){
	// 		for (auto jt = it->second.cbegin(); jt != it->second.cend(); ++jt){
	// 			std::cout << it->first << ' ' << jt->first << ' ' << std::fixed
	// 				<< jt->second << std::endl;
	// 		}
	// 	}
	align(argv[1], argv[2], t_scores);
}