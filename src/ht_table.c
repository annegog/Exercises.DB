#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "ht_table.h"
#include "record.h"

//error function that returns -1
#define CALL_BF_NUM(call)     \
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

int HT_CreateFile(char *fileName,  int buckets){
  BF_Block *block;
  BF_Block_Init(&block);

  HT_info info;
  void* data;
  int fd;
  
  //create and open the file
  CALL_BF_NUM(BF_CreateFile(fileName));
  CALL_BF_NUM(BF_OpenFile(fileName, &fd));

  memcpy(info.fileType, "hash", strlen("hash")+1); //write at the struct the type of file 
  info.capacityOfRecords = (BF_BLOCK_SIZE - sizeof(HT_block_info))/sizeof(Record); //the number of records a block can hold
  info.numBuckets=buckets; //the number of buckets
  
  //allocate the first block of the file
  CALL_BF_NUM(BF_AllocateBlock(fd, block));  
  data = BF_Block_GetData(block); 
  memcpy(data, &info, sizeof(HT_info)); //and copy the struct at the data of the block

  //set dirty the block and unpin it from the memory
  BF_Block_SetDirty(block);
  CALL_BF_NUM(BF_UnpinBlock(block));
  
  //close the file
  CALL_BF_NUM(BF_CloseFile(fd)); 
  BF_Block_Destroy(&block);

  return 0;
}

