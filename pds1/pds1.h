#include <stdio.h>
#include <stdbool.h>
#define SUCCESS 0
#define REC_NOT_FOUND 1
#define FAILURE 1
#define DB_OPEN 0
#define DB_CLOSE 1
#define DB_FULL 1
#define MAX_REC 100000

struct DB_Ndx {
    int key;
    long loc;
    bool is_deleted;
    int old_key;
};

struct DbInfo {
    FILE *dbfile;
    FILE *ndxfile;
    char dbname[50];
    char ndxname[50];
    int status;
    int rec_count;
    struct DB_Ndx ndxArray[MAX_REC];
    int rec_size;
};

extern struct DbInfo db_info;

int create_db(char *name);
int open_db(char *name, int rec_size);
int store_db(int key, void *c);
int get_db(int key, void *coutput);
int update_db(int key, void *c);
int delete_db(int key);
int undelete_db(int key);
int close_db();
void db_init();