#pragma once
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

#define UINT8_MAX   0xffui8
#define UINT16_MAX  0xffffui16
#define UINT32_MAX  0xffffffffui32
#define UINT64_MAX  0xffffffffffffffffui64

#define NOP_END	    -1
#define EXIT_END	-2

#define BGS_PER_PCH		  4
#define BKS_PER_BG		  4
#define ROWS_PER_BK		  8
#define COLS_PER_BK		  32
#define CELLS_PER_BK	  ROWS_PER_BK * COLS_PER_BK	// 256
#define SECTORS_PER_CELL  16

#define NUM_PCHS	16
#define NUM_BGS		NUM_PCHS * BGS_PER_PCH	// 64
#define NUM_BANKS	NUM_BGS * BKS_PER_BG	// 256
#define NUM_PIMS	NUM_BANKS / 2			// 128


// SIZE IS BYTE
#define SECTOR_SIZE		  4  // 4 byte = 32 bit
#define CELL_SIZE		  SECTORS_PER_CELL * SECTOR_SIZE
#define PHYSMEM_SIZE	  NUM_BANKS * CELLS_PER_BK * CELL_SIZE	// 256 * 256 * 16 * 4
#define PIM_PHYSMEM_SIZE  2 * CELLS_PER_BK * CELL_SIZE			// 2 * 256 * 16 * 4

#define GRF_SIZE	8 * SECTORS_PER_CELL * SECTOR_SIZE
#define SRF_SIZE	SECTORS_PER_CELL * SECTOR_SIZE


enum PIM_OPERATION {ADD=0, MUL, MAC, MAD, ADD_AAM, MUL_AAM, MAC_AAM, MAD_AAM, MOV, FILL, NOP, JUMP, EXIT};
enum PIM_OPERAND   {EVEN_BANK=0, ODD_BANK, GRF_A, GRF_B, SRF_A, SRF_M, 
					GRF_A0=10, GRF_A1, GRF_A2, GRF_A3, GRF_A4, GRF_A5, GRF_A6, GRF_A7,
					GRF_B0=20, GRF_B1, GRF_B2, GRF_B3, GRF_B4, GRF_B5, GRF_B6, GRF_B7
					};





