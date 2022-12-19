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

/////// !!!!!!!!!!!!!!! ////////
//ΝΑ ΦΤΙΑΞΟΥΜΕ ΤΑ DEFINE//

// #define CALL_BF_NULL(call)       \
// {                           \
//   BF_ErrorCode code = call; \
//   if (code != BF_OK) {         \
//     BF_PrintError(code);    \
//     return NULL;        \
//   }                         \
// }

int HP_CreateFile(char *fileName){
  BF_Block *block;
  BF_Block_Init(&block);

  HP_info info;
  void* data;
  int fd;

  CALL_BF_NUM(BF_CreateFile(fileName));
  CALL_BF_NUM(BF_OpenFile(fileName, &fd));

  memcpy(info.fileType, "heap", strlen("heap")+1);
  info.numOfRecords = (BF_BLOCK_SIZE - sizeof(HP_block_info))/sizeof(Record);
  info.lastBlockID = 0;
  info.fileDesc = fd;

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
  BF_Block *block;
  BF_Block_Init(&block);

  int fd;
  void* data;
  
  BF_OpenFile(fileName, &fd);
  BF_GetBlock(fd, 0, block); // λογικα εδω παίρνει το 1ο block
  data = BF_Block_GetData(block);  

  HP_info *info=data;
  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);

  if(strcmp(info->fileType, "heap") ==0 )
    return info;
  return NULL ;
}


int HP_CloseFile( HP_info* hp_info ){
  int fd=hp_info->fileDesc;
  CALL_BF_NUM(BF_CloseFile(fd));
}

int HP_InsertEntry(HP_info* hp_info, Record record){

  // printf("insert hp-infoss >> \n fd--%d\n type--%s\n lID--%d\n records--%d\n\n", hp_info->fileDesc,hp_info->fileType,hp_info->lastBlockID,hp_info->numOfRecords);
  
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
    // printRecord(rec[0]);
    
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
                            // θα τα κανουμε αυτα offset? αλλιως να σβησουμε την μεταβλητη  

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
    memcpy(data+(512-sizeof(HP_block_info)), &block_info, sizeof(HP_block_info));
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);

    hp_info->lastBlockID++; //αυξησε κατα 1 id του τελευταιου μπλοκ
   
    BF_Block_Destroy(&block);
    BF_Block_Destroy(&new_block);

    return hp_info->lastBlockID; 
  }
}

int HP_GetAllEntries(HP_info* hp_info, int value){

  BF_Block *block;
  BF_Block_Init(&block);

  int fd = hp_info->fileDesc;  
  void* data;
  
  int block_num;
  int last_record_block = 0;
  BF_GetBlockCounter(fd, &block_num);
  // printf("Blocks = %d\n", block_num);
    
  for(int i=1; i<block_num; i++){
    // printf("\nBlock: %d\t", i);
  
    BF_GetBlock(fd,i,block);
    data = BF_Block_GetData(block);
      
    Record* rec = data;

    // memcpy(&block_info, data+(512-sizeof(HP_block_info)), sizeof(HP_block_info));
    HP_block_info *block_info = data+(512-sizeof(HP_block_info));
    // printf("num of records: %d\n", block_info->numOfRecords);

    for(int record=0; record<block_info->numOfRecords; record++){   
      // printf("record = %d \tid: %d\n",record, rec[record].id);
      // printRecord(rec[record]);

      if(rec[record].id == value){
        // printf("\tfound it\n");
        last_record_block = i;
        printRecord(rec[record]);
      }
    }
    BF_UnpinBlock(block);
    
  }
  BF_Block_Destroy(&block);
  return last_record_block;
}