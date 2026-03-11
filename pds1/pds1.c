#include "pds.h"
#include <string.h>
#include <stdlib.h>

struct DbInfo db_info;

int create_db(char *name) {
    if (name == NULL)
        return FAILURE;
    db_init();
    char dbname[256], ndxname[256];
    snprintf(dbname, strlen(name) + 5, "%s.dat", name);
    snprintf(ndxname, strlen(name) + 5, "%s.ndx", name);
    FILE *fp = fopen(dbname, "wb");
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

int open_db(char *name, int rec_size) {
    if (db_info.status == DB_OPEN)
        return FAILURE;
    db_init();
    if (name == NULL)
        return FAILURE;
    char dbname[256], ndxname[256];
    snprintf(dbname, strlen(name) + 5, "%s.dat", name);
    snprintf(ndxname, strlen(name) + 5, "%s.ndx", name);
    strncpy(db_info.ndxname, ndxname, sizeof(db_info.ndxname)-1);
    db_info.ndxfile = fopen(db_info.ndxname, "rb+");
    if (db_info.status == DB_CLOSE) {
        db_info.dbfile = fopen(dbname, "rb+");
        if (db_info.dbfile == NULL)
            return FAILURE;
        strncpy(db_info.dbname, dbname, sizeof(db_info.dbname)-1);
    }
    if (db_info.dbfile == NULL || db_info.ndxfile == NULL) {
        if (db_info.dbfile)
            fclose(db_info.dbfile);
        if (db_info.ndxfile)
            fclose(db_info.ndxfile);
        db_info.dbfile = NULL;
        db_info.ndxfile = NULL;
        db_info.dbname[0] = '\0';
        db_info.ndxname[0] = '\0';
        return FAILURE;
    }
    fread(&db_info.rec_count, sizeof(int), 1, db_info.ndxfile);
    fread(db_info.ndxArray, sizeof(struct DB_Ndx), db_info.rec_count, db_info.ndxfile);
    db_info.status = DB_OPEN;
    db_info.rec_size = rec_size;
    fclose(db_info.ndxfile);
    return SUCCESS;
}

int store_db(int key, void *c) {
    if (db_info.dbfile == NULL)
        return FAILURE;
    // fseek to EOF, and then fwrite
    if (fseek(db_info.dbfile, 0, SEEK_END))
        return FAILURE;
    // check if MAX_REC already reached
    if (db_info.rec_count >= MAX_REC) {
        for (int i = 0; i < db_info.rec_count; i++) {
            if (db_info.ndxArray[i].is_deleted == true) {
                db_info.ndxArray[i].is_deleted = false;
                db_info.ndxArray[i].key = key;
                fseek(db_info.dbfile, db_info.ndxArray[i].loc, SEEK_SET);
                fwrite(&key, sizeof(int), 1, db_info.dbfile);
                fwrite(c, db_info.rec_size, 1, db_info.dbfile);
                return SUCCESS;
            }
        }
        return DB_FULL;
    }
    // get the loc
    long location = ftell(db_info.dbfile);
    // create a new index entry using loc and key
    db_info.ndxArray[db_info.rec_count].key = key;
    db_info.ndxArray[db_info.rec_count].loc = location;
    db_info.ndxArray[db_info.rec_count].is_deleted = false;
    db_info.ndxArray[db_info.rec_count].old_key = -1;
    // append the index array
    db_info.rec_count++;
    // fwrite (key, record)
    fwrite(&key, sizeof(int), 1, db_info.dbfile);
    fwrite(c, db_info.rec_size, 1, db_info.dbfile);
    return SUCCESS;
}

int get_db(int key, void *coutput) {
    if (db_info.dbfile == NULL)
        return FAILURE;
    long location = -1L;
    // Search in index array first
    for (int idx = 0; idx < db_info.rec_count; idx++)
        if (db_info.ndxArray[idx].key == key && db_info.ndxArray[idx].is_deleted == false) {
            location = db_info.ndxArray[idx].loc;
            break;
        }
    // Return -1 if not found
    if (location == -1L)
        return REC_NOT_FOUND;
    // fseek to the location (4 bytes ahead of key)
    fseek(db_info.dbfile, location + sizeof(int), SEEK_SET);
    // Do single fread
    fread(coutput, db_info.rec_size, 1, db_info.dbfile);
    return SUCCESS;
}

int update_db(int key, void *c) {
    if (db_info.dbfile == NULL)
        return FAILURE;
    fseek(db_info.dbfile, 0, SEEK_SET);
    for (int i = 0; i < db_info.rec_count; i++) {
        if (db_info.ndxArray[i].key == key && db_info.ndxArray[i].is_deleted == false) {
            fseek(db_info.dbfile, db_info.ndxArray[i].loc + sizeof(int), SEEK_SET);
            fwrite(c, db_info.rec_size, 1, db_info.dbfile);
            return SUCCESS;
        }
    }
    return FAILURE;
}

int delete_db(int key) {
    if (db_info.dbfile == NULL || db_info.status == DB_CLOSE)
        return FAILURE;
    for (int i = 0; i < db_info.rec_count; i++) {
        if (db_info.ndxArray[i].key == key && db_info.ndxArray[i].is_deleted == false) {
            db_info.ndxArray[i].old_key = key;
            db_info.ndxArray[i].key = -1;
            db_info.ndxArray[i].is_deleted = true;
            return SUCCESS;
        }
    }
    return FAILURE;
}

int undelete_db(int key) {
    if (db_info.dbfile == NULL)
        return FAILURE;
    for (int i = 0; i < db_info.rec_count; i++) {
        if (db_info.ndxArray[i].old_key == key && db_info.ndxArray[i].is_deleted == true) {
            db_info.ndxArray[i].key = db_info.ndxArray[i].old_key;
            db_info.ndxArray[i].is_deleted = false;
            return SUCCESS;
        }
    }
    return FAILURE;
}

int close_db() {
    if (db_info.dbfile == NULL)
        return FAILURE;
    db_info.ndxfile = fopen(db_info.ndxname, "wb");
    // fwrite rec_count
    fwrite(&db_info.rec_count, sizeof(int), 1, db_info.ndxfile);
    // Dump (fwrite) index array into the index file
    fwrite(db_info.ndxArray, sizeof(struct DB_Ndx), db_info.rec_count, db_info.ndxfile);
    // fclose
    fclose(db_info.dbfile);
    fclose(db_info.ndxfile);
    // Set status appropriately
    db_info.dbfile = NULL;
    db_info.ndxfile = NULL;
    db_info.dbname[0] = '\0';
    db_info.ndxname[0] = '\0';
    db_info.status = DB_CLOSE;
    return SUCCESS;
}

void db_init() {
    static bool is_system_initialized = false;
    if (is_system_initialized == true)
        return;
    is_system_initialized = true;
    db_info.dbfile = NULL;
    db_info.ndxfile = NULL;
    strcpy(db_info.dbname, "");
    strcpy(db_info.ndxname, "");
    db_info.rec_count = 0;
    db_info.rec_size = 0;
    db_info.status = DB_CLOSE;
}