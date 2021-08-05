#ifndef __UTILS_H
#define __UTILS_H

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <string.h>
#include "config.h"
using namespace std;

#define ABS(x)	  ( ((x)<0)?-(x):(x) )

#define C_NORMAL  "\033[0m"
#define C_RED	  "\033[031m"
#define C_GREEN	  "\033[032m"
#define C_YELLOW  "\033[033m"
#define C_BLUE	  "\033[034m"


/*  later,,,
string* StringSplitter(string str_line){
  string str[18];
  int num_parts = (str_line.size()-1)/10 + 1;
  for(int i=0; i<num_parts; i++)
	str[i] = (str_line.substr(i*10, 9)).substr(0, str_line.substr(i*10, 9).find(' '));
  
  return str;
}
*/

float StringToNum(string str);
PIM_OPERATION StringToPIM_OP(string str);
PIM_OPERAND StringToOperand(string str);
int StringToIndex(string str);

#endif
