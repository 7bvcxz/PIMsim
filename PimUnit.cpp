#include <iostream>
#include <fstream>
#include "config.h"
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
  
  void CRF_init(){	
	// TODO : read CRF.txt and write down to CRF //
	ifstream fp;
	fp.open("CRF.txt");
	while(!fp.eof()){
	  string str;
	  getline(fp, str);

	  
	}

	////////////////////////////////////////
  }

  // TODO : issue(pim_cmd) //
 

  ///////////////////////////
 
  // TODO : set_operand_addr(pim_cmd) //
 

  //////////////////////////////////////
 
  // TODO : execute(CRF[PPC] //
  
 
  /////////////////////////////
 
  // ~ the end ~ //
};

