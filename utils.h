#include <iostream>
#include <sstream>
#include <vector>
using namespace std;

#define C_NRML	"\033[0m"
#define C_RED	"\033[031m"
#define C_GREN	"\033[032m"
#define C_YLLW	"\033[033m"
#define C_BLUE	"\033[034m"

vector<string> split(string input, char delimiter) {
  vector<string> answer;
	stringstream ss(input);
	string temp;
			   
	while(getline(ss, temp, delimiter)){
	  answer.push_back(temp);
	}
				   
	return answer;
}



