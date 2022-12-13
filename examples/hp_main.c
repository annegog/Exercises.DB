#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "bf.h"
#include "hp_file.h"

#define RECORDS_NUM 1000 // you can change it if you want
#define FILE_NAME "data.db"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

int main() {
  BF_Init(LRU);

  HP_CreateFile(FILE_NAME);
  //sleep(30);
  //printf("no error so far 1\n");
  HP_info* info = HP_OpenFile(FILE_NAME);
  // printf("no error so far 1.1\n");
  
  Record record;
  srand(12569874);
  int r;
  printf("Insert Entries\n");
  for (int id = 0; id < 10; ++id) {
    record = randomRecord();
    HP_InsertEntry(info, record);
  }

  // printf("RUN PrintAllEntries\n");
  // int id = rand() % RECORDS_NUM;
  // printf("\nSearching for: %d",id);
  // HP_GetAllEntries(info, id);

  // HP_CloseFile(info); 
  
  BF_Close(); // τα έβγαλα απο σχόλια ώστε να κλείνει και απο την main
}
