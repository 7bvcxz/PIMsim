#pragma once
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

#define BGS_PER_PCH 4
#define BKS_PER_BG  4

#define NUM_PCHS	16
#define NUM_BGS		NUM_PCHS * BGS_PER_PCH	// 64
#define NUM_BANKS	NUM_BGS * BKS_PER_BG	// 256
#define NUM_PIMS	NUM_BANKS / 2			// 128
#define NUM_ROWS	8
#define NUM_COLS	32

#define NUM_CELLS	NUM_ROWS * NUM_COLS		// 256
#define NUM_SECTORS	16
#define SECTOR_SIZE	4 // 4 byte = 32 bit

#define PHYSMEM_SIZE  NUM_BANKS * NUM_CELLS * NUM_SECTORS * SECTOR_SIZE // 2097152 = 256 * 256 * 16 * 2

#define UINT8_MAX   0xffui8
#define UINT16_MAX  0xffffui16
#define UINT32_MAX  0xffffffffui32
#define UINT64_MAX  0xffffffffffffffffui64


enum PIM_OPERATION {ADD=0, MUL, MAC, MAD, ADD_AAM, MUL_AAM, MAC_AAM, MAD_AAM, MOV, FILL, NOP, JUMP, EXIT};
enum PIM_OPERAND   {ODD_BANK=0, EVEN_BANK, GRF_A, GRF_B, SRF_A, SRF_B, 
					GRF_A0=10, GRF_A1, GRF_A2, GRF_A3, GRF_A4, GRF_A5, GRF_A6, GRF_A7,
					GRF_B0=20, GRF_B1, GRF_B2, GRF_B3, GRF_B4, GRF_B5, GRF_B6, GRF_B7
					};





