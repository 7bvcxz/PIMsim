# PIMFuncSim

## 1. Code Structure
```
└── testers            # Offer a set of CRF.txt, PimCmd.txt to test computations easily (ADD, GEMV)
└── config             # Configurations. Can set unit size & debug mode
└── utils              # Several codes to support PimUnit
└── PimSimulator       # Define physical memory, program each PimUnit, and send transactions to a proper PimUnit
└── PimUnit            # A PimUnit that is attached to 2 banks. Has registers, and compute PIM with transactions from PimSimulator
└── Makefile           # 
└── CRF.txt            # A txt file that has μKernel to compute PIM
└── PimCmd.txt         # A txt file that has transactions to compute PIM
```

## 2. How it works
```
1. Initialize PimUnits (①)
2. Allocate physical memory + write data to test in physical memory (①)
3. Program μKernal in CRF.txt to PimUnits (②)
4. Send transactions in PimCmd.txt to PimUnits one by one --> Compute PIM (③)
5. Compute ERROR between answer(calculated normally) and result (calculated with PIM)
```
![PimFuncSim](https://user-images.githubusercontent.com/80901560/133019673-ebc2feb2-25b5-4704-a036-cee60ec7231a.png)

## 3. Build
```
make
```

## 4. HowToUse
```
Write μkernel in CRF.txt
Write transactions to run PIM in PimCmd.txt
Run PimSimulator
```

## 5. Run
```
./PimSimulator
```

## 6. Testers
```
There are several sets of [CRF.txt, PimCmd.txt] in testers folder
Move [CRF.txt, PimCmd.txt] to the original folder to test it
Should rename it to CRF.txt, PimCmd.txt each!
```

## 7. Different unit size & Debug mode
```
Change setting in config.h
Use different unit size by changing "typedef ( different unit size )  unit_t"
Use Debug mode by defining debug_mode
```

## 8. Clean
```
make clean
```
