#include <sys/mman.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include "./config.h"
#include "./PimUnit.h"

class PimSimulator {
 public:
    // Just a counter checking how many iterations it went
    uint64_t clk;
    // File name that has Pim Commands to be executed sequentially
    std::string pim_cmd_filename;
    // File name that has Micro Kernels to be programmed to each Pim Unit's CRF
    std::string pim_micro_kernel_filename;
    // Total Pim Units
    PimUnit _PimUnit[NUM_PIMS];
    // A simple physical memory
    unit_t *physmem;

    PimSimulator(std::string pcf, std::string pmkf) {
        clk = 0;
        pim_cmd_filename = pcf;
        pim_micro_kernel_filename = pmkf;
        for (int i = 0; i < NUM_PIMS; i++) {
            _PimUnit[i] = PimUnit();
             // Also set Pmk(pim_micro_kernel) Filename to each Pim Unit
            _PimUnit[i].SetPmkFilename(pim_micro_kernel_filename);
        }
    }

    // Allocate Physical Memory and initialize.
    void PhysmemInit() {
        std::cout << ">> Initializing PHYSMEM...\n";

        physmem = (unit_t *) mmap(NULL, PHYSMEM_SIZE, PROT_READ | PROT_WRITE,
                                  MAP_ANON | MAP_PRIVATE, -1, 0);
        if (physmem == (unit_t *) MAP_FAILED)
            perror("mmap");
        std::cout << ">> Allocated Space for PHYSMEM\n";

        // Also connect each Pim Unit's physmem to appropriate physical memory
        // address together
        for (int i = 0; i < NUM_PIMS; i++)
            _PimUnit[i].SetPhysmem(physmem + i * 2 * UNITS_PER_BK);

        std::cout << "<< Initialized PHYSMEM!\n\n";
    }

    // Read pim_cmd_filename and Program the whole Pim Unit's CRF with micro
    // kernels
    void CrfInit() {
        std::cout << ">> Initializing PimUnit's CRF...\n";
        for (int i = 0; i < NUM_PIMS; i++)
            this->_PimUnit[i].CrfInit();
        std::cout << "<< Initialized PimUnit's CRF!\n\n";
    }

    // Run Simulator that acts same as the actual PIM-DRAM AB-PIM mode
    void Run(){
        std::cout << ">> Run Simulator\n";
        std::ifstream fp;
        fp.open(pim_cmd_filename);

        std::string str;
        while (getline(fp, str) && !fp.eof()) {
            std::string cmd_part[18];
            int num_parts = (str.size()-1)/10 + 1;
            int flag = 0;

            if (str.size() == 0)
                continue;  // just to see PimCmd.txt easily
#ifdef debug_mode
            std::cout << "Cmd "<< clk+1 << " : ";
#endif
            for (int i = 0; i < num_parts; i++) {
                std::string part = str.substr(i*10, 9);
                cmd_part[i] = part.substr(0, part.find(' '));
#ifdef debug_mode
                std::cout << cmd_part[i] << " ";
#endif
            }

            for (int j = 0; j < NUM_PIMS; j++)
                flag = _PimUnit[j].Issue(cmd_part, num_parts);

            if (flag == EXIT_END) {
#ifdef debug_mode
                std::cout << "\t==> EXIT END!\n";
#endif
                break;
            } else if (flag == NOP_END) {
#ifdef debug_mode
                std::cout << "\t==> NOP END!\n";
#endif
                break;
            }
#ifdef debug_mode
            std::cout << "\t==> execute PPC : " << flag-1 << std::endl;
#endif
            clk++;
        }
        std::cout << "<< Run End\n";
    }
};

void AddTest(PimSimulator PimSim) {
    unit_t A[2048];
    unit_t B[2048];
    unit_t C[2048];
    unit_t D[2048];

    for (int i = 0; i < 2048; i++) {
        A[i] = (unit_t)rand() / RAND_MAX;
        B[i] = (unit_t)rand() / RAND_MAX;
        C[i] = A[i] + B[i];
    }

    for (int i = 0; i < NUM_PIMS; i++) {
        for (int j = 0; j < 16; j++) {
            int offset = 2 * UNITS_PER_BK * i + j;
            std::memcpy(PimSim.physmem + offset, &A[i*16+j], sizeof(unit_t));
            std::memcpy(PimSim.physmem + offset+16, &B[i*16+j], sizeof(unit_t));
        }
    }

  // PIM ~~~ //
  PimSim.CrfInit();
  PimSim.Run();
  // Ended,, //

    for (int i = 0; i < NUM_PIMS; i++) {
        for (int j = 0; j < 16; j++) {
            std::memcpy(&D[i*16 + j],
                PimSim.physmem + 2 * UNITS_PER_BK * i + j, sizeof(unit_t));
        }
    }

    unit_t err = 0;
    for (int i = 0; i < 2048; i++)
        err += C[i] - D[i];

    std::cout << "error : " << static_cast<float>(err) << std::endl;
}

