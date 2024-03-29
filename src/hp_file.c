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

  //allocate the memory for the struct HP_info
  HP_info *info=malloc(sizeof(HP_info));
  if (info == NULL) //if the memory was not allocated successfully
  {
    printf("Couldn't allocate the memory for HP_info struct!\n");
    return NULL; //return NULL
  }

  memcpy(info, data, sizeof(HP_info)); //copy the data at the struct
  info->fileDesc = fd; //write at the struct the file descriptor that is being used

  //set dirty the block and unpin it from the memory
  BF_Block_SetDirty(block);
  CALL_BF_NULL(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  if(strcmp(info->fileType, "heap") ==0 ) //if the file is heap type 
    return info; //return the struct info

  printf("Wrong type of file!\n");
  printf("The type should be heap, instead of %s!\n", info->fileType);
  return NULL ; //else return NULL
}


int HP_CloseFile( HP_info* hp_info ){
  int fd=hp_info->fileDesc; //find the file descriptor of the file
  free(hp_info); //free the memory
  CALL_BF_NUM(BF_CloseFile(fd)); //and close it
}

int HP_InsertEntry(HP_info* hp_info, Record record){  
  BF_Block *block;
  BF_Block_Init(&block);

  void* data;
  HP_block_info block_info;

  int id_of_last_block = hp_info->lastBlockID;
  int fd = hp_info->fileDesc;

  //if the block 0 is the last block then allocate a new
  //block and write there the record
  if(id_of_last_block == 0){
    CALL_BF_NUM(BF_AllocateBlock(fd, block)); 
    data = BF_Block_GetData(block); 
   
    Record* rec = data;
    rec[0] = record;
    
    block_info.numOfRecords = 1;
    block_info.nextBlock = NULL;
    memcpy(data+512-sizeof(HP_block_info), &block_info, sizeof(HP_block_info));
    BF_Block_SetDirty(block);
    CALL_BF_NUM(BF_UnpinBlock(block));

    hp_info->lastBlockID++; 

    BF_Block_Destroy(&block);

    printf("At the block %d, was inserted the first record: ",hp_info->lastBlockID);
    printRecord(record);

    return hp_info->lastBlockID;
  }

  CALL_BF_NUM(BF_GetBlock(fd, id_of_last_block, block));
  data = BF_Block_GetData(block); 

  memcpy(&block_info, data+(512-sizeof(HP_block_info)), sizeof(HP_block_info)); 
                             
  //if the block has empty space then write the record at the block
  if(block_info.numOfRecords < hp_info->capacityOfRecords){ 
    Record* rec = data;
    rec[block_info.numOfRecords++] = record;
    memcpy(data+512-sizeof(HP_block_info), &block_info, sizeof(HP_block_info)); 
    BF_Block_SetDirty(block);
    CALL_BF_NUM(BF_UnpinBlock(block));

    BF_Block_Destroy(&block); 

    printf("At block %d, was inserted the record: ",hp_info->lastBlockID);
    printRecord(record);

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

    printf("There was no space at block %d, so at block %d was inserted the record: ",id_of_last_block, hp_info->lastBlockID);
    printRecord(record);

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

  printf("Find all the records with id: %d\n", value);
  
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
        printf("At the block %d, was found the record: ",i);
        printRecord(rec[record]); //print the record
      }
    }
    CALL_BF_NUM(BF_UnpinBlock(block));
  }
  BF_Block_Destroy(&block);
  return last_record_block; //return the block you read
}