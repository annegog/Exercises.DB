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
  //exit(code);  ebgala ayto kai ebla to return

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
  info.capacityOfRecords = (BF_BLOCK_SIZE - sizeof(HT_block_info))/sizeof(Record);
  info.numBuckets=buckets;

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
  CALL_BF_NULL(BF_GetBlock(fd, 0, block)); // λογικα εδω παίρνει το 1ο block
  data = BF_Block_GetData(block);  

  SHT_info *info=data;
  info->hashTable=(int **)malloc(info->numBuckets*sizeof(int *));
  for (int i=0; i<info->numBuckets; i++)
    info->hashTable[i]=malloc(2*sizeof(int));
    //mhpws na elegxoyme an ontws gientai to malloc

  info->occupiedPosInHT=0;//info->numBuckets;
  info->sizeOfHT=info->numBuckets;
  //γιατι ακομα δεν εχει ανατεθει καποιο μπλοκ σε καποιο bucket
  //?? ΕΠΙΣΗΣ ΜΗΠΩΣ ΝΑ ΒΑΛΟΥΜΕ ΣΕ ΟΛΕΣ ΤΙΣ ΘΕΣΕΙΣ ΤΟ -1, γιατι δεν ξερω αν αρχικοποιει τον πινακα σε 0

  BF_Block_SetDirty(block);// prosuesa kai ayto logika xreiazetai
  CALL_BF_NULL(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  if(strcmp(info->fileType, "hash") ==0 )
    return info;
  return NULL;
}


int SHT_CloseSecondaryIndex( SHT_info* SHT_info ){
  int fd=SHT_info->fileDesc;
  
  //free the malloc   
  for (int i=0; i<SHT_info->sizeOfHT; i++)
  {
    free(SHT_info->hashTable[i]);
  }
  free(SHT_info->hashTable);

  CALL_BF_NUM(BF_CloseFile(fd));
  return 0;
}

int SHT_SecondaryInsertEntry(SHT_info* sht_info, Record record, int block_id){

}

int SHT_SecondaryGetAllEntries(HT_info* ht_info, SHT_info* sht_info, char* name){

}



