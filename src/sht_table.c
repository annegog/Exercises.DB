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
  info.capacityOfRecords = (BF_BLOCK_SIZE - sizeof(SHT_block_info))/sizeof(Record);
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
  info->hashTable = (int *)malloc(info->numBuckets*sizeof(int));
  for(int i=-0; i<info->numBuckets; i++)
    info->hashTable[i] = -1;//initialize to -1
  if (info->hashTable == NULL)
  {
    printf("Couldn't allocate the memory for hash table!\n");
    return NULL;
  }
  
  for(int i=-0; i<info->numBuckets; i++)
    info->hashTable[i]=-1;//initialize to -1

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

  //αρα 1ο βημα hash το ονομα για να δουμε σε ποιο μπλοκ του δευτερουοντος παει
  //2ο- βαζουμε την εγγραφη [name ,blockid] στο δευτερευον
  //αν δεν χωραει φτιαχουμε υπερχειλιση και ενημερωνουμε το nextblockid
  //αυτα

}

int SHT_SecondaryGetAllEntries(HT_info* ht_info, SHT_info* sht_info, char* name){
  //-για το 2ερευον
  ///εδω απλα παλι κανουμε hash το ονομα για να βρουμε το bucket
  // απο το bucket βρισκουμε την εγγραφη-ονομα και παιρνουμε το blockid
  //-παμε μετα στο 1ευον
  //και παιρνουμε ολη την εγγραφη

}



