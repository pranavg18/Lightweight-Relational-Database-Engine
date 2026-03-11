#include "pds2.h"
#include <string.h>
#include <stdlib.h>
#define NO_OF_TABLES 2

struct dbInfo db_info;

struct tableInfo* getTableInfo(char *name) {
    if (name == NULL)
        return NULL;
    for (int i = 0; i < NO_OF_TABLES; i++) {
        if (strcmp(db_info.tables[i].tname, name) == 0) {
            return &db_info.tables[i];
        }
    }
    return NULL;
}

int create_table(char *name) {
    if (name == NULL)
        return FAILURE;
    db_init();
    char tname[256], ndxname[256];
    snprintf(tname, strlen(name) + 5, "%s.dat", name);
    snprintf(ndxname, strlen(name) + 5, "%s.ndx", name);
    FILE *fp = fopen(tname, "wb");
    if (fp == NULL)
        return FAILURE;
    FILE *ndx = fopen(ndxname, "wb");
    if (ndx == NULL) {
        fclose(fp);
        return FAILURE;
    }
    int zero = 0;
    fwrite(&zero, sizeof(int), 1, ndx);
    fclose(fp);
    fclose(ndx);
    return SUCCESS;
}

int open_table(char *name, int rec_size) {
    if (name == NULL)
        return FAILURE;
    db_init();

    int idx = -1;
    for (int i = 0; i < NO_OF_TABLES; i++) {
        if (db_info.tables[i].tname[0] == '\0' || strcmp(db_info.tables[i].tname, name) == 0) {
            idx = i;
            break;
        }
    }
    if (idx == -1)
        return FAILURE;
    struct tableInfo *table = &db_info.tables[idx];
    // stop if already open
    if (table->status == DB_OPEN)
        return FAILURE;
    char tname[256], ndxname[256];
    snprintf(tname, strlen(name) + 5, "%s.dat", name);
    snprintf(ndxname, strlen(name) + 5, "%s.ndx", name);
    strncpy(table->ndxname, ndxname, sizeof(table->ndxname)-1);
    strncpy(table->tname, name, sizeof(table->tname)-1);
    table->ndxfile = fopen(table->ndxname, "rb+");
    if (table->status == DB_CLOSE) {
        table->tfile = fopen(tname, "rb+");
        if (table->tfile == NULL)
            return FAILURE;
    }
    if (table->tfile == NULL || table->ndxfile == NULL) {
        table->tfile = NULL;
        table->ndxfile = NULL;
        table->tname[0] = '\0';
        table->ndxname[0] = '\0';
        return FAILURE;
    }
    fread(&table->rec_count, sizeof(int), 1, table->ndxfile);
    fread(table->ndxArray, sizeof(struct RecNdx), table->rec_count, table->ndxfile);
    table->status = DB_OPEN;
    db_info.num_tables++;
    table->rec_size = rec_size;
    fclose(table->ndxfile);
    table->ndxfile = NULL;
    return SUCCESS;
}

int store_table(char *name, int key, void *c) {
    struct tableInfo *table = getTableInfo(name);
    if (table == NULL || table->tfile == NULL)
        return FAILURE;
    // fseek to EOF, and then fwrite
    if (fseek(table->tfile, 0, SEEK_END))
        return FAILURE;
    // check if MAX_REC already reached
    if (table->rec_count >= MAX_REC) {
        for (int i = 0; i < table->rec_count; i++) {
            if (table->ndxArray[i].is_deleted == true) {
                table->ndxArray[i].is_deleted = false;
                table->ndxArray[i].key = key;
                fseek(table->tfile, table->ndxArray[i].loc, SEEK_SET);
                fwrite(&key, sizeof(int), 1, table->tfile);
                fwrite(c, table->rec_size, 1, table->tfile);
                return SUCCESS;
            }
        }
        return DB_FULL;
    }
    // get the loc
    long location = ftell(table->tfile);
    // create a new index entry using loc and key
    table->ndxArray[table->rec_count].key = key;
    table->ndxArray[table->rec_count].loc = location;
    table->ndxArray[table->rec_count].is_deleted = false;
    table->ndxArray[table->rec_count].old_key = -1;
    // append the index array
    table->rec_count++;
    // fwrite (key, record)
    fwrite(&key, sizeof(int), 1, table->tfile);
    fwrite(c, table->rec_size, 1, table->tfile);
    return SUCCESS;
}

int get_table(char *name, int key, void *coutput) {
    struct tableInfo *table = getTableInfo(name);
    if (table == NULL || table->tfile == NULL)
        return FAILURE;
    long location = -1L;
    // Search in index array first
    for (int idx = 0; idx < table->rec_count; idx++)
        if (table->ndxArray[idx].key == key && table->ndxArray[idx].is_deleted == false) {
            location = table->ndxArray[idx].loc;
            break;
        }
    // Return -1 if not found
    if (location == -1L)
        return REC_NOT_FOUND;
    // fseek to the location
    fseek(table->tfile, location + sizeof(int), SEEK_SET);
    // Do single fread
    fread(coutput, table->rec_size, 1, table->tfile);
    return SUCCESS;
}

