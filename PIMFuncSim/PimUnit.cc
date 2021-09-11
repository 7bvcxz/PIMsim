#include <string>
#include "PimUnit.h"

PimUnit::PimUnit() {
    PPC = 0;
    LC  = -1;
    RA  = 0;

    _GRF_A = (unit_t*) malloc(GRF_SIZE);
    _GRF_B = (unit_t*) malloc(GRF_SIZE);
    _SRF_A = (unit_t*) malloc(SRF_SIZE);
    _SRF_M = (unit_t*) malloc(SRF_SIZE);
    even_data = (unit_t*) malloc(WORD_SIZE);
    odd_data = (unit_t*) malloc(WORD_SIZE);
}
 
void PimUnit::SetPmkFilename(std::string pim_micro_kernel_filename) {
    this->pim_micro_kernel_filename = pim_micro_kernel_filename;
}

void PimUnit::SetPhysmem(unit_t* physmem) {
    this->physmem = physmem;
}

void PimUnit::CrfInit() {
    std::ifstream fp;
    fp.open(pim_micro_kernel_filename);

    std::string str;
    while (getline(fp, str) && !fp.eof()) {
        std::string mk_part[4];
        int num_parts = (str.size()-1)/10 + 1;

        if (str.size() == 0)
            continue;

        for (int i = 0; i < num_parts; i++) {
            std::string part = str.substr(i*10, 9);
            mk_part[i] = part.substr(0, part.find(' '));
        }

        PushCrf(mk_part, num_parts);
        PPC += 1;
    }
    PPC = 0;

    fp.close();
}

void PimUnit::PushCrf(std::string* mk_part, int num_parts) {
    CRF[PPC].PIM_OP = StringToPIM_OP(mk_part[0]);
    CRF[PPC].is_aam = CheckAam(mk_part[0]);

    switch (CRF[PPC].PIM_OP) {
        case PIM_OPERATION::ADD:
        case PIM_OPERATION::MUL:
        case PIM_OPERATION::MAC:
        case PIM_OPERATION::MAD:
            CRF[PPC].pim_op_type = PIM_OP_TYPE::ALU;
            CRF[PPC].dst  = StringToOperand(mk_part[1]);
            CRF[PPC].src0 = StringToOperand(mk_part[2]);
            CRF[PPC].src1 = StringToOperand(mk_part[3]);
            CRF[PPC].dst_idx  = StringToIndex(mk_part[1]);
            CRF[PPC].src0_idx = StringToIndex(mk_part[2]);
            CRF[PPC].src1_idx = StringToIndex(mk_part[3]);
            break;
        case PIM_OPERATION::MOV:
        case PIM_OPERATION::FILL:
            CRF[PPC].pim_op_type = PIM_OP_TYPE::DATA;
            CRF[PPC].dst  = StringToOperand(mk_part[1]);
            CRF[PPC].src0 = StringToOperand(mk_part[2]);
            CRF[PPC].dst_idx  = StringToIndex(mk_part[1]);
            CRF[PPC].src0_idx = StringToIndex(mk_part[2]);
            break;
        case PIM_OPERATION::NOP:
            CRF[PPC].pim_op_type = PIM_OP_TYPE::CONTROL;
            CRF[PPC].imm0 = StringToNum(mk_part[1]);
            break;
        case PIM_OPERATION::JUMP:
            CRF[PPC].pim_op_type = PIM_OP_TYPE::CONTROL;
            CRF[PPC].imm0 = PPC + StringToNum(mk_part[1]);
            CRF[PPC].imm1 = StringToNum(mk_part[2]);
            break;
        case PIM_OPERATION::EXIT:
            CRF[PPC].pim_op_type = PIM_OP_TYPE::CONTROL;
            break;
        default:
            break;
    }
}

int PimUnit::Issue(std::string* pim_cmd, int num_parts) {
    // DRAM READ & WRITE //
    int CA = StringToNum(pim_cmd[1]);
    int offset = RA * UNITS_PER_ROW + CA * UNITS_PER_WORD;

    if (pim_cmd[0] == "WR") {
        for (int i = 0; i < 16; i++) {
            unit_t WR = (unit_t)StringToNum(pim_cmd[i+2]);
            std::memcpy(physmem + offset + i, &WR, sizeof(unit_t));
            std::memcpy(physmem + offset + UNITS_PER_BK + i,
                &WR, sizeof(unit_t));
        }
        even_data = physmem + offset;
        odd_data  = physmem + offset + UNITS_PER_BK;
    } else if (pim_cmd[0] == "RD") {
        even_data = physmem + offset;
        odd_data  = physmem + offset + UNITS_PER_BK;
    }

    // NOP & JUMP //
    if (CRF[PPC].PIM_OP == PIM_OPERATION::NOP) {
        if (LC == -1) {
            LC = CRF[PPC].imm0 - 1;
        } else if (LC > 0) {
            LC -= 1;
        } else if (LC == 0) {
            LC = -1;
            return NOP_END;
        }
        return 0;
    } else if (CRF[PPC].PIM_OP == PIM_OPERATION::JUMP) {
        if (LC == -1) {
            LC = CRF[PPC].imm1 - 1;
            PPC = CRF[PPC].imm0;
        } else if (LC > 0) {
            PPC = CRF[PPC].imm0;
            LC -= 1;
        } else if (LC == 0) {
            PPC += 1;
            LC  -= 1;
        }
    }

    if (CRF[PPC].PIM_OP == PIM_OPERATION::EXIT)
        return EXIT_END;

    // SET ADDR & EXECUTE //

    SetOperandAddr(pim_cmd);

    Execute();

    PPC += 1;

    return PPC;
}

