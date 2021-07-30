#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <sstream>
#include "config.h"
#include "utils.h"
using namespace std;

class PimInstruction{
private:
  enum PIM_OP {ADD, MUL, MAC, MAD, ADD_AAM, MUL_AAM, MAC_AAM, MAD_AAM, MOV, FILL, NOP, JUMP, EXIT};
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

	  this->PushCrf(mk_part, num_parts);
	  this->PPC += 1;
	  cout << endl;	  
	}
	this->PPC = 0;
	////////////////////////////////////////
  }

  void PushCrf(string* mk_part, int num_parts){
	this->CRF[PPC].PIM_OP = mk_part[0];
	
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

