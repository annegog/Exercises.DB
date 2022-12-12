#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

#define CALL_BF_NUM(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return -1;        \
  }                         \
}

// #define CALL_BF_NULL(call)       \
// {                           \
//   BF_ErrorCode code = call; \
//   if (code != BF_OK) {         \
//     BF_PrintError(code);    \
//     return NULL;        \
//   }                         \
// }

int HP_CreateFile(char *fileName){
  int fd;
  void* data;
  BF_Block *block;
  BF_Block_Init(&block);

  HP_info info;

  memcpy(info.fileType, "heap", strlen("heap")+1);
  info.fileDesc = fd;
  // printf(">> fd %d, %d\n",fd, info.fileDesc);
  
  CALL_BF_NUM(BF_CreateFile(fileName));
  CALL_BF_NUM(BF_OpenFile(fileName, &fd));
  
  CALL_BF_NUM(BF_AllocateBlock(fd, block));  // Δημιουργία καινούριου block
  data = BF_Block_GetData(block);  
  memcpy(data, &info, 5+sizeof(fd));

  BF_Block_SetDirty(block);
  CALL_BF_NUM(BF_UnpinBlock(block));

  CALL_BF_NUM(BF_CloseFile(fd)); //Κλείσιμο αρχείου και αποδέσμευση μνήμης
  BF_Block_Destroy(&block); 
  //CALL_BF_NUM(BF_Close()); //ΜΕ ΑΥΤΟ ΕΔΩ ΒΓΑΖΕΙ SEGMENTATION

  //CALL_BF_NUM(BF_Close()); // ΑΑΑΑΑΑΑΑΑΑΑΧ ειχα βάλει αυτο σε σχολιο και δεν ειχε seg... αλλα όταν το ξανα έβαλα εκανε! Τζααμπα χάρηκα
  // BF_Block_Destroy(&block);
  return 0;
}

HP_info* HP_OpenFile(char *fileName){
  int fd;
  void* data;
  BF_Block *block;

  BF_Block_Init(&block);
  //printf("Open the file\n");
  BF_OpenFile(fileName, &fd);
  //printf("Get Block\n");
  if(BF_GetBlock(fd, 0, block) != BF_OK) // λογικα εδω παίρνει το 1ο block
    printf("AAAAA \n");
  //printf("get data\n");
  data = BF_Block_GetData(block);  
  HP_info *info=data;
  //printf("type %s\n", info->fileType);
  //printf("fd %d\n",info->fileDesc);
 // if(strcmp(info->fileType, "heap")==0) // αν είναι ίδια
  if(strcmp(info->fileType, "heap") ==0 )
    {
      printf("It's a heap!!!!\n");
      return info;
    }
  printf("OOOOPS!!!!\n");
  return NULL ;
}
  //---------------------------//
/*
  int blocks_num;
  BF_GetBlockCounter(fd, &blocks_num);
  printf("Number of blocks: %d\n", blocks_num); //εμφανίζει πόσα μπλοκ έχουμε

  BF_GetBlock(fd, blocks_num-1, block); // απλα για να μην εχουμε το 0 
//με αυτο θα παρει το τελευταιο μπλοκ και εμεις θελουμε το 1ο (αν εχει πάνω απο 1 μπλοκς)
  data = BF_Block_GetData(block);
  HP_info *info = data;
  printf("Type of file:\t%s \n", info->fileType );
  printf("file descriptor: %d\n",info->fileDesc);

  if(strcmp(info->fileType, "heap") ==0 )
    {
      printf("It's a heap!!!!\n");
      return info;
    }
  printf("OOOOPS!!!!\n");
  return NULL ;
}
*/

int HP_CloseFile( HP_info* hp_info ){
  int fd=hp_info->fileDesc;
  CALL_BF_NUM(BF_CloseFile(fd));
  //μηπως εδω θελει το BF_close??
}

int HP_InsertEntry(HP_info* hp_info, Record record){
  /*CALL_OR_DIE(BF_AllocateBlock(fd1, block));  // Δημιουργία καινούριου block
    data = BF_Block_GetData(block);             // Τα περιεχόμενα του block στην ενδιάμεση μνήμη
    Record* rec = data;                         // Ο δείκτης rec δείχνει στην αρχή της περιοχής μνήμης data
    rec[0] = randomRecord();
    rec[1] = randomRecord();
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));*/
    return 0;
}

int HP_GetAllEntries(HP_info* hp_info, int value){
  /*printf("Contents of Block %d\n\t",i);
    CALL_OR_DIE(BF_GetBlock(fd1, i, block));
    data = BF_Block_GetData(block);
    Record* rec= data;
    printRecord(rec[0]);
    printf("\t");
    printRecord(rec[1]);
    CALL_OR_DIE(BF_UnpinBlock(block));*/
   return 0;
}

