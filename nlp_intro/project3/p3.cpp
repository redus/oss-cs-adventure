#include <unordered_map>
#include <unordered_set>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "StringHelper.hpp"


// REQ: assuming input file's words are delimted by space for each word

// abstract base class of IBM models
// pure virtual function for calculating t scores
class IBM{
protected:
	// Model t(f|o) by hash[original][foreign]
	// t(f|o): probability that foreign word is mapped to original word
	typedef std::unordered_map<std::string, std::unordered_map<std::string, 
		double>> TScores;
	TScores t_scores;

	// model c(o, f) and c(o) for updating t values
	typedef std::unordered_map<std::string, double> counts;

	
	// special word in original language that can map to all foreign words
	const std::string null = "_NULL_";
	const char DELIM = '_';

	std::ifstream o_in, f_in;
	// reset file reader to start
	void reset(std::ifstream &fin){
		fin.clear();
		fin.seekg(0);
	}

public:
	// EFF: print t_scores
	void print_t_scores(){
		for (auto it = t_scores.cbegin(); it != t_scores.cend(); ++it){
			for (auto jt = it->second.cbegin(); jt != it->second.cend(); ++jt){
				std::cout << it->first << ' ' << jt->first << ' ' << std::fixed
					<< jt->second << std::endl;
			}
		}
	}

	// REQ: o_in, f_in point to the beginning of each file
	// MOD: o_in, f_in, t_scores
	// EFF: perform EM algorithm from files in original and foreign lang and
	//		reset o_in, f_in back to start
	virtual void EM(int times) = 0;

	// EFF: align the test data using the result from EM and
	//		output it to STDOUT following dev.key convention
	virtual void align(std::string o_name, std::string f_name) = 0;
};


class IBM1: public IBM{
	// map unique foreign words to a single original word
	typedef std::unordered_map<std::string, std::unordered_set<std::string>> 
		unique_counts;
public:

	IBM1(std::string o, std::string f) {
		o_in.open(o);
		f_in.open(f);
		std::string o_line, f_line;
		unique_counts o_count;

		// map unique foreign words to original word
		while(std::getline(o_in, o_line) && std::getline(f_in, f_line)){
			for(std::string f : StringHelper::split(f_line, ' ')){
				for(std::string o : StringHelper::split(o_line, ' ')){
				o_count[o].insert(f);
				}
				o_count[null].insert(f);
			}			
		}
		reset(o_in);
		reset(f_in);

		// init t_scores
		while(std::getline(o_in, o_line) && std::getline(f_in, f_line)){
			for(std::string f : StringHelper::split(f_line, ' ')){
				for(std::string o : StringHelper::split(o_line, ' ')){
					t_scores[o][f] = 1.0 / o_count[o].size();
				}
				t_scores[null][f] = 1.0 / o_count[null].size();
			}
		}

		reset(o_in);
		reset(f_in);
	}

	~IBM1(){
		o_in.close();
		f_in.close();
	}

	// EFF: perform EM considering only t_values (no alignment consideration)
	void EM(int times){
		for (int i = 0; i < times; ++i){
			// init c(o, f) and c(o) to all 0 for each iteration
			counts of_count, o_count;
			std::string o_line, f_line;

			// c(o, f) and c(o) += delta where
			// delta = t(o|f) / <sum of t(o|f) over all o inculding null)
			while(std::getline(o_in, o_line) && std::getline(f_in, f_line)){
				for(std::string f : StringHelper::split(f_line, ' ')){
					// get denominator of delta first
					double denom = t_scores[null][f];
					for (std::string o : StringHelper::split(o_line, ' ')){
						if (t_scores[o].count(f) > 0){
							denom += t_scores[o][f];
						} 
					}

					if (denom == 0){
						continue;
					}

					for (std::string o : StringHelper::split(o_line, ' ')){
						double delta = t_scores[o][f] / denom;
						of_count[o + DELIM + f] += delta;
						o_count[o] += delta;
					}
					double delta = t_scores[null][f] / denom;
					of_count[null + DELIM + f] += delta;
					o_count[null] += delta;
				}	
			}
			reset(o_in);
			reset(f_in);

			// new t = c(o, f) / c(o)
			for (auto it = t_scores.begin(); it != t_scores.end(); ++it){
				for (auto jt = it->second.begin(); jt != it->second.end(); ++jt){
					std::string of_key = it->first + DELIM + jt->first;
					jt->second = of_count[of_key] / o_count[it->first];
				}
			}
		}
	}

