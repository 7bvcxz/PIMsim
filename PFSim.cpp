#include <iostream> 
#include <string>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include "config.h"
#include "PIM_Unit.cpp"
using namespace std;

class PimSimulator{
private:
  uint64_t clk;
  string pim_cmd_filename;
  string pim_micro_kernel_filename;
  int fd;
  uint8_t *physmem;
  PimUnit pim_unit[NUM_PIMS];

public:
  PimSimulator(){
	this->clk = 0;
	this->pim_cmd_filename = "Pim_cmd.txt";
	this->pim_micro_kernel_filename = "CRF.txt";
	this->fd = -1;
	for(int i=0; i<NUM_PIMS; i++)
	  this->pim_unit[i] = PimUnit();	
  }

  void Physmem_init(){
	cout << "initializing PHYSMEM...\n";
	// TODO : initialize phymem //
	//physmem = (uint8_t*) mmap(NULL, PHYSMEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, this->fd, 0);
	

	this->physmem = (uint8_t*)mmap(NULL, 40, PROT_READ | PROT_WRITE, MAP_SHARED, this->fd, 0);
	cout << "Allocated Space for PHYSMEM\n";

	memcpy("abcdefg", this->physmem, 7);
	
	cout << "data : " << *(this->physmem) << endl;
		
	///////////////////////////////
	cout << "initialized PHYSMEM\n\n";
	return;
  }

  void CRF_init(){
	for(int i=0; i<NUM_PIMS; i++)
	  this->pim_unit[i].CRF_init();	  
  }

  void Run(){
	// TODO : Fetch Pim Command and send to PIM_Units //
		
	
	////////////////////////////////////////////////////
	
	this->clk ++;
  }

  // ~ the end ~ //
};


int main(){
  PimSimulator PimSim = PimSimulator();
  
  PimSim.Physmem_init();
  PimSim.CRF_init();

  PimSim.Run();

  
  return 0;
}









