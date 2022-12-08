#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

#define CALL_BF_NUM(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return -1;        \
  }                         \
}

#define CALL_BF_NULL(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return NULL;        \
  }                         \
}

int HP_CreateFile(char *fileName){
  int fd;
  void* data;
  BF_Block *block;
  BF_Block_Init(&block);

  HP_info info;
  memcpy(info.fileType, "heap", strlen("heap")+1);

  CALL_BF(BF_CreateFile(fileName));
  CALL_BF(BF_OpenFile(fileName, &fd));
  
  CALL_BF(BF_AllocateBlock(fd, block));  // Δημιουργία καινούριου block
  data = BF_Block_GetData(block);  
  memcpy(data, &info, 5);

  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  CALL_BF(BF_CloseFile(fd));               //Κλείσιμο αρχείου και αποδέσμευση μνήμης
  CALL_BF(BF_Close());
  return 0;
}

HP_info* HP_OpenFile(char *fileName){
  int fd;
  void* data;
  BF_Block *block;
  BF_Block_Init(&block);
  BF_OpenFile(fileName, &fd);
  BF_GetBlock(fd, 0, block); // λογικα εδω παίρνει το 1ο block
  data = BF_Block_GetData(block);  
  HP_info *info=data;
  printf("no error so far 2\n");
  if(strcmp(info->fileType, "heap")==0) // αν είναι ίδια
    {
      printf("bhke mesa!!!!\n");
      return info;
    }
    printf("den bhke mesa!!!!\n");
  return NULL ;// αν δεν είναι γυρνάει NULL
}


int HP_CloseFile( HP_info* hp_info ){
    return 0;
}

int HP_InsertEntry(HP_info* hp_info, Record record){
    return 0;
}

int HP_GetAllEntries(HP_info* hp_info, int value){
   return 0;
}

