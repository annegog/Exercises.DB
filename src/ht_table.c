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

  info.lastBlockID=0;
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
  memcpy(info, data, sizeof(HT_info)); 
  info->fileDesc = fd;  //write at the struct the file descriptor that is being used
  //and allocate the memory for the hash table
  
  info->hashTable=(int *)malloc(info->numBuckets*sizeof(int));
  //??mhpws na elegxoyme an ontws gientai to malloc
  if (info->hashTable == NULL)
  {
    printf("Couldn't allocate the memory for hash table!\n");
    return NULL;
  }
    
  for(int i=-0; i<info->numBuckets; i++)
    info->hashTable[i]=-1;//initialize to -1
  
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

  int bytes_of_hash=ht_info->numBuckets*sizeof(int);
  memcpy(data, ht_info, sizeof(HT_info)+bytes_of_hash); //and copy the struct at the data of the block

  BF_Block_SetDirty(block);
  CALL_BF_NUM(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  //free(ht_info->hashTable); //na to trejw na dv ean doyleyei ayto!!
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
  
  int blockId = ht_info->hashTable[bucket]; 

  if(blockId==-1) //the bucket has no block
  {
    // printf("blockid %d\n", blockId);
    //allocate one
    //and place the record there
    CALL_BF_NUM(BF_AllocateBlock(fd, block));
    data = BF_Block_GetData(block);
    Record* rec = data;
    rec[0] = record;

    block_info.numOfRecords = 1;
    block_info.previousBlockId = -1; //ειναι το πρωτο block για το bucket
    block_info.nextBlockId = -1; //αρα δεν εχει ουτε προηγουμενο ουτε επομενο ακόμα
    memcpy(data+512-sizeof(HT_block_info), &block_info, sizeof(HT_block_info));
    BF_Block_SetDirty(block);
    CALL_BF_NUM(BF_UnpinBlock(block));

    ht_info->hashTable[bucket]= ++(ht_info->lastBlockID);
    // printf("ht_info->hashTable[bucket] %d\n", ht_info->hashTable[bucket]);
    BF_Block_Destroy(&block);

    return ht_info->lastBlockID;
  }
  //else
  // printf("1-blockid %d\n", blockId);
  CALL_BF_NUM(BF_GetBlock(fd, blockId, block));
  data = BF_Block_GetData(block);

  memcpy(&block_info, data+512-sizeof(HT_block_info), sizeof(HT_block_info));
  // printf("block_info.nextBlockId %d\n ",block_info.nextBlockId );
  while(block_info.nextBlockId != -1) //!!-- Μπορουμε να το κανουμε να μοιαζει με το πιο κατω while
  {
    blockId=block_info.nextBlockId;
   // printf("--blockid %d\n", blockId);
    CALL_BF_NUM(BF_GetBlock(fd, blockId, block));
    data = BF_Block_GetData(block); 

    memcpy(&block_info, data+512-sizeof(HT_block_info), sizeof(HT_block_info));
  }


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
    new_block_info.nextBlockId = -1;
    block_info.nextBlockId = ++(ht_info->lastBlockID);
    // printf("WHEN THERE IS NO SPACE -- block_info.nextBlockId %d\n",block_info.nextBlockId);
    memcpy(new_data+(512-sizeof(HT_block_info)), &new_block_info, sizeof(HT_block_info));

    BF_Block_SetDirty(new_block);
    CALL_BF_NUM(BF_UnpinBlock(new_block));

    memcpy(data+512-sizeof(HT_block_info), &block_info, sizeof(HT_block_info));
    BF_Block_SetDirty(block);
    CALL_BF_NUM(BF_UnpinBlock(block));

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
  //int block_num = 0;
  long int numBuckets = ht_info->numBuckets; 
  int bucket = value%numBuckets; //get the bucket that the record is placed

  HT_block_info *block_info;
  int block_counter=0;

  int current_block = ht_info->hashTable[bucket];
  while(current_block != -1){
    printf("current_block = %d\n", current_block);
    CALL_BF_NUM(BF_GetBlock(fd,current_block,block));
    data = BF_Block_GetData(block);

    Record* rec = data;    
    block_info = data+(512-sizeof(HT_block_info)); //no memcopy??

    //check every record in the block 
    for (int record=0; record < block_info->numOfRecords; record++){
      printRecord(rec[record]);
      if(rec[record].id == value){ //if you find the record with the specific value
        printf("\nit's here!");
        printRecord(rec[record]); //print the record
        printf("Blocks until i found youuu: %d\n\n",block_counter+1);
      }
    }
    block_counter++; //count the blocks we have read
    current_block = block_info->nextBlockId; 
    CALL_BF_NUM(BF_UnpinBlock(block));
  }

  BF_Block_Destroy(&block);
  return block_counter;
  //return -1;  // no record here! return -1

}


