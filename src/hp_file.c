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
  
  info.numOfRecords = (BF_BLOCK_SIZE - sizeof(HP_block_info))/sizeof(Record) ;
  printf("create info.numOfRecords-- %d\n",info.numOfRecords);
  info.lastBlockID = 0;
  // printf(">> fd %d, %d\n",fd, info.fileDesc);
  // printf("num of records = %d\n", info.numOfRecords);

  CALL_BF_NUM(BF_CreateFile(fileName));
  CALL_BF_NUM(BF_OpenFile(fileName, &fd));
  info.fileDesc = fd;
  printf("create fd-- %d\n",fd);

  CALL_BF_NUM(BF_AllocateBlock(fd, block));  // Δημιουργία καινούριου block
  data = BF_Block_GetData(block); 
  memcpy(data, &info, sizeof(HP_info));

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
  BF_ErrorCode code=BF_OpenFile(fileName, &fd);
  printf("open fd-- %d\n",fd);

  BF_PrintError(code);
  //printf("Get Block\n");
  if(BF_GetBlock(fd, 0, block) != BF_OK) // λογικα εδω παίρνει το 1ο block
    printf("AAAAA \n");
  //printf("get data\n");
  data = BF_Block_GetData(block);  
  HP_info *info=data;
  //printf("type %s\n", info->fileType);
  //printf("open fd-- %d\n",info->fileDesc);
  BF_UnpinBlock(block);
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

  printf("insert hp-infoss> fd--%d type--%s lID--%d records--%d\n", hp_info->fileDesc,hp_info->fileType,hp_info->lastBlockID,hp_info->numOfRecords);
  BF_Block *block;
  BF_Block_Init(&block);

  void* data;
  int id_of_last_block = hp_info->lastBlockID;
  int fd = hp_info->fileDesc;
  //printf("insert fd-- %d\n",fd);

  HP_block_info block_info;

  if(id_of_last_block == 0){
    printf("mphke edv giati exoyme to 0 block\n"); 

    BF_ErrorCode code=BF_AllocateBlock(fd, block);
    BF_PrintError(code); 
    
    // hp_info->lastBlockID++;
   // block_info.numOfRecords = 1;
   // block_info.nextBlock = NULL;

    data = BF_Block_GetData(block);
   
    //printf("meta get data\n");
    Record* rec = data;
    //printf("--rec 1\n");
    rec[0] = record;
    //printf("---rec 1\n");
    printRecord(rec[0]);
    // printf("after puting rec 1\n");
    // printf("prin memcopy\n");
    // printf("sizeof(HP_block_info)--%ld\n",sizeof(HP_block_info));
    // printf("sizeof(block_info)--%ld\n",sizeof(block_info));

    hp_info->lastBlockID++;
    block_info.numOfRecords = 1;
    block_info.nextBlock = NULL;

    int offset=512-sizeof(HP_block_info);
    memcpy(data+offset, &block_info, sizeof(block_info));
    printf("meta memcopy\n");
    //Record* rec = data;
    //rec[0] = record;
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
    return hp_info->lastBlockID;
  }

  BF_GetBlock(fd, id_of_last_block, block);
  data = BF_Block_GetData(block); 

  memcpy(&block_info, data+(512-sizeof(HP_block_info)), sizeof(HP_block_info));

  if(block_info.numOfRecords != hp_info->numOfRecords){ //exoyme xwro
    printf("1.block_info.numOfRecords -- %d\n",block_info.numOfRecords );
    printf("1.hp_info->numOfRecords -- %d\n",hp_info->numOfRecords );
    Record* rec = data;
    rec[block_info.numOfRecords++] = record;
    int offset=512-sizeof(HP_block_info);
    printf("2.block_info.numOfRecords -- %d\n",block_info.numOfRecords );
    memcpy(data+offset, &block_info, sizeof(block_info));
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
    return hp_info->lastBlockID;
  }  
  else{ // den exoyme
    printf("hello else\n");
    BF_AllocateBlock(fd, block);
    block_info.nextBlock = block;
    hp_info->lastBlockID++;

    HP_block_info new_block_info;
    new_block_info.numOfRecords=1;
    new_block_info.nextBlock = NULL;

    //void* new_data;
    data = BF_Block_GetData(block);

    memcpy(data+(512-sizeof(HP_block_info)), &new_block_info, sizeof(HP_block_info));

    Record* rec = data;
    rec[0] = record;
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
    return hp_info->lastBlockID; 
  }//lala
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

    
  //while(block!=NULL)
  //{
    //get data toy block
    //diabasma eggrafwn //fooorrr i=0;i<block_info.numOfRecords
    //rec[i].id==id
    //
    //kai ellegxow id
    //kai print
    //block=block_info.next_block
  //}

   return 0;
}

