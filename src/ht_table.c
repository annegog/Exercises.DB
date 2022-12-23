#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "ht_table.h"
#include "record.h"

#define CALL_BF_NUM(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }


int HT_CreateFile(char *fileName,  int buckets){
  BF_Block *block;
  BF_Block_Init(&block);

  HT_info info;
  void* data;
  int fd;

  CALL_BF_NUM(BF_CreateFile(fileName));
  CALL_BF_NUM(BF_OpenFile(fileName, &fd));

  memcpy(info.fileType, "hash", strlen("hash")+1);
  info.capacityOfRecords = (BF_BLOCK_SIZE - sizeof(HT_block_info))/sizeof(Record);
  info.fileDesc = fd;
  info.numBuckets=buckets;
  
  //------------------------------------------------
  // ΝΑ ΤΟ ΣΥΖΗΤΗΣΟΥΜΕ ΓΙΑΤΙ ΒΑΡΙΕΜΑΙ ΝΑ ΓΡΑΦΩ
  // int hashTable[] = {0, 1, 2, 3, 4, 5, 6, 7, 9};
  // // ένα block για κάθε κουβά για αρχικοποίηση
  // for (int i=0; i<info.numBuckets; i++){
  //     // allocate block i
  // }

  // CALL_BF_NUM(BF_AllocateBlock(fd, block));  // Δημιουργία καινούριου block
  // data = BF_Block_GetData(block); 
  // memcpy(data, &info, sizeof(HT_info)-sizeof(int*));
  
  // for (int i=0; i<buckets; i++) { // for every bucket, p 
  //   memcpy(data+sizeof(HT_info)-sizeof(int*) + i*sizeof(int), hashTable[i], sizeof(int));
  // }
  //------------------------------------------------

  CALL_BF_NUM(BF_AllocateBlock(fd, block));  // Δημιουργία καινούριου block
  data = BF_Block_GetData(block); 
  memcpy(data, &info, sizeof(HT_info));

  BF_Block_SetDirty(block);
  CALL_BF_NUM(BF_UnpinBlock(block));

  CALL_BF_NUM(BF_CloseFile(fd)); //Κλείσιμο αρχείου και αποδέσμευση μνήμης
  BF_Block_Destroy(&block);
  return 0;
}

HT_info* HT_OpenFile(char *fileName){
  BF_Block *block;
  BF_Block_Init(&block);

  int fd;
  void* data;
  
  BF_OpenFile(fileName, &fd);
  BF_GetBlock(fd, 0, block); // λογικα εδω παίρνει το 1ο block
  data = BF_Block_GetData(block);  

  HT_info *info=data;
  info->hashTable=(int **)malloc(info->numBuckets*sizeof(int *));
  for (int i=0; i<info->numBuckets; i++)
    info->hashTable[i]=malloc(2*sizeof(int));
  info->occupiedPosInHT=0;//info->numBuckets;
  info->sizeOfHT=info->numBuckets;
  //γιατι ακομα δεν εχει ανατεθει καποιο μπλοκ σε καποιο bucket
  //?? ΕΠΙΣΗΣ ΜΗΠΩΣ ΝΑ ΒΑΛΟΥΜΕ ΣΕ ΟΛΕΣ ΤΙΣ ΘΕΣΕΙΣ ΤΟ -1, γιατι δεν ξερω αν αρχικοποιει τον πινακα σε 0

  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);

  if(strcmp(info->fileType, "hash") ==0 )
    return info;
  return NULL;
}


int HT_CloseFile( HT_info* HT_info ){
  int fd=HT_info->fileDesc;
  
  //free the malloc   
  for (int i=0; i<HT_info->sizeOfHT; i++)
  {
    free(HT_info->hashTable[i]);
  }
  free(HT_info->hashTable);

  CALL_BF_NUM(BF_CloseFile(fd));
  return 0;
}

