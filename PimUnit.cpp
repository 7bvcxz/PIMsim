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
private:
  PimInstruction CRF[32];
  uint8_t PPC;
  int RA;
  int LC;
  sector_t *GRF_A;
  sector_t *GRF_B;
  sector_t *SRF_A;
  sector_t *SRF_M;

  sector_t *dst;
  sector_t *src0;
  sector_t *src1;

  sector_t *physmem;
  sector_t *even_data;
  sector_t *odd_data;

public:
  PimUnit(){
	PPC = 0;
	LC  = -1;
	RA  = 0;
	GRF_A = (sector_t*)mmap(NULL, GRF_SIZE,  PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	GRF_B = (sector_t*)mmap(NULL, GRF_SIZE,  PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	SRF_A = (sector_t*)mmap(NULL, SRF_SIZE,  PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	SRF_M = (sector_t*)mmap(NULL, SRF_SIZE,  PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	even_data  = (sector_t*)mmap(NULL, CELL_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	odd_data   = (sector_t*)mmap(NULL, CELL_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  }
  
  void SetPhysmem(sector_t* physmem){
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
	  CRF[PPC].imm0 = (int)StringToNum(mk_part[1]);
	}
	else if(CRF[PPC].PIM_OP < 12){  // jump
	  CRF[PPC].imm0 = PPC + (int)StringToNum(mk_part[1]);
	  CRF[PPC].imm1 = (int)StringToNum(mk_part[2]);
	}
  }

  int Issue(string* pim_cmd, int num_parts){

	// DRAM READ & WRITE // 
	if(pim_cmd[0] == "WR"){
	  for(int i=0; i<16; i++){
		sector_t WR = (sector_t)StringToNum(pim_cmd[i+2]);
		memcpy(physmem + RA*SECTORS_PER_ROW + (int)StringToNum(pim_cmd[1])*SECTORS_PER_CELL + i, &WR, sizeof(sector_t));
		memcpy(physmem + RA*SECTORS_PER_ROW + (int)StringToNum(pim_cmd[1])*SECTORS_PER_CELL + SECTORS_PER_BK + i, &WR, sizeof(sector_t));
	  }
	  even_data = physmem + RA*SECTORS_PER_ROW + (int)StringToNum(pim_cmd[1])*SECTORS_PER_CELL;
	  odd_data  = physmem + RA*SECTORS_PER_ROW + (int)StringToNum(pim_cmd[1])*SECTORS_PER_CELL + SECTORS_PER_BK;
	}
	else if(pim_cmd[0] == "RD"){
	  //sector_t RD = 0;
	  //memcpy(&RD, physmem + (int)StringToNum(pim_cmd[1])*SECTORS_PER_CELL, sizeof(sector_t));
	  //cout << "RD even_data[0] : " << RD << endl;
	  even_data = physmem + RA*SECTORS_PER_ROW + (int)StringToNum(pim_cmd[1])*SECTORS_PER_CELL;
	  odd_data  = physmem + RA*SECTORS_PER_ROW + (int)StringToNum(pim_cmd[1])*SECTORS_PER_CELL + SECTORS_PER_BK;
	}

	// NOP & JUMP // 
	if(CRF[PPC].PIM_OP == NOP){
	  if(LC == -1)	LC = CRF[PPC].imm0 -1;
	  else if(LC > 0) LC -= 1;
	  else if(LC == 0){
		LC = -1;
		return NOP_END;
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

	if(CRF[PPC].PIM_OP == EXIT)
	  return EXIT_END;

	// SET ADDR & EXECUTE // 
	cout << "execute PPC : " << (int)PPC << endl;

	SetOperandAddr(pim_cmd);

	Execute();	
	
	PPC += 1;

	cout << endl;
	return 0;
 }

  void SetOperandAddr(string* pim_cmd){

	if(CRF[PPC].PIM_OP == 12) return;  // EXIT

	// GRF_A, GRF_B
	else if(CRF[PPC].PIM_OP>=4 && CRF[PPC].PIM_OP<=7){  // AAM mode
	  int A_idx = (int)StringToNum(pim_cmd[1])%8;
	  int B_idx = (int)StringToNum(pim_cmd[1])/8; // + RA % 2 * 4
	  
	  if     (CRF[PPC].dst == 2)  dst = GRF_A + A_idx * 16;
	  else if(CRF[PPC].dst == 3)  dst = GRF_B + B_idx * 16;
	  if     (CRF[PPC].src0 == 2) src0 = GRF_A + A_idx * 16;
	  else if(CRF[PPC].src0 == 3) src0 = GRF_B + B_idx * 16;
	  if     (CRF[PPC].src1 == 2) src1 = GRF_A + A_idx * 16;
	  else if(CRF[PPC].src1 == 3) src1 = GRF_B + B_idx * 16;
	}
	else{  // non-AAM mode
	  if(CRF[PPC].dst >=10 && CRF[PPC].dst < 20)		dst = GRF_A + CRF[PPC].dst % 10 * 16;	// %10 = GRF index
	  else if(CRF[PPC].dst >= 20 && CRF[PPC].dst <30)	dst = GRF_B + CRF[PPC].dst % 10 * 16;  
	  if(CRF[PPC].src0 >=10 && CRF[PPC].src0 < 20)		src0 = GRF_A + CRF[PPC].src0 % 10 * 16;
	  else if(CRF[PPC].src0 >= 20 && CRF[PPC].src0 <30)	src0 = GRF_B + CRF[PPC].src0 % 10 * 16;
	  if(CRF[PPC].PIM_OP < 4){ // ADD, MUL, MAC, MAD --> dst, src0, src1
		if(CRF[PPC].src1 >=10 && CRF[PPC].src1 < 20)		src1 = GRF_A + CRF[PPC].src1 % 10 * 16;
		else if(CRF[PPC].src1 >= 20 && CRF[PPC].src1 <30)	src1 = GRF_B + CRF[PPC].src1 % 10 * 16;	
	  }
	}

	// bank, SRF //
	if     (CRF[PPC].dst  == 0)	dst  = even_data; 
	else if(CRF[PPC].dst  == 1)	dst  = odd_data; 
	else if(CRF[PPC].dst  == 4)	dst  = SRF_A;
	else if(CRF[PPC].dst  == 5)	dst  = SRF_M;
	if     (CRF[PPC].src0 == 0)	src0 = even_data; 
	else if(CRF[PPC].src0 == 1)	src0 = odd_data; 
	else if(CRF[PPC].src0 == 4) src0 = SRF_A;
	else if(CRF[PPC].src0 == 5)	src0 = SRF_M;
	if(CRF[PPC].PIM_OP < 4){ // ADD, MUL, MAC, MAD --> dst, src0, src1
	  if     (CRF[PPC].src1 == 0)	src1 = even_data; 
	  else if(CRF[PPC].src1 == 1)	src1 = odd_data; 
	  else if(CRF[PPC].src1 == 4)	src1 = SRF_A;
	  else if(CRF[PPC].src1 == 5)	src1 = SRF_M;
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
	  cout << *src0 << " + " << *src1 << endl;
	  *dst = *src0 + *src1;
	  if(CRF[PPC].dst  != 4 && CRF[PPC].dst  !=5) dst  += 1;  // checking SRF
	  if(CRF[PPC].src0 != 4 && CRF[PPC].src0 !=5) src0 += 1;
	  if(CRF[PPC].src1 != 4 && CRF[PPC].src1 !=5) src1 += 1;
	}
  }
 
  void _MUL(){
	for(int i=0; i<16; i++){
	  *dst = (*src0) * (*src1);
	  if(CRF[PPC].dst  != 4 && CRF[PPC].dst  !=5) dst  += 1;  // checking SRF
	  if(CRF[PPC].src0 != 4 && CRF[PPC].src0 !=5) src0 += 1;
	  if(CRF[PPC].src1 != 4 && CRF[PPC].src1 !=5) src1 += 1;
	}
  }

  void _MAC(){
	for(int i=0; i<16; i++){
	  *dst += (*src0) * (*src1);
	  if(CRF[PPC].dst  != 4 && CRF[PPC].dst  !=5) dst  += 1;  // checking SRF
	  if(CRF[PPC].src0 != 4 && CRF[PPC].src0 !=5) src0 += 1;
	  if(CRF[PPC].src1 != 4 && CRF[PPC].src1 !=5) src1 += 1;
	}
  }

  void _MAD(){
	cout << "not yet\n";
  }

  void _MOV(){
	for(int i=0; i<16; i++){
	  *dst = *src0;
	  if(CRF[PPC].dst  != 4 && CRF[PPC].dst  !=5) dst  += 1;  // checking SRF
	  if(CRF[PPC].src0 != 4 && CRF[PPC].src0 !=5) src0 += 1;
	}
  }
  
  // ~ the end ~ //
};


