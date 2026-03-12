#include "pds4.h"
#include <string.h>
#include <stdlib.h>

struct dbInfo db_info;

void save_schema() {
    char sch_name[256];
    snprintf(sch_name, sizeof(sch_name), "%s.sch", db_info.db_name);
    FILE *fp = fopen(sch_name, "wb");
    if (fp == NULL)
        return;
    fwrite(&db_info.num_tables, sizeof(int), 1, fp);
    fwrite(&db_info.num_relations, sizeof(int), 1, fp);
    if (db_info.num_tables > 0)
        fwrite(db_info.tables, sizeof(struct tableInfo), db_info.num_tables, fp);
    if (db_info.num_relations > 0)
        fwrite(db_info.relations, sizeof(struct RelInfo), db_info.num_relations, fp);
    fclose(fp);
}

int create_schema(char *db_name) {
    if (db_name == NULL)
        return FAILURE;
    strncpy(db_info.db_name, db_name, 49);
    db_info.db_name[49] = '\0';
    db_info.tables = NULL;
    db_info.relations = NULL;
    db_info.num_tables = 0;
    db_info.num_relations = 0;
    db_info.db_status = DB_OPEN;
    save_schema();
    return SUCCESS;
}

int open_schema(char *db_name) {
    if (db_name == NULL)
        return FAILURE;
    strncpy(db_info.db_name, db_name, 49);
    db_info.db_name[49] = '\0';

    char sch_name[256];
    snprintf(sch_name, sizeof(sch_name), "%s.sch", db_name);
    FILE *fp = fopen(sch_name, "rb");
    if (fp == NULL)
        return FAILURE;
    fread(&db_info.num_tables, sizeof(int), 1, fp);
    fread(&db_info.num_relations, sizeof(int), 1, fp);
    if (db_info.num_tables > 0) {
        db_info.tables = malloc(sizeof(struct tableInfo) * db_info.num_tables);
        fread(db_info.tables, sizeof(struct tableInfo), db_info.num_tables, fp);
    }
    else
        db_info.tables = NULL;
    if (db_info.num_relations > 0) {
        db_info.relations = malloc(sizeof(struct RelInfo) * db_info.num_relations);
        fread(db_info.relations, sizeof(struct RelInfo), db_info.num_relations, fp);
    }
    else
        db_info.relations = NULL;
    fclose(fp);
    for (int i = 0; i < db_info.num_tables; i++) {
        db_info.tables[i].tfile = NULL;
        db_info.tables[i].ndxfile = NULL;
        db_info.tables[i].status = DB_CLOSE;
    }
    for (int i = 0; i < db_info.num_relations; i++)
        db_info.relations[i].relFile = NULL;
    db_info.db_status = DB_OPEN;
    return SUCCESS;
}

int close_schema() {
    if (db_info.db_status == DB_CLOSE)
        return FAILURE;
    for (int i = 0; i < db_info.num_tables; i++)
        if (db_info.tables[i].status == DB_OPEN)
            close_table(db_info.tables[i].tname);
    for (int i = 0; i < db_info.num_relations; i++)
        if (db_info.relations[i].relFile != NULL)
            close_relation(db_info.relations[i].relname);
    save_schema();
    if (db_info.tables != NULL)
        free(db_info.tables);
    if (db_info.relations != NULL)
        free(db_info.relations);
    db_info.tables = NULL;
    db_info.relations = NULL;
    db_info.db_status = DB_CLOSE;
    return SUCCESS;
}

struct tableInfo* getTableInfo(char *name) {
    if (name == NULL || db_info.tables == NULL)
        return NULL;
    for (int i = 0; i < db_info.num_tables; i++) {
        if (strcmp(db_info.tables[i].tname, name) == 0) {
            return &db_info.tables[i];
        }
    }
    return NULL;
}

