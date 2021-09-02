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

uint64_t PimFuncSim::ReverseAddressMapping(Address& addr) {
    uint64_t hex_addr = 0;
    hex_addr += (uint64_t)addr.channel << config_.ch_pos;
    hex_addr += (uint64_t)addr.rank << config_.ra_pos;
    hex_addr += (uint64_t)addr.bankgroup << config_.bg_pos;
    hex_addr += (uint64_t)addr.bank << config_.ba_pos;
    hex_addr += (uint64_t)addr.row << config_.ro_pos;
    hex_addr += (uint64_t)addr.column << config_.co_pos;
    return hex_addr << config_.shift_bits;
}

uint64_t PimFuncSim::GetPimIndex(Address& addr) {
	return (addr.channel * config_.banks + addr.bankgroup * config_.banks_per_group + addr.bank) / 2;
}

bool PimFuncSim::DebugMode(uint64_t hex_addr) {
	Address addr = config_.AddressMapping(hex_addr);

	if(addr.channel == 0 && addr.bank == 0)
		return true;
	else
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

	bool is_mode_change = ModeChanger(hex_addr);   // Just change bankmode register
	if (is_mode_change)
		return;

	if(PIM_OP_MODE[addr.channel] == false) {
		
		// send to PimUnit and execute
		if(bankmode[addr.channel] == "SB") {  // Single Bank Mode
			if(DebugMode(hex_addr))
				std::cout << "SB ";
			if(addr.row == 0x3ffa) {  // set SRF_A, SRF_M
				int pim_index = GetPimIndex(addr);
				pim_unit_[pim_index]->SetSrf(hex_addr, DataPtr);
			}
			else if(addr.row == 0x3ffb) {  // set GRF_A, GRF_B
				int pim_index = GetPimIndex(addr);
				pim_unit_[pim_index]->SetGrf(hex_addr, DataPtr);
			}
			else if(addr.row == 0x3ffc) {  // set CRF
				int pim_index = GetPimIndex(addr);
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
		else if (bankmode[addr.channel] == "AB") {
			if(!PIM_OP_MODE[addr.channel]) {  // All Bank Mode
				if(DebugMode(hex_addr))
					std::cout << "AB ";
				if(addr.row == 0x3ffa) {  // set SRF_A, SRF_M
					for(int i=0; i<config_.banks/2; i++) {
						int pim_index = GetPimIndex(addr) + i;
						pim_unit_[pim_index]->SetSrf(hex_addr, DataPtr);
					}
				}
				else if(addr.row == 0x3ffb) {  // set GRF_A, GRF_B
					for(int i=0; i<config_.banks/2; i++) {
						int pim_index = GetPimIndex(addr) + i;
						pim_unit_[pim_index]->SetGrf(hex_addr, DataPtr);
					}
				}
				else if(addr.row == 0x3ffc) {  // set CRF
					if(DebugMode(hex_addr))
						std::cout << "SetCrf\n";
					for(int i=0; i<config_.banks/2; i++) {
						int pim_index = GetPimIndex(addr) + i;
						pim_unit_[pim_index]->SetCrf(hex_addr, DataPtr);
					}
				}
				else {  // RD, WR
					int evenodd = addr.bank % 2;    // check if it is evenbank or oddbank
					for(int i=evenodd; i<config_.banks; i+=2) {
						Address tmp_addr = Address(addr.channel, addr.rank, i/4, i%4, addr.row, addr.column);
						uint64_t tmp_hex_addr = ReverseAddressMapping(tmp_addr);

						if (is_write)
							PmemWrite(tmp_hex_addr, DataPtr);
						else
							PmemRead(tmp_hex_addr, DataPtr);
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
				int pim_index = GetPimIndex(addr) + i;
				pim_unit_[pim_index]->SetSrf(hex_addr, DataPtr);
			}
		}
		else if(addr.row == 0x3ffb) {  // set GRF_A, GRF_B
			for(int i=0; i<config_.banks/2; i++) {
				int pim_index = GetPimIndex(addr) + i;
				pim_unit_[pim_index]->SetGrf(hex_addr, DataPtr);
			}
		}
		else if(addr.row == 0x3ffc) {  // set CRF
			for(int i=0; i<config_.banks/2; i++) {
				int pim_index = GetPimIndex(addr) + i;
				pim_unit_[pim_index]->SetCrf(hex_addr, DataPtr);
			}
		}
		else {  // RD, WR
			int evenodd = addr.bank % 2;    // check if it is evenbank or oddbank
			if (addr.channel == 0) {
				std::cout << "H: " << addr.bankgroup << " " << addr.bank << " " << addr.row << " " << addr.column << std::endl;
			}
			for(int i=evenodd; i<config_.banks; i+=2) {
				Address tmp_addr = Address(addr.channel, addr.rank, i/4, i%4, addr.row, addr.column);
				uint64_t tmp_hex_addr = ReverseAddressMapping(tmp_addr);

				int pim_index = GetPimIndex(addr) + i/2;
				int ret = pim_unit_[pim_index]->AddTransaction(tmp_hex_addr, is_write, DataPtr);
				if(ret == EXIT_END) {
					if(DebugMode(hex_addr))
						std::cout << "CHANGEtoNONPIM\n";
					PIM_OP_MODE[addr.channel] = false;	
				}
			}
		}
	}
}

} // namespace dramsim3
// mmm <<