HT_info* HT_OpenFile(char *fileName){
  BF_Block *block;
  BF_Block_Init(&block);

  int fd;
  void* data;
  
  CALL_BF_NULL(BF_OpenFile(fileName, &fd)); //open file
  CALL_BF_NULL(BF_GetBlock(fd, 0, block)); 
  data = BF_Block_GetData(block); //get the data of the fisrt block
  
  // HT_info *info=data;
  HT_info *info = malloc(sizeof(HT_info));
  memcpy( info, data, sizeof(HT_info)); 

  info->fileDesc = fd;  //write at the struct the file descriptor that is being used
  //and allocate the memory for the hash table
  info->hashTable=(int **)malloc(info->numBuckets*sizeof(int *));
  for (int i=0; i<info->numBuckets; i++)
    info->hashTable[i]=malloc(2*sizeof(int));
    //??mhpws na elegxoyme an ontws gientai to malloc

  info->occupiedPosInHT=0; //initialize the occupied positions to zero
  info->sizeOfHT=info->numBuckets; //initiliaze the size of hash table so far
 
  //set dirty the block and unpin it from the memory
  BF_Block_SetDirty(block);
  CALL_BF_NULL(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  if(strcmp(info->fileType, "hash") ==0 ) //if the file is hash type 
    return info; //return the struct info
  return NULL; //else return NULL
}


int HT_CloseFile( HT_info* ht_info ){
  BF_Block *block;
  BF_Block_Init(&block);
  void* data;
  int fd=ht_info->fileDesc; //find the file descriptor of the file

  BF_GetBlock(fd, 0, block);
  data = BF_Block_GetData(block); 

  memcpy(data, ht_info, sizeof(HT_info)); //and copy the struct at the data of the block
  BF_Block_SetDirty(block);
  CALL_BF_NUM(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  //free the allocated memory of the hash table
  for (int i=0; i<ht_info->sizeOfHT; i++)
  {
    free(ht_info->hashTable[i]);
  }
  free(ht_info->hashTable);
  free(ht_info);

  CALL_BF_NUM(BF_CloseFile(fd)); //and close the file
  return 0;
}

int HT_InsertEntry(HT_info* ht_info, Record record){
  BF_Block *block;
  BF_Block_Init(&block);

  void* data;
  HT_block_info block_info;

  int fd=ht_info->fileDesc;
  long int numBuckets = ht_info->numBuckets;
  int bucket = record.id%numBuckets; //the bucket that the record must be written
  int blockId; 
  int check=0;

  //if we have reached the maximum size of the hash table
  //then reallocate the memory by adding "numBuckets" more positions
  if(ht_info->occupiedPosInHT==ht_info->sizeOfHT)
  {
    int new_size=ht_info->sizeOfHT+ht_info->numBuckets;
    int old_size=ht_info->sizeOfHT;
    ht_info->hashTable = realloc(ht_info->hashTable, new_size*sizeof(int *));
    for (int i=0; i<ht_info->numBuckets; i++)
    {
      ht_info->hashTable[old_size+i]=malloc(2*sizeof(int));
    }
    ht_info->sizeOfHT=new_size;
  }

  //find the last block that the specific bucket has
  for (int i=0; i<ht_info->occupiedPosInHT; i++)
  {
    if (ht_info->hashTable[i][0]==bucket)
    {
      check=1;
      blockId=ht_info->hashTable[i][1];
    }
  }

  //if the flag "check" is 0 that means that the specific bucket has no blocks
  //so we must allocate a block and update properly the has table
  if (check==0)
  {    
    CALL_BF_NUM(BF_AllocateBlock(fd, block));
    data = BF_Block_GetData(block);
   
    Record* rec = data;
    rec[0] = record;
    
    block_info.numOfRecords = 1;
    block_info.previousBlockId = 0;
    memcpy(data+512-sizeof(HT_block_info), &block_info, sizeof(HT_block_info));
    BF_Block_SetDirty(block);
    CALL_BF_NUM(BF_UnpinBlock(block));

    ht_info->lastBlockID++;
    ht_info->hashTable[ht_info->occupiedPosInHT][0] = bucket;
    ht_info->hashTable[ht_info->occupiedPosInHT][1] = ht_info->lastBlockID;
    ht_info->occupiedPosInHT++;

    BF_Block_Destroy(&block);

    return ht_info->lastBlockID;
  }

  //else get the data of the block
  CALL_BF_NUM(BF_GetBlock(fd, blockId, block));
  data = BF_Block_GetData(block); 

  memcpy(&block_info, data+512-sizeof(HT_block_info), sizeof(HT_block_info));

  //if the block has empty space then write the record at the block
  if(block_info.numOfRecords < ht_info->capacityOfRecords){
    Record* rec = data;
    rec[block_info.numOfRecords++] = record; 
    memcpy(data+512-sizeof(HT_block_info), &block_info, sizeof(HT_block_info)); 
    BF_Block_SetDirty(block);
    CALL_BF_NUM(BF_UnpinBlock(block));

    BF_Block_Destroy(&block);

    return blockId;
  }
  //else allocate a new block and write the record at the new block 
  //also update the hash table
  else{
    BF_Block *new_block;
    HT_block_info new_block_info;
    void* new_data;

    BF_Block_Init(&new_block);
    CALL_BF_NUM(BF_AllocateBlock(fd, new_block)); 
    new_data = BF_Block_GetData(new_block);

    Record* rec = new_data;
    rec[0] = record;

    new_block_info.numOfRecords=1;
    new_block_info.previousBlockId = blockId; 
    memcpy(new_data+(512-sizeof(HT_block_info)), &new_block_info, sizeof(HT_block_info));

    BF_Block_SetDirty(new_block);
    CALL_BF_NUM(BF_UnpinBlock(new_block));

    memcpy(data+512-sizeof(HT_block_info), &block_info, sizeof(HT_block_info));
    BF_Block_SetDirty(block);
    CALL_BF_NUM(BF_UnpinBlock(block));

    ht_info->lastBlockID++;
    ht_info->hashTable[ht_info->occupiedPosInHT][0] = bucket;
    ht_info->hashTable[ht_info->occupiedPosInHT][1] = ht_info->lastBlockID;
    ht_info->occupiedPosInHT++;

    BF_Block_Destroy(&block);
    BF_Block_Destroy(&new_block);

    return ht_info->lastBlockID;
  }
}

int HT_GetAllEntries(HT_info* ht_info, int value ){
  BF_Block *block;
  BF_Block_Init(&block);

  int fd = ht_info->fileDesc;  
  void* data;
  int block_num;
  long int numBuckets = ht_info->numBuckets; 
  int bucket = value%numBuckets; //get the bucket that the record is placed
  
  //search the hash (from bottom to top) table and find the last block the bucket has
  //printf("1.occ- %d\n2.siz- %d\n3.Buckets- %d\n", ht_info->occupiedPosInHT, ht_info->sizeOfHT, ht_info->numBuckets);
  for (int i=ht_info->occupiedPosInHT-1; i>=0; i--){
    if(ht_info->hashTable[i][0]==bucket){
      block_num=ht_info->hashTable[i][1];
      break;
    }
  }

  HT_block_info *block_info;
  int block_counter=0;
  

  do{
    CALL_BF_NUM(BF_GetBlock(fd,block_num,block));
    data = BF_Block_GetData(block);

    Record* rec = data;    
    block_info = data+(512-sizeof(HT_block_info));

    //check every record in the block 
    for (int record=0; record < block_info->numOfRecords; record++){
      if(rec[record].id == value){ //if you find the record with the specific value
        printRecord(rec[record]); //print the record
      }
    }
    block_counter++; //count the blocks we have read
    CALL_BF_NUM(BF_UnpinBlock(block));

  } while(( block_num = block_info->previousBlockId ) != 0);
  //get the previous block of the bucket and check again
  
  BF_Block_Destroy(&block);
  return block_counter; //return the block you read
}

int HashStatistics(char* filename /* όνομα του αρχείου που ενδιαφέρει */ ){
  BF_Block *block;
  BF_Block_Init(&block);

  //HT_info* ht_info = HT_OpenFile(filename);

  int fd;
  void* ht_info_data;
  
  CALL_BF_NUM(BF_OpenFile(filename, &fd)); //open file
  CALL_BF_NUM(BF_GetBlock(fd, 0, block)); 
  ht_info_data = BF_Block_GetData(block); //get the data of the fisrt block
  
  // HT_info *ht_info = malloc(sizeof(HT_info));
  // memcpy( ht_info, data, sizeof(HT_info)); 
  HT_info *ht_info = ht_info_data;
  //εδω σκεφτηκα οτι μπορουμε κατευθειαν να δειξουμε στα data αφου δεν θα τα πειραξουμε και δεν
  //θα κλεισουμε το block -οποτε δεν χρειαζομαστε το μαλλοκ

  printf("fileee type %s\n", ht_info->fileType);
  if(strcmp(ht_info->fileType, "hash") != 0 ) //if the file is hash type 
    return -1;
  printf("SKATAAAAAAAAAAAAAA 2\n");

  //int fd = ht_info->fileDesc;
  printf("1 ht_info->numBuckets %d\n", ht_info->numBuckets);
  printf("2 ht_info->capacityOfRecords %d\n", ht_info->capacityOfRecords);
  printf("3 ht_info->sizeOfHT %d\n", ht_info->sizeOfHT);
  printf("4 ht_info->occupiedPosInHT %d\n", ht_info->occupiedPosInHT);
  printf("5 ht_info->fileDesc %d\n", ht_info->fileDesc);
  printf("6 ht_info->fileType %s\n", ht_info->fileType);
  printf("7 ht_info->lastBlockID %d\n", ht_info->lastBlockID);
  //printf("8 ht_info->occupiedPosInHT %d\n", ht_info->);


  HT_block_info *block_info;
  int blockID;
  void* data;
  
  printf("Number of blocks in file %s: %d\n", filename, ht_info->occupiedPosInHT);
 
  int recordsOfBuckets[ht_info->numBuckets];
  int blocksOfBuckets[ht_info->numBuckets];
  for (int i=0; i<ht_info->numBuckets; i++){
    recordsOfBuckets[i] = 0;
    blocksOfBuckets[i] = 0;
  }

  for (int i=0; i<ht_info->occupiedPosInHT; i++){
    blockID = ht_info->hashTable[i][1];
    
    CALL_BF_NUM(BF_GetBlock(fd,blockID,block));
    data = BF_Block_GetData(block);
    printf("faaa!!\n");
    memcpy(&block_info, data+512-sizeof(HT_block_info), sizeof(HT_block_info));
    printf("faaa %d\n", block_info->numOfRecords);

    recordsOfBuckets[ht_info->hashTable[i][0]] += block_info->numOfRecords;
    blocksOfBuckets[ht_info->hashTable[i][0]]++;
  }

  int min = 1000;
  int max = 0;
  int total_records = 0;
  int total_blocks = 0;
  for (int i=0; i<ht_info->numBuckets; i++){
    if(recordsOfBuckets[i] < min)
      min = recordsOfBuckets[i];

    if(recordsOfBuckets[i] > max)
      max = recordsOfBuckets[i];
    
    total_records += recordsOfBuckets[i];
    total_blocks += blocksOfBuckets[i];
  }
  printf("Minimum records are: %d\n", min);
  printf("Maximum records are: %d\n", max);
  printf("Mean records are: %ld\n", total_records/ht_info->numBuckets);
  printf("Mean blocks are: %ld\n", total_blocks/ht_info->numBuckets);

  int overflowedBuckets = 0;
  for(int i=0; i<ht_info->numBuckets; i++){
    if(blocksOfBuckets[i] > 1){
      printf("The bucket %d has %d overflowed blocks\n", i, blocksOfBuckets[i]-1);
      overflowedBuckets++;
    }
  }
  printf("The buckets with overflowed blockes are: %d\n", overflowedBuckets);
  
  CALL_BF_NUM(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  //HT_CloseFile(ht_info);
  CALL_BF_NUM(BF_CloseFile(fd));
  return 0;
}