int HT_InsertEntry(HT_info* ht_info, Record record){
  BF_Block *block;
  BF_Block_Init(&block);

  void* data;
  HT_block_info block_info;

  int fd=ht_info->fileDesc;
  long int numBuckets = ht_info->numBuckets;
  int bucket = record.id%numBuckets;
  // get blockId for bucket
  int offset=512-sizeof(HT_block_info);
  int blockId; //= ht_info->hashTable[idHash];
  int check=0;

  if(ht_info->occupiedPosInHT==ht_info->sizeOfHT)
  {
    //realloc 
    int new_size=ht_info->sizeOfHT+ht_info->numBuckets;
    int old_size=ht_info->sizeOfHT;
    ht_info->hashTable = realloc(ht_info->hashTable, new_size*sizeof(int *));
    for (int i=0; i<ht_info->numBuckets; i++)
    {
      ht_info->hashTable[old_size+i]=malloc(2*sizeof(int));
    }
    ht_info->sizeOfHT=new_size;
  }

  for (int i=0; i<ht_info->occupiedPosInHT; i++)
  {
    if (ht_info->hashTable[i][0]==bucket)
    {
      //απλα να κραταμε το τελευταιο μπλοκ που βρισκουμε για αυτο το bucket
      //αφου ουσιαστικα φτιαχνουμε καινουριο block αφου πρωτα γεμισουν τα αλλα.
      check=1;
      blockId=ht_info->hashTable[i][1];
    }
  }

  if (check==0)
  {
    //lastBlockID
    //θα φτιαξουμε καινουριο μπλοκ αρα δεν μας ενδιαφερει να 
    //να ελεγξουμε αν το last block ειναι το 0
    //αρα δημιουργουμε ενα και ενημερωνουμρ το hashtable
    //και βαζουμε και την εγγραφη
    
    BF_AllocateBlock(fd, block); //φτιαξε καινουριο μπλοκ
    data = BF_Block_GetData(block); //και φερτο στην ενδιαεση μνημη
   
    Record* rec = data;
    rec[0] = record;
    // printRecord(rec[0]);
    
    block_info.numOfRecords = 1;
    block_info.previousBlockId = 0;
    memcpy(data+offset, &block_info, sizeof(HT_block_info));
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);

    ht_info->lastBlockID++; //αυξησε κατα 1 id του τελευταιου μπλοκ

    BF_Block_Destroy(&block);

    return ht_info->lastBlockID;
    
  }

  // get blockID and metadata
  BF_GetBlock(fd, blockId, block);
  //επειτα θα ελεγχουμε αν ειναι γεματο
  //αν ειναι φτιαχνουμε καινουριο block για αυτο το bucket
  //αλλιως το βαζουμε στο ηδη υπαρχον
  data = BF_Block_GetData(block); 

  memcpy(&block_info, data+offset, sizeof(HT_block_info));

  if(block_info.numOfRecords < ht_info->capacityOfRecords){
    Record* rec = data;
    rec[block_info.numOfRecords++] = record; //βαλε στην ασχη του μλποκ το record
    memcpy(data+offset, &block_info, sizeof(HT_block_info)); // και στο τελος ενημερωσε το bock info
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);

    BF_Block_Destroy(&block);

    return blockId;
  }
  else{
    BF_Block *new_block;
    HT_block_info new_block_info;
    void* new_data;

    BF_Block_Init(&new_block);
    BF_AllocateBlock(fd, new_block); //φτιάξε καινουριο μπλοκ
    new_data = BF_Block_GetData(new_block);

    Record* rec = new_data;
    rec[0] = record;

    new_block_info.numOfRecords=1;
    new_block_info.previousBlockId = blockId; 
    //new_block_info.nextBlock = NULL;
    memcpy(new_data+(512-sizeof(HT_block_info)), &new_block_info, sizeof(HT_block_info));

    BF_Block_SetDirty(new_block);
    BF_UnpinBlock(new_block);

    //block_info.nextBlock = new_block;
    memcpy(data+offset, &block_info, sizeof(HT_block_info));
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);

    ht_info->lastBlockID++; //αυξησε κατα 1 id του τελευταιου μπλοκ
   
    BF_Block_Destroy(&block);
    BF_Block_Destroy(&new_block);

    return ht_info->lastBlockID;
  }
  
  // check if blockID έχει χωρόοο;;;;;;;;
  // int numRecords; // = block_info.numOfRecords

  // if (numRecords < ht_info->capacityOfRecords){
  //   // insert record
  // } 
  // else{ // create new block (+ 1)
    
  //   int newBlockId; 
  //   BF_GetBlockCounter(ht_info->fileDesc, &newBlockId);
  //   newBlockId += 1;
    
  //   // set its previous block to the previous blockId
  //   HT_block_info* new_ht_block_info;
  //   new_ht_block_info->previousBlockId = blockId;

  //   ht_info->hashTable[idHash] = newBlockId;

  //   // insert entry 
  // } 
  //   return 0;
}

int HT_GetAllEntries(HT_info* ht_info, int value ){
  BF_Block *block;
  BF_Block_Init(&block);

  int fd = ht_info->fileDesc;  
  void* data;
  
  int block_num;
  long int numBuckets = ht_info->numBuckets;
  int bucket = value%numBuckets; 

  for (int i=ht_info->occupiedPosInHT-1; i>=0; i--){
    if(ht_info->hashTable[i][0]==bucket){
      block_num=ht_info->hashTable[i][1];
      break;
    }
  }

  HT_block_info *block_info;
  int block_counter=0;

  do{
    BF_GetBlock(fd,block_num,block);
    data = BF_Block_GetData(block);
    
    Record* rec = data;
    block_info = data+(512-sizeof(HT_block_info));
  
    for (int record=0; record < block_info->numOfRecords; record++){
      if(rec[record].id == value){
        printf("\tfound it\n");
        printRecord(rec[record]);
      }
    }
    block_counter++;
    BF_UnpinBlock(block);

  } while(( block_num = block_info->previousBlockId ) != 0);

  BF_Block_Destroy(&block);
  return block_counter;
}