int update_table(char *name, int key, void *c) {
    struct tableInfo *table = getTableInfo(name);
    if (table == NULL || table->tfile == NULL)
        return FAILURE;
    fseek(table->tfile, 0, SEEK_SET);
    for (int i = 0; i < table->rec_count; i++) {
        if (table->ndxArray[i].key == key && table->ndxArray[i].is_deleted == false) {
            fseek(table->tfile, table->ndxArray[i].loc + sizeof(int), SEEK_SET);
            fwrite(c, table->rec_size, 1, table->tfile);
            return SUCCESS;
        }
    }
    return REC_NOT_FOUND;
}

int delete_table(char *name, int key) {
    struct tableInfo *table = getTableInfo(name);
    if (table == NULL || table->tfile == NULL || table->status == DB_CLOSE)
        return FAILURE;
    for (int i = 0; i < table->rec_count; i++) {
        if (table->ndxArray[i].key == key && table->ndxArray[i].is_deleted == false) {
            table->ndxArray[i].old_key = key;
            table->ndxArray[i].key = -1;
            table->ndxArray[i].is_deleted = true;
            return SUCCESS;
        }
    }
    return FAILURE;
}

int undelete_table(char *name, int key) {
    struct tableInfo *table = getTableInfo(name);
    if (table == NULL || table->tfile == NULL)
        return FAILURE;
    for (int i = 0; i < table->rec_count; i++) {
        if (table->ndxArray[i].old_key == key && table->ndxArray[i].is_deleted == true) {
            table->ndxArray[i].key = table->ndxArray[i].old_key;
            table->ndxArray[i].is_deleted = false;
            return SUCCESS;
        }
    }
    return FAILURE;
}

int close_table(char *name) {
    struct tableInfo *table = getTableInfo(name);
    if (table == NULL || table->tfile == NULL || table->status == DB_CLOSE)
        return FAILURE;
    table->ndxfile = fopen(table->ndxname, "wb");
    if (table->ndxfile == NULL)
        return FAILURE;
    // fwrite rec_count
    fwrite(&table->rec_count, sizeof(int), 1, table->ndxfile);
    // Dump (fwrite) index array into the index file
    fwrite(table->ndxArray, sizeof(struct RecNdx), table->rec_count, table->ndxfile);
    // fclose
    fclose(table->tfile);
    fclose(table->ndxfile);
    // Set status appropriately
    table->tfile = NULL;
    table->tname[0] = '\0';
    table->status = DB_CLOSE;
    db_info.num_tables--;
    return SUCCESS;
}

void db_init() {
    static bool is_system_initialized = false;
    if (is_system_initialized == true)
        return;
    is_system_initialized = true;
    db_info.num_tables = 0;
    db_info.status = DB_CLOSE;
    for (int i = 0; i < NO_OF_TABLES; i++) {
        if (db_info.tables[i].is_initialized == false) {
            db_info.tables[i].is_initialized = true;
            db_info.tables[i].tfile = NULL;
            db_info.tables[i].ndxfile = NULL;
            db_info.tables[i].tname[0] = '\0';
            db_info.tables[i].ndxname[0] = '\0';
            db_info.tables[i].rec_count = 0;
            db_info.tables[i].rec_size = 0;
            db_info.tables[i].status = DB_CLOSE;
            for (int j = 0; j < MAX_REC; j++) {
                db_info.tables[i].ndxArray[j].key = -1;
                db_info.tables[i].ndxArray[j].old_key = -1;
                db_info.tables[i].ndxArray[j].is_deleted = false;
            }
        }
    }
}

int db_create(char *table1, char *table2) {
    db_init();
    if (create_table(table1) != SUCCESS)
        return FAILURE;
    if (create_table(table2) != SUCCESS)
        return FAILURE;
    return SUCCESS;
}

int db_open(char *table1, int recSize1, char *table2, int recSize2) {
    db_init();
    db_info.num_tables = 0;
    if (open_table(table1, recSize1) != SUCCESS)
        return FAILURE;
    if (open_table(table2, recSize2) != SUCCESS)
        return FAILURE;
    db_info.status = DB_OPEN;
    return SUCCESS;
}

int db_close() {
    if (db_info.status == DB_CLOSE)
        return FAILURE;
    for (int i = 0; i < NO_OF_TABLES; i++) {
        if (db_info.tables[i].tname[0] != '\0')
            close_table(db_info.tables[i].tname);
    }
    db_info.status = DB_CLOSE;
    db_info.num_tables = 0;
    return SUCCESS;
}