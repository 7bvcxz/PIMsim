#ifndef __PIM_FUNC_SIM_H
#define __PIM_FUNC_SIM_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "pim_unit.h"
#include "configuration.h"
#include "common.h"


namespace dramsim3 {

class PimFuncSim {
 public:
    PimFuncSim(Config &config);
    void AddTransaction(Transaction *trans);
    bool DebugMode(uint64_t hex_addr);
    bool ModeChanger(uint64_t hex_addr);

    std::vector<string> bankmode;
    std::vector<bool> PIM_OP_MODE;
    std::vector<PimUnit*> pim_unit_;

    uint8_t* pmemAddr;
    uint64_t pmemAddr_size;
    unsigned int burstSize;

    uint64_t ReverseAddressMapping(Address& addr);
    uint64_t GetPimIndex(Address& addr);
    void PmemWrite(uint64_t hex_addr, uint8_t* DataPtr);
    void PmemRead(uint64_t hex_addr, uint8_t* DataPtr);
    void init(uint8_t* pmemAddr, uint64_t pmemAddr_size,
              unsigned int burstSize);

 protected:
    Config &config_;
};

}  // namespace dramsim3

#endif  // __PIM_FUNC_SIM_H
