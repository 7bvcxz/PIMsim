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

#define INVALID		-32768


