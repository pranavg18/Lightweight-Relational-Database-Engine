#ifndef PDS4_H
#define PDS4_H

#include <stdio.h>
#include <stdbool.h>
#define SUCCESS 0
#define REC_NOT_FOUND 1
#define FAILURE 1
#define DB_OPEN 0
#define DB_CLOSE 1
#define DB_FULL 1
#define MAX_REC 10000

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
    struct tableInfo *tables;
    struct RelInfo *relations;
    char db_name[50];
    int num_tables;
    int db_status;
    int num_relations;
};

extern struct dbInfo db_info;

// schema functions
int create_schema(char *db_name);
int open_schema(char *db_name);
void save_schema();
int close_schema();

struct tableInfo* getTableInfo(char *name); // helper function to help get the tableInfo using the table name
int create_table(char *name, int rec_size);
int open_table(char *name);
int store_table(char *name, int key, void *c);
int get_table(char *name, int key, void *coutput);
int get_table_non_key(char *name, void *search_value, void *coutput, int (*match_func)(void *rec, void *search_value)); // non-key field retrieval
int update_table(char *name, int key, void *c);
int delete_table(char *name, int key);
int undelete_table(char *name, int key);
int close_table(char *name);

// relation functions
int create_relation(char *relname, char *pt, char *st);
int open_relation(char *rel);
int store_relation(char *relname, int pk, int fk);

int get_relation(char *relname, void *related_rec, int search_rel_key);
int delete_relation(char *relname, int key);
int undelete_relation(char *relname, int key);
int close_relation(char *relname);

// rebuild .ndx file
int rebuild_index(char *name);

#endif