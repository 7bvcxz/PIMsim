# 이 문서는 Transaction Generator, DRAMsim3, Pim Function Simulator 실험 환경을 만드는 방법과, PIMsim 실험 환경을 만드는 방법을 가이드 합니다.

## Build PIM Simulator
```
$ mkdir build
$ cd build
$ cmake ..
$ make -j4
```

## Run ADD 
```
./pimdramsim3main
    - ../configs/( memory setting )
        - only support 'HBM2_4Gb_test.ini' yet
    - --pim-api=add
    - --add-n= ( vector length )
        - only support multiple of 4096 yet

e.g., 
$ ./pimdramsim3main ../configs/HBM2_4Gb_test.ini --pim-api=add --add-n=4096
$ ./pimdramsim3main ../configs/HBM2_4Gb_test.ini --pim-api=add --add-n=8192
```
## Run GEMV
```
./pimdramsim3main
    - ../configs/( memory setting )
        - only support 'HBM2_4Gb_test.ini' yet
    - --pim-api=gemv
    - --gemv-m= ( output length )
        - only support multiple of 4096 yet
    - --gemv-n= ( input length )
        - only support multiple of 32 yet
        
e.g., 
$ ./pimdramsim3main ../configs/HBM2_4Gb_test.ini --pim-api=gemv --gemv-m=4096 --gemv-n=32
$ ./pimdramsim3main ../configs/HBM2_4Gb_test.ini --pim-api=gemv --gemv-m=8192 --gemv-n=32
$ ./pimdramsim3main ../configs/HBM2_4Gb_test.ini --pim-api=gemv --gemv-m=8192 --gemv-n=96
```

## Debug Mode
```
Can set debug mode in pim_config.h
Upgrade in progress,,,
```

## Clean
```
$ make clean
```