#include <stdio.h>
#include <string.h>

char  CRF[32][28];
int	  PPC;

int main(){
  FILE *fp = fopen("CRF.txt", "r");
  int idx = 0;

  printf("1111111\n");
  while(fgets(CRF[idx], sizeof(CRF[idx]), fp) && !feof(fp)){
	printf("%s\n", CRF[idx]);
	idx ++;
  }

  char *tmp;
  char str[20] = "hi i am.";
  printf("2222222\n");
  //tmp = strtok(str, " ");
  tmp = strtok(CRF[0], " ");
  printf("%s\n", tmp);
  /*
  while(tmp != NULL){
	printf("%s\n", tmp);
	tmp = strtok(NULL, " ");
  }
  */


  return 0;
}
