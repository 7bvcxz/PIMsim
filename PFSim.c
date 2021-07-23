#include <stdio.h>
#include "utils.h"
#include "config.h"

int DRAM[NUM_PCHS][NUM_BANKS][NUM_ROWS][NUM_COLS];
char bank_mode = 'S';

void simulate(){
  FILE *fp = fopen("DRAM_cmd.txt", "r");

  int pch, bank, row, col, data, is_write;

  printf("\tpch \tbank \trow \tcol\n");
  while(fscanf(fp, "%d %d %d %d %d %d", &pch,  &bank, &row, &col, &data, &is_write) && !feof(fp)){
	printf("Addr:\t%d\t %d\t %d\t %d\t Data: %d\t is_write: %d\n", pch, bank, row, col, data, is_write);
	printf("Addr:\t%d\t %d\t %d\t %d\t Data: %d\t is_write: %d\n", pch, bank, row, col, data, is_write);

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
		  PIM_compute(data);
		}	
	  }
	  else{
		for(int i=0; i<NUM_BANKS; i++){
		  data = DRAM[pch][i][row][col];
		  PIM_compute(data);
		}
	  }
	}
  
  fclose(fp);
  return; 
}


int main(){
  simulate();

  return 0;
}
