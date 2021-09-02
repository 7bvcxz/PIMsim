#ifndef __PIM_UNIT_H
#define __PIM_UNIT_H

#define SRF_SIZE 16		// =2Byte*8
#define WORD_SIZE 32	// =2Byte*16
#define GRF_SIZE 256	// =WORD_SIZE*8
#define PIM_INST_SIZE 4
#define CRF_SIZE 128	// =PIM_INST_SIZE*32

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "configuration.h"
#include "common.h"

// >> mmm
namespace dramsim3 {

class PimUnit {
	public:
	  PimUnit(Config &config);
	  void AddTransaction(uint64_t hex_addr, bool is_write, uint8_t* DataPtr);
	  void SetSRF(uint64_t hex_addr, uint8_t* DataPtr);
	  void SetGRF(uint64_t hex_addr, uint8_t* DataPtr);
	  void SetCRF(uint64_t hex_addr, uint8_t* DataPtr);
	  void init(uint8_t* pmemAddr, uint64_t pmemAddr_size, unsigned int burstSize);
	  uint8_t* pmemAddr_;
	  uint64_t pmemAddr_size_;
	  unsigned int burstSize_;

	  uint8_t *SRF_A_;
	  uint8_t *SRF_M_;
	  uint8_t *GRF_A_;
	  uint8_t *GRF_B_;
	  uint8_t *CRF_;

	  uint8_t *bank_data_;

	  int PPC;
	  int LC;

	protected:
	  Config &config_;
};

} // namespace dramsim3 

// mmm << 
#endif
