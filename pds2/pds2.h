#ifndef PDS2_H
#define PDS2_H

#include <stdio.h>
#include <stdbool.h>
#define SUCCESS 0
#define REC_NOT_FOUND 1
#define FAILURE 1
#define DB_OPEN 0
#define DB_CLOSE 1
#define DB_FULL 1
#define MAX_REC 10000

// struct PdsInfo {
//     FILE *table_ptr_1;
//     FILE *table_ptr_2;
//     FILE *ndx_ptr_1;
//     FILE *ndx_ptr_2;
// };

struct RecNdx {
    int key;
    long loc;
    bool is_deleted;
    int old_key;
};

struct tableInfo {
    FILE *tfile;
    FILE *ndxfile;
    char tname[50];
    char ndxname[50];
    int rec_count;
    struct RecNdx ndxArray[MAX_REC];
    int rec_size;
    bool is_initialized;
    int status;
};

struct dbInfo {
    struct tableInfo tables[2];
    int num_tables;
    int status;
};

extern struct dbInfo db_info;

int create_table(char *name);
int open_table(char *name, int rec_size);
int store_table(char *name, int key, void *c);
int get_table(char *name, int key, void *coutput);
int update_table(char *name, int key, void *c);
int delete_table(char *name, int key);
int undelete_table(char *name, int key);
int close_table(char *name);
void db_init();
int db_create(char *table1, char *table2);
int db_open(char *table1, int recSize1, char *table2, int recSize2);
int db_close();

#endif