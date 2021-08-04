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
};

class PimUnit{
public:  
  PimInstruction CRF[32];
  string pim_micro_kernel_filename;
  uint8_t PPC;
  int RA;
  int LC;
  
  unit_t *_GRF_A;
  unit_t *_GRF_B;
  unit_t *_SRF_A;
  unit_t *_SRF_M;

  unit_t *dst;
  unit_t *src0;
  unit_t *src1;

  unit_t *physmem;
  unit_t *even_data;
  unit_t *odd_data;

  PimUnit(){
	PPC = 0;
	LC  = -1;
	RA  = 0;
	_GRF_A = (unit_t*)mmap(NULL, GRF_SIZE,  PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	_GRF_B = (unit_t*)mmap(NULL, GRF_SIZE,  PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	_SRF_A = (unit_t*)mmap(NULL, SRF_SIZE,  PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	_SRF_M = (unit_t*)mmap(NULL, SRF_SIZE,  PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	even_data  = (unit_t*)mmap(NULL, WORD_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	odd_data   = (unit_t*)mmap(NULL, WORD_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  }
 
  void SetPmkFilename(string pim_micro_kernel_filename){
	this->pim_micro_kernel_filename = pim_micro_kernel_filename;
  }

  void SetPhysmem(unit_t* physmem){
	this->physmem = physmem;
  }

  void CrfInit(){	
	ifstream fp;
	fp.open(pim_micro_kernel_filename);
	
	string str;
	while(getline(fp, str) && !fp.eof()){
	  string mk_part[4];
	  int num_parts = (str.size()-1)/10 + 1;
	  
	  if(str.size() == 0) continue; // just to see CRF.txt easily

	  for(int i=0; i< num_parts; i++){
		mk_part[i] = (str.substr(i*10, 9)).substr(0, str.substr(i*10, 9).find(' '));
		//cout << mk_part[i] << " ";
	  }
	  //cout << endl;	  

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
	else if(CRF[PPC].PIM_OP == NOP){  // nop 
	  CRF[PPC].imm0 = (int)StringToNum(mk_part[1]);
	}
	else if(CRF[PPC].PIM_OP == JUMP){  // jump
	  CRF[PPC].imm0 = PPC + (int)StringToNum(mk_part[1]);
	  CRF[PPC].imm1 = (int)StringToNum(mk_part[2]);
	}
  }

  int Issue(string* pim_cmd, int num_parts){
	//cout << "a\n";
	// DRAM READ & WRITE // 
	if(pim_cmd[0] == "WR"){
	  for(int i=0; i<16; i++){
		unit_t WR = (unit_t)StringToNum(pim_cmd[i+2]);
		memcpy(physmem + RA*UNITS_PER_ROW + (int)StringToNum(pim_cmd[1])*UNITS_PER_WORD + i, &WR, sizeof(unit_t));
		memcpy(physmem + RA*UNITS_PER_ROW + (int)StringToNum(pim_cmd[1])*UNITS_PER_WORD + UNITS_PER_BK + i, &WR, sizeof(unit_t));
	  }
	  even_data = physmem + RA*UNITS_PER_ROW + (int)StringToNum(pim_cmd[1])*UNITS_PER_WORD;
	  odd_data  = physmem + RA*UNITS_PER_ROW + (int)StringToNum(pim_cmd[1])*UNITS_PER_WORD + UNITS_PER_BK;
	}
	else if(pim_cmd[0] == "RD"){
	  even_data = physmem + RA*UNITS_PER_ROW + (int)StringToNum(pim_cmd[1])*UNITS_PER_WORD;
	  odd_data  = physmem + RA*UNITS_PER_ROW + (int)StringToNum(pim_cmd[1])*UNITS_PER_WORD + UNITS_PER_BK;
	}

	// NOP & JUMP // 
	if(CRF[PPC].PIM_OP == NOP){
	  if(LC == -1)	LC = CRF[PPC].imm0 -1;
	  else if(LC > 0) LC -= 1;
	  else if(LC == 0){
		LC = -1;
		return NOP_END;
	  }
	  return 0;
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

	if(CRF[PPC].PIM_OP == EXIT)
	  return EXIT_END;

	// SET ADDR & EXECUTE //
	
	SetOperandAddr(pim_cmd);

	Execute();	
	
	PPC += 1;

	return (int)PPC;
 }

  void SetOperandAddr(string* pim_cmd){
	// set _GRF_A, _GRF_B operand address when AAM mode
	if(CRF[PPC].PIM_OP>=4 && CRF[PPC].PIM_OP<=7){
	  int A_idx = (int)StringToNum(pim_cmd[1])%8;
	  int B_idx = (int)StringToNum(pim_cmd[1])/8 + RA % 2 * 4;
	  
	  // set dst address (AAM)
	  if(CRF[PPC].dst == GRF_A)
		dst = _GRF_A + A_idx * 16;
	  else if(CRF[PPC].dst == GRF_B)
		dst = _GRF_B + B_idx * 16;

	  // set src0 address (AAM)
	  if(CRF[PPC].src0 == GRF_A)
		src0 = _GRF_A + A_idx * 16;
	  else if(CRF[PPC].src0 == GRF_B)
		src0 = _GRF_B + B_idx * 16;
	  
	  // set src1 address (AAM)
	  if(CRF[PPC].src1 == GRF_A)
		src1 = _GRF_A + A_idx * 16;
	  else if(CRF[PPC].src1 == GRF_B)
		src1 = _GRF_B + B_idx * 16;
	}
	// set _GRF_A, _GRF_B operand address when non-AAM mode
	else{
	  // set dst address
	  if(CRF[PPC].dst >=10 && CRF[PPC].dst < 20)
		dst = _GRF_A + CRF[PPC].dst % 10 * 16;	// % 10 = GRF index
	  else if(CRF[PPC].dst >= 20 && CRF[PPC].dst <30)
		dst = _GRF_B + CRF[PPC].dst % 10 * 16;  

	  // set src0 address
	  if(CRF[PPC].src0 >=10 && CRF[PPC].src0 < 20)
		src0 = _GRF_A + CRF[PPC].src0 % 10 * 16;
	  else if(CRF[PPC].src0 >= 20 && CRF[PPC].src0 <30)
		src0 = _GRF_B + CRF[PPC].src0 % 10 * 16;

	  // set src1 address
	  if(CRF[PPC].PIM_OP < 4){ // PIM_OP == ADD, MUL, MAC, MAD -> uses src1 for operand
		if(CRF[PPC].src1 >=10 && CRF[PPC].src1 < 20)
		  src1 = _GRF_A + CRF[PPC].src1 % 10 * 16;
		else if(CRF[PPC].src1 >= 20 && CRF[PPC].src1 <30)
		  src1 = _GRF_B + CRF[PPC].src1 % 10 * 16;	
	  }
	}

	// set EVEN_BANK, ODD_BANK, SRF operand address 
	// set dst address
	if(CRF[PPC].dst == 0)
	  dst = even_data;
	else if(CRF[PPC].dst == 1)
	  dst = odd_data;
	else if(CRF[PPC].dst >= 30)
	  dst = _SRF_A + CRF[PPC].dst % 10;
	else if(CRF[PPC].dst >= 40)
	  dst = _SRF_M + CRF[PPC].dst % 10;

	// set src0 address
	if(CRF[PPC].src0 == 0)
	  src0 = even_data;
	else if(CRF[PPC].src0 == 1)
	  src0 = odd_data;
	else if(CRF[PPC].src0 >= 30)
	  src0 = _SRF_A + CRF[PPC].src0 % 10;
	else if(CRF[PPC].src0 >= 40)
	  src0 = _SRF_M + CRF[PPC].src0 % 10;
	
	// set src1 address only if PIM_OP == ADD, MUL, MAC, MAD -> uses src1 for operand
	if(CRF[PPC].PIM_OP < 8){
	  if(CRF[PPC].src1 == 0)
		src1 = even_data;
	  else if(CRF[PPC].src1 == 1)
		src1 = odd_data;
	  else if(CRF[PPC].src1 >= 30)
		src1 = _SRF_A + CRF[PPC].src1 % 10;
	  else if(CRF[PPC].src1 >= 40)
		src1 = _SRF_M + CRF[PPC].src1 % 10;
	}
  }
  
  void Execute(){
	switch(CRF[PPC].PIM_OP){
	  case(ADD):
	  case(ADD_AAM):
		_ADD();
		break;

	  case(MUL):
	  case(MUL_AAM):
		_MUL();
		break;

	  case(MAC):
	  case(MAC_AAM):
		_MAC();
		break;

	  case(MAD):
	  case(MAD_AAM):
		_MAD();
		break;

	  case(MOV):
	  case(FILL):
		_MOV();
		break;
	}
  }

  void _ADD(){
	for(int i=0; i<16; i++){
	  *dst = *src0 + *src1;
	  if(CRF[PPC].dst < 30) dst  += 1;	// if operand == SRF -> do not change
	  if(CRF[PPC].src0 < 30) src0 += 1;
	  if(CRF[PPC].src1 < 30) src1 += 1;
	}
  }
 
  void _MUL(){
	for(int i=0; i<16; i++){
	  *dst = (*src0) * (*src1);
	  if(CRF[PPC].dst < 30) dst  += 1;	// if operand == SRF -> do not change
	  if(CRF[PPC].src0 < 30) src0 += 1;
	  if(CRF[PPC].src1 < 30) src1 += 1;
	}
  }

  void _MAC(){
	for(int i=0; i<16; i++){
	  *dst += (*src0) * (*src1);
	  if(CRF[PPC].dst < 30) dst  += 1;	// if operand == SRF -> do not change
	  if(CRF[PPC].src0 < 30) src0 += 1;
	  if(CRF[PPC].src1 < 30) src1 += 1;
	}
  }

  void _MAD(){
	cout << "not yet\n";
  }

  void _MOV(){
	for(int i=0; i<16; i++){
	  *dst = *src0;
	  if(CRF[PPC].dst < 30) dst += 1;	// if operand == SRF -> do not change
	  if(CRF[PPC].src0 < 30) src0 += 1;
	}
  }
  
  // ~ the end ~ //
};


