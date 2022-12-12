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
  info.numOfRecords = (BF_BLOCK_SIZE - sizeof(HP_block_info))/sizeof(Record) ;
  // printf(">> fd %d, %d\n",fd, info.fileDesc);
  // printf("num of records = %d\n", info.numOfRecords);

  CALL_BF_NUM(BF_CreateFile(fileName));
  CALL_BF_NUM(BF_OpenFile(fileName, &fd));
  
  CALL_BF_NUM(BF_AllocateBlock(fd, block));  // Δημιουργία καινούριου block
  data = BF_Block_GetData(block); 
  memcpy(data, &info, 5+sizeof(fd));
  info.lastBlockID = 0;



  BF_Block_SetDirty(block);
  CALL_BF_NUM(BF_UnpinBlock(block));

  CALL_BF_NUM(BF_CloseFile(fd)); //Κλείσιμο αρχείου και αποδέσμευση μνήμης
  BF_Block_Destroy(&block); 
  
  return 0;
}

HP_info* HP_OpenFile(char *fileName){
  int fd;
  void* data;
  BF_Block *block;

  BF_Block_Init(&block);
  //printf("Open the file\n");
  BF_OpenFile(fileName, &fd);
  //printf("Get Block\n");
  if(BF_GetBlock(fd, 0, block) != BF_OK) // λογικα εδω παίρνει το 1ο block
    printf("AAAAA \n");
  //printf("get data\n");
  data = BF_Block_GetData(block);  
  HP_info *info=data;
  //printf("type %s\n", info->fileType);
  printf("fd %d\n",info->fileDesc);
  BF_Block_Destroy(&block);
  if(strcmp(info->fileType, "heap") ==0 )
    {
      printf("It's a heap!!!!\n");
      return info;
    }
  printf("OOOOPS!!!!\n");
  return NULL ;
}


int HP_CloseFile( HP_info* hp_info ){
  int fd=hp_info->fileDesc;
  printf("fd = %d", fd);
  CALL_BF_NUM(BF_CloseFile(fd));
  //μηπως εδω θελει το BF_close??
  // CALL_BF_NUM(BF_Close(fd));
}

int HP_InsertEntry(HP_info* hp_info, Record record){

  BF_Block *block;
  BF_Block_Init(&block);

  void* data;
  int id_of_last_block = hp_info->lastBlockID;
  int fd = hp_info->fileDesc;
  HP_block_info block_info;

  if(id_of_last_block == 0){
    BF_AllocateBlock(fd, block);
    hp_info->lastBlockID++;
    block_info.numOfRecords = 1;
    block_info.nextBlock = NULL;

    data = BF_Block_GetData(block);

    memcpy(data+(512-(512-sizeof(HP_block_info))), &block_info, sizeof(HP_block_info));

    Record* rec = data;
    rec[0] = record;
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
    return hp_info->lastBlockID;
  }

  BF_GetBlock(fd, id_of_last_block, block);
  data = BF_Block_GetData(block); 

  memcpy(&block_info, data+(512-(512-sizeof(HP_block_info))), sizeof(HP_block_info));

  if(block_info.numOfRecords != hp_info->numOfRecords){ //exoyme xwro
    Record* rec = data;
    rec[block_info.numOfRecords++] = record;
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
    return hp_info->lastBlockID;
  }
  else{ // den exoyme
    BF_AllocateBlock(fd, block);
    block_info.nextBlock = block;
    hp_info->lastBlockID++;

    HP_block_info new_block_info;
    new_block_info.numOfRecords = 1;
    new_block_info.nextBlock = NULL;

    void* new_data;
    data = BF_Block_GetData(block);

    memcpy(new_data+(512-(512-sizeof(HP_block_info))), &new_block_info, sizeof(HP_block_info));

    Record* rec = new_data;
    rec[0] = record;
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
    return hp_info->lastBlockID;
  }

  return -1;
}

int HP_GetAllEntries(HP_info* hp_info, int value){
  /*printf("Contents of Block %d\n\t",i);
    CALL_OR_DIE(BF_GetBlock(fd1, i, block));
    data = BF_Block_GetData(block);
    Record* rec= data;
    printRecord(rec[0]);
    printf("\t");
    printRecord(rec[1]);
    CALL_OR_DIE(BF_UnpinBlock(block));*/
   return 0;
}

