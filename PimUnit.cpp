#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <sstream>
#include "config.h"
#include "utils.h"
using namespace std;

enum PIM_OPERATION {ADD=0, MUL, MAC, MAD, ADD_AAM, MUL_AAM, MAC_AAM, MAD_AAM, MOV, FILL, NOP, JUMP, EXIT};

class PimInstruction{
public:
  enum PIM_OPERATION PIM_OP;
  uint16_t *dst;
  uint16_t *src0;
  uint16_t *src1;
  uint32_t imm0;
  uint32_t imm1;
  uint8_t *physmem;
};

class PimUnit{
private:
  uint8_t PPC;
  PimInstruction CRF[32];

public:
  PimUnit(){
	this->PPC = 0;
  }
  
  void CrfInit(){	
	// TODO : read CRF.txt and write down to CRF //
	ifstream fp;
	fp.open("CRF.txt");
	
	string str;
	while(getline(fp, str) && !fp.eof()){
	  string mk_part[4];
	  int num_parts = (str.size()-1)/10 + 1;

	  cout << "# parts : " << num_parts << endl;
	  for(int i=0; i< num_parts; i++){
		mk_part[i] = (str.substr(i*10, 9)).substr(0, str.substr(i*10, 9).find(' '));
		cout << mk_part[i] << " ";
	  }
	  cout << endl;	  

	  this->PushCrf(mk_part, num_parts);
	  this->PPC += 1;
	}
	this->PPC = 0;
	////////////////////////////////////////
  }

  void PushCrf(string* mk_part, int num_parts){
	if(mk_part[0].find("ADD_AAM") != string::npos){
	
	}
	else if(mk_part[0].find("MUL_AAM") != string::npos){
	  	
	}
	else if(mk_part[0].find("MAC_AAM") != string::npos){
	  	
	}
	else if(mk_part[0].find("MAD_AAM") != string::npos){
	  	
	}
	else if(mk_part[0].find("ADD") != string::npos){
	  	
	}
	else if(mk_part[0].find("MUL") != string::npos){
	  	
	}
	else if(mk_part[0].find("MAC") != string::npos){
	  	
	}
	else if(mk_part[0].find("MAD") != string::npos){
	  	
	}
	else if(mk_part[0].find("MOV") != string::npos){
	  	
	}
	else if(mk_part[0].find("FILL") != string::npos){
	  	
	}
	else if(mk_part[0].find("NOP") != string::npos){
	  	
	}
	else if(mk_part[0].find("JUMP") != string::npos){
	  cout << "JUMP!\n";	
	}
	else if(mk_part[0].find("EXIT") != string::npos){
	  	
	}
	else{
	  cout << "what?!\n";
	}

	return;
  }


  // TODO : issue(pim_cmd) //
 

  ///////////////////////////


  // TODO : set_operand_addr(pim_cmd) //
 

  //////////////////////////////////////
 

  // TODO : execute(CRF[PPC] //
  
 
  /////////////////////////////
 
  // ~ the end ~ //
};

