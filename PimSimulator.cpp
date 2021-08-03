#include <iostream>
#include <fstream>
#include <string>
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
  float *physmem;
  PimUnit _PimUnit[NUM_PIMS];

public:
  PimSimulator(){
	clk = 0;
	pim_cmd_filename = "PimCmd.txt";
	pim_micro_kernel_filename = "CRF.txt";
	for(int i=0; i<NUM_PIMS; i++)
	  _PimUnit[i] = PimUnit();
  }

  void PhysmemInit(){
	cout << ">> initializing PHYSMEM...\n";

	physmem = (float*)mmap(NULL, PHYSMEM_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (physmem == (float*) MAP_FAILED) perror("mmap");
	cout << ">> Allocated Space for PHYSMEM\n";
	
	for(int i=0; i<NUM_PIMS; i++)
	  _PimUnit[i].SetPhysmem(physmem + i * PIM_PHYSMEM_SIZE / 4);

	cout << "<< initialized PHYSMEM!\n\n";
	return;
  }

  void CrfInit(){
	cout << ">> initializing PimUnit's CRF...\n";
	for(int i=0; i<1; i++)
	  this->_PimUnit[i].CrfInit(); 
	cout << "<< initialized PimUnit's CRF!\n\n";
  }

  void Run(){
	ifstream fp;
	fp.open("PimCmd.txt");

	string str;
	while(getline(fp, str) && !fp.eof()){
	  string cmd_part[3];
	  int num_parts = (str.size()-1)/10 + 1;
	  int flag = 0;

	  cout << "Cmd "<< clk+1 << " : ";
	  for(int i=0; i<num_parts; i++){
		cmd_part[i] = (str.substr(i*10, 9)).substr(0, str.substr(i*10, 9).find(' '));
		cout << cmd_part[i] << " ";
	  }
	  cout << endl;	
	  
	  for(int j=0; j<1; j++)
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


int main(){
  PimSimulator PimSim = PimSimulator();
  
  PimSim.PhysmemInit();
  PimSim.CrfInit();

  PimSim.Run();
  
  return 0;
}

