#include <unordered_map>
#include <set>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "StringHelper.hpp"


// abstract base class of IBM models
// pure virtual function for calculating t scores
class IBM{
protected:
	// Model t(f|e) by hash[original][foreign]
	typedef std::unordered_map<std::string, std::unordered_map<std::string, 
		double>> TScores;
	TScores t_scores;
	typedef std::unordered_map<std::string, double> counts;
	const char DELIM = '_';
	// special original word that can map to all foreign word
	const std::string null = "_NULL_";
	std::ifstream o_in, f_in;

	

	IBM(std::string o, std::string f){
		o_in.open(o);
		f_in.open(f);

		// init t_scores:
		// mapping each original word in line + null to all the foreign words
		//	with probability of 1 / #<original_words>
		std::string o_line, f_line;
		counts of_count, o_count;
		std::set<std::string> all_f_words;

		while (std::getline(o_in, o_line) && std::getline(f_in, f_line)){
			for(std::string o : StringHelper::split(o_line, ' ')){
				for(std::string f : StringHelper::split(f_line, ' ')){
					of_count[o + DELIM + f] += 1;
					o_count[o] += 1;
					all_f_words.insert(f);
				}
			}
		}
		reset(o_in);
		reset(f_in);


		for (auto it = of_count.begin(); it != of_count.end(); ++it){
			std::string o_f = it->first;
			std::string o = o_f.substr(0, o_f.find(DELIM));
			std::string f = o_f.substr(o_f.find(DELIM) + 1);
			// std::cout << o << '~' << f << std::endl;
		
			t_scores[o][f] = 1.0 / (it->second + o_count[o]);
		}
		for (auto it = all_f_words.cbegin(); it != all_f_words.cend(); ++it){
			t_scores[null][*it] = 1.0 / all_f_words.size();
		}

	}

	~IBM(){
		o_in.close();
		f_in.close();
	}

	// reset file reader to start
	void reset(std::ifstream &fin){
		fin.clear();
		fin.seekg(0);
	}

public:
	void print_scores(){
		for (auto it = t_scores.cbegin(); it != t_scores.cend(); ++it){
			for (auto jt = it->second.cbegin(); jt != it->second.cend(); ++jt){
				std::cout << it->first << ' ' << jt->first << ' ' << std::fixed
					<< jt->second << std::endl;
			}
		}
	}

	// REQ: o_in, f_in are in start position (EOL cleared)
	// MOD: t_scores, o_in, f_in 
	// EFF: perform EM algorithm on t_scores and
	//		reset o_in, f_in back to start
	virtual void EM(int times) = 0;
};


class IBM1: public IBM{	
	// model c(o, f) and c(o) for updating t values
	
	
public:
	IBM1(std::string o, std::string f) : IBM(o, f) {}

	// EFF: perform EM considering only t values (no alignment consideration)
	void EM(int times){
		for (int i = 0; i < times; ++i){
			// init c(o, f) and c(o) to all 0 for each iteration
			counts of_count, o_count;
			std::string o_line, f_line;

			while(std::getline(o_in, o_line) && std::getline(f_in, f_line)){

				for(std::string f : StringHelper::split(f_line, ' ')){
					// get denominator of delta first
					double denom = 0;
					for (std::string o : StringHelper::split(o_line, ' ')){
						if (t_scores[o].count(f) > 0){
							denom += t_scores[o][f];
						} else {
							denom += t_scores[null][f];
						}
					}

					// blank translation, update with null
					if (denom == 0){
						double delta = 1;
						of_count[null + DELIM + f] += delta;
						o_count[null] += delta;
					}
					
					for (std::string o : StringHelper::split(o_line, ' ')){
						double delta = t_scores[o][f] / denom;
						of_count[o + DELIM + f] += delta;
						o_count[o] += delta;
					}

				}	
			}
			reset(o_in);
			reset(f_in);

			for (auto it = t_scores.begin(); it != t_scores.end(); ++it){
				for (auto jt = it->second.begin(); jt != it->second.end(); ++jt){
					std::string of_key = it->first + DELIM + jt->first;
					// std::cout << of_count[of_key] << ' ' << o_count[it->first] << std::endl;
					// new t = c(o, f) / c(o)
					jt->second = of_count[of_key] / o_count[it->first];
				}
			}
		}
	}



};

int main(int argc, char** argv){
	IBM1 model("corpus.en", "corpus.es");

	IBM &IBM = model;
	IBM.EM(5);
	IBM.print_scores();
}


