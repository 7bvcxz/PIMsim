#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include "config.h"
#include "PimUnit.cpp"
using namespace std;

class PimSimulator{
private:
  uint64_t clk;
  string pim_cmd_filename;
  string pim_micro_kernel_filename;
  PimUnit _PimUnit[NUM_PIMS];

public:
  sector_t *physmem;
  
  PimSimulator(){
	clk = 0;
	pim_cmd_filename = "PimCmd.txt";
	pim_micro_kernel_filename = "CRF.txt";
	for(int i=0; i<NUM_PIMS; i++)
	  _PimUnit[i] = PimUnit();
  }

  void PhysmemInit(){
	cout << ">> initializing PHYSMEM...\n";

	physmem = (sector_t*)mmap(NULL, PHYSMEM_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (physmem == (sector_t*) MAP_FAILED) perror("mmap");
	cout << ">> Allocated Space for PHYSMEM\n";
	
	for(int i=0; i<NUM_PIMS; i++)
	  _PimUnit[i].SetPhysmem(physmem + i * 2 * SECTORS_PER_BK);

	cout << ">> Allocated to each PimUnit\n";

	cout << "<< initialized PHYSMEM!\n\n";
	return;
  }

  void CrfInit(){
	cout << ">> initializing PimUnit's CRF...\n";
	for(int i=0; i<NUM_PIMS; i++)
	  this->_PimUnit[i].CrfInit(); 
	cout << "<< initialized PimUnit's CRF!\n\n";
  }

  void Run(){
	cout << ">> Run Simulator\n";
	ifstream fp;
	fp.open("PimCmd.txt");

	string str;
	while(getline(fp, str) && !fp.eof()){
	  string cmd_part[18];
	  int num_parts = (str.size()-1)/10 + 1;
	  int flag = 0;

	  cout << "Cmd "<< clk+1 << " : ";
	  for(int i=0; i<num_parts; i++){
		cmd_part[i] = (str.substr(i*10, 9)).substr(0, str.substr(i*10, 9).find(' '));
		cout << cmd_part[i] << " ";
	  }
	  cout << endl;	

	  for(int j=0; j<NUM_PIMS; j++)
	    flag = _PimUnit[j].Issue(cmd_part, num_parts);
	 
	  if(flag == EXIT_END){
		cout << "EXIT END!\n";
		break;
	  }
	  else if(flag == NOP_END){
		cout << "NOP END!\n";
		break;
	  }

	  clk ++;
	}
  }

  // ~ the end ~ //
};

void AddTest(PimSimulator PimSim){
  srand((unsigned)time(NULL));
  sector_t A[2048];
  sector_t B[2048];
  sector_t C[2048];
  sector_t D[2048];

  for(int i=0; i<2048; i++){
	A[i] = (sector_t)rand() / RAND_MAX;
	B[i] = (sector_t)rand() / RAND_MAX;
	C[i] = A[i] + B[i];
  }

  for(int i=0; i<NUM_PIMS; i++){
	for(int j=0; j<16; j++){
	   memcpy(PimSim.physmem + 2 * SECTORS_PER_BK * i + j, &A[i*16 + j], sizeof(sector_t));
	   memcpy(PimSim.physmem + 2 * SECTORS_PER_BK * i + j + 16, &B[i*16 + j], sizeof(sector_t));
	}
  }

  PimSim.CrfInit();
  PimSim.Run();

  for(int i=0; i<NUM_PIMS; i++){
	for(int j=0; j<16; j++){
	  memcpy(&D[i*16 + j], PimSim.physmem + 2 * SECTORS_PER_BK * i + j, sizeof(sector_t));
	}
  }

  sector_t err = 0;
  for(int i=0; i<2048; i++)
	err += C[i] - D[i];

  cout << "error : " << err << endl;
}

int main(){
  PimSimulator PimSim = PimSimulator();
  
  PimSim.PhysmemInit();

  AddTest(PimSim);

  return 0;
}

