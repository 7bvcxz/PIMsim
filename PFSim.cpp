#include <iostream> 
#include <string>
#include "config.h"
#include "PIM_Unit.cpp"
using namespace std;

class PimSimulator{
private:
  PimUnit pim_unit[NUM_PIMS];
  unsigned long long clk;	//uint64
  unsigned char	*physmem;
  string pim_cmd_filename;
  string pim_micro_kernel_filename;

public:
  PimSimulator(){
	for(int i=0; i<NUM_PIMS; i++)
	  this->pim_unit[i] = PimUnit();
	this->clk = 0;
	this->pim_cmd_filename = "Pim_cmd.txt";
	this->pim_micro_kernel_filename = "CRF.txt";
	
  }

  void Physmem_init(){
	cout << "initializing PHYSMEM...\n";
	// todo : initialize phymem //
   

	///////////////////////////////
	cout << "ended\n";
	return;
  }

  void CRF_init(){
	for(int i=0; i<NUM_PIMS; i++)
	  this->pim_unit[i].CRF_init();	  
  }

  void Run(){
	// todo : Fetch Pim Command and send to PIM_Units //
		
	
	////////////////////////////////////////////////////
	
	this->clk ++;
  }
};


int main(){
  PimSimulator PimSim = PimSimulator();
  
  PimSim.Physmem_init();
  PimSim.CRF_init();

  PimSim.Run();

  return 0;
}









