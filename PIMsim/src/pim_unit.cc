#include "pim_unit.h"

using half_float::half;

namespace dramsim3 {

int debug_cnt;
PimUnit::PimUnit(Config &config, int id)
  : config_(config),
	pim_id(id)
{
    PPC = 0;
    LC  = 0;
	debug_cnt = 0;

    GRF_A_ = (unit_t*) malloc(GRF_SIZE);
    GRF_B_ = (unit_t*) malloc(GRF_SIZE);
    SRF_A_ = (unit_t*) malloc(SRF_SIZE);
    SRF_M_ = (unit_t*) malloc(SRF_SIZE);
    bank_data_ = (unit_t*) malloc(WORD_SIZE);
	dst = (unit_t*) malloc(WORD_SIZE);

    for(int i=0; i< WORD_SIZE / sizeof(unit_t); i++) dst[i] = 0;
    for(int i=0; i< GRF_SIZE / sizeof(unit_t); i++) {
        GRF_A_[i] = 0;
        GRF_B_[i] = 0;
    }
    for(int i=0; i< SRF_SIZE / sizeof(unit_t); i++) {
        SRF_A_[i] = 0;
        SRF_M_[i] = 0;
    }
}

void PimUnit::init(uint8_t* pmemAddr, uint64_t pmemAddr_size, unsigned int burstSize) {
	pmemAddr_ = pmemAddr;
	pmemAddr_size_ = pmemAddr_size;
	burstSize_ = burstSize;
}

bool PimUnit::DebugMode() {
    #ifndef debug_mode
    return false;
    #endif

	if(pim_id == watch_pimindex) return true;
	return false;
}

void PimUnit::PrintOperand(int op_id) {
	if(op_id == 0) std::cout << "BANK";
	else if(op_id == 1) std::cout << "GRF_A";
	else if(op_id == 2) std::cout << "GRF_B";
	else if(op_id == 3) std::cout << "SRF_A";
	else if(op_id == 4) std::cout << "SRF_M";
}

void PimUnit::PrintPIM_IST(PimInstruction inst) {
	if(inst.PIM_OP == (PIM_OPERATION)0) std::cout << "NOP\t";
	else if(inst.PIM_OP == (PIM_OPERATION) 1) std::cout << "JUMP\t";
	else if(inst.PIM_OP ==(PIM_OPERATION) 2) std::cout << "EXIT\t";
	else if(inst.PIM_OP == (PIM_OPERATION)4) std::cout << "MOV\t";
	else if(inst.PIM_OP == (PIM_OPERATION)5) std::cout << "FILL\t";
	else if(inst.PIM_OP == (PIM_OPERATION)8) std::cout << "ADD\t";
	else if(inst.PIM_OP == (PIM_OPERATION)9) std::cout << "MUL\t";
	else if(inst.PIM_OP == (PIM_OPERATION)10) std::cout << "MAC\t";
	else if(inst.PIM_OP == (PIM_OPERATION)11) std::cout << "MAD\t";

	if(inst.pim_op_type == (PIM_OP_TYPE)0) { // CONTROL
		std::cout << (int)inst.imm0 << "\t";
		std::cout << (int)inst.imm1 << "\t";
	}
	else if(inst.pim_op_type == (PIM_OP_TYPE)1) { // DATA
		PrintOperand((int)inst.dst);
		if(inst.is_aam == 0)  std::cout << inst.dst_idx;
		std::cout << "\t";
		PrintOperand((int)inst.src0);
		if(inst.is_aam == 0)  std::cout << inst.src0_idx;
		std::cout << "\t";
	
	}
	else if(inst.pim_op_type == (PIM_OP_TYPE)2) { // ALU
		PrintOperand((int)inst.dst);
		if(inst.is_aam == 0)  std::cout << inst.dst_idx;
		std::cout << "\t";
		PrintOperand((int)inst.src0);
		if(inst.is_aam == 0)  std::cout << inst.src0_idx;
		std::cout << "\t";
		PrintOperand((int)inst.src1);
		if(inst.is_aam == 0)  std::cout << inst.src1_idx;
		std::cout << "\t";
	}
	std::cout << "\n";
}

void PimUnit::SetSrf(uint64_t hex_addr, uint8_t* DataPtr) {
	if(DebugMode()) std::cout << " PU: SetSrf\n";
	memcpy(SRF_A_, DataPtr, SRF_SIZE);
	memcpy(SRF_M_, DataPtr + SRF_SIZE, SRF_SIZE);
}

void PimUnit::SetGrf(uint64_t hex_addr, uint8_t* DataPtr) {
	if(DebugMode()) std::cout << "  PU: SetGrf\n";
  	Address addr = config_.AddressMapping(hex_addr);
	if(addr.column < 8) {  // GRF_A
		unit_t* target = GRF_A_ + addr.column *WORD_SIZE / sizeof(unit_t); 
		memcpy(target, DataPtr, WORD_SIZE);
	}
	else {  // GRF_B
        GRF_B_[15] = 0;
		unit_t* target = GRF_B_ + (addr.column-8) *WORD_SIZE / sizeof(unit_t); 
		memcpy(target, DataPtr, WORD_SIZE);
	}
}

void PimUnit::SetCrf(uint64_t hex_addr, uint8_t* DataPtr) {
	if(DebugMode()) std::cout << "  PU: SetCrf\n";
	Address addr = config_.AddressMapping(hex_addr);
	int CRF_idx = addr.column * 8;
	for(int i=0; i<8; i++) {
		PushCrf(CRF_idx+i, DataPtr + 4*i);
	}
}

void PimUnit::PushCrf(int CRF_idx, uint8_t* DataPtr) {
    CRF[CRF_idx].PIM_OP = BitToPIM_OP(DataPtr);
    CRF[CRF_idx].is_aam = CheckAam(DataPtr);

    switch (CRF[CRF_idx].PIM_OP) {
        case PIM_OPERATION::ADD:
        case PIM_OPERATION::MUL:
        case PIM_OPERATION::MAC:
        case PIM_OPERATION::MAD:
            CRF[CRF_idx].pim_op_type = PIM_OP_TYPE::ALU;
            CRF[CRF_idx].dst  = BitToDst(DataPtr);
            CRF[CRF_idx].src0 = BitToSrc0(DataPtr);
            CRF[CRF_idx].src1 = BitToSrc1(DataPtr);
            CRF[CRF_idx].dst_idx  = BitToDstIdx(DataPtr);
            CRF[CRF_idx].src0_idx = BitToSrc0Idx(DataPtr);
            CRF[CRF_idx].src1_idx = BitToSrc1Idx(DataPtr);
            break;
        case PIM_OPERATION::MOV:
        case PIM_OPERATION::FILL:
            CRF[CRF_idx].pim_op_type = PIM_OP_TYPE::DATA;
            CRF[CRF_idx].dst  = BitToDst(DataPtr);
            CRF[CRF_idx].src0 = BitToSrc0(DataPtr);
            CRF[CRF_idx].src1 = BitToSrc1(DataPtr);
            CRF[CRF_idx].dst_idx  = BitToDstIdx(DataPtr);
            CRF[CRF_idx].src0_idx = BitToSrc0Idx(DataPtr);
            break;
        case PIM_OPERATION::NOP:
            CRF[CRF_idx].pim_op_type = PIM_OP_TYPE::CONTROL;
            CRF[CRF_idx].imm1 = BitToImm1(DataPtr);
            break;
        case PIM_OPERATION::JUMP:
            CRF[CRF_idx].pim_op_type = PIM_OP_TYPE::CONTROL;
            CRF[CRF_idx].imm0 = CRF_idx + BitToImm0(DataPtr);
            CRF[CRF_idx].imm1 = BitToImm1(DataPtr);
            break;
        case PIM_OPERATION::EXIT:
            CRF[CRF_idx].pim_op_type = PIM_OP_TYPE::CONTROL;
            break;
        default:
            break;
    }
	if(DebugMode()) {
		std::cout << "  PU: program  ";
		PrintPIM_IST(CRF[CRF_idx]);
	}
}

int PimUnit::AddTransaction(uint64_t hex_addr, bool is_write, uint8_t* DataPtr) {
    // DRAM READ & WRITE //
    if (!is_write)
        memcpy(bank_data_ , pmemAddr_ + hex_addr, WORD_SIZE);

    //SET ADDR & EXECUTE //
    SetOperandAddr(hex_addr);

    Execute();

    if (CRF[PPC].PIM_OP == PIM_OPERATION::MOV && 
        CRF[PPC].dst == PIM_OPERAND::BANK) {        
        memcpy(pmemAddr_ + hex_addr, dst, WORD_SIZE);
    }

    PPC += 1;

    // NOP & JUMP //
    if (CRF[PPC].PIM_OP == PIM_OPERATION::NOP) {
        if (LC == 0) {
            LC = CRF[PPC].imm1;
        } else if (LC > 1) {
            LC -= 1;
        } else if (LC == 1) {
            PPC += 1;
            LC = 0;
            return NOP_END;
        }
        if(DebugMode()) {
            std::cout << "  PU: NOP left (" << LC << ")\n";
        }
        return 0;
    } else if (CRF[PPC].PIM_OP == PIM_OPERATION::JUMP) {
        if (LC == 0) {
            LC = CRF[PPC].imm1;
            PPC = CRF[PPC].imm0;
        } else if (LC > 1) {
            PPC = CRF[PPC].imm0;
            LC -= 1;
        } else if (LC == 1) {
            PPC += 1;
            LC = 0;
        }
        if(DebugMode()) {
            std::cout << "  PU: JUMP left (" << LC << ")\n";
        }
    }

    if (CRF[PPC].PIM_OP == PIM_OPERATION::EXIT){
        if(DebugMode()) {
            std::cout << "  PU: EXIT\n";
        }
        PPC = 0;

        return EXIT_END;
    }
    
    return 0; // NORMAL_END
}

void PimUnit::SetOperandAddr(uint64_t hex_addr) {
    // set _GRF_A, _GRF_B operand address when AAM mode
	Address addr = config_.AddressMapping(hex_addr);
    if (CRF[PPC].is_aam) {
        int CA = addr.column;
		int RA = addr.row;
        int A_idx = CA % 8;
        int B_idx = CA / 8 + RA % 2 * 4;

        // set dst address (AAM)
        if (CRF[PPC].dst == PIM_OPERAND::GRF_A)
            dst = GRF_A_ + A_idx * 16; // >> mmm << 
        else if (CRF[PPC].dst == PIM_OPERAND::GRF_B)
            dst = GRF_B_ + B_idx * 16; // >> mmm <<

        // set src0 address (AAM)
        if (CRF[PPC].src0 == PIM_OPERAND::GRF_A)
            src0 = GRF_A_ + A_idx * 16;
        else if (CRF[PPC].src0 == PIM_OPERAND::GRF_B)
            src0 = GRF_B_ + B_idx * 16;
        else if (CRF[PPC].src0 == PIM_OPERAND::SRF_A)
            src0 = SRF_A_ + A_idx; // >> mmm <<

        // set src1 address (AAM)
        if (CRF[PPC].src1 == PIM_OPERAND::GRF_A)
            src1 = GRF_A_ + A_idx * 16;
        else if (CRF[PPC].src1 == PIM_OPERAND::GRF_B)
            src1 = GRF_B_ + B_idx * 16;
        else if (CRF[PPC].src1 == PIM_OPERAND::SRF_A)
            src1 = SRF_A_ + A_idx;
        else if (CRF[PPC].src1 == PIM_OPERAND::SRF_M)
            src1 = SRF_M_ + A_idx;

    } else {      // set _GRF_A, _GRF_B operand address when non-AAM mode
        // set dst address
        if (CRF[PPC].dst == PIM_OPERAND::GRF_A)
            dst = GRF_A_ + CRF[PPC].dst_idx * 16;
        else if (CRF[PPC].dst == PIM_OPERAND::GRF_B)
            dst = GRF_B_ + CRF[PPC].dst_idx * 16;

        // set src0 address
        if (CRF[PPC].src0 == PIM_OPERAND::GRF_A)
            src0 = GRF_A_ + CRF[PPC].src0_idx * 16;
        else if (CRF[PPC].src0 == PIM_OPERAND::GRF_B)
            src0 = GRF_B_ + CRF[PPC].src0_idx * 16;
        else if(CRF[PPC].src0 == PIM_OPERAND::SRF_A)
            src1 = SRF_A_ + CRF[PPC].src1_idx;

        // set src1 address
        // PIM_OP == ADD, MUL, MAC, MAD -> uses src1 for operand
        if (CRF[PPC].pim_op_type == PIM_OP_TYPE::ALU) {
            if (CRF[PPC].src1 == PIM_OPERAND::GRF_A)
                src1 = GRF_A_ + CRF[PPC].src1_idx * 16;
            else if (CRF[PPC].src1 == PIM_OPERAND::GRF_B)
                src1 = GRF_B_ + CRF[PPC].src1_idx * 16;
            else if (CRF[PPC].src1 == PIM_OPERAND::SRF_A)
                src1 = SRF_A_ + CRF[PPC].src1_idx;
            else if (CRF[PPC].src1 == PIM_OPERAND::SRF_M)
                src1 = SRF_M_ + CRF[PPC].src1_idx;
        }
    }

    // set BANK, operand address
    // . set dst address
    if (CRF[PPC].dst == PIM_OPERAND::BANK)
        dst = bank_data_;

    // . set src0 address
    if (CRF[PPC].src0 == PIM_OPERAND::BANK)
        src0 = bank_data_;

    // . set src1 address only if PIM_OP_TYPE == ALU
    //   -> uses src1 for operand
    if (CRF[PPC].pim_op_type == PIM_OP_TYPE::ALU)
        if (CRF[PPC].src1 == PIM_OPERAND::BANK)
            src1 = bank_data_;
}

void PimUnit::Execute() {
	if(DebugMode()) {
		std::cout << "  PU: execute  ";
		PrintPIM_IST(CRF[PPC]);
	}
    switch (CRF[PPC].PIM_OP) {
        case PIM_OPERATION::ADD:
            _ADD();
            break;
        case PIM_OPERATION::MUL:
            _MUL();
            break;
        case PIM_OPERATION::MAC:
            _MAC();
            break;
        case PIM_OPERATION::MAD:
            _MAD();
            break;
        case PIM_OPERATION::MOV:
        case PIM_OPERATION::FILL:
            _MOV();
            break;
        default:
            break;
    }
}

void PimUnit::_ADD() {
    if (CRF[PPC].src1 == PIM_OPERAND::SRF_A) {
        for (int i = 0; i < UNITS_PER_WORD; i++){
            half h_src0(*reinterpret_cast<half*>(&src0[i]));
            half h_src1(*reinterpret_cast<half*>(&src1[0]));
            half h_dst = h_src0 + h_src1;
            dst[i] = *reinterpret_cast<unit_t*>(&h_dst);
        }
    } else {
        for (int i = 0; i < UNITS_PER_WORD; i++) {
            half h_src0(*reinterpret_cast<half*>(&src0[i]));
            half h_src1(*reinterpret_cast<half*>(&src1[i]));
            half h_dst = h_src0 + h_src1;
            dst[i] = *reinterpret_cast<unit_t*>(&h_dst);
        }
    }
}

void PimUnit::_MUL() {
    for (int i = 0; i < UNITS_PER_WORD; i++) {
        dst[i] = src0[i] * src1[i];
    }
}

void PimUnit::_MAC() {
    if (CRF[PPC].src1 == PIM_OPERAND::SRF_M) {
        for (int i = 0; i < UNITS_PER_WORD; i++){
            half h_dst(*reinterpret_cast<half*>(&dst[i]));
            half h_src0(*reinterpret_cast<half*>(&src0[i]));
            half h_src1(*reinterpret_cast<half*>(&src1[0]));
            h_dst = fma(h_src0, h_src1, h_dst);
            dst[i] = *reinterpret_cast<unit_t*>(&h_dst);
        }
    } else {
        for (int i = 0; i < UNITS_PER_WORD; i++) {
            half h_dst(*reinterpret_cast<half*>(&dst[i]));
            half h_src0(*reinterpret_cast<half*>(&src0[i]));
            half h_src1(*reinterpret_cast<half*>(&src1[i]));
            h_dst = fma(h_src0, h_src1, h_dst);
            dst[i] = *reinterpret_cast<unit_t*>(&h_dst);
        }
    }
}
void PimUnit::_MAD() {
    std::cout << "not yet\n";
}

void PimUnit::_MOV() {
    for (int i = 0; i < UNITS_PER_WORD; i++) {
        dst[i] = src0[i];
    }
}

} // namespace dramsim3
