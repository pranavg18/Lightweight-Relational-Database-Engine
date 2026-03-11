#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pds3.h"

typedef struct Hospital {
    int hospital_id;
    char name[100];
    char address[200];
    char email[50];
} Hospital;

#define TREPORT(a1,a2) do { printf("Status: %s - %s\n\n", a1, a2); fflush(stdout); } while(0)

void process_line(char *test_case);

int main(int argc, char *argv[]) {
    FILE *cfptr;
    char test_case[500];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s testcasefile\n", argv[0]);
        exit(1);
    }

    cfptr = fopen(argv[1], "r");
    if (!cfptr) {
        fprintf(stderr, "Error opening file %s\n", argv[1]);
        exit(1);
    }

    while (fgets(test_case, sizeof(test_case) - 1, cfptr)) {
        if (!strcmp(test_case, "\n") || !strcmp(test_case, ""))
            continue;
        process_line(test_case);
    }

    fclose(cfptr);
    return 0;
}

void process_line(char *test_case) {
    char command[20], tname1[50], tname2[50], tname3[50], info[1024];
    char name[100], address[200], email[50];
    int key1, key2, expected_status, status;
    Hospital testHospital;

    sscanf(test_case, "%s", command);
    printf("Test case: %s", test_case); fflush(stdout);

    if (!strcmp(command, "CREATE")) {
        sscanf(test_case, "%*s %s %s %d", tname1, tname2, &expected_status);
        status = db_create(tname1, tname2);
        if (status == expected_status) TREPORT("PASS", "");
        else { sprintf(info, "db_create returned %d", status); TREPORT("FAIL", info); }
    }
    else if (!strcmp(command, "OPEN")) {
        sscanf(test_case, "%*s %s %s %d", tname1, tname2, &expected_status);
        status = db_open(tname1, sizeof(Hospital), tname2, sizeof(Hospital));
        if (status == expected_status) TREPORT("PASS", "");
        else { sprintf(info, "db_open returned %d", status); TREPORT("FAIL", info); }
    }
    else if (!strcmp(command, "STORE")) {
        sscanf(test_case, "%*s %s %d %d %s %s %s", tname1, &key1, &expected_status, name, address, email);
        testHospital.hospital_id = key1;
        strcpy(testHospital.name, name);
        strcpy(testHospital.address, address);
        strcpy(testHospital.email, email);
        status = store_table(tname1, key1, &testHospital);
        if (status == expected_status) TREPORT("PASS", "");
        else { sprintf(info, "store_table returned %d", status); TREPORT("FAIL", info); }
    }
    else if (!strcmp(command, "SEARCH")) {
        sscanf(test_case, "%*s %s %d %d", tname1, &key1, &expected_status);
        status = get_table(tname1, key1, &testHospital);
        if (status != expected_status) { sprintf(info, "search returned %d", status); TREPORT("FAIL", info); }
        else if (status == SUCCESS) {
            sscanf(test_case, "%*s %*s %*d %*d %s %s %s", name, address, email);
            if (testHospital.hospital_id == key1 && !strcmp(testHospital.name, name)) TREPORT("PASS", "");
            else TREPORT("FAIL", "Data mismatch");
        }
        else TREPORT("PASS", "");
    }
    else if (!strcmp(command, "UPDATE")) {
        sscanf(test_case, "%*s %s %d %d %s %s %s", tname1, &key1, &expected_status, name, address, email);
        testHospital.hospital_id = key1;
        strcpy(testHospital.name, name);
        strcpy(testHospital.address, address);
        strcpy(testHospital.email, email);
        status = update_table(tname1, key1, &testHospital);
        if (status == expected_status) TREPORT("PASS", "");
        else { sprintf(info, "update_table returned %d", status); TREPORT("FAIL", info); }
    }
    else if (!strcmp(command, "DELETE")) {
        sscanf(test_case, "%*s %s %d %d", tname1, &key1, &expected_status);
        status = delete_table(tname1, key1);
        if (status == expected_status) TREPORT("PASS", "");
        else { 
            sprintf(info, "delete_table returned %d", status); 
            TREPORT("FAIL", info); 
        }
    }
    else if (!strcmp(command, "UNDELETE")) {
        sscanf(test_case, "%*s %s %d %d", tname1, &key1, &expected_status);
        status = undelete_table(tname1, key1);
        if (status == expected_status) TREPORT("PASS", "");
        else { 
            sprintf(info, "undelete_table returned %d", status); 
            TREPORT("FAIL", info); 
        }
    }
    else if (!strcmp(command, "REL_CREATE")) {
        sscanf(test_case, "%*s %s %s %s %d", tname1, tname2, tname3, &expected_status);
        status = create_relation(tname1, tname2, tname3);
        if (status == expected_status) TREPORT("PASS", "");
        else TREPORT("FAIL", "create_relation failed");
    }
    else if (!strcmp(command, "REL_OPEN")) {
        sscanf(test_case, "%*s %s %d", tname1, &expected_status);
        status = open_relation(tname1);
        if (status == expected_status) TREPORT("PASS", "");
        else TREPORT("FAIL", "open_relation failed");
    }
    else if (!strcmp(command, "REL_STORE")) {
        sscanf(test_case, "%*s %d %d %d", &key1, &key2, &expected_status);
        status = store_relation(key1, key2);
        if (status == expected_status) TREPORT("PASS", "");
        else TREPORT("FAIL", "store_relation failed");
    }
    else if (!strcmp(command, "REL_SEARCH")) {
        sscanf(test_case, "%*s %d %d", &key1, &expected_status);
        status = get_relation(&testHospital, key1);
        if (status != expected_status) { 
            sprintf(info, "get_relation returned %d", status); 
            TREPORT("FAIL", info); 
        }
        else if (status == SUCCESS) {
            int expected_fkey;
            char temp_name[100], temp_addr[200], temp_email[50];
            sscanf(test_case, "%*s %*d %*d %d %s %s %s", &expected_fkey, temp_name, temp_addr, temp_email);
            if (!strcmp(testHospital.name, temp_name) && testHospital.hospital_id == expected_fkey) TREPORT("PASS", "");
            else TREPORT("FAIL", "Relational data mismatch");
        }
        else TREPORT("PASS", "");
    }
    else if (!strcmp(command, "REL_DELETE")) {
        sscanf(test_case, "%*s %d %s %d", &key1, tname1, &expected_status);
        status = delete_relation(key1, tname1);
        if (status == expected_status) TREPORT("PASS", "");
        else { 
            sprintf(info, "delete_relation returned %d", status); 
            TREPORT("FAIL", info); 
        }
    }
    else if (!strcmp(command, "REL_UNDELETE")) {
        sscanf(test_case, "%*s %d %s %d", &key1, tname1, &expected_status);
        status = undelete_relation(key1, tname1);
        if (status == expected_status) TREPORT("PASS", "");
        else { 
            sprintf(info, "undelete_relation returned %d", status); 
            TREPORT("FAIL", info); 
        }
    }
    else if (!strcmp(command, "REL_CLOSE")) {
        sscanf(test_case, "%*s %d", &expected_status);
        status = close_relation();
        if (status == expected_status) TREPORT("PASS", "");
        else TREPORT("FAIL", "close_relation failed");
    }
    else if (!strcmp(command, "CLOSE")) {
        sscanf(test_case, "%*s %d", &expected_status);
        status = db_close();
        if (status == expected_status) TREPORT("PASS", "");
        else TREPORT("FAIL", "db_close failed");
    }
}