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
  uint32_t LC;
  vector<vector<uint16_t> > GRF_A;
  vector<vector<uint16_t> > GRF_B;
  vector<vector<uint16_t> > SRF_A;
  vector<vector<uint16_t> > SRF_M;
  uint8_t *physmem;

public:
  PimUnit(){
	this->PPC = 0;
	this->LC  = -1;
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

	  this->PushCrf(mk_part, num_parts);
	  this->PPC += 1;
	}
	this->PPC = 0;

	fp.close();
  }

  void PushCrf(string* mk_part, int num_parts){
	this->CRF[PPC].PIM_OP = StringToPIM_OP(mk_part[0]);

	if(this->CRF[PPC].PIM_OP < 8){
	  this->CRF[PPC].dst  = StringToOperand(mk_part[1]);
	  this->CRF[PPC].src0 = StringToOperand(mk_part[2]);
	  this->CRF[PPC].src1 = StringToOperand(mk_part[3]);
	}
	else if(this->CRF[PPC].PIM_OP < 10){
	  this->CRF[PPC].dst  = StringToOperand(mk_part[1]);
	  this->CRF[PPC].src0 = StringToOperand(mk_part[2]); 
	}
	else if(this->CRF[PPC].PIM_OP < 11){
	  this->CRF[PPC].imm0 = StringToNum(mk_part[1]);
	}
	else if(this->CRF[PPC].PIM_OP < 12){
	  this->CRF[PPC].imm0 = this->PPC + StringToNum(mk_part[1]);
	  this->CRF[PPC].imm1 = StringToNum(mk_part[2]);
	}

  }


  // TODO : issue(pim_cmd) //
  void Issue(string* pim_cmd, int num_parts){
	if(pim_cmd[0] == "WR"){
	  // WR  3  156
	}
	else if(pim_cmd[0] == "RD"){
	  // RD  3 
	}

	if(this->CRF[PPC].PIM_OP == NOP){
	  if(this->LC == -1)	this->LC = this->CRF[PPC].imm0 -1;
	  else if(this->LC > 0)	this->LC -= 1;
	  else if(this->LC == 0){
		this->LC = -1;
		return;
	  }
	}

	else if(this->CRF[PPC].PIM_OP == JUMP){
	  if(this->LC == -1){
		this->LC = this->CRF[PPC].imm1 -1;
		this->PPC = this->CRF[PPC].imm0;
	  }
	  else if(this->LC > 0){
		this->PPC = this->CRF[PPC].imm0;
		this->LC -= 1;
	  }
	  else if(this->LC == 0){
		this->PPC += 1;
		this->LC  -= 1;
	  }
	}
	
	cout << "execute PPC : " << (int)this->PPC << endl;
	
	this->SetOperandAddr(pim_cmd);

	this->Execute();	
	
	this->PPC += 1;

	cout << endl;
 }

  ///////////////////////////


  // TODO : set_operand_addr(pim_cmd) //
  void SetOperandAddr(string* pim_cmd){
	

  }
  //////////////////////////////////////
 

  // TODO : execute(CRF[PPC] //
  void Execute(){
	// run CRF[PPC] // 
  

	// ta da !      //
  }
  /////////////////////////////

  // ~ the end ~ //
};

