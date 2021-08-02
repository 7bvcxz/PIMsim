#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <sstream>
#include "config.h"
#include "utils.h"
using namespace std;

class PimInstruction{
public:
  enum PIM_OPERATION PIM_OP;
  enum PIM_OPERAND dst;
  enum PIM_OPERAND src0;
  enum PIM_OPERAND src1;
  int imm0;
  int imm1;
  uint8_t *physmem;
};

class PimUnit{
private:
  PimInstruction CRF[32];
  uint8_t PPC;
  int LC;
  float* GRF_A;
  float* GRF_B;
  float* SRF_A;
  float* SRF_M;

  float* dst;
  float* src0;
  float* src1;

  float *physmem;
  float *data;

public:
  PimUnit(){
	PPC = 0;
	LC  = -1;
	GRF_A = (float*)mmap(NULL, 8 * 16 * 32 / 8, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	GRF_B = (float*)mmap(NULL, 8 * 16 * 32 / 8, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	SRF_A = (float*)mmap(NULL, 8 * 32 / 8, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	SRF_M = (float*)mmap(NULL, 8 * 32 / 8, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  }
  
  void SetPhysmem(float* physmem){
	this->physmem = physmem;
  }

  void CrfInit(){	
	ifstream fp;
	fp.open("CRF.txt");
	
	string str;
	while(getline(fp, str) && !fp.eof()){
	  string mk_part[4];
	  int num_parts = (str.size()-1)/10 + 1;

	  for(int i=0; i< num_parts; i++){
		mk_part[i] = (str.substr(i*10, 9)).substr(0, str.substr(i*10, 9).find(' '));
		cout << mk_part[i] << " ";
	  }
	  cout << endl;	  

	  PushCrf(mk_part, num_parts);
	  PPC += 1;
	}
	PPC = 0;

	fp.close();
  }

  void PushCrf(string* mk_part, int num_parts){
	CRF[PPC].PIM_OP = StringToPIM_OP(mk_part[0]);

	if(CRF[PPC].PIM_OP < 8){
	  CRF[PPC].dst  = StringToOperand(mk_part[1]);
	  CRF[PPC].src0 = StringToOperand(mk_part[2]);
	  CRF[PPC].src1 = StringToOperand(mk_part[3]);
	}
	else if(CRF[PPC].PIM_OP < 10){
	  CRF[PPC].dst  = StringToOperand(mk_part[1]);
	  CRF[PPC].src0 = StringToOperand(mk_part[2]); 
	}
	else if(CRF[PPC].PIM_OP < 11){  // nop 
	  CRF[PPC].imm0 = StringToNum(mk_part[1]);
	}
	else if(CRF[PPC].PIM_OP < 12){  // jump
	  CRF[PPC].imm0 = PPC + StringToNum(mk_part[1]);
	  CRF[PPC].imm1 = StringToNum(mk_part[2]);
	}
  }


  // TODO : issue(pim_cmd) //
  void Issue(string* pim_cmd, int num_parts){

	// DRAM READ & WRITE // 
	if(pim_cmd[0] == "WR"){ // WR  3  156
	  float WR = (float)StringToNum(pim_cmd[2]);
	  memcpy(physmem + StringToNum(pim_cmd[1])*CELL_SIZE/8, &WR, 4);
	}
	else if(pim_cmd[0] == "RD"){ // RD  3
	  float RD = 0;
	  memcpy(&RD, physmem + StringToNum(pim_cmd[1])*CELL_SIZE/8, 4);
	  cout << "RD : " << RD << endl;
	  data = physmem + StringToNum(pim_cmd[1])*CELL_SIZE/8, 4;
	}

	// NOP & JUMP // 
	if(CRF[PPC].PIM_OP == NOP){
	  if(LC == -1)	LC = CRF[PPC].imm0 -1;
	  else if(LC > 0) LC -= 1;
	  else if(LC == 0){
		LC = -1;
		return;
	  }
	}

	else if(CRF[PPC].PIM_OP == JUMP){
	  if(LC == -1){
		LC = CRF[PPC].imm1 -1;
		PPC = CRF[PPC].imm0;
	  }
	  else if(LC > 0){
		PPC = CRF[PPC].imm0;
		LC -= 1;
	  }
	  else if(LC == 0){
		PPC += 1;
		LC  -= 1;
	  }
	}

	// SET ADDR & EXECUTE // 
	cout << "execute PPC : " << (int)PPC << endl;
	
	SetOperandAddr(pim_cmd);

	Execute();	
	
	PPC += 1;

	cout << endl;
 }

  ///////////////////////////

  // TODO : set_operand_addr(pim_cmd) //
  void SetOperandAddr(string* pim_cmd){
	// GRF_A, GRF_B
	if(CRF[PPC].PIM_OP>=4 && CRF[PPC].PIM_OP<=7){  // AAM mode
	  int A_idx = StringToNum(pim_cmd[1])%8;
	  int B_idx = StringToNum(pim_cmd[1])/8; // + RA % 2 * 4
	  
	  if     (CRF[PPC].dst == 2)  dst = GRF_A + A_idx * 16;
	  else if(CRF[PPC].dst == 3)  dst = GRF_B + B_idx * 16;
	  if     (CRF[PPC].src0 == 2) src0 = GRF_A + A_idx * 16;
	  else if(CRF[PPC].src0 == 3) src0 = GRF_B + B_idx * 16;
	  if     (CRF[PPC].src1 == 2) src1 = GRF_A + A_idx * 16;
	  else if(CRF[PPC].src1 == 3) src1 = GRF_B + B_idx * 16;
	}
	else{  // non-AAM mode
	  if(CRF[PPC].dst >=10 && CRF[PPC].dst < 20)		dst = GRF_A + CRF[PPC].dst % 10 * 16;
	  else if(CRF[PPC].dst >= 20 && CRF[PPC].dst <30)	dst = GRF_B + CRF[PPC].dst % 10 * 16;  
	  if(CRF[PPC].src0 >=10 && CRF[PPC].src0 < 20)		src0 = GRF_A + CRF[PPC].src0 % 10 * 16;
	  else if(CRF[PPC].src0 >= 20 && CRF[PPC].src0 <30)	src0 = GRF_B + CRF[PPC].src0 % 10 * 16;
	  if(CRF[PPC].src1 >=10 && CRF[PPC].src1 < 20)		src1 = GRF_A + CRF[PPC].src1 % 10 * 16;
	  else if(CRF[PPC].src1 >= 20 && CRF[PPC].src1 <30)	src1 = GRF_B + CRF[PPC].src1 % 10 * 16;
	}

	// bank, SRF //
	if     (CRF[PPC].dst <= 1)	dst  = data; 
	else if(CRF[PPC].dst == 4)	dst  = SRF_A;
	else if(CRF[PPC].dst == 5)	dst  = SRF_M;
	if     (CRF[PPC].src0 <= 1)	src0 = data; 
	else if(CRF[PPC].src0 == 4) src0 = SRF_A;
	else if(CRF[PPC].src0 == 5)	src0 = SRF_M;
	if     (CRF[PPC].src1 <= 1)	src1 = data; 
	else if(CRF[PPC].src1 == 4)	src1 = SRF_A;
	else if(CRF[PPC].src1 == 5) src1 = SRF_M;
  }
  //////////////////////////////////////

  // TODO : execute(CRF[PPC] //
  void Execute(){
	switch(CRF[PPC].PIM_OP){
	  case(ADD):
		cout << "ADD~~\n";
		_ADD();
		break;
	  case(MUL):
		cout << "MUL~~\n";
		break;
	  case(MAC):
		cout << "MAC~~\n";
		break;
	  case(MAD):
		cout << "MAD~~\n";
		break;
	  case(ADD_AAM):
		cout << "ADD_AAM~~\n";
		break;
	  case(MUL_AAM):
		cout << "MUL_AAM~~\n";
		break;
	  case(MAC_AAM):
		cout << "MAC_AAM~~\n";
		break;
	  case(MAD_AAM):
		cout << "MAD_AAM~~\n";
		break;
	  case(MOV):
		cout << "MOV~~\n";
		break;
	  case(FILL):
		cout << "FILL~~\n";
		break;
	  case(EXIT):
		cout << "EXIT~~\n";
		break;
	}
  }

  void _ADD(){
	for(int i=0; i<16; i++){
	  *dst = *src0 + *src1;
	  if(CRF[PPC].dst  != 4 && CRF[PPC].dst  !=5) dst  = dst+1;
	  if(CRF[PPC].src0 != 4 && CRF[PPC].src0 !=5) src0 = src0+1;
	  if(CRF[PPC].src1 != 4 && CRF[PPC].src1 !=5) src1 = src1+1;
	}
  }
  /////////////////////////////

  // ~ the end ~ //
};

