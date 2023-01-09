Κ18 - Database Systems Implementation - Exercise 1, 2
Atalanti Papadaki (1115201800148) - Annna Gogoula (115201800305)
----------------------------------------------------------------

make bf
./build/bf_main

make ht
./build/ht_main

make sht
./build/sht_main

----------------------------------------------------------------
-> HP
 - hp_file.h:
    * stuct HP_info: is a struct for the data of the file, it contains the file type, file descriptor,
                    the id of the last block and the capacity of the block.
    * struct HP_block_info: is a struct for the metadata of a block, it contains the number of the records and 
                            a pointer to the next block. 
 - hp_file.c:
    * we made 2 defines. The one returns null and the other -1, if the BF function executed incorrectly.
    * HP_CreateFile: Create a file with fileName name, create a block and create and initialize the metadata, then copy them on the first block of the file.
    * HP_OpenFile: Get the first block and check if it's heap type, if it's not returnt null.
    * HP_CloseFile: 
    * HP_InsertEntry: Check if the last block ID is 0 (the first block of the file), make a new block and place the record in the file
                     and update the metadata of the block. If it's not get the last block and if there is space in it place the record there and update the metadata.
                     If the block is full, make a new block and place the record there with the metadata of the block.
    * HP_GetAllEntries: take the number of blocks and for every block and every record of it, check if the request ID is the same as the record's. If yes print the record.
 - hp_main: RECORDS_NUM = 594, is the maximum we can make without segmentation fault. :(


-> HT
 - ht_table.h:
    * stuct HT_info: is a struct for the data of the file it also contains the file type, file descriptor, the capacity of the records, the number of the buckets we are 
                     going to use, a hash table, the number of the occupied positions in a hash table, the size of it and the ID of the last block.
    * struct HT_block_info: it contains the number of the records in every block and the ID of the previous block.
 - ht_table.c:
    * HT_CreateFile: Create a file with fileName name, create a block and initialize the metadata of the file.
    * HT_OpenFile: Make space for the hash table, open the first block and check the file type.
    * HT_CloseFile: Free the memory of the hash table and close the file.
    * HT_InsertEntry: If you reach the maximum size of the hash table reallocate memory. Find the last block of the bucket,
                     if there are no blocks in it make a new one and put the record. Else get the data of the block, 
                     if the block is empty place the record on the block, else make a new one.
    * HT_GetAllEntries: Get the number of the last block of the bucket, and for every block (from the last to the first) check if you find the record with the ID you're looking for. 
                     If you find it print it and when the searching stops return the number of the blocks that you read.
 - ht_main.c: RECORDS_NUM = 619 is also the maximum!

-> HashStatistics: 

-> SHT
 - sht_table.h:
    * struct SHT_block_info:
 - sht_table.c:
    * SHT_CreateSecondaryIndex:
    * SHT_OpenSecondaryIndex: 
    * SHT_CloseSecondaryIndex: 
    * SHT_SecondaryInsertEntry: 
    * SHT_SecondaryGetAllEntries: 
 - sht_main.c: 


----------------------------------------------------------------
**Further observations**
- HT: 
   - 10 buckets with maximum 619 RECORDS_NUM (BF Error: BF memory is full)
   - 30 buckets with maximum 759 RECORDS_NUM
   - 50 buckets with maximum 699 RECORDS_NUM
   - 20 buckets with maximum 639 RECORDS_NUM