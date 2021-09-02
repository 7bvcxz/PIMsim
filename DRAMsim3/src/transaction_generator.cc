#include "transaction_generator.h"

namespace dramsim3 {

void TransactionGenerator::ReadCallBack(uint64_t addr, uint8_t *DataPtr) {
    return;
}
void TransactionGenerator::WriteCallBack(uint64_t addr) {
    return;
}

uint64_t TransactionGenerator::ReverseAddressMapping(Address& addr) {
    uint64_t hex_addr = 0;
    hex_addr += (uint64_t)addr.channel << config_->ch_pos;
    hex_addr += (uint64_t)addr.rank << config_->ra_pos;
    hex_addr += (uint64_t)addr.bankgroup << config_->bg_pos;
    hex_addr += (uint64_t)addr.bank << config_->ba_pos;
    hex_addr += (uint64_t)addr.row << config_->ro_pos;
    hex_addr += (uint64_t)addr.column << config_->co_pos;
    return hex_addr << config_->shift_bits;
}

uint64_t TransactionGenerator::Ceiling(uint64_t num, uint64_t stride) {
    return ((num + stride - 1) / stride) * stride;
}

void TransactionGenerator::TryAddTransaction(uint64_t hex_addr, bool is_write, uint8_t *DataPtr) {
    while (!memory_system_.WillAcceptTransaction(hex_addr, is_write)) {
        memory_system_.ClockTick();
        clk_++;
    }
    if (is_write) {
        uint8_t *new_data = (uint8_t *) malloc(burstSize_);
        std::memcpy(new_data, DataPtr, burstSize_);
        memory_system_.AddTransaction(hex_addr, is_write, new_data);
    } else {
        memory_system_.AddTransaction(hex_addr, is_write, DataPtr);
    }
}

void TransactionGenerator::Barrier() {
    memory_system_.SetWriteBufferThreshold(0);
    while (memory_system_.IsPendingTransaction()) {
        memory_system_.ClockTick();
        clk_++;
    }
    memory_system_.SetWriteBufferThreshold(-1);
}


void AddTransactionGenerator::Initialize() {
    addr_x_ = 0;
    addr_y_ = Ceiling(n_ * UNIT_SIZE, SIZE_ROW * NUM_BANK);

    ukernel_access_size_ = SIZE_WORD * 8 * NUM_BANK;
    ukernel_count_per_pim_ = Ceiling(n_ * UNIT_SIZE, ukernel_access_size_) / ukernel_access_size_;

    // Define ukernel
    ukernel_ = (uint32_t *) malloc(sizeof(uint32_t) * 32);
    ukernel_[0]  = 0b01000010000000001000000000000000; // MOV(AAM)  GRF_A  BANK
    ukernel_[1]  = 0b00010000000001000000100000000111; // JUMP      -1     7
    ukernel_[2]  = 0b10000010000010001000000000000000; // ADD(AAM)  GRF_A  BANK  GRF_A
    ukernel_[3]  = 0b00010000000001000000100000000111; // JUMP      -1     7
    ukernel_[4]  = 0b01000000010000001000000000000000; // MOV(AAM)  BANK   GRF_A
    ukernel_[5]  = 0b00010000000001000000100000000111; // JUMP      -1     7
    ukernel_[6]  = 0b01000010000000001000000000000000; // MOV(AAM)  GRF_A  BANK
    ukernel_[7]  = 0b00010000000001000000100000000111; // JUMP      -1     7
    ukernel_[8]  = 0b10000010000010001000000000000000; // ADD(AAM)  GRF_A  BANK  GRF_A
    ukernel_[9]  = 0b00010000000001000000100000000111; // JUMP      -1     7
    ukernel_[10] = 0b01000000010000001000000000000000; // MOV(AAM)  BANK   GRF_A
    ukernel_[11] = 0b00010000000001000000100000000111; // JUMP      -1     7
    ukernel_[12] = 0b00100000000000000000000000000000; // EXIT
	// 4 1 8 1 4 1 4 1 8 1 4 1 2
}

void AddTransactionGenerator::SetData() {
    uint64_t strided_size = Ceiling(n_ * UNIT_SIZE, SIZE_WORD * NUM_BANK);

    // Write input data x
    for (int offset = 0; offset < strided_size ; offset += SIZE_WORD)
        TryAddTransaction(addr_x_ + offset, true, x_ + offset);

    // Write input data y
    for (int offset = 0; offset < strided_size ; offset += SIZE_WORD)
        TryAddTransaction(addr_y_ + offset, true, y_ + offset);
    Barrier();

    // Mode transition: SB -> AB
	std::cout << "\n1>>>>>>>>>>>> SB -> AB <<<<<<<<<<<\n";
    for (int ch = 0; ch < NUM_CHANNEL; ch++) {
        Address addr(ch, 0, 0, 0, MAP_ABMR, 0);
        uint64_t hex_addr = ReverseAddressMapping(addr);
        TryAddTransaction(hex_addr, false, data_temp_);
    }
    Barrier();

    // Program ukernel
    for (int ch = 0; ch < NUM_CHANNEL; ch++) {
        for (int co = 0; co < 4; co++) {
            Address addr(ch, 0, 0, 0, MAP_CRF, co);
            uint64_t hex_addr = ReverseAddressMapping(addr);
            TryAddTransaction(hex_addr, true, (uint8_t*)&ukernel_[co*8]);
        }
    }
    Barrier();
}

void AddTransactionGenerator::Execute() {
	std::cout << "\n2>>>>>>>>>>>> AB -> PIM <<<<<<<<<<<\n";
    for (int ro = 0; ro * NUM_WORD_PER_ROW / 8 < ukernel_count_per_pim_; ro++) {
        for (int co_o = 0; co_o < NUM_WORD_PER_ROW / 8; co_o++) {
            // Check that all data operations have been completed
            if (ro * NUM_WORD_PER_ROW / 8 + co_o > ukernel_count_per_pim_)
                break;

            // Mode transition: AB -> AB-PIM
            *data_temp_ |= 1;
            for (int ch = 0; ch < NUM_CHANNEL; ch++) {
                Address addr(ch, 0, 0, 0, MAP_PIM_OP_MODE, 0);
                uint64_t hex_addr = ReverseAddressMapping(addr);
                TryAddTransaction(hex_addr, true, data_temp_);
            }
            Barrier();

            // Execute ukernel 0-1
            for (int co_i = 0; co_i < 8; co_i++) {
                uint64_t co = co_o * 8 + co_i;
                for (int ch = 0; ch < NUM_CHANNEL; ch++) {
                    Address addr(ch, 0, 0, EVEN_BANK, ro, co);
                    uint64_t hex_addr = ReverseAddressMapping(addr);
                    TryAddTransaction(addr_x_ + hex_addr, false, data_temp_);
                }
            }
            Barrier();
            
            // Execute ukernel 2-3
            for (int co_i = 0; co_i < 8; co_i++) {
                uint64_t co = co_o * 8 + co_i;
                for (int ch = 0; ch < NUM_CHANNEL; ch++) {
                    Address addr(ch, 0, 0, EVEN_BANK, ro, co);
                    uint64_t hex_addr = ReverseAddressMapping(addr);
                    TryAddTransaction(addr_y_ + hex_addr, false, data_temp_);
                }
            }
            Barrier();
            
            // Execute ukernel 4-5
            for (int co_i = 0; co_i < 8; co_i++) {
                uint64_t co = co_o * 8 + co_i;
                for (int ch = 0; ch < NUM_CHANNEL; ch++) {
                    Address addr(ch, 0, 0, EVEN_BANK, ro, co);
                    uint64_t hex_addr = ReverseAddressMapping(addr);
                    TryAddTransaction(addr_y_ + hex_addr, true, data_temp_);
                }
            }
            Barrier();
            
            // Execute ukernel 6-7
            for (int co_i = 0; co_i < 8; co_i++) {
                uint64_t co = co_o * 8 + co_i;
                for (int ch = 0; ch < NUM_CHANNEL; ch++) {
                    Address addr(ch, 0, 0, ODD_BANK, ro, co);
                    uint64_t hex_addr = ReverseAddressMapping(addr);
                    TryAddTransaction(addr_x_ + hex_addr, false, data_temp_);
                }
            }
            Barrier();
            
            // Execute ukernel 8-9
            for (int co_i = 0; co_i < 8; co_i++) {
                uint64_t co = co_o * 8 + co_i;
                for (int ch = 0; ch < NUM_CHANNEL; ch++) {
                    Address addr(ch, 0, 0, ODD_BANK, ro, co);
                    uint64_t hex_addr = ReverseAddressMapping(addr);
                    TryAddTransaction(addr_y_ + hex_addr, false, data_temp_);
                }
            }
            Barrier();
            
            // Execute ukernel 10-11
            for (int co_i = 0; co_i < 8; co_i++) {
                uint64_t co = co_o * 8 + co_i;
                for (int ch = 0; ch < NUM_CHANNEL; ch++) {
                    Address addr(ch, 0, 0, ODD_BANK, ro, co);
                    uint64_t hex_addr = ReverseAddressMapping(addr);
                    TryAddTransaction(addr_y_ + hex_addr, true, data_temp_);
                }
            }
            Barrier();

            // Mode transition: AB-PIM -> AB
			std::cout << "\n3>>>>>>>>>>>> PIM -> AB <<<<<<<<<<<\n";
        }
    }
}

void AddTransactionGenerator::GetResult() {
	std::cout << "\n4>>>>>>>>>>>> AB -> SB <<<<<<<<<<<\n";
    // Mode transition: AB -> SB
    for (int ch = 0; ch < NUM_CHANNEL; ch++) {
        Address addr(ch, 0, 0, 0, MAP_SBMR, 0);
        uint64_t hex_addr = ReverseAddressMapping(addr);
        TryAddTransaction(hex_addr, false, data_temp_);
    }
    Barrier();

    uint64_t strided_size = Ceiling(n_ * UNIT_SIZE, SIZE_WORD * NUM_BANK);
    // Read output data z
    for (int offset = 0; offset < strided_size ; offset += SIZE_WORD)
        TryAddTransaction(addr_y_ + offset, false, z_ + offset);
    Barrier();
}

void AddTransactionGenerator::CheckResult() {
	int err = 0;
    uint8_t *answer = (uint8_t *) malloc(sizeof(uint16_t) * n_);

    for (int i=0; i<n_; i++) {
        ((uint16_t*)answer)[i] = ((uint16_t*)x_)[i] + ((uint16_t*)y_)[i];
    }

	for(int i=0; i<n_; i++){
		err += ((uint16_t*)answer)[i] - ((uint16_t*)z_)[i];
	}
	std::cout << "ERROR : " << err << std::endl;
}

void GemvTransactionGenerator::Initialize() {
    // TODO: currently only support m=4096

    addr_A_ = 0;
    addr_y_ = Ceiling(m_ * n_ * UNIT_SIZE, SIZE_ROW * NUM_BANK);

    ukernel_access_size_ = SIZE_WORD * 8 * NUM_BANK;
    ukernel_count_per_pim_ = Ceiling(m_ * n_ * UNIT_SIZE, ukernel_access_size_) / ukernel_access_size_;

    // Define ukernel
    ukernel_gemv_ = (uint32_t *) malloc(sizeof(uint32_t) * 32);
    ukernel_gemv_[0] = 0b10100100001000001000000000000000; // MAC(AAM)  GRF_B  BANK  SRF_M
    ukernel_gemv_[1] = 0b00010000000001000000100000000111; // JUMP      -1     7
    ukernel_gemv_[2] = 0b00100000000000000000000000000000; // EXIT

    ukernel_reduce_ = (uint32_t *) malloc(sizeof(uint32_t) * 32);
    ukernel_reduce_[0] = 0b10000100100100000000000000000001; // ADD  GRF_B[0]  GRF_B[0]  GRF_B[1]
    ukernel_reduce_[1] = 0b10000100100100000000000000000010; // ADD  GRF_B[0]  GRF_B[0]  GRF_B[2]
    ukernel_reduce_[2] = 0b10000100100100000000000000000011; // ADD  GRF_B[0]  GRF_B[0]  GRF_B[3]
    ukernel_reduce_[3] = 0b10000100100100000000000000000100; // ADD  GRF_B[0]  GRF_B[0]  GRF_B[4]
    ukernel_reduce_[4] = 0b10000100100100000000000000000101; // ADD  GRF_B[0]  GRF_B[0]  GRF_B[5]
    ukernel_reduce_[5] = 0b10000100100100000000000000000110; // ADD  GRF_B[0]  GRF_B[0]  GRF_B[6]
    ukernel_reduce_[6] = 0b10000100100100000000000000000111; // ADD  GRF_B[0]  GRF_B[0]  GRF_B[7]
    ukernel_reduce_[7] = 0b01000000100000000000000000000000; // MOV  BANK      GRF_B[0]
    ukernel_reduce_[8] = 0b00100000000000000000000000000000; // EXIT
}
void GemvTransactionGenerator::SetData() {
    uint64_t strided_size = Ceiling(m_ * n_ * UNIT_SIZE, SIZE_WORD * NUM_BANK);

    // Transpose input data A
    A_T_ = (uint8_t *) malloc(sizeof(uint16_t) * m_ * n_);
    for (int m = 0; m < m_; m++) {
        for (int n = 0; n < n_; n++) {
            ((uint16_t*)A_T_)[n * m_ + m] = ((uint16_t*)A_)[m * n_ + m];
        }
    }

    // Write input data A
    for (int offset = 0; offset < strided_size; offset += SIZE_WORD)
        TryAddTransaction(addr_A_ + offset, true, A_T_ + offset);
    Barrier();

    // Mode transition: SB -> AB
    for (int ch = 0; ch < NUM_CHANNEL; ch++) {
        Address addr(ch, 0, 0, 0, MAP_ABMR, 0);
        uint64_t hex_addr = ReverseAddressMapping(addr);
        TryAddTransaction(hex_addr, false, data_temp_);
    }
    Barrier();
}
void GemvTransactionGenerator::Execute() {
    ExecuteBank(EVEN_BANK);
    ExecuteBank(ODD_BANK);
}
void GemvTransactionGenerator::GetResult() {
    // Mode transition: AB -> SB
    for (int ch = 0; ch < NUM_CHANNEL; ch++) {
        Address addr(ch, 0, 0, 0, MAP_SBMR, 0);
        uint64_t hex_addr = ReverseAddressMapping(addr);
        TryAddTransaction(hex_addr, false, data_temp_);
    }
    Barrier();

    uint64_t strided_size = Ceiling(m_ * UNIT_SIZE, SIZE_WORD * NUM_BANK);
    // Read output data z
    for (int offset = 0; offset < strided_size ; offset += SIZE_WORD)
        TryAddTransaction(addr_y_ + offset, false, y_ + offset);
    Barrier();
}

void GemvTransactionGenerator::CheckResult() {
    int err = 0;
    uint8_t *answer = (uint8_t *) malloc(sizeof(uint16_t) * m_);

    for (int m=0; m<m_; m++) {
        ((uint16_t*)answer)[m] = 0;
        for (int n=0; n<n_; n++) {
            ((uint16_t*)answer)[m] += ((uint16_t*)A_)[m*n_+n] * ((uint16_t*)x_)[n];
        }
    }

    for (int m=0; m<m_; m++) {
        err += ((uint16_t*)answer)[m] - ((uint16_t*)y_)[m];
    }
    std::cout << "ERROR : " << err << std::endl;
}

void GemvTransactionGenerator::ExecuteBank(int bank) {
    // Program gemv ukernel
    for (int ch = 0; ch < NUM_CHANNEL; ch++) {
        for (int co = 0; co < 4; co++) {
            Address addr(ch, 0, 0, 0, MAP_CRF, co);
            uint64_t hex_addr = ReverseAddressMapping(addr);
            TryAddTransaction(hex_addr, true, (uint8_t*)&ukernel_gemv_[co*8]);
        }
    }
    Barrier();

    // Execute for EVEN_BANK
    for (int ro = 0; ro * NUM_WORD_PER_ROW / 8 < ukernel_count_per_pim_; ro++) {
        for (int co_o = 0; co_o < NUM_WORD_PER_ROW / 8; co_o++) {     
            // Write input data x to SRF_M
            std::memcpy(data_temp_ + 16, ((uint16_t*)x_) + (ro * NUM_WORD_PER_ROW + co_o * 8), 16);
            for (int ch = 0; ch < NUM_CHANNEL; ch++) {
                Address addr(ch, 0, 0, bank, MAP_SRF, 0);
                uint64_t hex_addr = ReverseAddressMapping(addr);
                TryAddTransaction(hex_addr, true, data_temp_);
            }
            Barrier();

            // Mode transition: AB -> AB-PIM
            *data_temp_ |= 1;
            for (int ch = 0; ch < NUM_CHANNEL; ch++) {
                Address addr(ch, 0, 0, 0, MAP_PIM_OP_MODE, 0);
                uint64_t hex_addr = ReverseAddressMapping(addr);
                TryAddTransaction(hex_addr, true, data_temp_);
            }
            Barrier();

            // Execute ukernel 0-1
            for (int co_i = 0; co_i < 8; co_i++) {
                uint64_t co = co_o * 8 + co_i;
                for (int ch = 0; ch < NUM_CHANNEL; ch++) {
                    Address addr(ch, 0, 0, bank, ro, co);
                    uint64_t hex_addr = ReverseAddressMapping(addr);
                    TryAddTransaction(hex_addr, false, data_temp_);
                }
            }
            Barrier();

            // Mode transition: AB-PIM -> AB

            // Check that all data operations have been completed
            if (ro * NUM_WORD_PER_ROW / 8 + co_o >= ukernel_count_per_pim_)
                break;
        }
    }
    
    // Program reduce ukernel
    for (int ch = 0; ch < NUM_CHANNEL; ch++) {
        for (int co = 0; co < 4; co++) {
            Address addr(ch, 0, 0, 0, MAP_CRF, co);
            uint64_t hex_addr = ReverseAddressMapping(addr);
            TryAddTransaction(hex_addr, true, (uint8_t*)&ukernel_reduce_[co*8]);
        }
    }
    Barrier();

    // Mode transition: AB -> AB-PIM
    *data_temp_ |= 1;
    for (int ch = 0; ch < NUM_CHANNEL; ch++) {
        Address addr(ch, 0, 0, 0, MAP_PIM_OP_MODE, 0);
        uint64_t hex_addr = ReverseAddressMapping(addr);
        TryAddTransaction(hex_addr, true, data_temp_);
    }
    Barrier();
    
    // Execute ukernel 7
    for (int ch = 0; ch < NUM_CHANNEL; ch++) {
        Address addr(ch, 0, 0, bank, 0, 0);
        uint64_t hex_addr = ReverseAddressMapping(addr);
        TryAddTransaction(addr_y_ + hex_addr, true, data_temp_);
    }
    Barrier();
}


}  // namespace dramsim3
