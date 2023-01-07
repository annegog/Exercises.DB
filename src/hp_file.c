#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

//error function that returns -1
#define CALL_BF_NUM(call)   \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return -1;              \
  }                         \
}
//error function that returns NULL
#define CALL_BF_NULL(call)  \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return NULL;            \
  }                         \
}

int HP_CreateFile(char *fileName){
  BF_Block *block;
  BF_Block_Init(&block);

  HP_info info;
  void* data;
  int fd;
  //create and open the file
  CALL_BF_NUM(BF_CreateFile(fileName));
  CALL_BF_NUM(BF_OpenFile(fileName, &fd));

  memcpy(info.fileType, "heap", strlen("heap")+1); //write at the struct the type of file 
  info.capacityOfRecords = (BF_BLOCK_SIZE - sizeof(HP_block_info))/sizeof(Record); //the number of records a block can hold
  info.lastBlockID = 0; //and the last block of the file

  //allocate the first block of the file
  CALL_BF_NUM(BF_AllocateBlock(fd, block));  
  data = BF_Block_GetData(block); 
  memcpy(data, &info, sizeof(HP_info)); //and copy the struct at the data of the block

  //set dirty the block and unpin it from the memory
  BF_Block_SetDirty(block);
  CALL_BF_NUM(BF_UnpinBlock(block));

  //close the file
  CALL_BF_NUM(BF_CloseFile(fd)); 
  BF_Block_Destroy(&block); 
  
  return 0;
}

HP_info* HP_OpenFile(char *fileName){
  BF_Block *block;
  BF_Block_Init(&block);

  int fd;
  void* data;
  
  CALL_BF_NULL(BF_OpenFile(fileName, &fd)); //open file
  CALL_BF_NULL(BF_GetBlock(fd, 0, block)); 
  data = BF_Block_GetData(block); //get the data of the fisrt block

  HP_info *info=data;
  info->fileDesc = fd; //write at the struct the file descriptor that is being used

  //set dirty the block and unpin it from the memory
  BF_Block_SetDirty(block);
  CALL_BF_NULL(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  if(strcmp(info->fileType, "heap") ==0 ) //if the file is heap type 
    return info; //return the struct info
  return NULL ; //else return NULL
}


int HP_CloseFile( HP_info* hp_info ){
  int fd=hp_info->fileDesc; //find the file descriptor of the file
  CALL_BF_NUM(BF_CloseFile(fd)); //and close it
}

int HP_InsertEntry(HP_info* hp_info, Record record){  
  BF_Block *block;
  BF_Block_Init(&block);

  void* data;
  HP_block_info block_info;

  int id_of_last_block = hp_info->lastBlockID;
  int fd = hp_info->fileDesc;

  int offset=512-sizeof(HP_block_info);

  //if the block 0 is the last block then allocate a new
  //block and write there the record
  if(id_of_last_block == 0){
    CALL_BF_NUM(BF_AllocateBlock(fd, block)); 
    data = BF_Block_GetData(block); 
   
    Record* rec = data;
    rec[0] = record;
    
    block_info.numOfRecords = 1;
    block_info.nextBlock = NULL;
    memcpy(data+offset, &block_info, sizeof(HP_block_info));
    BF_Block_SetDirty(block);
    CALL_BF_NUM(BF_UnpinBlock(block));

    hp_info->lastBlockID++; 

    BF_Block_Destroy(&block);

    return hp_info->lastBlockID;
  }

  CALL_BF_NUM(BF_GetBlock(fd, id_of_last_block, block));
  data = BF_Block_GetData(block); 

  memcpy(&block_info, data+(512-sizeof(HP_block_info)), sizeof(HP_block_info)); 
                            // θα τα κανουμε αυτα offset? αλλιως να σβησουμε την μεταβλητη  

  if(block_info.numOfRecords < hp_info->capacityOfRecords){ //αν υπάρχει χώρος στο μπλοκ
    Record* rec = data;
    rec[block_info.numOfRecords++] = record; //βαλε στην ασχη του μλποκ το record
    memcpy(data+offset, &block_info, sizeof(HP_block_info)); // και στο τελος ενημερωσε το bock info
    BF_Block_SetDirty(block);
    CALL_BF_NUM(BF_UnpinBlock(block));

    BF_Block_Destroy(&block); 

    return hp_info->lastBlockID;
  } 
  //allocate a new block and write the record at the new block 
  else{ 
    BF_Block *new_block;
    HP_block_info new_block_info;
    void* new_data;

    BF_Block_Init(&new_block);
    CALL_BF_NUM(BF_AllocateBlock(fd, new_block)); 
    new_data = BF_Block_GetData(new_block);

    Record* rec = new_data;
    rec[0] = record;

    new_block_info.numOfRecords=1;
    new_block_info.nextBlock = NULL;
    memcpy(new_data+(512-sizeof(HP_block_info)), &new_block_info, sizeof(HP_block_info));

    BF_Block_SetDirty(new_block);
    CALL_BF_NUM(BF_UnpinBlock(new_block));

    block_info.nextBlock = new_block;
    memcpy(data+(512-sizeof(HP_block_info)), &block_info, sizeof(HP_block_info));
    BF_Block_SetDirty(block);
    CALL_BF_NUM(BF_UnpinBlock(block));

    hp_info->lastBlockID++;
   
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
  CALL_BF_NUM(BF_GetBlockCounter(fd, &block_num));
  
  //for every block at the file except the fisrt one
  for(int i=1; i<block_num; i++){
  
    BF_GetBlock(fd,i,block);
    data = BF_Block_GetData(block);
      
    Record* rec = data;
    HP_block_info *block_info = data+(512-sizeof(HP_block_info));
    //check every record
    for(int record=0; record<block_info->numOfRecords; record++){   
      if(rec[record].id == value){ //if you find the record with the specific value
        last_record_block = i;
        printRecord(rec[record]); //print the record
      }
    }
    CALL_BF_NUM(BF_UnpinBlock(block));
  }
  BF_Block_Destroy(&block);
  return last_record_block; //return the block you read
}