int HT_HashStatistics(char* filename /*όνομα του αρχείου που ενδιαφέρει  */){
  BF_Block *ht_info_block;
  BF_Block_Init(&ht_info_block);

  //HT_info* ht_info = HT_OpenFile(filename);

  int fd;
  void* ht_info_data;
  
  CALL_BF_NUM(BF_OpenFile(filename, &fd)); //open file
  CALL_BF_NUM(BF_GetBlock(fd, 0, ht_info_block)); 
  ht_info_data = BF_Block_GetData(ht_info_block); //get the data of the fisrt block
  
  // HT_info *ht_info = malloc(sizeof(HT_info));
  // memcpy( ht_info, ht_info_data, sizeof(HT_info)); 
  HT_info *ht_info = ht_info_data;
  //εδω σκεφτηκα οτι μπορουμε κατευθειαν να δειξουμε στα data αφου δεν θα τα πειραξουμε και δεν
  //θα κλεισουμε το block -οποτε δεν χρειαζομαστε το μαλλοκ

  printf(">> file type %s\n", ht_info->fileType);
  if(strcmp(ht_info->fileType, "hash") != 0 ) //if the file is not hash type 
    return -1;

  int buckets = ht_info->numBuckets;
  int current_block;

  BF_Block *block;
  BF_Block_Init(&block);

  HT_block_info *block_info;
  void* data;
  
  int recordsOfBuckets[buckets];
  int blocksOfBuckets[buckets];
  for (int i=0; i<buckets; i++){
    recordsOfBuckets[i] = 0;
    blocksOfBuckets[i] = 0;
  }

  for(int i=0; i<buckets; i++){ //for every bucket
    printf("hash table-- %d\n",ht_info->hashTable[i]);
    current_block = ht_info->hashTable[i];

    while(current_block != -1){ //find every block that each bucket has
      CALL_BF_NUM(BF_GetBlock(fd,current_block,block));
      data = BF_Block_GetData(block);
      //η παρακατω εντολη εχει θεμα ενα πανω στην insert δεν εχει???
      //memcpy(&block_info, data+512-sizeof(HT_block_info), sizeof(HT_block_info));
      block_info = data+(512-sizeof(HT_block_info));
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
  printf("Mean records are: %ld\n", total_records/buckets);
  printf("Mean blocks are: %ld\n", total_blocks/buckets);

  int overflowedBuckets = 0;
  for(int i=0; i<buckets; i++){
    if(blocksOfBuckets[i] > 1){
      printf("The bucket %d has %d overflowed blocks\n", i, blocksOfBuckets[i]-1);
      overflowedBuckets++;
    }
  }
  printf("There are %d buckets with overflowed blocks\n", overflowedBuckets);
  
  printf("The file: %s has in total %d blocks\n", filename, ht_info->lastBlockID+1); //+1 for block with id 0

  CALL_BF_NUM(BF_UnpinBlock(block));
  CALL_BF_NUM(BF_UnpinBlock(ht_info_block));
  BF_Block_Destroy(&block);
  BF_Block_Destroy(&ht_info_block);

  CALL_BF_NUM(BF_CloseFile(fd));
  return 0;
}