	void align(std::string o_name, std::string f_name){
		std::ifstream test_o_in, test_f_in;
		test_o_in.open(o_name);
		test_f_in.open(f_name);

		std::string o_line, f_line;
		int line = 1;
		while (std::getline(test_o_in, o_line) && std::getline(test_f_in, f_line)){
			StringHelper::string_vec o_words = StringHelper::split(o_line, ' ');
			StringHelper::string_vec f_words = StringHelper::split(f_line, ' ');

			for (int i = 0; i < f_words.size(); ++i){
				double max = t_scores[null][f_words[i]];
				int match_o = -1;
				for (int j = 0; j < o_words.size(); ++j){
					if (max < t_scores[o_words[j]][f_words[i]]){
						max = t_scores[o_words[j]][f_words[i]];
						match_o = j;
					}
				}
				
				// 1 based output, map null to 0
				std::cout << line << ' ' << match_o + 1 << ' ' << i + 1 << std::endl;
			}
			line++;
		}

		test_o_in.close();
		test_f_in.close();
	}

};

class IBM2 : public IBM{
	// model alignment probability as q(j|i, l, m)
	// j, i: position of word in original sentence, foreign sentence.
	// l, m: # words in original sentence, foreign sentence.
	// map key: string "l + DELIM + m"
	// vector key: [i][j]
	std::unordered_map<std::string, std::vector<std::vector<double>>> q;

	// model c(j|i, l, m) and c(i, l, m)
	// first key: string "l + DELIM + m"
	// second key: 1D index (squash if 2D)
	typedef std::unordered_map<std::string, std::unordered_map<int, double>> align_counts;

public:
	IBM2(std::string o_name, std::string f_name, std::string t_name){
		o_in.open(o_name);
		f_in.open(f_name);
		std::string o_line, f_line;

		// init q to 1 / (<length of original> + 1)
		while(std::getline(o_in, o_line) && std::getline(f_in, f_line)){
			StringHelper::string_vec o_words = StringHelper::split(o_line, ' ');
			StringHelper::string_vec f_words = StringHelper::split(f_line, ' ');
			std::string key = make_length_key(o_words.size(), f_words.size());

			// f by o + 1 (null is at the LAST element, not first)
			q[key] = std::vector<std::vector<double>>(f_words.size(),
				std::vector<double>(o_words.size() + 1, 1.0 / (o_words.size() + 1)));
		}
		reset(o_in);
		reset(f_in);

		// init t_scores from IBM1::EM(5) result
		std::ifstream t_stream;
		t_stream.open(t_name);
		std::string o, f;
		double value;

		while (t_stream >> o >> f >> value){
			if (value > 0){
				t_scores[o][f] = value;
			}
		}
		t_stream.close();
	}

	~IBM2(){
		o_in.close();
		f_in.close();
	}

	// EFF: print q_scores
	void print_q(){
		for (auto it = q.begin(); it != q.end(); ++it){
			std::cout << it->first << std::endl;
			for (int i = 0; i < it->second.size(); ++i){
				for (int j = 0; j < it->second[i].size(); ++j){
					std::cout << it->second[i][j] << ' ';
				}
				std::cout << std::endl;
			}
			std::cout << std::endl;
		}
	}


