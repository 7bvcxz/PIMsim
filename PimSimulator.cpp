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
  int fd;
  uint8_t *physmem;
  PimUnit _PimUnit[NUM_PIMS];

public:
  PimSimulator(){
	this->clk = 0;
	this->pim_cmd_filename = "Pim_cmd.txt";
	this->pim_micro_kernel_filename = "CRF.txt";
	this->fd = -1;
	for(int i=0; i<NUM_PIMS; i++)
	  this->_PimUnit[i] = PimUnit();	
  }

  void PhysmemInit(){
	cout << "initializing PHYSMEM...\n";

	this->physmem = (uint8_t*)mmap(NULL, PHYSMEM_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, this->fd, 0);
	if (this->physmem == (uint8_t*) MAP_FAILED) perror("mmap");
	cout << "Allocated Space for PHYSMEM\n";
	
	float wr = 20, rd = 0;
	memcpy(this->physmem, &wr, 4);
	memcpy(&rd, this->physmem, 4);
	cout << "wr : " << wr << " -> physmem -> " << "rd : " << rd << endl;
	
	cout << "initialized PHYSMEM!\n\n";
	return;
  }

  void CrfInit(){
	cout << "initializing PimUnit's CRF...\n";
	for(int i=0; i<1; i++)
	  this->_PimUnit[i].CrfInit(); 
	cout << "initialized PimUnit's CRF!\n\n";
  }

  void Run(){
	// TODO : Fetch Pim Command and send to PIM_Units //
	ifstream fp;
	fp.open("PimCmd.txt");

	string str;
	while(getline(fp, str) && !fp.eof()){
	  string cmd_part[3];
	  int num_parts = (str.size()-1)/10 + 1;

	  for(int i=0; i<num_parts; i++){
		cmd_part[i] = (str.substr(i*10, 9)).substr(0, str.substr(i*10, 9).find(' '));
		cout << cmd_part[i] << " ";
	  }
	  cout << endl;	
	  
	  for(int j=0; j<1; j++)
	    this->_PimUnit[j].Issue(cmd_part, num_parts);

	  this->clk ++;
	}
	////////////////////////////////////////////////////
	
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









