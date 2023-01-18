#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "sht_table.h"
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


int SHT_CreateSecondaryIndex(char *sfileName,  int buckets, char* fileName){
  BF_Block *block;
  BF_Block_Init(&block);

  SHT_info info;
  void* data;
  int fd;

  //create and open the file
  CALL_BF_NUM(BF_CreateFile(sfileName));
  CALL_BF_NUM(BF_OpenFile(sfileName, &fd));

  memcpy(info.fileType, "shash", strlen("shash")+1); //write at the struct the type of file 
  memcpy(info.fileName, fileName, strlen(fileName)+1); //the name of the first file
  info.capacityOfRecords = (BF_BLOCK_SIZE - sizeof(SHT_block_info))/sizeof(SHT_record); //the number of records a block can hold
  info.numBuckets=buckets;//the number of buckets
  info.lastBlockID = 0;

  //allocate the first block of the file
  CALL_BF_NUM(BF_AllocateBlock(fd, block));  
  data = BF_Block_GetData(block); 
  memcpy(data, &info, sizeof(SHT_info)); //and copy the struct at the data of the block

  //set dirty the block and unpin it from the memory
  BF_Block_SetDirty(block);
  CALL_BF_NUM(BF_UnpinBlock(block));

  //close the file
  CALL_BF_NUM(BF_CloseFile(fd)); 
  BF_Block_Destroy(&block);
  return 0;
}

