#ifndef PDS3_H
#define PDS3_H

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

struct RelPair {
    int pkey;
    int fkey;
    int is_deleted;
};

struct RelInfo {
    FILE *relFile;
    char relname[50];
    char pname[50];
    char secname[50];
};

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
    struct RelInfo relation;
    int num_tables;
    int db_status;
    int rel_status;
};

extern struct dbInfo db_info;

struct tableInfo* getTableInfo(char *name); // helper function to help get the tableInfo using the table name
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

// relation functions
int create_relation(char *relname, char *pt, char *st);
int open_relation(char *rel);
int store_relation(int pk, int rk);

int get_relation(void *related_rec, int search_rel_key);
int delete_relation(int key, char *name);
int undelete_relation(int key, char *name);
int close_relation();

// rebuild .ndx file
int rebuild_index(char *name, int rec_size);

#endif