#include <iostream>
#include <sstream>
#include <vector>
#include <string.h>
#include "config.h"
using namespace std;

#define C_NRML	"\033[0m"
#define C_RED	"\033[031m"
#define C_GREN	"\033[032m"
#define C_YLLW	"\033[033m"
#define C_BLUE	"\033[034m"

int StringToNum(string str){
  int i = 0;
  stringstream ssInt(str);
  ssInt >> i;
  return i;
}

PIM_OPERATION StringToPIM_OP(string str){
  if(str == "ADD") return ADD;
  else if(str == "MUL") return MUL;
  else if(str == "MAC") return MAC;
  else if(str == "MAD") return MAD;
  else if(str == "ADD_AAM") return ADD_AAM;
  else if(str == "MUL_AAM") return MUL_AAM;
  else if(str == "MAC_AAM") return MAC_AAM;
  else if(str == "MAD_AAM") return MAD_AAM;
  else if(str == "MOV") return MOV;
  else if(str == "FILL") return FILL;
  else if(str == "NOP") return NOP;
  else if(str == "JUMP") return JUMP;
  else if(str == "EXIT") return EXIT;
}

PIM_OPERAND StringToOperand(string str){
  if(str == "ODD_BANK") return ODD_BANK;
  else if(str == "EVEN_BANK") return EVEN_BANK;
  else if(str == "GRF_A") return GRF_A;
  else if(str == "GRF_B") return GRF_B;
  else if(str == "SRF_A") return SRF_A;
  else if(str == "SRF_B") return SRF_B;
  else if(str == "GRF_A0") return GRF_A0;
  else if(str == "GRF_A1") return GRF_A1;
  else if(str == "GRF_A2") return GRF_A2;
  else if(str == "GRF_A3") return GRF_A3;
  else if(str == "GRF_A4") return GRF_A4;
  else if(str == "GRF_A5") return GRF_A5;
  else if(str == "GRF_A6") return GRF_A6;
  else if(str == "GRF_A7") return GRF_A7;
  else if(str == "GRF_B0") return GRF_B0;
  else if(str == "GRF_B1") return GRF_B1;
  else if(str == "GRF_B2") return GRF_B2;
  else if(str == "GRF_B3") return GRF_B3;
  else if(str == "GRF_B4") return GRF_B4;
  else if(str == "GRF_B5") return GRF_B5;
  else if(str == "GRF_B6") return GRF_B6;
  else if(str == "GRF_B7") return GRF_B7;
}
