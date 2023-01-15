#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "sht_table.h"
#include "ht_table.h"
#include "record.h"

#define CALL_BF_NUM(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      return -1;              \
    }                         \
  }

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

  CALL_BF_NUM(BF_CreateFile(sfileName));
  CALL_BF_NUM(BF_OpenFile(sfileName, &fd));

  memcpy(info.fileType, "hash", strlen("hash")+1);
  memcpy(info.fileName, fileName, strlen(fileName)+1);
  info.capacityOfRecords = (BF_BLOCK_SIZE - sizeof(SHT_block_info))/sizeof(SHT_record);
  info.numBuckets=buckets;
  info.lastBlockID = 0;

  CALL_BF_NUM(BF_AllocateBlock(fd, block));  // Δημιουργία καινούριου block
  data = BF_Block_GetData(block); 
  memcpy(data, &info, sizeof(SHT_info));

  BF_Block_SetDirty(block);
  CALL_BF_NUM(BF_UnpinBlock(block));

  CALL_BF_NUM(BF_CloseFile(fd)); //Κλείσιμο αρχείου και αποδέσμευση μνήμης
  BF_Block_Destroy(&block);
  return 0;
}

SHT_info* SHT_OpenSecondaryIndex(char *indexName){
  BF_Block *block;
  BF_Block_Init(&block);

  int fd;
  void* data;
  
  CALL_BF_NULL(BF_OpenFile(indexName, &fd));
  CALL_BF_NULL(BF_GetBlock(fd, 0, block)); 
  data = BF_Block_GetData(block);  

  // SHT_info *info=data;
  // info->hashTable=(int **)malloc(info->numBuckets*sizeof(int *));
  // for (int i=0; i<info->numBuckets; i++)
  //   info->hashTable[i]=malloc(2*sizeof(int));
  //   //mhpws na elegxoyme an ontws gientai to malloc
  
  SHT_info *info = malloc(sizeof(SHT_info));
  memcpy( info, data, sizeof(SHT_info)); 
  
  info->fileDesc = fd;
  info->shashTable = (int *)malloc(info->numBuckets*sizeof(int));
  for(int i=-0; i<info->numBuckets; i++)
    info->shashTable[i] = -1;//initialize to -1
  if (info->shashTable == NULL)
  {
    printf("Couldn't allocate the memory for hash table!\n");
    return NULL;
  }
  
  for(int i=-0; i<info->numBuckets; i++)
    info->shashTable[i]=-1;//initialize to -1

  BF_Block_SetDirty(block);// prosuesa kai ayto logika xreiazetai
  CALL_BF_NULL(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  if(strcmp(info->fileType, "hash") == 0 )
    return info;
  return NULL;
}


int SHT_CloseSecondaryIndex( SHT_info* sht_info ){
  BF_Block *block;
  BF_Block_Init(&block);
  void* data;
  int fd = sht_info->fileDesc; //find the file descriptor of the file

  BF_GetBlock(fd, 0, block);
  data = BF_Block_GetData(block); 

  int bytes_of_hash = sht_info->numBuckets*sizeof(int);
  //printf("sizeof(hash) %ld\n",bytes_of_hash);
  memcpy(data, sht_info, sizeof(HT_info)+bytes_of_hash); //and copy the struct at the data of the block

  BF_Block_SetDirty(block);
  CALL_BF_NUM(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  //free(ht_info->hashTable); //na to trejw na dv ean doyleyei ayto!!
  free(sht_info);

  CALL_BF_NUM(BF_CloseFile(fd));
  return 0;
}

int SHT_SecondaryInsertEntry(SHT_info* sht_info, Record record, int block_id){
  //πρπει να κραταμε ζευγος [ονομα εγγραφης , μπλοκ που βρισκεται στο πρωτευον]
  BF_Block *block;
  BF_Block_Init(&block);

  void* data;
  SHT_block_info block_info;

  int fd=sht_info->fileDesc;
  long int numBuckets = sht_info->numBuckets;
  
  //αρα 1ο βημα hash το ονομα για να δουμε σε ποιο μπλοκ του δευτερουοντος παει

  char first_letter = record.name[0];
  int first_num = (int)first_letter;

  int bucket = first_num%numBuckets;

  int blockId = sht_info->shashTable[bucket];

  SHT_record current_record;
  current_record.blockID = block_id;
  memcpy(current_record.name, record.name, sizeof(record.name));

  //2ο- βαζουμε την εγγραφη [name ,blockid] στο δευτερευον
  //αν δεν χωραει φτιαχουμε υπερχειλιση και ενημερωνουμε το nextblockid
  //αυτα

  if(blockId==-1) //the bucket has no block
  {
    //allocate one
    //and place the record there
    CALL_BF_NUM(BF_AllocateBlock(fd, block));
    data = BF_Block_GetData(block);

    SHT_record* rec = data;
    rec[0] = current_record;

    block_info.numOfRecords = 1;
    block_info.previousBlockId = -1; //ειναι το πρωτο block για το bucket
    block_info.nextBlockId = -1; //αρα δεν εχει ουτε προηγουμενο ουτε επομενο ακόμα
    memcpy(data+512-sizeof(SHT_block_info), &block_info, sizeof(SHT_block_info));
    BF_Block_SetDirty(block);
    CALL_BF_NUM(BF_UnpinBlock(block));

    sht_info->shashTable[bucket]= ++(sht_info->lastBlockID);
    // printf("ht_info->hashTable[bucket] %d\n", ht_info->hashTable[bucket]);
    BF_Block_Destroy(&block);

    return 0;
  }
  //else
  CALL_BF_NUM(BF_GetBlock(fd, blockId, block));
  data = BF_Block_GetData(block);

  memcpy(&block_info, data+512-sizeof(SHT_block_info), sizeof(SHT_block_info));
  while(block_info.nextBlockId != -1)
  {
    blockId=block_info.nextBlockId;
    CALL_BF_NUM(BF_GetBlock(fd, blockId, block));
    data = BF_Block_GetData(block); 

    memcpy(&block_info, data+512-sizeof(SHT_block_info), sizeof(SHT_block_info));
  }

  //if the block has empty space then write the record at the block
  if(block_info.numOfRecords < sht_info->capacityOfRecords){
    SHT_record* rec = data;
    rec[block_info.numOfRecords++] = current_record; 
    memcpy(data+512-sizeof(SHT_block_info), &block_info, sizeof(SHT_block_info)); 
    BF_Block_SetDirty(block);
    CALL_BF_NUM(BF_UnpinBlock(block));

    BF_Block_Destroy(&block);

    return 0;
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

    SHT_record* rec = new_data;
    rec[0] = current_record;

    new_block_info.numOfRecords = 1;
    new_block_info.previousBlockId = blockId; 
    new_block_info.nextBlockId = -1;
    block_info.nextBlockId = ++(sht_info->lastBlockID);
    memcpy(new_data+(512-sizeof(SHT_block_info)), &new_block_info, sizeof(SHT_block_info));

    BF_Block_SetDirty(new_block);
    CALL_BF_NUM(BF_UnpinBlock(new_block));

    memcpy(data+512-sizeof(SHT_block_info), &block_info, sizeof(SHT_block_info));
    BF_Block_SetDirty(block);
    CALL_BF_NUM(BF_UnpinBlock(block));

    BF_Block_Destroy(&block);
    BF_Block_Destroy(&new_block);

    return 0;
  }
}

int SHT_SecondaryGetAllEntries(HT_info* ht_info, SHT_info* sht_info, char* name){
  //-για το 2ερευον
  ///εδω απλα παλι κανουμε hash το ονομα για να βρουμε το bucket
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
  
  char first_letter = name[0];
  int first_num = (int)first_letter;

  int bucket = first_num%numBuckets;

  int current_block = sht_info->shashTable[bucket];

  int block_counter = 0;
  int HashBlockID = 0;

  while(current_block != -1){
    printf("current_block = %d\n", current_block);
    CALL_BF_NUM(BF_GetBlock(fd,current_block,block));
    data = BF_Block_GetData(block);

    SHT_record* rec = data;    
    block_info = data+(512-sizeof(SHT_block_info)); //no memcopy??

    //check every record in the block 
    for (int record=0; record < block_info->numOfRecords; record++){
      if(strcmp(rec[record].name, name) == 0){ //if you find the record with the specific value
        printf("\nfound it !");
        HashBlockID = rec[record].blockID;
        CALL_BF_NUM(BF_GetBlock(ht_info->fileDesc, HashBlockID, HashBlock));
        HashData = BF_Block_GetData(HashBlock);
        
        Record *HashRecord = HashData;
        hash_block_info = HashData+(512-sizeof(HT_block_info)); 
        for (int record=0; record < hash_block_info->numOfRecords; record++){
          if(strcmp(HashRecord[record].name, name) == 0){ //if you find the record with the specific value
            printf("\nit's here!");
            printRecord(HashRecord[record]); //print the record
            printf("Blocks until i found youuu: %d\n\n",block_counter+1);
          }
        }
        block_counter++;
      }
    }
    block_counter++; //count the blocks we have read
    current_block = block_info->nextBlockId; 
    CALL_BF_NUM(BF_UnpinBlock(block));
    CALL_BF_NUM(BF_UnpinBlock(HashBlock));
  }
  
  printf("aaaaaaaaaaa  last %d --- %d\n\n", sht_info->lastBlockID, block_counter);

  BF_Block_Destroy(&HashBlock);
  BF_Block_Destroy(&block);
  return block_counter;

  // απο το bucket βρισκουμε την εγγραφη-ονομα και παιρνουμε το blockid
  //-παμε μετα στο 1ευον
  //και παιρνουμε ολη την εγγραφη
}



int SHT_HashStatistics(char* filename /*όνομα του αρχείου που ενδιαφέρει  */){
  BF_Block *sht_info_block;
  BF_Block_Init(&sht_info_block);

  //HT_info* ht_info = HT_OpenFile(filename);

  int fd;
  void* sht_info_data;
  
  CALL_BF_NUM(BF_OpenFile(filename, &fd)); //open file
  CALL_BF_NUM(BF_GetBlock(fd, 0, sht_info_block)); 
  sht_info_data = BF_Block_GetData(sht_info_block); //get the data of the fisrt block
  
  // HT_info *ht_info = malloc(sizeof(HT_info));
  // memcpy( ht_info, ht_info_data, sizeof(HT_info)); 
  SHT_info *sht_info = sht_info_data;
  //εδω σκεφτηκα οτι μπορουμε κατευθειαν να δειξουμε στα data αφου δεν θα τα πειραξουμε και δεν
  //θα κλεισουμε το block -οποτε δεν χρειαζομαστε το μαλλοκ

  printf(">> file type %s\n", sht_info->fileType);
  if(strcmp(sht_info->fileType, "shash") != 0 ) //if the file is not hash type 
    return -1;

  int buckets = sht_info->numBuckets;
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
    printf("hash table-- %d\n",sht_info->shashTable[i]);
    current_block = sht_info->shashTable[i];

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
  
  printf("The file: %s has in total %d blocks\n", filename, sht_info->lastBlockID+1); //+1 for block with id 0

  CALL_BF_NUM(BF_UnpinBlock(block));
  CALL_BF_NUM(BF_UnpinBlock(sht_info_block));
  BF_Block_Destroy(&block);
  BF_Block_Destroy(&sht_info_block);

  CALL_BF_NUM(BF_CloseFile(fd));
  return 0;
}
