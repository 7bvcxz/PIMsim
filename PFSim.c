#include <stdio.h>
#include "utils.h"
#include "config.h"

int DRAM[NUM_PCHS][NUM_BANKS][NUM_ROWS][NUM_COLS];
char bank_mode = 'S';

void PIM_compute(int, int, int, int, int);

void simulate(){
  FILE *fp = fopen("DRAM_cmd.txt", "r");

  int pch, bank, row, col, data, is_write;

  printf("\tpch \tbank \trow \tcol\n");
  while(fscanf(fp, "%d %d %d %d %d %d", &pch,  &bank, &row, &col, &data, &is_write) && !feof(fp)){
	printf("Addr:\t%d\t %d\t %d\t %d\t Data: %d\t is_write: %d\n", pch, bank, row, col, data, is_write);

	if(pch == 15 && bank == 15 && row == 7){  // change to all-bank mode (simply)
	  bank_mode = 'A';
	  printf("%sSB mode -> AB mode%s\n", C_YLLW, C_NRML);
	}

	if(bank_mode = 'S'){
	  if(is_write){
		printf("%swrite%s: %d\n", C_RED, C_NRML, data);
		DRAM[pch][bank][row][col] = data;
	  }
	  else{
		data = DRAM[pch][bank][row][col];
		printf("%sread%s: %d\n", C_BLUE, C_NRML, data);
	  }

	  printf("\n");
	}

	else{
	  if(is_write){
		for(int i=0; i<NUM_BANKS; i++){
		  DRAM[pch][i][row][col] = data;
		  PIM_compute(pch, i, row, col, data);
		}	
	  }
	  else{
		for(int i=0; i<NUM_BANKS; i++){
		  data = DRAM[pch][i][row][col];
		  PIM_compute(pch, i, row, col, data);
		}
	  }
	}
  }

  fclose(fp);
  return; 
}

void PIM_compute(int pch, int bank, int row, int col, int data){
  int A_indx = col%8;
  int B_indx = row%2 + col/8;
  
  FILE *fp = fopen("DRAM_cmd.txt", "r");
  int pch, bank, row, col, data, is_write;
  // yugi yugi hanunjung !!!

  printf("\tpch \tbank \trow \tcol\n");
  while(fscanf(fp, "%d %d %d %d %d %d", &pch,  &bank, &row, &col, &data, &is_write) && !feof(fp)){
	printf("Addr:\t%d\t %d\t %d\t %d\t Data: %d\t is_write: %d\n", pch, bank, row, col, data, is_write);

	
   
}



int main(){
  simulate();

  return 0;
}


