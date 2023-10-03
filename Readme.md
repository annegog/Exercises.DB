### Κ18 - Database Systems Implementation - Exercise 1, 2

Atalanti Papadaki - Annna Gogoula

This exercise aims to provide an understanding of how Database Systems manage data at the block and record levels. It also explores the impact of indexing on the performance of a Database Management System (DBMS). In this exercise, you will implement functions for managing files using two different methods: heap file organization and static hashing.

----------------------------------------------------------------

- make bf:
    * ./build/bf_main

- make ht:
    * ./build/ht_main

- make sht:
    * ./build/sht_main

----------------------------------------------------------------
-> HP
 - hp_file.h:
    * stuct HP_info: is a struct for the data of the file, it contains the file type, file descriptor,
                  the id of the last block and the capacity of the block.
    * struct HP_block_info: is a struct for the metadata of a block, it contains the number of the records and 
                        a pointer to the next block. 
 - hp_file.c:
    * we made 2 defines. The one returns null and the other -1, if the BF function is executed incorrectly.
    * HP_CreateFile: Create a file with fileName name, create a block and create and initialize the metadata, then copy them on the first block of the file.
    * HP_OpenFile: Get the first block and check if it's heap type, if it's not return null.
    * HP_CloseFile: 
    * HP_InsertEntry: Check if the last block ID is 0 (the first block of the file), make a new block place the record in the file,
                  and update the metadata of the block. If it's not get the last block and if there is space in it place the record there and update the metadata.
                  If the block is full, make a new block and place the record there with the metadata of the block.
    * HP_GetAllEntries: take the number of blocks and for every block and every record of it, check if the request ID is the same as the records. If yes print the record.
 - hp_main: RECORDS_NUM = 594, is the maximum we can make without segmentation fault. :(


-> HT
 - ht_table.h:
    * struct HT_info: is a struct for the data of the file it also contains the file type, file descriptor, the capacity of the records, the number of the buckets we are 
                     going to use, a hash table, the number of the occupied positions in a hash table, the size of it, and the ID of the last block.
    * struct HT_block_info: it contains the number of the records in every block, the ID of the next block, and the ID of the previous block.
 - ht_table.c:
    * HT_CreateFile: Create a file with fileName name, create a block and initialize the metadata of the file.
    * HT_OpenFile: Make space for the hash table, open the first block and check the file type.
    * HT_CloseFile: Free the memory of the hash table and close the file.
    * HT_InsertEntry: If you reach the maximum size of the hash table reallocate memory. Find the last block of the bucket,
                  if there are no blocks in it make a new one and put the record. Else get the data of the block, 
                  if the block is empty place the record on the block, else make a new one.
    * HT_GetAllEntries: Get the number of the last block of the bucket, and for every block (from the last to the first) check if you find the record with the ID you're looking for. 
                  If you find it print it and when the searching stops return the number of the blocks that you read.


-> SHT
 - sht_table.h:
    * struct SHT_info: is a struct for the data of the file it also contains the file type, the file name of the primary index, the file descriptor, the capacity of the records, 
                     the number of the buckets we are going to use, a hash table and the ID of the last block.
    * struct SHT_block_info: it contains the number of the records in every block, the ID of the next block and the ID of the previous block.
    * struct SHT_record: it contains the name (of the ht record) and the block ID of the block.
 - sht_table.c:
    * SHT_CreateSecondaryIndex: Create a file with fileName name, create a block and initialize the metadata of the file.
    * SHT_OpenSecondaryIndex: Make space for the secondary hash table, open the first block and check the file type.
    * SHT_CloseSecondaryIndex: Free the memory of the secondary hash table and close the file.
    * SHT_SecondaryInsertEntry: Get the first letter of the name and find the bucket you will write the record. If the bucket has no blocks, make a new one and place the record there. 
                              Else find the last block of the bucket, and check if the record already exists. If it doesn't already exist and if the block has space in it places the record there!
                              But if there is no space in the block, make a new one and place the record there.
    * SHT_SecondaryGetAllEntries: For every block and for every record in that block, if you find a record name you are looking for go to the hash table block. 
                              And for every record in that block id, if you find the name print the record. Return the number of blocks (ht blocks + sht blocks).


-> HashStatistics(HT and SHT): Open the file, if the file is not the file type you want return -1. Else for every bucket in the file, find every block that the bucket has and get the number of 
                           records for the specific block and increase the blocks that the bucket has by one. Find the bucket with the minimum and the maximum records. Find the overflowed blocks and print them.


----------------------------------------------------------------
**Further observations**
-> HT - SHT hash table: We store the first block of each bucket, in order to find the next blocks we have the variable nextBlockId, where we save the id of the next block.


----------------------------------------------------------------
**Problems**
-> SHT: After the SHT_CloseSecondaryIndex there is something wrong with the way of saving the information of the SHT_block_info. Specifically, we noticed that even though we write the correct number at the variable
      in SHT_HashStatistics, the numbers are wrong. As a result, the function SHT_HashStatistics isn't working! But it works with 1 bucket (sht). AND we don't why. :(
-> HT - SHT: When we try to free the hash table at the function HT_CloseFile and SHT_CloseSecondaryIndex, even though we tried to save the information of the hash table, the information is erased! 
         (That's why we commented the lines 119-128 in the file ht_table.c. We tried 2 different ways of saving the hash tables data, but it didn't work)
