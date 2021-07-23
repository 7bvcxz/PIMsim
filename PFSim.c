#include <stdio.h>
#include "utils.h"
#include "config.h"

int DRAM[NUM_PCHS][NUM_BANKS][NUM_ROWS][NUM_COLS];

void simulate(){
  FILE *fp = fopen("DRAM_cmd.txt", "r");

  int bank, row, col, data, is_write;
 
  while(fscanf(fp, "%d %d %d %d %d", &bank, &row, &col, &data, &is_write) && !feof(fp)){
	printf("%d %d %d %d %d\n", bank, row, col, data, is_write);

	if(is_write)
	  DRAM[0][bank][row][col] = data;
	else
	  printf("read : %d\n", DRAM[0][bank][row][col]);
  }
  
  fclose(fp);
  return; 
}


int main(){
  simulate();

  return 0;
}
