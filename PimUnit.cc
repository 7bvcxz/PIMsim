#include "PimUnit.h"

PimUnit::PimUnit() {
    PPC = 0;
    LC  = -1;
    RA  = 0;

    _GRF_A = (unit_t*)mmap(NULL, GRF_SIZE, PIM_MMAP_PROT, PIM_MMAP_FLAGS, -1, 0);
    _GRF_B = (unit_t*)mmap(NULL, GRF_SIZE, PIM_MMAP_PROT, PIM_MMAP_FLAGS, -1, 0);
    _SRF_A = (unit_t*)mmap(NULL, SRF_SIZE, PIM_MMAP_PROT, PIM_MMAP_FLAGS, -1, 0);
    _SRF_M = (unit_t*)mmap(NULL, SRF_SIZE, PIM_MMAP_PROT, PIM_MMAP_FLAGS, -1, 0);
    even_data = (unit_t*)mmap(NULL, WORD_SIZE, PIM_MMAP_PROT, PIM_MMAP_FLAGS, -1, 0);
    odd_data = (unit_t*)mmap(NULL, WORD_SIZE, PIM_MMAP_PROT, PIM_MMAP_FLAGS, -1, 0);
}
 
void PimUnit::SetPmkFilename(string pim_micro_kernel_filename) {
    this->pim_micro_kernel_filename = pim_micro_kernel_filename;
}

void PimUnit::SetPhysmem(unit_t* physmem) {
    this->physmem = physmem;
}

void PimUnit::CrfInit() {
    ifstream fp;
    fp.open(pim_micro_kernel_filename);
    
    string str;
    while (getline(fp, str) && !fp.eof()) {
        string mk_part[4];
        int num_parts = (str.size()-1)/10 + 1;
      
        if (str.size() == 0)
            continue; // just to see CRF.txt easily

        for (int i = 0; i < num_parts; i++) {
            mk_part[i] = (str.substr(i*10, 9)).substr(0, str.substr(i*10, 9).find(' '));
            //std::cout << mk_part[i] << " ";
        }
        //std::cout << endl;      

        PushCrf(mk_part, num_parts);
        PPC += 1;
    }
    PPC = 0;

    fp.close();
}

void PimUnit::PushCrf(string* mk_part, int num_parts) {
    CRF[PPC].PIM_OP = StringToPIM_OP(mk_part[0]);

    switch (CRF[PPC].PIM_OP) {
        case PIM_OPERATION::ADD:
        case PIM_OPERATION::MUL:
        case PIM_OPERATION::MAC:
        case PIM_OPERATION::MAD:
        case PIM_OPERATION::ADD_AAM:
        case PIM_OPERATION::MUL_AAM:
        case PIM_OPERATION::MAC_AAM:
        case PIM_OPERATION::MAD_AAM:
            CRF[PPC].dst  = StringToOperand(mk_part[1]);
            CRF[PPC].src0 = StringToOperand(mk_part[2]);
            CRF[PPC].src1 = StringToOperand(mk_part[3]);
            CRF[PPC].dst_idx  = StringToIndex(mk_part[1]);
            CRF[PPC].src0_idx = StringToIndex(mk_part[2]);
            CRF[PPC].src1_idx = StringToIndex(mk_part[3]);
            break;
        case PIM_OPERATION::MOV:
        case PIM_OPERATION::FILL:
            CRF[PPC].dst  = StringToOperand(mk_part[1]);
            CRF[PPC].src0 = StringToOperand(mk_part[2]); 
            CRF[PPC].dst_idx  = StringToIndex(mk_part[1]);
            CRF[PPC].src0_idx = StringToIndex(mk_part[2]);
            break;
        case PIM_OPERATION::NOP:
            CRF[PPC].imm0 = (int)StringToNum(mk_part[1]);
            break;
        case PIM_OPERATION::JUMP:
            CRF[PPC].imm0 = PPC + (int)StringToNum(mk_part[1]);
            CRF[PPC].imm1 = (int)StringToNum(mk_part[2]);
            break;
        default:
            break;
    }
}

