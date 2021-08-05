#ifndef __PIM_UNIT_H
#define __PIM_UNIT_H

#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <sstream>
#include <sys/mman.h>

#include "config.h"
#include "utils.h"

#define PIM_MMAP_PROT (PROT_READ | PROT_WRITE)
#define PIM_MMAP_FLAGS (MAP_ANON | MAP_PRIVATE)

class PimInstruction{
  public:
    PIM_OPERATION PIM_OP;

    PIM_OPERAND dst;
    PIM_OPERAND src0;
    PIM_OPERAND src1;

    int dst_idx;
    int src0_idx;
    int src1_idx;
    int src2_idx;

    int imm0;
    int imm1;
};

class PimUnit{
  public:  
	PimUnit();
	void SetPmkFilename(string pim_micro_kernel_filename);
	void SetPhysmem(unit_t* physmem);
	void CrfInit();
	void PushCrf(string* mk_part, int num_parts);
	int Issue(string* pim_cmd, int num_parts);
	void SetOperandAddr(string* pim_cmd);
	void Execute();
	void _ADD();
	void _MUL();
	void _MAC();
	void _MAD();
	void _MOV();

    PimInstruction CRF[32];
    string pim_micro_kernel_filename;
    uint8_t PPC;
    int RA;
    int LC;

    unit_t *_GRF_A;
    unit_t *_GRF_B;
    unit_t *_SRF_A;
    unit_t *_SRF_M;

    unit_t *dst;
    unit_t *src0;
    unit_t *src1;

    unit_t *physmem;
    unit_t *even_data;
    unit_t *odd_data;
};

#endif