SHT_info* SHT_OpenSecondaryIndex(char *indexName){
  BF_Block *block;
  BF_Block_Init(&block);

  int fd;
  void* data;
  
  CALL_BF_NULL(BF_OpenFile(indexName, &fd)); //open file
  CALL_BF_NULL(BF_GetBlock(fd, 0, block)); 
  data = BF_Block_GetData(block); //get the data of the fisrt block

  //allocate the memory for the struct SHT_info
  SHT_info *info = malloc(sizeof(SHT_info));
  if (info == NULL) //if the memory was not allocated successfully
  {
    printf("Couldn't allocate the memory for SHT_info struct!\n");
    return NULL; //return NULL
  }

  memcpy( info, data, sizeof(SHT_info)); //copy the data at the struct
  info->fileDesc = fd; //write at the struct the file descriptor that is being used

  //and allocate the memory for the secondary hash table
  info->shashTable = (int *)malloc(info->numBuckets*sizeof(int));
  for(int i=-0; i<info->numBuckets; i++)
    info->shashTable[i] = -1;//initialize to -1
  if (info->shashTable == NULL) //if the memory was not allocated successfully
  {
    printf("Couldn't allocate the memory for secondary hash table!\n");
    return NULL; //return NULL
  }
  
  for(int i=-0; i<info->numBuckets; i++)
    info->shashTable[i]=-1;//initialize to -1

  //set dirty the block and unpin it from the memory
  BF_Block_SetDirty(block);
  CALL_BF_NULL(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  if(strcmp(info->fileType, "shash") == 0 ) //if the file is shash type
    return info; //return the struct info

  printf("The type should be shash, instead of %s!\n", info->fileType);
  printf("Wrong type of file!\n");
  return NULL; //else return NULL
}


int SHT_CloseSecondaryIndex( SHT_info* sht_info ){
  BF_Block *block;
  BF_Block_Init(&block);
  void* data;
  int fd = sht_info->fileDesc; //find the file descriptor of the file

  BF_GetBlock(fd, 0, block);
  data = BF_Block_GetData(block);  //get the data of the block 0

  int bytes_of_shash = sht_info->numBuckets*sizeof(int); //calculate the extra bytes of the shash table
  memcpy(data, sht_info, sizeof(SHT_info)+bytes_of_shash); //and copy the struct at the data of the block

  //set dirty the block and unpin it from the memory
  BF_Block_SetDirty(block);
  CALL_BF_NUM(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  //free the allocated memories
  //free(sht_info->shashTable); //!!
  free(sht_info);

  CALL_BF_NUM(BF_CloseFile(fd)); //and close the file
  return 0;
}

int SHT_SecondaryInsertEntry(SHT_info* sht_info, Record record, int block_id){
  BF_Block *block;
  BF_Block_Init(&block);

  void* data;
  SHT_block_info block_info;

  int fd=sht_info->fileDesc; //find the file descriptor of the file
  long int numBuckets = sht_info->numBuckets;  //get the buckets
  
  //get the first letter of the name
  char first_letter = record.name[0];
  int first_num = (int)first_letter;

  //and find the bucket the record must be written
  int bucket = first_num%numBuckets;

  int blockId = sht_info->shashTable[bucket];

  //create the secondary record
  SHT_record current_record;
  current_record.blockID = block_id;
  memcpy(current_record.name, record.name, sizeof(record.name));

  if(blockId==-1) //the bucket has no block
  {
    //allocate one
    //and place the record there
    CALL_BF_NUM(BF_AllocateBlock(fd, block));
    data = BF_Block_GetData(block);

    SHT_record* rec = data;
    rec[0] = current_record;

    block_info.numOfRecords = 1;
    block_info.previousBlockId = -1;  //it's the first block so there is neither previous
    block_info.nextBlockId = -1; //nor next block
    memcpy(data+512-sizeof(SHT_block_info), &block_info, sizeof(SHT_block_info));
    
    //set dirty the block and unpin it from the memory
    BF_Block_SetDirty(block);
    CALL_BF_NUM(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);

    sht_info->shashTable[bucket]= ++(sht_info->lastBlockID); //update the secondary hash table
    
    printf("At the first block with id %d, of bucket %d, was inserted the first (secondary) record: (%s , %d)\n",sht_info->lastBlockID, bucket, current_record.name, current_record.blockID);

    return 0;
  }
  //else
  CALL_BF_NUM(BF_GetBlock(fd, blockId, block));
  data = BF_Block_GetData(block);
  SHT_record *sht_rec=data;

  memcpy(&block_info, data+512-sizeof(SHT_block_info), sizeof(SHT_block_info));
  while(block_info.nextBlockId != -1) //find the last block of the bucket
  {
    for(int i=0; i<block_info.numOfRecords; i++) //check if the record already exists in this block of the bucket
    {
      if(strcmp(sht_rec[i].name, current_record.name) == 0 && sht_rec[i].blockID == current_record.blockID){
        printf("The (secondary) record (%s , %d) already exists!\n", current_record.name, current_record.blockID);
        return 0; //return because the secondary record already exist
      }
    }

    blockId=block_info.nextBlockId; 
    CALL_BF_NUM(BF_UnpinBlock(block));
    CALL_BF_NUM(BF_GetBlock(fd, blockId, block));
    data = BF_Block_GetData(block); 
    sht_rec=data;
    memcpy(&block_info, data+512-sizeof(SHT_block_info), sizeof(SHT_block_info));
  }

  for(int i=0; i<block_info.numOfRecords; i++) //check if the record already exists in the last block of the bucket
  {
    if(strcmp(sht_rec[i].name, current_record.name) == 0 && sht_rec[i].blockID == current_record.blockID){
      printf("The (secondary) record (%s , %d) already exists!\n", current_record.name, current_record.blockID);
      return 0; //return because the secondary record already exist
    }
  }

  //if the block has empty space then write the record at the block
  if(block_info.numOfRecords < sht_info->capacityOfRecords){
    SHT_record* rec = data;
    rec[block_info.numOfRecords++] = current_record; 
    memcpy(data+512-sizeof(SHT_block_info), &block_info, sizeof(SHT_block_info)); 
    
    //set dirty the block and unpin it from the memory
    BF_Block_SetDirty(block);
    CALL_BF_NUM(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);

    printf("At block %d of bucket %d, was inserted the (secondary) record: (%s , %d)\n",blockId, bucket, current_record.name, current_record.blockID);

    return 0;
  }
  //else allocate a new block and write the record at the new block 
  //also update the hash table
  else{
    BF_Block *new_block;
    SHT_block_info new_block_info;
    void* new_data;

    BF_Block_Init(&new_block);
    CALL_BF_NUM(BF_AllocateBlock(fd, new_block)); 
    new_data = BF_Block_GetData(new_block);

    SHT_record* rec = new_data;
    rec[0] = current_record;

    new_block_info.numOfRecords = 1;
    new_block_info.previousBlockId = blockId; 
    new_block_info.nextBlockId = -1;
    memcpy(new_data+(512-sizeof(SHT_block_info)), &new_block_info, sizeof(SHT_block_info));

    //set dirty the new block and unpin it from the memory
    BF_Block_SetDirty(new_block);
    CALL_BF_NUM(BF_UnpinBlock(new_block));

    block_info.nextBlockId = ++(sht_info->lastBlockID); //update the next block of previous block
    memcpy(data+(512-sizeof(SHT_block_info)), &block_info, sizeof(SHT_block_info));
    BF_Block_SetDirty(block);
    CALL_BF_NUM(BF_UnpinBlock(block));

    //set dirty the old block and unpin it from the memory
    BF_Block_Destroy(&block);
    BF_Block_Destroy(&new_block);

    printf("There was no space at block %d, so at block %d of bucket %d, was inserted the (secondary) record: (%s , %d)\n", blockId ,sht_info->lastBlockID, bucket, current_record.name, current_record.blockID);

    return 0;
  }
}

int SHT_SecondaryGetAllEntries(HT_info* ht_info, SHT_info* sht_info, char* name){
  BF_Block *block;
  BF_Block_Init(&block);
  BF_Block *HashBlock;
  BF_Block_Init(&HashBlock);

  void* data;
  void* HashData;
  SHT_block_info* block_info;
  HT_block_info* hash_block_info;

  int fd=sht_info->fileDesc;
  long int numBuckets = sht_info->numBuckets;
  
  //get the key for the hash function
  char first_letter = name[0];
  int first_num = (int)first_letter;

  int bucket = first_num%numBuckets; //get the bucket that the secondary record is placed

  int current_block = sht_info->shashTable[bucket]; //get the first block of the bucket

  int block_counter = 0;
  int HashBlockID = 0;

  printf("Find all the records with name: %s\n", name);

  //for every block at the bucket
  while(current_block != -1){
    CALL_BF_NUM(BF_GetBlock(fd,current_block,block));
    data = BF_Block_GetData(block);

    SHT_record* rec = data;    
    block_info = data+(512-sizeof(SHT_block_info)); 

    //check every record in the block 
    for (int record=0; record < block_info->numOfRecords; record++){
      if(strcmp(rec[record].name, name) == 0){ //if you find the record with the specific value
        printf("At the secondary index, was found the record (%s , %d) at the block %d of bucket %d\n",rec[record].name ,rec[record].blockID, current_block, bucket);
        //get the block id if the primary index
        HashBlockID = rec[record].blockID;
        CALL_BF_NUM(BF_GetBlock(ht_info->fileDesc, HashBlockID, HashBlock));
        HashData = BF_Block_GetData(HashBlock);
        
        Record *HashRecord = HashData;
        hash_block_info = HashData+(512-sizeof(HT_block_info)); 
        //and check every record in the block
        for (int record=0; record < hash_block_info->numOfRecords; record++){
          if(strcmp(HashRecord[record].name, name) == 0){ //if you find the record with the specific value
            printf("At the primary index, at the block %d was found the record", HashBlockID);
            printRecord(HashRecord[record]); //print the record
          }
        }
        block_counter++;
        CALL_BF_NUM(BF_UnpinBlock(HashBlock));
      }
    }
    block_counter++; //count the blocks we have read
    current_block = block_info->nextBlockId;   
    CALL_BF_NUM(BF_UnpinBlock(block));  
  }
  
  BF_Block_Destroy(&HashBlock);
  BF_Block_Destroy(&block);
  return block_counter;
}


int SHT_HashStatistics(char* filename /*όνομα του αρχείου που ενδιαφέρει  */){
  BF_Block *sht_info_block;
  BF_Block_Init(&sht_info_block);

  int fd;
  void* sht_info_data;
  
  CALL_BF_NUM(BF_OpenFile(filename, &fd)); //open file
  CALL_BF_NUM(BF_GetBlock(fd, 0, sht_info_block)); 
  sht_info_data = BF_Block_GetData(sht_info_block); //get the data of the fisrt block

  SHT_info *sht_info = sht_info_data;
 
  if(strcmp(sht_info->fileType, "shash") != 0 ) //if the file is not shash type 
    return -1;

  printf("\nGet the statistics of the file %s!\n", filename);

  int buckets = sht_info->numBuckets;
  int current_block;

  BF_Block *block;
  BF_Block_Init(&block);

  SHT_block_info *block_info;
  void* data;
  
  int recordsOfBuckets[buckets]; //store the records of every bucket
  int blocksOfBuckets[buckets]; //store the blocks of every bucket
  for (int i=0; i<buckets; i++){
    recordsOfBuckets[i] = 0;
    blocksOfBuckets[i] = 0;
  }

  for(int i=0; i<buckets; i++){ //for every bucket
    current_block = sht_info->shashTable[i];

    while(current_block != -1){ //find every block that each bucket has
      CALL_BF_NUM(BF_GetBlock(fd,current_block,block));
      data = BF_Block_GetData(block);

      block_info = data+(512-sizeof(SHT_block_info));
      recordsOfBuckets[i]+=block_info->numOfRecords; //get the number of records for the specific block
      blocksOfBuckets[i]++; //increase the blocks that the bucket has by one
      current_block = block_info->nextBlockId; //go to the next block

      BF_UnpinBlock(block);
    }
  }

  int min_records = 1000;
  int min_bucket;
  int max_records = 0;
  int max_bucket;
  int total_records = 0;
  int total_blocks = 0;

  //find the buckets with max and min records
  for (int i=0; i<buckets; i++){
    if(recordsOfBuckets[i] < min_records)
    {
      min_records = recordsOfBuckets[i];
      min_bucket=i;
    }
    if(recordsOfBuckets[i] > max_records)
    {  
      max_records = recordsOfBuckets[i];
      max_bucket=i;
    }
    total_records += recordsOfBuckets[i];
    total_blocks += blocksOfBuckets[i];
  }
  printf("Minimum records are: %d in bucket %d\n", min_records, min_bucket);
  printf("Maximum records are: %d in bucket %d\n", max_records, max_bucket);
  printf("Mean records are: %d\n", total_records/buckets);
  printf("Mean blocks are: %d\n", total_blocks/buckets);

  int overflowedBuckets = 0;
  //find the overflowed buckets
  for(int i=0; i<buckets; i++){
    printf("%d\n", blocksOfBuckets[i]);
    if(blocksOfBuckets[i] > 1){
      printf("The bucket %d has %d overflowed blocks\n", i, blocksOfBuckets[i]-1);
      overflowedBuckets++;
    }
  }
  printf("There are %d buckets with overflowed blocks\n", overflowedBuckets);
  
  printf("The file: %s has in total %d blocks\n", filename, sht_info->lastBlockID+1); //+1 for block with id 0

  CALL_BF_NUM(BF_UnpinBlock(block));
  CALL_BF_NUM(BF_UnpinBlock(sht_info_block));
  BF_Block_Destroy(&block);
  BF_Block_Destroy(&sht_info_block);

  CALL_BF_NUM(BF_CloseFile(fd));
  return 0;
}
