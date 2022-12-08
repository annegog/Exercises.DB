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

// #define CALL_BF_NULL(call)       \
// {                           \
//   BF_ErrorCode code = call; \
//   if (code != BF_OK) {         \
//     BF_PrintError(code);    \
//     return NULL;        \
//   }                         \
// }

int HP_CreateFile(char *fileName){
  int fd;
  void* data;
  BF_Block *block;
  BF_Block_Init(&block);

  HP_info info;

  memcpy(info.fileType, "heap", strlen("heap")+1);
  info.fileDesc = fd;
  // printf(">> fd %d, %d\n",fd, info.fileDesc);
  
  CALL_BF_NUM(BF_CreateFile(fileName));
  CALL_BF_NUM(BF_OpenFile(fileName, &fd));
  
  CALL_BF_NUM(BF_AllocateBlock(fd, block));  // Δημιουργία καινούριου block
  data = BF_Block_GetData(block);  
  memcpy(data, &info, 5+sizeof(fd));

  BF_Block_SetDirty(block);
  CALL_BF_NUM(BF_UnpinBlock(block));

  CALL_BF_NUM(BF_CloseFile(fd)); //Κλείσιμο αρχείου και αποδέσμευση μνήμης
  // CALL_BF_NUM(BF_Close());
  
  // BF_Block_Destroy(&block); 
  
  return 0;
}

HP_info* HP_OpenFile(char *fileName){
  int fd;
  void* data;
  BF_Block *block;

  BF_Block_Init(&block);
  BF_OpenFile(fileName, &fd);

  //---------------------------//

  int blocks_num;
  BF_GetBlockCounter(fd, &blocks_num);
  printf("Number of blocks: %d\n", blocks_num); //εμφανίζει πόσα μπλοκ έχουμε

  BF_GetBlock(fd, blocks_num-1, block); // απλα για να μην εχουμε το 0

  data = BF_Block_GetData(block);
  HP_info *info = data;
  printf("Type of file:\t%s \n", info->fileType );
  printf("file descriptor: %d\n",info->fileDesc);

  if(strcmp(info->fileType, "heap") ==0 )
    {
      printf("It's a heap!!!!\n");
      return info;
    }
  printf("OOOOPS!!!!\n");
  return NULL ;
}


int HP_CloseFile( HP_info* hp_info ){

}

int HP_InsertEntry(HP_info* hp_info, Record record){
    return 0;
}

int HP_GetAllEntries(HP_info* hp_info, int value){
   return 0;
}