int PimUnit::Issue(string* pim_cmd, int num_parts) {
    //std::cout << "a\n";
    // DRAM READ & WRITE // 
    int CA = StringToNum(pim_cmd[1]);
    int offset = RA * UNITS_PER_ROW + CA * UNITS_PER_WORD;

    if (pim_cmd[0] == "WR") {
        for (int i = 0; i < 16; i++) {
            unit_t WR = (unit_t)StringToNum(pim_cmd[i+2]);
            memcpy(physmem + offset + i, &WR, sizeof(unit_t));
            memcpy(physmem + offset + UNITS_PER_BK + i, &WR, sizeof(unit_t));
        }
        even_data = physmem + offset;
        odd_data  = physmem + offset + UNITS_PER_BK;
    }
    else if (pim_cmd[0] == "RD") {
        even_data = physmem + offset;
        odd_data  = physmem + offset + UNITS_PER_BK;
    }

    // NOP & JUMP // 
    if (CRF[PPC].PIM_OP == PIM_OPERATION::NOP) {
        if (LC == -1)
            LC = CRF[PPC].imm0 - 1;
        else if(LC > 0)
            LC -= 1;
        else if(LC == 0) {
            LC = -1;
            return NOP_END;
        }
        return 0;
    }

    else if (CRF[PPC].PIM_OP == PIM_OPERATION::JUMP) {
        if (LC == -1) {
            LC = CRF[PPC].imm1 - 1;
            PPC = CRF[PPC].imm0;
        }
        else if (LC > 0) {
            PPC = CRF[PPC].imm0;
            LC -= 1;
        }
        else if (LC == 0) {
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

    return (int)PPC;
}

void PimUnit::SetOperandAddr(string* pim_cmd) {
    // set _GRF_A, _GRF_B operand address when AAM mode
    if (CRF[PPC].PIM_OP == PIM_OPERATION::ADD_AAM ||
        CRF[PPC].PIM_OP == PIM_OPERATION::MUL_AAM ||
        CRF[PPC].PIM_OP == PIM_OPERATION::MAC_AAM ||
        CRF[PPC].PIM_OP == PIM_OPERATION::MAD_AAM) {
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
    }
    // set _GRF_A, _GRF_B operand address when non-AAM mode
    else {
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
        if (CRF[PPC].PIM_OP < PIM_OPERATION::ADD_AAM) {
            if (CRF[PPC].src1 == PIM_OPERAND::GRF_A)
                src1 = _GRF_A + CRF[PPC].src1_idx * 16;
            else if (CRF[PPC].src1 == PIM_OPERAND::GRF_B)
                src1 = _GRF_B + CRF[PPC].src1_idx * 16;    
        }
    }

    // set EVEN_BANK, ODD_BANK, SRF operand address 
    // set dst address
    if (CRF[PPC].dst == PIM_OPERAND::EVEN_BANK)
        dst = even_data;
    else if (CRF[PPC].dst == PIM_OPERAND::ODD_BANK)
        dst = odd_data;
    else if (CRF[PPC].dst == PIM_OPERAND::SRF_A)
        dst = _SRF_A + CRF[PPC].dst_idx;
    else if (CRF[PPC].dst == PIM_OPERAND::SRF_M)
        dst = _SRF_M + CRF[PPC].dst_idx;

    // set src0 address
    if (CRF[PPC].src0 == PIM_OPERAND::EVEN_BANK)
        src0 = even_data;
    else if (CRF[PPC].src0 == PIM_OPERAND::ODD_BANK)
        src0 = odd_data;
    else if (CRF[PPC].src0 == PIM_OPERAND::SRF_A)
        src0 = _SRF_A + CRF[PPC].src0_idx;
    else if (CRF[PPC].src0 == PIM_OPERAND::SRF_M)
        src0 = _SRF_M + CRF[PPC].src0_idx;

    // set src1 address only if PIM_OP == ADD, MUL, MAC, MAD -> uses src1 for operand
    if (CRF[PPC].PIM_OP < PIM_OPERATION::MOV) {
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
        case PIM_OPERATION::ADD_AAM:
            _ADD();
            break;
        case PIM_OPERATION::MUL:
        case PIM_OPERATION::MUL_AAM:
            _MUL();
            break;
        case PIM_OPERATION::MAC:
        case PIM_OPERATION::MAC_AAM:
            _MAC();
            break;
        case PIM_OPERATION::MAD:
        case PIM_OPERATION::MAD_AAM:
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
    for (int i = 0; i < 16; i++) {
        *dst = *src0 + *src1;

        if (CRF[PPC].dst != PIM_OPERAND::SRF_A)
            dst += 1;    // if operand == SRF -> do not change

        if (CRF[PPC].src0 != PIM_OPERAND::SRF_A)
            src0 += 1;

        if (CRF[PPC].src1 != PIM_OPERAND::SRF_A)
            src1 += 1;
    }
}

void PimUnit::_MUL() {
    for (int i = 0; i < 16; i++) {
        *dst = (*src0) * (*src1);
        
        if (CRF[PPC].dst != PIM_OPERAND::SRF_M)
            dst += 1;    // if operand == SRF -> do not change

        if (CRF[PPC].src0 != PIM_OPERAND::SRF_M)
            src0 += 1;

        if (CRF[PPC].src1 != PIM_OPERAND::SRF_M)
            src1 += 1;
    }
}

void PimUnit::_MAC() {
    for (int i = 0; i < 16; i++) {
        *dst += (*src0) * (*src1);

        if (CRF[PPC].dst != PIM_OPERAND::SRF_M)
            dst += 1;    // if operand == SRF -> do not change

        if (CRF[PPC].src0 != PIM_OPERAND::SRF_M)
            src0 += 1;

        if (CRF[PPC].src1 != PIM_OPERAND::SRF_M)
            src1 += 1;
    }
}

void PimUnit::_MAD() {
    std::cout << "not yet\n";
}

void PimUnit::_MOV() {
    for (int i = 0; i < 16; i++) {
        *dst = *src0;

        if (CRF[PPC].dst < PIM_OPERAND::SRF_A)
            dst += 1;    // if operand == SRF -> do not change

        if (CRF[PPC].src0 < PIM_OPERAND::SRF_A)
            src0 += 1;
    }
}
