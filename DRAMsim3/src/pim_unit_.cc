#include "pim_unit.h"

// >> mmm
namespace dramsim3 {

PimUnit::PimUnit(Config &config)
	: config_(config)
{
	SRF_A_ = (uint8_t*)malloc(SRF_SIZE);
	SRF_M_ = (uint8_t*)malloc(SRF_SIZE);
	GRF_A_ = (uint8_t*)malloc(GRF_SIZE);
	GRF_B_ = (uint8_t*)malloc(GRF_SIZE);
	CRF_   = (uint8_t*)malloc(GRF_SIZE);
	bank_data_ = (uint8_t*)malloc(WORD_SIZE);
	PPC = 0;
	LC = 0;
}

void PimUnit::init(uint8_t* pmemAddr, uint64_t pmemAddr_size, unsigned int burstSize) {
	pmemAddr_ = pmemAddr;
	pmemAddr_size_ = pmemAddr_size;
	burstSize_ = burstSize;
}

void PimUnit::AddTransaction(uint64_t hex_addr, bool is_write, uint8_t* DataPtr) {
	//std::cout << "play the PIM~\n";		
	memcpy(bank_data_, DataPtr, burstSize_);

	
}

void PimUnit::SetSRF(uint64_t hex_addr, uint8_t* DataPtr) {
	//std::cout << "set SRF\n";
	memcpy(SRF_A_, DataPtr, SRF_SIZE);
	memcpy(SRF_M_, DataPtr + SRF_SIZE, SRF_SIZE);
}

void PimUnit::SetGRF(uint64_t hex_addr, uint8_t* DataPtr) {
	//std::cout << "set GRF\n";
	Address addr = config_.AddressMapping(hex_addr);
	if(addr.column < 8) {  // GRF_A
		uint8_t* target = GRF_A_ + addr.column *WORD_SIZE; 
		memcpy(target, DataPtr, WORD_SIZE);
	}
	else {  // GRF_B
		uint8_t* target = GRF_B_ + (addr.column-8) *WORD_SIZE; 
		memcpy(target, DataPtr, WORD_SIZE);
	}
}

void PimUnit::SetCRF(uint64_t hex_addr, uint8_t* DataPtr) {
	//std::cout << "set CRF\n";
	Address addr = config_.AddressMapping(hex_addr);
	uint8_t* target = CRF_ + addr.column *PIM_INST_SIZE; 
	memcpy(target, DataPtr, PIM_INST_SIZE);
}



} // namespace dramsim3





