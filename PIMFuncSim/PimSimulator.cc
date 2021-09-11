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
    // A[N] + B[N] = C[N]
    int N = 2048;
    unit_t *A = (unit_t*) malloc(N * sizeof(unit_t));
    unit_t *B = (unit_t*) malloc(N * sizeof(unit_t));
    unit_t *C_CPU = (unit_t*) malloc(N * sizeof(unit_t));
    unit_t *C_PIM = (unit_t*) malloc(N * sizeof(unit_t));

    for (int n = 0; n < N; n++) {
        A[n] = (unit_t)rand() / RAND_MAX;
        B[n] = (unit_t)rand() / RAND_MAX;
        C_CPU[n] = A[n] + B[n];
    }

    for (int p = 0; p < NUM_PIMS; p++) {
        for (int n = 0; n < N/NUM_PIMS; n++) {
            int offset = 2 * UNITS_PER_BK * p + n;
            std::memcpy(PimSim.physmem + offset,
                &A[p*N/NUM_PIMS + n], sizeof(unit_t));
            std::memcpy(PimSim.physmem + offset + N/NUM_PIMS,
                &B[p*N/NUM_PIMS + n], sizeof(unit_t));
        }
    }

    // PIM ~~~ //
    PimSim.CrfInit();
    PimSim.Run();
    // Ended,, //

    for (int p = 0; p < NUM_PIMS; p++) {
        for (int n = 0; n < N/NUM_PIMS; n++) {
            std::memcpy(&C_PIM[p*N/NUM_PIMS + n],
                PimSim.physmem + 2 * UNITS_PER_BK * p + n, sizeof(unit_t));
        }
    }

    unit_t err = 0;
    for (int n = 0; n < N; n++)
        err += ABS(C_CPU[n] - C_PIM[n]);

    std::cout << "error : " << static_cast<float>(err) << std::endl;
}

void AddAamTest(PimSimulator PimSim) {
    // A[n] + B[n] = C[n]
    int N = 16384;
    unit_t *A = (unit_t*) malloc(N * sizeof(unit_t));
    unit_t *B = (unit_t*) malloc(N * sizeof(unit_t));
    unit_t *C_CPU = (unit_t*) malloc(N * sizeof(unit_t));
    unit_t *C_PIM = (unit_t*) malloc(N * sizeof(unit_t));

    for (int n = 0; n < N; n++) {
        A[n] = (unit_t)rand() / RAND_MAX;
        B[n] = (unit_t)rand() / RAND_MAX;
        C_CPU[n] = A[n] + B[n];
    }

    for (int p = 0; p < NUM_PIMS; p++) {
        for (int n = 0; n < N/NUM_PIMS; n++) {
            int offset = 2 * UNITS_PER_BK * p + n;
            std::memcpy(PimSim.physmem + offset,
                &A[p*N/NUM_PIMS + n], sizeof(unit_t));
            std::memcpy(PimSim.physmem + offset + N/NUM_PIMS,
                &B[p*N/NUM_PIMS + n], sizeof(unit_t));
        }
    }

  // PIM ~~~ //
  PimSim.CrfInit();
  PimSim.Run();
  // Ended,, //

    for (int p = 0; p < NUM_PIMS; p++) {
        for (int n = 0; n < N/NUM_PIMS; n++) {
            std::memcpy(&C_PIM[p*N/NUM_PIMS + n],
                PimSim.physmem + 2 * UNITS_PER_BK * p + n,
                sizeof(unit_t));
        }
    }

    unit_t err = 0;
    for (int n = 0; n < N; n++)
        err += ABS(C_CPU[n] - C_PIM[n]);

    std::cout << "error : " << static_cast<float>(err) << std::endl;
}

void GemvTest(PimSimulator PimSim) {
    // A[N][M] x B[M] = C[N]
    int M = 512;
    int N = 128;

    unit_t *A = (unit_t*) malloc(N * M * sizeof(unit_t));
    unit_t *B = (unit_t*) malloc(M * sizeof(unit_t));
    unit_t *C_CPU = (unit_t*) malloc(N * sizeof(unit_t));
    unit_t *C_PIM = (unit_t*) malloc(N * sizeof(unit_t));

    for (int m = 0; m < M; m++) {
        B[m] = (unit_t)((double)rand() / RAND_MAX * 100);
        for (int n = 0; n < N; n++)
            A[n * M + m] = (unit_t)((double)rand() / RAND_MAX * 100);
    }

    for (int m = 0; m < M; m++)
        for (int n = 0; n < N; n++)
            C_CPU[n] += B[m] * A[n * M + m];

    for (int p = 0; p < NUM_PIMS; p++) {
        for (int m = 0; m < M/NUM_PIMS; m++) {
            for (int n = 0; n < N; n++) {
                // Set bank data
                std::memcpy(PimSim.physmem + 2 * UNITS_PER_BK * p + m * N + n,
                    &A[n * M + p*M/NUM_PIMS + m], sizeof(unit_t));
            }
            // Set SRF registers
            std::memcpy(PimSim._PimUnit[p]._SRF_M + m,
                &B[p*M/NUM_PIMS + m], sizeof(unit_t));
        }
    }

    // PIM ~~~ //
    PimSim.CrfInit();
    PimSim.Run();
    // Ended,, //

    for (int n = 0; n < N; n++) {
        C_PIM[n] = 0;
        for (int p = 0; p < NUM_PIMS; p++)
            C_PIM[n] += PimSim.physmem[2 * UNITS_PER_BK * p + n];
    }

    unit_t err = 0;
    for (int n = 0; n < N; n++) {
#ifdef debug_mode
        std::cout << static_cast<float>(C_CPU[n]) << "\t"
             << static_cast<float>(C_PIM[n]) << "\t sub : "
             << static_cast<float>(C_CPU[n]) - static_cast<float>(C_PIM[n])
             << std::endl;
#endif
        err += ABS(C_CPU[n] - C_PIM[n]);
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
