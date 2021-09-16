#include "./pim_func_sim.h"
#include <assert.h>

namespace dramsim3 {

PimFuncSim::PimFuncSim(Config &config)
    : config_(config) {
    // Set pim_unit's id by its order of pim_unit (= pim_index)
    for (int i=0; i< config_.channels * config_.banks / 2; i++) {
        pim_unit_.push_back(new PimUnit(config_, i));
    }
}

void PimFuncSim::init(uint8_t* pmemAddr_, uint64_t pmemAddr_size_,
                      unsigned int burstSize_) {
    burstSize = burstSize_;
    pmemAddr_size = pmemAddr_size_;
    pmemAddr = pmemAddr_;

    // Set default bankmode of channel to "SB"
    for (int i=0; i< config_.channels; i++) {
        bankmode.push_back("SB");
        PIM_OP_MODE.push_back(false);
    }
    std::cout << "PimFuncSim initialized!\n";

    for (int i=0; i< config_.channels * config_.banks / 2; i++) {
        pim_unit_[i]->init(pmemAddr, pmemAddr_size, burstSize);
    }
    std::cout << "pim_units initialized!\n";
}

// Map 64-bit hex_address into structured address
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

// Return pim_index of pim_unit that input address accesses
uint64_t PimFuncSim::GetPimIndex(Address& addr) {
    return (addr.channel * config_.banks +
            addr.bankgroup * config_.banks_per_group +
            addr.bank) / 2;
}

// Return to print out debugging information or not
//  Can set debug_mode and watch_pimindex at pim_config.h
bool PimFuncSim::DebugMode(uint64_t hex_addr) {
    #ifdef debug_mode
    Address addr = config_.AddressMapping(hex_addr);
    int pim_index = GetPimIndex(addr);
    if (pim_index == watch_pimindex / (config_.banks / 2)) return true;

    #endif
    return false;
}

// Change bankmode when transaction with certain row address is recieved
bool PimFuncSim::ModeChanger(uint64_t hex_addr) {
    Address addr = config_.AddressMapping(hex_addr);
    if (addr.row == 0x3fff) {
        if (bankmode[addr.channel] == "AB") {
            bankmode[addr.channel] = "SB";
        }
        if (DebugMode(hex_addr))
            std::cout << " Pim_func_sim: AB → SB mode change\n";
        return true;
    } else if (addr.row == 0x3ffe) {
        if (bankmode[addr.channel] == "SB") {
            bankmode[addr.channel] = "AB";
        }
        if (DebugMode(hex_addr))
            std::cout << " Pim_func_sim: SB → AB mode change\n";
        return true;
    } else if (addr.row == 0x3ffd) {
        PIM_OP_MODE[addr.channel] = true;
        if (DebugMode(hex_addr))
            std::cout << " Pim_func_sim: AB → PIM mode change\n";
        return true;
    }
    return false;
}

// Write DataPtr data to physical memory address of hex_addr
void PimFuncSim::PmemWrite(uint64_t hex_addr, uint8_t* DataPtr) {
    uint8_t *host_addr = pmemAddr + hex_addr;
    memcpy(host_addr, DataPtr, burstSize);
}

// Read data from physical memory address of hex_addr to DataPtr
void PimFuncSim::PmemRead(uint64_t hex_addr, uint8_t* DataPtr) {
    uint8_t *host_addr = pmemAddr + hex_addr;
    memcpy(DataPtr, host_addr, burstSize);
}


//  Performs physical memory RD/WR, bank mode change, set PIM register,
//  execute PIM computation and write result to physical memory
void PimFuncSim::AddTransaction(Transaction *trans) {
    uint64_t hex_addr = (*trans).addr;
    bool is_write = (*trans).is_write;
    uint8_t* DataPtr = (*trans).DataPtr;
    Address addr = config_.AddressMapping(hex_addr);
    (*trans).executed_bankmode = bankmode[addr.channel];

    // Change bankmode register if transaction has certain row address
    bool is_mode_change = ModeChanger(hex_addr);
    if (is_mode_change)
        return;

    if (PIM_OP_MODE[addr.channel] == false) {
        if (bankmode[addr.channel] == "SB") {
            // Execute transaction on SB(Single Bank) mode
            (*trans).executed_bankmode = "SB";
            if (DebugMode(hex_addr))
                std::cout << " Pim_func_sim: SB mode → ";

            // Set PIM registers or RD/WR to Physical memory
            //  Discerned with certain row address
            if (addr.row == 0x3ffa) {  // set SRF_A, SRF_M
                if (DebugMode(hex_addr))
                    std::cout << "SetSrf\n";
                int pim_index = GetPimIndex(addr);
                pim_unit_[pim_index]->SetSrf(hex_addr, DataPtr);

            } else if (addr.row == 0x3ffb) {  // set GRF_A, GRF_B
                if (DebugMode(hex_addr))
                    std::cout << "SetGrf\n";
                int pim_index = GetPimIndex(addr);
                pim_unit_[pim_index]->SetGrf(hex_addr, DataPtr);

            } else if (addr.row == 0x3ffc) {  // set CRF
                if (DebugMode(hex_addr))
                    std::cout << "SetCrf\n";
                int pim_index = GetPimIndex(addr);
                pim_unit_[pim_index]->SetCrf(hex_addr, DataPtr);

            } else {  // RD, WR
                if (DebugMode(hex_addr))
                    std::cout << "RD/WR\n";
                if (is_write) {
                    PmemWrite(hex_addr, DataPtr);
                } else {
                    PmemRead(hex_addr, DataPtr);
                }
            }

        } else if (bankmode[addr.channel] == "AB") {
            // Execute transaction on AB(All Bank) mode
            (*trans).executed_bankmode = "AB";
            if (!PIM_OP_MODE[addr.channel]) {
                if (DebugMode(hex_addr))
                    std::cout << " Pim_func_sim: AB mode → ";

                // Set (PIM registers or RD/WR to Physical memory) of all
                // banks in a channel
                //  Discerned with certain row address
                if (addr.row == 0x3ffa) {  // set SRF_A, SRF_M
                    if (DebugMode(hex_addr))
                        std::cout << "SetSrf\n";
                    for (int i=0; i< config_.banks/2; i++) {
                        int pim_index = GetPimIndex(addr) + i;
                        pim_unit_[pim_index]->SetSrf(hex_addr, DataPtr);
                    }

                } else if (addr.row == 0x3ffb) {  // set GRF_A, GRF_B
                    if (DebugMode(hex_addr))
                        std::cout << "SetGrf\n";
                    for (int i=0; i< config_.banks/2; i++) {
                        int pim_index = GetPimIndex(addr) + i;
                        pim_unit_[pim_index]->SetGrf(hex_addr, DataPtr);
                    }

                } else if (addr.row == 0x3ffc) {  // set CRF
                    if (DebugMode(hex_addr))
                        std::cout << "SetCrf\n";
                    for (int i=0; i< config_.banks/2; i++) {
                        int pim_index = GetPimIndex(addr) + i;
                        pim_unit_[pim_index]->SetCrf(hex_addr, DataPtr);
                    }

                } else {  // RD, WR
                    // check if it is evenbank or oddbank
                    int evenodd = addr.bank % 2;
                    if (DebugMode(hex_addr))
                        std::cout << "RD/WR\n";
                    for (int i=evenodd; i< config_.banks; i+=2) {
                        Address tmp_addr = Address(addr.channel, addr.rank, i/4,
                                                   i%4, addr.row, addr.column);
                        uint64_t tmp_hex_addr = ReverseAddressMapping(tmp_addr);

                        if (is_write)
                            PmemWrite(tmp_hex_addr, DataPtr);
                        else
                            PmemRead(tmp_hex_addr, DataPtr);
                    }
                }
            }
        }
    } else {
        // Execute transaction on AB-PIM(All Bank PIM) mode
        (*trans).executed_bankmode = "PIM";
        if (DebugMode(hex_addr))
            std::cout << " Pim_func_sim: PIM mode → ";

        // Same as AB mode except, sends Transaction to proper pim_unit
        // when RD/WR transaction is recieved
        //  Discerned with certain row address
        if (addr.row == 0x3ffa) {  // set SRF_A, SRF_M
            if (DebugMode(hex_addr))
                std::cout << "SetSrf\n";
            for (int i=0; i< config_.banks/2; i++) {
                int pim_index = GetPimIndex(addr) + i;
                pim_unit_[pim_index]->SetSrf(hex_addr, DataPtr);
            }
        } else if (addr.row == 0x3ffb) {  // set GRF_A, GRF_B
            if (DebugMode(hex_addr))
                std::cout << "SetGrf\n";
            for (int i=0; i< config_.banks/2; i++) {
                int pim_index = GetPimIndex(addr) + i;
                pim_unit_[pim_index]->SetGrf(hex_addr, DataPtr);
            }
        } else if (addr.row == 0x3ffc) {  // set CRF
            if (DebugMode(hex_addr))
                std::cout << "SetCrf\n";
            for (int i=0; i< config_.banks/2; i++) {
                int pim_index = GetPimIndex(addr) + i;
                pim_unit_[pim_index]->SetCrf(hex_addr, DataPtr);
            }
        } else {  // RD, WR
            // check if it is evenbank or oddbank
            int evenodd = addr.bank % 2;
            if (DebugMode(hex_addr))
                std::cout << "RD/WR\n";
            for (int i=evenodd; i< config_.banks; i+=2) {
                Address tmp_addr = Address(addr.channel, addr.rank, i/4,
                                           i%4, addr.row, addr.column);
                uint64_t tmp_hex_addr = ReverseAddressMapping(tmp_addr);

                int pim_index = GetPimIndex(addr) + i/2;
                int ret = pim_unit_[pim_index]->AddTransaction(tmp_hex_addr,
                                                               is_write,
                                                               DataPtr);
                // Change bankmode to PIM → AB when programmed μkernel is
                // finished and returns EXIT_END
                if (ret == EXIT_END) {
                    if (DebugMode(hex_addr))
                        std::cout << " Pim_func_sim: PIM → AB mode change\n";
                    PIM_OP_MODE[addr.channel] = false;
                }
            }
        }
    }
}

} // namespace dramsim3