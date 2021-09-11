# PIMFuncSim

## 1. Build
```
make
```

## 2. HowToUse
```
Write μkernel in CRF.txt
Write transactions to run PIM in PimCmd.txt
Run PimSimulator
```

## 3. Run
```
./PimSimulator
```

## 4. Testers
```
There are several sets of [CRF.txt, PimCmd.txt] in testers folder
Move [CRF.txt, PimCmd.txt] to the original folder to test it
Should rename it to CRF.txt, PimCmd.txt each!
```

## 5. Different unit size & Debug mode
```
Change setting in config.h
Use different unit size by changing "typedef ( different unit size )  unit_t"
Use Debug mode by defining debug_mode
```

## 6. Clean
```
make clean
```