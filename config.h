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

#define INVALID		-32768
#define UINT8_MAX   0xffui8
#define UINT16_MAX  0xffffui16
#define UINT32_MAX  0xffffffffui32
#define UINT64_MAX  0xffffffffffffffffui64

