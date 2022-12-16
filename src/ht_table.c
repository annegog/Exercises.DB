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
  {
    info->hashTable[i]=malloc(2*sizeof(int));
  }
  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);

  if(strcmp(info->fileType, "heap") ==0 )
    return info;

  return NULL;
}


int HT_CloseFile( HT_info* HT_info ){
  int fd=HT_info->fileDesc;
  //free the malloc 
  CALL_BF_NUM(BF_CloseFile(fd));
  return 0;
}

int HT_InsertEntry(HT_info* ht_info, Record record){
    return 0;
}

int HT_GetAllEntries(HT_info* ht_info, void *value ){
    return 0;
}




