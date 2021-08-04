/////////////////- set unit size - ///////////////////
typedef float		unit_t;
#define debug_mode
//////////////////////////////////////////////////////

#pragma once
#define NOP_END		77
#define EXIT_END	777

#define BGS_PER_PCH		  4
#define BKS_PER_BG		  4
#define ROWS_PER_BK		  8
#define COLS_PER_BK		  32
#define WORDS_PER_BK	  ROWS_PER_BK * COLS_PER_BK	// 256
#define UNITS_PER_WORD	  16
#define UNITS_PER_ROW     COLS_PER_BK * UNITS_PER_WORD
#define UNITS_PER_BK	  WORDS_PER_BK * UNITS_PER_WORD

#define NUM_PCHS	16
#define NUM_BGS		NUM_PCHS * BGS_PER_PCH	// 64
#define NUM_BANKS	NUM_BGS * BKS_PER_BG	// 256
#define NUM_PIMS	NUM_BANKS / 2			// 128

// SIZE IS BYTE
#define UNIT_SIZE		  sizeof(unit_t)
#define WORD_SIZE		  UNITS_PER_WORD * UNIT_SIZE
#define BANK_SIZE		  WORDS_PER_BK * WORD_SIZE		  // 256 * 16 * 4
#define PIM_PHYSMEM_SIZE  2 * BANK_SIZE					  // 2 * 256 * 16 * 4
#define PHYSMEM_SIZE	  NUM_BANKS * BANK_SIZE			  // 256 * 256 * 16 * 4

#define GRF_SIZE	8 * UNITS_PER_WORD * UNIT_SIZE
#define SRF_SIZE	UNITS_PER_WORD * UNIT_SIZE

enum PIM_OPERATION {ADD=0, MUL, MAC, MAD, ADD_AAM, MUL_AAM, MAC_AAM, MAD_AAM, MOV, FILL, NOP, JUMP, EXIT};
enum PIM_OPERAND   {EVEN_BANK=0, ODD_BANK, GRF_A, GRF_B, 
					GRF_A0=10, GRF_A1, GRF_A2, GRF_A3, GRF_A4, GRF_A5, GRF_A6, GRF_A7,
					GRF_B0=20, GRF_B1, GRF_B2, GRF_B3, GRF_B4, GRF_B5, GRF_B6, GRF_B7,
					SRF_A0=30, SRF_A1, SRF_A2, SRF_A3, SRF_A4, SRF_A5, SRF_A6, SRF_A7,
					SRF_M0=40, SRF_M1, SRF_M2, SRF_M3, SRF_M4, SRF_M5, SRF_M6, SRF_M7
					};