void AddAamTest(PimSimulator PimSim) {
    unit_t A[16384];
    unit_t B[16384];
    unit_t C[16384];
    unit_t D[16384];

    for (int i = 0; i < 16384; i++) {
        A[i] = (unit_t)rand() / RAND_MAX;
        B[i] = (unit_t)rand() / RAND_MAX;
        C[i] = A[i] + B[i];
    }

    for (int i = 0; i < NUM_PIMS; i++) {
        for (int j = 0; j < 8; j++) {
            for (int k = 0; k < 16; k++) {
                int offset = 2 * UNITS_PER_BK * i + j * 16 + k;
                std::memcpy(PimSim.physmem + offset,
                    &A[i*8*16 + j*16 + k], sizeof(unit_t));
                std::memcpy(PimSim.physmem + offset + 8 * 16,
                    &B[i*8*16 + j*16 + k], sizeof(unit_t));
            }
        }
    }

  // PIM ~~~ //
  PimSim.CrfInit();
  PimSim.Run();
  // Ended,, //

    for (int i = 0; i < NUM_PIMS; i++) {
        for (int j = 0; j < 8; j++) {
            for (int k = 0; k < 16; k++) {
                std::memcpy(&D[i*8*16 + j*16 + k],
                    PimSim.physmem + 2 * UNITS_PER_BK * i + j * 16 + k,
                    sizeof(unit_t));
            }
        }
    }

    unit_t err = 0;
    for (int i = 0; i < 16384; i++)
        err += C[i] - D[i];

    std::cout << "error : " << static_cast<float>(err) << std::endl;
}

void GemvTest(PimSimulator PimSim) {
    // A[M][N], B[M], O[N]
    unit_t A[512][128];
    unit_t B[512];
    unit_t C[128];
    unit_t _D[128];
    unit_t D[128];

    for (int m = 0; m < 512; m++) {
        B[m] = (unit_t)((double)rand() / RAND_MAX * 100);
        for (int n = 0; n < 128; n++)
            A[m][n] = (unit_t)((double)rand() / RAND_MAX * 100);
    }

    for (int m = 0; m < 512; m++)
        for (int n = 0; n < 128; n++)
            C[n] += B[m] * A[m][n];

    for (int i = 0; i < NUM_PIMS; i++) {
        for (int m = 0; m < 4; m++) {
            for (int n = 0; n < 128; n++) {
                std::memcpy(PimSim.physmem + 2 * UNITS_PER_BK * i + m * 128 + n,
                    &A[i*4 + m][n], sizeof(unit_t));
            }
            std::memcpy(PimSim._PimUnit[i]._SRF_M + m,
                &B[i*4 + m], sizeof(unit_t));
        }
    }

  // PIM ~~~ //
  PimSim.CrfInit();
  PimSim.Run();
  // Ended,, //

    for (int i = 0; i < NUM_PIMS; i++) {
        for (int j = 0; j < 128; j++) {
            memcpy(&_D[j],
                PimSim.physmem + 2 * UNITS_PER_BK * i + j, sizeof(unit_t));
            D[j] += _D[j];
        }
    }

    unit_t err = 0;
    for (int n = 0; n < 128; n++) {
#ifdef debug_mode
        std::cout << static_cast<float>(C[n]) << "\t"
             << static_cast<float>(D[n]) << "\t sub : "
             << static_cast<float>(C[n]) - static_cast<float>(D[n])
             << std::endl;
#endif
        err += ABS(C[n] - D[n]);
    }
    std::cout << "error : " << static_cast<float>(err) << std::endl;
}


int main(int argc, char* argv[]) {
    std::string pim_cmd_filename = "PimCmd.txt";
    std::string pim_micro_kernel_filename = "CRF.txt";

    if (argc == 1 || std::string(argv[1]) == "ADD") {
        pim_cmd_filename = "testers/Add_PimCmd.txt";
        pim_micro_kernel_filename = "testers/Add_CRF.txt";
    } else if (std::string(argv[1]) == "ADD_AAM") {
        pim_cmd_filename = "testers/AddAam_PimCmd.txt";
        pim_micro_kernel_filename = "testers/AddAam_CRF.txt";
    } else if (std::string(argv[1]) == "GEMV") {
        pim_cmd_filename = "testers/Gemv_PimCmd.txt";
        pim_micro_kernel_filename = "testers/Gemv_CRF.txt";
    } else {
        std::cout << "Invalid arguments" << std::endl;
        return -1;
    }

    //srand((unsigned)time(NULL));
    srand(0);
    PimSimulator PimSim = PimSimulator(pim_cmd_filename, pim_micro_kernel_filename);

    PimSim.PhysmemInit();

    if (argc == 1 || std::string(argv[1]) == "ADD")
        AddTest(PimSim);
    else if (std::string(argv[1]) == "ADD_AAM")
        AddAamTest(PimSim);
    else if (std::string(argv[1]) == "GEMV")
        GemvTest(PimSim);

    return 0;
}