int create_table(char *name, int rec_size) {
    if (name == NULL)
        return FAILURE;
    if (getTableInfo(name) != NULL) // table already exists
        return FAILURE;
    struct tableInfo *temp = realloc(db_info.tables, sizeof(struct tableInfo) * (db_info.num_tables + 1));
    if (temp == NULL)
        return FAILURE;
    db_info.tables = temp;
    struct tableInfo *new_table = &db_info.tables[db_info.num_tables];
    strncpy(new_table->tname, name, 49);
    new_table->tname[49] = '\0';
    snprintf(new_table->ndxname, 50, "%s.ndx", name);
    new_table->rec_size = rec_size;
    new_table->rec_count = 0;
    new_table->status = DB_CLOSE;
    new_table->tfile = NULL;
    new_table->ndxfile = NULL;
    new_table->is_initialized = true;
    for (int i = 0; i < MAX_REC; i++) {
        new_table->ndxArray[i].key = -1;
        new_table->ndxArray[i].is_deleted = false;
    }
    db_info.num_tables++;
    char tname[256];
    snprintf(tname, sizeof(tname), "%s.dat", name);
    FILE *fp = fopen(tname, "wb");
    if (fp == NULL)
        return FAILURE;
    FILE *ndx = fopen(new_table->ndxname, "wb");
    if (ndx == NULL) {
        fclose(fp);
        return FAILURE;
    }
    save_schema();
    int zero = 0;
    fwrite(&zero, sizeof(int), 1, ndx);
    fclose(fp);
    fclose(ndx);
    return SUCCESS;
}

