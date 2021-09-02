#include "pim_func_sim.h"
#include <assert.h>

// >> mmm
namespace dramsim3 {

PimFuncSim::PimFuncSim(Config &config)
	: config_(config)
{
	std::cout << "PimFuncSim called!\n";
	for (int i=0; i<config_.channels * config_.banks / 2; i++){
		pim_unit_.push_back(new PimUnit(config_, i));  // >> pim_unit~ <<
	}
} 

void PimFuncSim::init(uint8_t* pmemAddr_, uint64_t pmemAddr_size_, unsigned int burstSize_) {
	burstSize = burstSize_;
	pmemAddr_size = pmemAddr_size_;
	pmemAddr = pmemAddr_;
	
	for (int i=0; i< config_.channels; i++){
		bankmode.push_back("SB");
		PIM_OP_MODE.push_back(false);
	}
	std::cout << "PimFuncSim initialized!\n";
	
	for (int i=0; i< config_.channels * config_.banks / 2; i++) {
		pim_unit_[i]->init(pmemAddr, pmemAddr_size, burstSize);
	}
}

uint64_t PimFuncSim::UnAddressMapping(int channel, int rank, int bankgroup, int bank, int row, int column){
	return (channel << config_.ch_pos) +
		   (rank << config_.ra_pos) + 
		   (bankgroup << config_.bg_pos) + 
		   (bank << config_.ba_pos) +
	       (row << config_.ro_pos) + 
		   (column << config_.co_pos);
}

bool PimFuncSim::DebugMode(uint64_t hex_addr) {
	Address addr = config_.AddressMapping(hex_addr);

	if(addr.channel == 0 && addr.bank == 0) return true;
	return false;
}

bool PimFuncSim::ModeChanger(uint64_t hex_addr){
	Address addr = config_.AddressMapping(hex_addr);
	if(addr.row == 0x3fff) {
		if(bankmode[addr.channel] == "AB") {
			bankmode[addr.channel] = "SB";  
		}
		if(DebugMode(hex_addr))
			std::cout << "AB -> SB\n";
		return true;
	}
	else if(addr.row == 0x3ffe) {
		if(bankmode[addr.channel] == "SB") {
			bankmode[addr.channel] = "AB";
		}
		if(DebugMode(hex_addr))
			std::cout << "SB -> AB\n";
		return true;
	}
	else if(addr.row == 0x3ffd) {
		if(DebugMode(hex_addr))
			std::cout << "CHANGEtoPIM\n";
		PIM_OP_MODE[addr.channel] = true;
		return true;
	}
	return false;
} 

void PimFuncSim::PmemWrite(uint64_t hex_addr, uint8_t* DataPtr) {
	uint8_t *host_addr = pmemAddr + hex_addr;
	memcpy(host_addr, DataPtr, burstSize);
}

void PimFuncSim::PmemRead(uint64_t hex_addr, uint8_t* DataPtr) {
	uint8_t *host_addr = pmemAddr + hex_addr;
	memcpy(DataPtr, host_addr, burstSize);
}

void PimFuncSim::AddTransaction(uint64_t hex_addr, bool is_write, uint8_t* DataPtr) {
	//std::cout << "Trans -> PimFuncSim\n";
	Address addr = config_.AddressMapping(hex_addr);
	if(PIM_OP_MODE[addr.channel] == false) {
		if(ModeChanger(hex_addr)) {   // Just change bankmode register
		// Mode Change Completed~
		}
		else {	  // send to PimUnit and execute
			if(bankmode[addr.channel] == "SB") {  // Single Bank Mode
				if(DebugMode(hex_addr))
					std::cout << "SB ";
				if(addr.row == 0x3ffa) {  // set SRF_A, SRF_M
					int pim_index = (addr.channel*config_.banks + addr.bank)/2; 
					pim_unit_[pim_index]->SetSrf(hex_addr, DataPtr);
				}
				else if(addr.row == 0x3ffb) {  // set GRF_A, GRF_B
					int pim_index = (addr.channel*config_.banks + addr.bank)/2; 
					pim_unit_[pim_index]->SetGrf(hex_addr, DataPtr);
				}
				else if(addr.row == 0x3ffc) {  // set CRF
					int pim_index = (addr.channel*config_.banks + addr.bank)/2; 
					pim_unit_[pim_index]->SetCrf(hex_addr, DataPtr);
				}
				else {  // RD, WR
					if(is_write) {
						PmemWrite(hex_addr, DataPtr);
					}
					else {
						PmemRead(hex_addr, DataPtr);
					}
				}
			}
			else {
				if(!PIM_OP_MODE[addr.channel]) {  // All Bank Mode
					if(DebugMode(hex_addr))
						std::cout << "AB ";
					if(addr.row == 0x3ffa) {  // set SRF_A, SRF_M
						for(int i=0; i<config_.banks/2; i++) {
							int pim_index = addr.channel*config_.banks/2 + i;
							pim_unit_[pim_index]->SetSrf(hex_addr, DataPtr);
						}
					}
					else if(addr.row == 0x3ffb) {  // set GRF_A, GRF_B
						for(int i=0; i<config_.banks/2; i++) {
							int pim_index = addr.channel*config_.banks/2 + i;
							pim_unit_[pim_index]->SetGrf(hex_addr, DataPtr);
						}
					}
					else if(addr.row == 0x3ffc) {  // set CRF
						if(DebugMode(hex_addr))
							std::cout << "SetCrf\n";
						for(int i=0; i<config_.banks/2; i++) {
							int pim_index = addr.channel*config_.banks/2 + i;
							pim_unit_[pim_index]->SetCrf(hex_addr, DataPtr);
						}
					}
					else {  // RD, WR
						int evenodd = addr.bank % 2;    // check if it is evenbank or oddbank
						uint64_t base_addr = hex_addr - (addr.bank << config_.ba_pos);		
						if(is_write) {
							for(int i=evenodd; i<config_.banks; i+=2) {
								uint64_t tmp_addr = base_addr + (i >> config_.ba_pos);
								PmemWrite(tmp_addr, DataPtr);
							}
					  	}
						else {
							for(int i=evenodd; i<config_.banks; i+=2) {
								uint64_t tmp_addr = base_addr + (i >> config_.ba_pos);
								PmemRead(tmp_addr, DataPtr);
							}
						}
					}
				}
			}
		}
	}
	else {	// PIM Mode
		if(DebugMode(hex_addr))
			std::cout << "DRAM CMD --> compute PIM \n";
		if(addr.row == 0x3ffa) {  // set SRF_A, SRF_M
			for(int i=0; i<config_.banks/2; i++) {
				int pim_index = addr.channel*config_.banks/2 + i;
				pim_unit_[pim_index]->SetSrf(hex_addr, DataPtr);
			}
		}
		else if(addr.row == 0x3ffb) {  // set GRF_A, GRF_B
			for(int i=0; i<config_.banks/2; i++) {
				int pim_index = addr.channel*config_.banks/2 + i;
				pim_unit_[pim_index]->SetGrf(hex_addr, DataPtr);
			}
		}
		else if(addr.row == 0x3ffc) {  // set CRF
			for(int i=0; i<config_.banks/2; i++) {
				int pim_index = addr.channel*config_.banks/2 + i;
				pim_unit_[pim_index]->SetCrf(hex_addr, DataPtr);
			}
		}
		else {  // RD, WR
			int evenodd = addr.bank % 2;    // check if it is evenbank or oddbank
			uint64_t base_addr = hex_addr - (addr.bank << config_.ba_pos);		
			if(is_write) {
				for(int i=evenodd; i<config_.banks; i+=2) {
					uint64_t tmp_addr = base_addr + (i >> config_.ba_pos);
					PmemWrite(tmp_addr, DataPtr);
					int pim_index = (addr.channel*config_.banks + i)/2; 
					
					if(pim_unit_[pim_index]->AddTransaction(tmp_addr, is_write, DataPtr) == EXIT_END) {
						
						if(DebugMode(hex_addr))
							std::cout << "CHANGEtoNONPIM\n";
						PIM_OP_MODE[addr.channel] = false;
					}
				}
			}
			else {
				for(int i=evenodd; i<config_.banks; i+=2) {
					uint64_t tmp_addr = base_addr + (i >> config_.ba_pos);
					PmemRead(tmp_addr, DataPtr);
					int pim_index = (addr.channel*config_.banks + i)/2; 
					if(pim_unit_[pim_index]->AddTransaction(tmp_addr, is_write, DataPtr) == EXIT_END) {
						if(DebugMode(hex_addr))
							std::cout << "!CHANGEtoNONPIM! ";
						PIM_OP_MODE[addr.channel] = false;
					}
				}
			}
		}
	}
}

} // namespace dramsim3
// mmm <<
