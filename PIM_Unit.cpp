#include <iostream>
using namespace std;

class PimInstruction{
private:
  enum PIM_OP {ADD, MUL, MAC, MAD, ADD_AAM, MUL_AAM, MAC_AAM, MAD_AAM, MOV, FILL, NOP, JUMP, EXIT};
  unsigned short *dst;	  // uint16
  unsigned short *src0;	  // uint16
  unsigned short *src1;	  // uint16
  int imm0;
  int imm1;
  unsigned char *physmem  // uint8
};

class PimUnit{
private:
  unsigned int PPC;
  PimInstruction CRF[32];

public:
  PimUnit(){
	this->PPC = 0;
  }
  
  void CRF_init(){	
	// todo : read CRF.txt and write down //
	

	////////////////////////////////////////
  }
};