int open_table(char *name) {
    if (name == NULL)
        return FAILURE;
    struct tableInfo *table = getTableInfo(name);
    if (table == NULL)
        return FAILURE;
    if (table->status == DB_OPEN)
        return FAILURE;
    char tname[256];
    snprintf(tname, sizeof(tname), "%s.dat", name);
    table->ndxfile = fopen(table->ndxname, "rb+");
    if (table->ndxfile == NULL) {
        if (rebuild_index(name) != SUCCESS)
            return FAILURE;
        table->ndxfile = fopen(table->ndxname, "rb+");
        if (table->ndxfile == NULL)
            return FAILURE;
    }
    table->tfile = fopen(tname, "rb+");
    if (table->tfile == NULL) {
        fclose(table->ndxfile);
        table->ndxfile = NULL;
        return FAILURE;
    }
    int rec_count = 0;
    fread(&rec_count, sizeof(int), 1, table->ndxfile);
    if (rec_count < 0  || rec_count > MAX_REC) {
        fclose(table->ndxfile);
        table->ndxfile = NULL;
        if (rebuild_index(name) != SUCCESS)
            return FAILURE;
        table->ndxfile = fopen(table->ndxname, "rb+");
        if (table->ndxfile == NULL)
            return FAILURE;
        fread(&rec_count, sizeof(int), 1, table->ndxfile);
        if (rec_count < 0 || rec_count > MAX_REC) {
            fclose(table->ndxfile);
            table->ndxfile = NULL;
            return FAILURE;
        }
    }
    table->rec_count = rec_count;
    fread(table->ndxArray, sizeof(struct RecNdx), table->rec_count, table->ndxfile);
    table->status = DB_OPEN;
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
    // fwrite the record
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

int get_table_non_key(char *name, void *search_value, void *coutput, int (*match_func)(void *rec, void *search_value)) {
    struct tableInfo *table = getTableInfo(name);
    if (table == NULL || table->tfile == NULL)
        return FAILURE;
    void *temp_rec = malloc(table->rec_size);
    if (temp_rec == NULL)
        return FAILURE;
    for (int i = 0; i < table->rec_count; i++)
        if (table->ndxArray[i].is_deleted == false) {
            fseek(table->tfile, table->ndxArray[i].loc + sizeof(int), SEEK_SET);
            fread(temp_rec, table->rec_size, 1, table->tfile);
            if (match_func(temp_rec, search_value) == 0) {
                memcpy(coutput, temp_rec, table->rec_size);
                free(temp_rec);
                return SUCCESS;
            }
        }
    free(temp_rec);
    return REC_NOT_FOUND;
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
            for (int j = 0; j < db_info.num_relations; j++) {
                if (db_info.relations[j].relFile != NULL)
                    if (strcmp(db_info.relations[j].pname, name) == 0 || strcmp(db_info.relations[j].secname, name) == 0)
                        delete_relation(db_info.relations[j].relname, key);
            }
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
    table->status = DB_CLOSE;
    return SUCCESS;
}

int create_relation(char *relname, char *pt, char *st) {
    if (relname == NULL || pt == NULL || st == NULL)
        return FAILURE;
    struct RelInfo *temp = realloc(db_info.relations, sizeof(struct RelInfo) * (db_info.num_relations + 1));
    if (temp == NULL)
        return FAILURE;
    db_info.relations = temp;
    struct RelInfo *new_rel = &db_info.relations[db_info.num_relations];
    strncpy(new_rel->relname, relname, 49);
    strncpy(new_rel->pname, pt, 49);
    strncpy(new_rel->secname, st, 49);
    new_rel->relFile = NULL;
    db_info.num_relations++;
    save_schema();
    char rel_file_name[256];
    snprintf(rel_file_name, sizeof(rel_file_name), "%s.dat", relname);
    FILE *fp = fopen(rel_file_name, "wb");
    if (fp == NULL)
        return FAILURE;
    fwrite(pt, 50, 1, fp);
    fwrite(st, 50, 1, fp);
    fclose(fp);
    return SUCCESS;
}

struct RelInfo* getRelInfo(char *relname) {
    if (relname == NULL || db_info.relations == NULL)
        return NULL;
    for (int i = 0; i < db_info.num_relations; i++)
        if (strcmp(db_info.relations[i].relname, relname) == 0)
            return &db_info.relations[i];
    return NULL;
}

int open_relation(char *rel) {
    struct RelInfo *rel_ptr = getRelInfo(rel);
    if (rel == NULL || rel_ptr == NULL || rel_ptr->relFile != NULL)
        return FAILURE;
    char rel_name[256];
    snprintf(rel_name, sizeof(rel_name), "%s.dat", rel);
    FILE *fp = fopen(rel_name, "rb+");
    if (fp == NULL)
        return FAILURE;
    if (getTableInfo(rel_ptr->pname) == NULL || getTableInfo(rel_ptr->secname) == NULL) {
        fclose(fp);
        return FAILURE;
    }
    rel_ptr->relFile = fp;
    return SUCCESS;
}

int store_relation(char *relname, int pk, int rk) {
    struct RelInfo *rel_ptr = getRelInfo(relname);
    if (rel_ptr == NULL || rel_ptr->relFile == NULL)
        return FAILURE;
    struct tableInfo *pTable = getTableInfo(rel_ptr->pname);
    struct tableInfo *sTable = getTableInfo(rel_ptr->secname);
    if (pTable == NULL || sTable == NULL)
        return FAILURE;
    // check if pk and rk actually exist in ndxArray
    void *temp1 = malloc(pTable->rec_size);
    void *temp2 = malloc(sTable->rec_size);
    int pk_found = get_table(rel_ptr->pname, pk, temp1);
    int rk_found = get_table(rel_ptr->secname, rk, temp2);
    free(temp1); free(temp2);
    if (pk_found != SUCCESS || rk_found != SUCCESS)
        return FAILURE;
    struct RelPair pair = {pk, rk, 0};
    if (fseek(rel_ptr->relFile, 0, SEEK_END) == -1)
        return FAILURE;
    fwrite(&pair, sizeof(struct RelPair), 1, rel_ptr->relFile);
    return SUCCESS;
}

int get_relation(char *relname, void *related_rec, int search_rel_key) {
    struct RelInfo *rel_ptr = getRelInfo(relname);
    if (rel_ptr == NULL || rel_ptr->relFile == NULL)
        return FAILURE;
    if (fseek(rel_ptr->relFile, 100, SEEK_SET) == -1)
        return FAILURE;
    struct RelPair pair;
    bool found = false;
    while (fread(&pair, sizeof(struct RelPair), 1, rel_ptr->relFile)) {
        if (pair.pkey == search_rel_key && pair.is_deleted == 0) {
            found = true;
            break;
        }
    }
    if (!found)
        return REC_NOT_FOUND;
    return get_table(rel_ptr->secname, pair.fkey, related_rec);
}

int delete_relation(char *relname, int key) {
    struct RelInfo *rel_ptr = getRelInfo(relname);
    if (rel_ptr == NULL || rel_ptr->relFile == NULL)
        return FAILURE;
    if (fseek(rel_ptr->relFile, 100, SEEK_SET) == -1)
        return FAILURE;
    struct RelPair pair;
    bool found = false;
    while (fread(&pair, sizeof(struct RelPair), 1, rel_ptr->relFile)) {
        if (pair.is_deleted == 0) {
            if (pair.pkey == key || pair.fkey == key) {
                found = true;
                if (fseek(rel_ptr->relFile, -(long)sizeof(struct RelPair), SEEK_CUR) == -1)
                    return FAILURE;
                pair.is_deleted = 1;
                fwrite(&pair, sizeof(struct RelPair), 1, rel_ptr->relFile);
            }
        }
    }
    if (!found)
        return REC_NOT_FOUND;
    return SUCCESS;
}

int undelete_relation(char *relname, int key) {
    struct RelInfo *rel_ptr = getRelInfo(relname);
    if (rel_ptr == NULL || rel_ptr->relFile == NULL)
        return FAILURE;
    if (fseek(rel_ptr->relFile, 100, SEEK_SET) == -1)
        return FAILURE;
    struct RelPair pair;
    bool found = false;
    while (fread(&pair, sizeof(struct RelPair), 1, rel_ptr->relFile)) {
        if (pair.is_deleted == 1) {
            if (pair.pkey == key || pair.fkey == key) {
                found = true;
                if (fseek(rel_ptr->relFile, -(long)sizeof(struct RelPair), SEEK_CUR) == -1)
                    return FAILURE;
                pair.is_deleted = 0;
                fwrite(&pair, sizeof(struct RelPair), 1, rel_ptr->relFile);
            }
        }
    }
    if (!found)
        return REC_NOT_FOUND;
    return SUCCESS;
}

int close_relation(char *relname) {
    struct RelInfo *rel_ptr = getRelInfo(relname);
    if (relname == NULL || rel_ptr->relFile == NULL)
        return FAILURE;
    fclose(rel_ptr->relFile);
    rel_ptr->relFile = NULL;
    return SUCCESS;
}

int rebuild_index(char *name) {
    struct tableInfo *table = getTableInfo(name);
    if (table == NULL)
        return FAILURE;
    char tname[256], ndxname[256];
    snprintf(tname, sizeof(tname), "%s.dat", name);
    snprintf(ndxname, sizeof(ndxname), "%s.ndx", name);
    FILE *fp = fopen(tname, "rb");
    if (fp == NULL)
        return FAILURE;
    FILE *ndx = fopen(ndxname, "wb");
    if (ndx == NULL) {
        fclose(fp);
        return FAILURE;
    }
    int rec_count = 0; // dummy count for now
    fwrite(&rec_count, sizeof(int), 1, ndx);
    int key;
    while (true) {
        long loc = ftell(fp);
        if (fread(&key, sizeof(int), 1, fp) != 1) // EOF
            break;
        struct RecNdx record;
        record.key = key;
        record.loc = loc;
        record.is_deleted = false; // records deleted in ndxArray will be brought back here
        record.old_key = -1;
        fwrite(&record, sizeof(struct RecNdx), 1, ndx);
        rec_count++;
        fseek(fp, table->rec_size, SEEK_CUR);
    }
    fseek(ndx, 0, SEEK_SET);
    fwrite(&rec_count, sizeof(int), 1, ndx);
    fclose(fp);
    fclose(ndx);
    return SUCCESS;
}