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

// should use "g++ -std=c++11 PimSimulator.cpp" for need of some library

class PimSimulator{
public:
  uint64_t clk;						  // Just a counter checking how many iterations it went 
  string pim_cmd_filename;			  // File name that has Pim Commands to be executed sequentially
  string pim_micro_kernel_filename;	  // File name that has Micro Kernels to be programmed to each Pim Unit's CRF
  PimUnit _PimUnit[NUM_PIMS];		  // Total Pim Units
  unit_t *physmem;				  // A simple physical memory
  
  PimSimulator(){
	clk = 0;
	pim_cmd_filename = "PimCmd.txt";
	pim_micro_kernel_filename = "CRF.txt";
	for(int i=0; i<NUM_PIMS; i++){
	  _PimUnit[i] = PimUnit();
	  _PimUnit[i].SetPmkFilename(pim_micro_kernel_filename);    // Also set Pmk(pim_micro_kernel) Filename to each Pim Unit
	}
  }

  // Allocate Physical Memory and initialize.  
  void PhysmemInit(){
	cout << ">> Initializing PHYSMEM...\n";

	physmem = (unit_t*)mmap(NULL, PHYSMEM_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (physmem == (unit_t*) MAP_FAILED) perror("mmap");
	cout << ">> Allocated Space for PHYSMEM\n";

	// Also connect each Pim Unit's physmem to appropriate physical memory address together
	for(int i=0; i<NUM_PIMS; i++)
	  _PimUnit[i].SetPhysmem(physmem + i * 2 * UNITS_PER_BK);

	cout << "<< Initialized PHYSMEM!\n\n";
	return;
  }

  // Read pim_cmd_filename and Program the whole Pim Unit's CRF with micro kernels
  void CrfInit(){
	cout << ">> Initializing PimUnit's CRF...\n";
	for(int i=0; i<NUM_PIMS; i++)
	  this->_PimUnit[i].CrfInit(); 
	cout << "<< Initialized PimUnit's CRF!\n\n";
  }

  // Run Simulator that acts same as the actual PIM-DRAM AB-PIM mode
  void Run(){
	cout << ">> Run Simulator\n";
	ifstream fp;
	fp.open(pim_cmd_filename);

	string str;
	while(getline(fp, str) && !fp.eof()){
	  string cmd_part[18];
	  int num_parts = (str.size()-1)/10 + 1;
	  int flag = 0;

	  if(str.size() == 0) continue; // just to see PimCmd.txt easily
#ifdef debug_mode
	  cout << "Cmd "<< clk+1 << " : ";
#endif
	  for(int i=0; i<num_parts; i++){
		cmd_part[i] = (str.substr(i*10, 9)).substr(0, str.substr(i*10, 9).find(' '));
#ifdef debug_mode
		cout << cmd_part[i] << " ";
#endif
	  }

	  for(int j=0; j<NUM_PIMS; j++)
	    flag = _PimUnit[j].Issue(cmd_part, num_parts);
	 
	  if(flag == EXIT_END){
#ifdef debug_mode
		cout << "\t==> EXIT END!\n";
#endif
		break;
	  }
	  else if(flag == NOP_END){
#ifdef debug_mode
		cout << "\t==> NOP END!\n";
#endif
		break;
	  }
#ifdef debug_mode
	  cout << "\t==> execute PPC : " << flag-1 << endl;
#endif

	  clk ++;
	}
	cout << "<< Run End\n";
  }
  // ~ the end ~ //
};

void AddTest(PimSimulator PimSim){
  unit_t A[2048];
  unit_t B[2048];
  unit_t C[2048];
  unit_t D[2048];

  for(int i=0; i<2048; i++){
	A[i] = (unit_t)rand() / RAND_MAX;
	B[i] = (unit_t)rand() / RAND_MAX;
	C[i] = A[i] + B[i];
  }

  for(int i=0; i<NUM_PIMS; i++){
	for(int j=0; j<16; j++){
	   memcpy(PimSim.physmem + 2 * UNITS_PER_BK * i + j, &A[i*16 + j], sizeof(unit_t));
	   memcpy(PimSim.physmem + 2 * UNITS_PER_BK * i + j + 16, &B[i*16 + j], sizeof(unit_t));
	}
  }

  // PIM ~~~ //
  PimSim.CrfInit();
  PimSim.Run();
  // Ended,, //

  for(int i=0; i<NUM_PIMS; i++){
	for(int j=0; j<16; j++){
	  memcpy(&D[i*16 + j], PimSim.physmem + 2 * UNITS_PER_BK * i + j, sizeof(unit_t));
	}
  }

  unit_t err = 0;
  for(int i=0; i<2048; i++)
	err += C[i] - D[i];

  cout << "error : " << (float)err << endl;
}

void AddAamTest(PimSimulator PimSim){
  unit_t A[16384];
  unit_t B[16384];
  unit_t C[16384];
  unit_t D[16384];

  for(int i=0; i<16384; i++){
	A[i] = (unit_t)rand() / RAND_MAX;
	B[i] = (unit_t)rand() / RAND_MAX;
	C[i] = A[i] + B[i];
  }

  for(int i=0; i<NUM_PIMS; i++){
	for(int j=0; j<8; j++){
	  for(int k=0; k<16; k++){
		memcpy(PimSim.physmem + 2 * UNITS_PER_BK * i + j * 16 + k, &A[i*8*16 + j*16 + k], sizeof(unit_t));
		memcpy(PimSim.physmem + 2 * UNITS_PER_BK * i + j * 16 + k + 8 * 16, &B[i*8*16 + j*16 + k], sizeof(unit_t));
	  }
	}
  }

  // PIM ~~~ //
  PimSim.CrfInit();
  PimSim.Run();
  // Ended,, //

  for(int i=0; i<NUM_PIMS; i++){
	for(int j=0; j<8; j++){
	  for(int k=0; k<16; k++){
		memcpy(&D[i*8*16 + j*16 + k], PimSim.physmem + 2 * UNITS_PER_BK * i + j * 16 + k, sizeof(unit_t));
	  }
	}
  }

  unit_t err = 0;
  for(int i=0; i<16384; i++)
	err += C[i] - D[i];

  cout << "error : " << (float)err << endl;
}

void GemvTest(PimSimulator PimSim){
  // A[M][N], B[M], O[N]
  unit_t A[512][128];
  unit_t B[512];
  unit_t C[128];
  unit_t _D[128];
  unit_t D[128];

  for(int m=0; m<512; m++){
	B[m] = (unit_t)((double)rand()/RAND_MAX*100);
	for(int n=0; n<128; n++)
	  A[m][n] = (unit_t)((double)rand()/RAND_MAX*100);
  }
  
  for(int m=0; m<512; m++)
	for(int n=0; n<128; n++)
	  C[n] += B[m] * A[m][n];

  for(int i=0; i<NUM_PIMS; i++){
	for(int m=0; m<4; m++){
	  for(int n=0; n<128; n++){
		memcpy(PimSim.physmem + 2 * UNITS_PER_BK * i + m * 128 + n, &A[i*4 + m][n], sizeof(unit_t));
	  }
	  memcpy(PimSim._PimUnit[i]._SRF_A + m, &B[i*4 + m], sizeof(unit_t));
	}
  }

  // PIM ~~~ //
  PimSim.CrfInit();
  PimSim.Run();
  // Ended,, //

  for(int i=0; i<NUM_PIMS; i++){
	for(int j=0; j<128; j++){
	  memcpy(&_D[j], PimSim.physmem + 2 * UNITS_PER_BK * i + j, sizeof(unit_t));
	  D[j] += _D[j];
	}
  }
  
  unit_t err = 0;
  for(int n=0; n<128; n++){
#ifdef debug_mode
	cout << (float)C[n] << "\t" << (float)D[n] << "\t sub : " << (float)C[n] - (float)D[n]<< endl;
#endif
	err += ABS(C[n] - D[n]);
  }
  cout << "error : " << (float)err << endl;
}


int main(){
  srand((unsigned)time(NULL));
  PimSimulator PimSim = PimSimulator();
  
  PimSim.PhysmemInit();

  //AddTest(PimSim);
  //AddAamTest(PimSim);
  GemvTest(PimSim);

  //PimSim.CrfInit();
  //PimSim.Run();

  return 0;
}
