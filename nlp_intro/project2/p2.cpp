#include <json/json.h>

int main(){
	json::Value root;
	std::cout << "input tree sample\n";
	std::cin >> root;
	std::cout << root << std::endl;

	return 0;
}