void PimUnit::SetOperandAddr(std::string* pim_cmd) {
    // set _GRF_A, _GRF_B operand address when AAM mode
    if (CRF[PPC].is_aam) {
        int CA = StringToNum(pim_cmd[1]);
        int A_idx = CA % 8;
        int B_idx = CA / 8 + RA % 2 * 4;

        // set dst address (AAM)
        if (CRF[PPC].dst == PIM_OPERAND::GRF_A)
            dst = _GRF_A + A_idx * 16;
        else if (CRF[PPC].dst == PIM_OPERAND::GRF_B)
            dst = _GRF_B + B_idx * 16;

        // set src0 address (AAM)
        if (CRF[PPC].src0 == PIM_OPERAND::GRF_A)
            src0 = _GRF_A + A_idx * 16;
        else if (CRF[PPC].src0 == PIM_OPERAND::GRF_B)
            src0 = _GRF_B + B_idx * 16;

        // set src1 address (AAM)
        if (CRF[PPC].src1 == PIM_OPERAND::GRF_A)
            src1 = _GRF_A + A_idx * 16;
        else if (CRF[PPC].src1 == PIM_OPERAND::GRF_B)
            src1 = _GRF_B + B_idx * 16;
    } else {      // set _GRF_A, _GRF_B operand address when non-AAM mode
        // set dst address
        if (CRF[PPC].dst == PIM_OPERAND::GRF_A)
            dst = _GRF_A + CRF[PPC].dst_idx * 16;
        else if (CRF[PPC].dst == PIM_OPERAND::GRF_B)
            dst = _GRF_B + CRF[PPC].dst_idx * 16;

        // set src0 address
        if (CRF[PPC].src0 == PIM_OPERAND::GRF_A)
            src0 = _GRF_A + CRF[PPC].src0_idx * 16;
        else if (CRF[PPC].src0 == PIM_OPERAND::GRF_B)
            src0 = _GRF_B + CRF[PPC].src0_idx * 16;

        // set src1 address
        // PIM_OP == ADD, MUL, MAC, MAD -> uses src1 for operand
        if (CRF[PPC].pim_op_type == PIM_OP_TYPE::ALU) {
            if (CRF[PPC].src1 == PIM_OPERAND::GRF_A)
                src1 = _GRF_A + CRF[PPC].src1_idx * 16;
            else if (CRF[PPC].src1 == PIM_OPERAND::GRF_B)
                src1 = _GRF_B + CRF[PPC].src1_idx * 16;
        }
    }

    // set EVEN_BANK, ODD_BANK, operand address
    // set dst address
    if (CRF[PPC].dst == PIM_OPERAND::EVEN_BANK)
        dst = even_data;
    else if (CRF[PPC].dst == PIM_OPERAND::ODD_BANK)
        dst = odd_data;

    // set src0 address
    if (CRF[PPC].src0 == PIM_OPERAND::EVEN_BANK)
        src0 = even_data;
    else if (CRF[PPC].src0 == PIM_OPERAND::ODD_BANK)
        src0 = odd_data;

    // set src1 address only if PIM_OP_TYPE == ALU
    //  -> uses src1 for operand
    if (CRF[PPC].pim_op_type == PIM_OP_TYPE::ALU) {
        if (CRF[PPC].src1 == PIM_OPERAND::EVEN_BANK)
            src1 = even_data;
        else if (CRF[PPC].src1 == PIM_OPERAND::ODD_BANK)
            src1 = odd_data;
        else if (CRF[PPC].src1 == PIM_OPERAND::SRF_A)
            src1 = _SRF_A + CRF[PPC].src1_idx;
        else if (CRF[PPC].src1 == PIM_OPERAND::SRF_M)
            src1 = _SRF_M + CRF[PPC].src1_idx;
    }
}

void PimUnit::Execute() {
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
        for (int i = 0; i < UNITS_PER_WORD; i++)
            dst[i] = src0[i] + src1[0];
    } else {
        for (int i = 0; i < UNITS_PER_WORD; i++)
            dst[i] = src0[i] + src1[i];
    }
}

void PimUnit::_MUL() {
    if (CRF[PPC].src1 == PIM_OPERAND::SRF_M) {
        for (int i = 0; i < UNITS_PER_WORD; i++)
            dst[i] = src0[i] * src1[0];
    } else {
        for (int i = 0; i < UNITS_PER_WORD; i++)
            dst[i] = src0[i] * src1[i];
    }
}

void PimUnit::_MAC() {
    if (CRF[PPC].src1 == PIM_OPERAND::SRF_M) {
        for (int i = 0; i < UNITS_PER_WORD; i++)
            dst[i] += src0[i] * src1[0];
    } else {
        for (int i = 0; i < UNITS_PER_WORD; i++)
            dst[i] += src0[i] * src1[i];
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