	// EFF: perform EM considering alignment and t+scores
	void EM(int times){
		for (int tm = 0; tm < times; ++tm){
			counts of_count, o_count;
			align_counts ji_count, i_count;
			std::string o_line, f_line;

			// c(o,f), c(o), c(j|i,l,m) += delta where
			// delta = t(o|f) * q(j|i,l,m) / <sum of t(o|f)*q(j|i,l,m) 
			//		over all o inculding null>
			// c(i,l,m) += number of all original words + NULL
			while(std::getline(o_in, o_line) && std::getline(f_in, f_line)){
				StringHelper::string_vec f_words = StringHelper::split(f_line, ' ');
				StringHelper::string_vec o_words = StringHelper::split(o_line, ' ');
				std::string length_key = make_length_key(o_words.size(), f_words.size());

				for(int i = 0; i < f_words.size(); ++i){
					std::string f = f_words[i];
					double denom = 0;

					for (int j = 0; j <= o_words.size(); ++j){
						std::string o = j < o_words.size() ? o_words[j] : null;
						if (t_scores[o].count(f) > 0){
							denom += t_scores[o][f] * q[length_key][i][j];
						}
					}

					if (denom == 0){
						continue;
					} 

					for (int j = 0; j <= o_words.size(); ++j){
						std::string o = j < o_words.size() ? o_words[j] : null;
						double delta = t_scores[o][f] * q[length_key][i][j] / 
							denom;
						// std::cout << i << ' ' << j << ": " << delta << std::endl;
						of_count[o + DELIM + f] += delta;
						o_count[o] += delta;
						ji_count[length_key][i * (o_words.size()+1) + j] += delta;
						
					}
					i_count[length_key][i] = o_words.size() + 1;
				}	
			}
			reset(o_in);
			reset(f_in);

			for (auto it = t_scores.begin(); it != t_scores.end(); ++it){
				for (auto jt = it->second.begin(); jt != it->second.end(); ++jt){
					std::string of_key = it->first + DELIM + jt->first;
					jt->second = of_count[of_key] / o_count[it->first];
				}
			}

			// new q = c(j|i,l,m) / c(i,l,m)
			for (auto it = q.begin(); it != q.end(); ++it){
				for (int i = 0; i < it->second.size(); ++i){
					size_t limit = it->second[i].size();
					for (int j = 0; j < limit; ++j){
						it->second[i][j] = ji_count[it->first][i * limit + j] / 
							i_count[it->first][i];
					}
				}
			}
		}
		// print_q();
	}

	void align(std::string o_name, std::string f_name){
		std::ifstream test_o_in, test_f_in;
		test_o_in.open(o_name);
		test_f_in.open(f_name);

		std::string o_line, f_line;
		int line = 1;
		while (std::getline(test_o_in, o_line) && std::getline(test_f_in, f_line)){
			StringHelper::string_vec o_words = StringHelper::split(o_line, ' ');
			StringHelper::string_vec f_words = StringHelper::split(f_line, ' ');
			std::string length_key = make_length_key(o_words.size(), f_words.size());
			
			for (int i = 0; i < f_words.size(); ++i){
				double max = t_scores[null][f_words[i]] * q[length_key][i][
					o_words.size()];
				int match_o = -1;
				for (int j = 0; j < o_words.size(); ++j){
					double current = t_scores[o_words[j]][f_words[i]] * 
						q[length_key][i][j];
					if (max < current){
						max = current;
						match_o = j;
					}
				}
				std::cout << line << ' ' << match_o + 1 << ' ' << i + 1 << std::endl;
			}
			line++;
		}

		test_o_in.close();
		test_f_in.close();
	}

private:
	// EFF: return constructed key for align_counts
	std::string make_length_key(std::size_t o, std::size_t f){
		return std::to_string(o) + DELIM + std::to_string(f);
	}
};

int main(int argc, char** argv){
	if (argc < 4){
		printf("USAGE: p3 [original file] [foreign file] [t_score_file] \n");
		exit(1);
	}
	IBM2 model(argv[1], argv[2], argv[3]);

	IBM &IBM = model;
	// IBM.print_t_scores();
	IBM.EM(5);
	// IBM.print_t_scores();
	IBM.align("dev.en", "dev.es");
}


