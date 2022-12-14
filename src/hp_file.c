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
  //printf("fd = %d", fd);
  CALL_BF_NUM(BF_CloseFile(fd));
  //μηπως εδω θελει το BF_close??
  // CALL_BF_NUM(BF_Close(fd));
}
/////////////////////////να βαλουμε και destroy of block
int HP_InsertEntry(HP_info* hp_info, Record record){

  printf("insert hp-infoss >> \n fd--%d\n type--%s\n lID--%d\n records--%d\n\n", hp_info->fileDesc,hp_info->fileType,hp_info->lastBlockID,hp_info->numOfRecords);
  
  BF_Block *block;
  BF_Block_Init(&block);

  void* data;
  HP_block_info block_info;

  int id_of_last_block = hp_info->lastBlockID;
  int fd = hp_info->fileDesc;

  int offset=512-sizeof(HP_block_info);

  if(id_of_last_block == 0){
    BF_AllocateBlock(fd, block); //φτιαξε καινουριο μπλοκ
    data = BF_Block_GetData(block); //και φερτο στην ενδιαεση μνημη
   
    Record* rec = data;
    rec[0] = record;
    //printRecord(rec[0]);
    
    block_info.numOfRecords = 1;
    block_info.nextBlock = NULL;
    memcpy(data+offset, &block_info, sizeof(HP_block_info));
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);

    hp_info->lastBlockID++; //αυξησε κατα 1 id του τελευταιου μπλοκ

    BF_Block_Destroy(&block);

    return hp_info->lastBlockID;
  }

  BF_GetBlock(fd, id_of_last_block, block);
  data = BF_Block_GetData(block); 

  memcpy(&block_info, data+(512-sizeof(HP_block_info)), sizeof(HP_block_info));

  if(block_info.numOfRecords < hp_info->numOfRecords){ //αν υπάρχει χώρος στο μπλοκ
    Record* rec = data;
    rec[block_info.numOfRecords++] = record; //βαλε στην ασχη του μλποκ το record
    memcpy(data+offset, &block_info, sizeof(HP_block_info)); // και στο τελος ενημερωσε το bock info
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);

    BF_Block_Destroy(&block);

    return hp_info->lastBlockID;
  }  
  else{ //αν δεν υπάρχει χώρος στο μπλοκ
    BF_Block *new_block;
    HP_block_info new_block_info;
    void* new_data;

    BF_Block_Init(&new_block);
    BF_AllocateBlock(fd, new_block); //φτιάξε καινουριο μπλοκ
    new_data = BF_Block_GetData(new_block);

    Record* rec = new_data;
    rec[0] = record;

    new_block_info.numOfRecords=1;
    new_block_info.nextBlock = NULL;
    memcpy(new_data+(512-sizeof(HP_block_info)), &new_block_info, sizeof(HP_block_info));

    BF_Block_SetDirty(new_block);
    BF_UnpinBlock(new_block);

    block_info.nextBlock = new_block;
    memcpy(data+(512-sizeof(HP_block_info)), &new_block_info, sizeof(HP_block_info));
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);

    hp_info->lastBlockID++; //αυξησε κατα 1 id του τελευταιου μπλοκ
   
    BF_Block_Destroy(&block);
    BF_Block_Destroy(&new_block);

    return hp_info->lastBlockID; 
  }
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

