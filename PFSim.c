#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "config.h"

enum PIM_OP {
  JUMP,
  NOP,
  EXIT,
  MOV,
  FILL,
  MAC,
  MAD,
  ADD,
  MUL,
};

class CRF {
  
}

char  DRAM[NUM_PCHS][NUM_BANKS][NUM_ROWS][NUM_COLS];
int	  GRF_A[NUM_PCHS * NUM_BANKS / 2][8], GRF_B[NUM_PCHS * NUM_BANKS /2][8];
char  CRF[32][28];
int	  PPC=0;
char  bank_mode = 'S';

void PIM_compute(int, int, int, int, int);

void read_CRF(){
  FILE *fp = fopen("CRF.txt", "r");
  int idx = 0;
  
  while(fgets(CRF[idx], sizeof(CRF[idx]), fp) && !feof(fp)){
	idx ++;
  }
  fclose(fp);
}

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

	if(bank_mode == 'S'){
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
  printf("\nPIM_compute!\n");
  int A_indx = col%8;
  int B_indx = row%2 + col/8;
  
  FILE *fp = fopen("CRF.txt", "r");

  char *tmp;
  tmp = strtok(CRF[PPC], " ");

  while(tmp != NULL){
	printf("%s", tmp);
	tmp = strtok(NULL, "\n");
  }

  PPC ++;

  /*
  char OP[7], DST[7], SRC0[7], SRC1[7];

  strcpy(OP,	CRF[PPC][0]);
  strcpy(DST,	CRF[PPC][1]);
  strcpy(SRC0,	CRF[PPC][2]);
  strcpy(SRC1,	CRF[PPC][3]);

  printf("%s\t %s\t %s\t %s\n", OP, DST, SRC0, SRC1);

  //MOV
  if(OP[0] == 'M'){
	 printf("mov\n"); 
	
  }

  //ADD
  else if(OP[0] == 'A'){
    printf("add\n");
  }

  //JUMP
  else if(OP[0] == 'J'){
	  
  }
  else{
    printf("not yet implemented,,,\n");
  }
  */
}


int main(){
  
  read_CRF();

  simulate();

  return 0;
}


