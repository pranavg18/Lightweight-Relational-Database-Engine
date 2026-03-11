#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "pds2.h" // Modify this if your filename is different

typedef struct Hospital {
    int hospital_id;
    char name[100];
    char address[200];
    char email[50];
} Hospital;

// FIXED: Added curly braces so it acts as a single block in if-else statements
#define TREPORT(a1,a2) do { printf("Status: %s - %s\n\n", a1, a2); fflush(stdout); } while(0)
void process_line( char *test_case );

// PDS function wrappers mapped to your new parameter order
int pds_create(char* tname1, char* tname2) {
    return db_create(tname1, tname2); 
}

int pds_open(char* tname1, int rec_size1, char* tname2, int rec_size2) { 
    return db_open(tname1, rec_size1, tname2, rec_size2); 
}

int add_record(char* tname, int key, void* record) {
    return store_table(tname, key, record); 
}

int search_record(char* tname, int key, void* record) {
    return get_table(tname, key, record); 
}

int update_record(char* tname, int key, void* record) {
    return update_table(tname, key, record); 
}

int delete_record(char* tname, int key) {
    return delete_table(tname, key); 
}

int undelete_record(char* tname, int key) {
    return undelete_table(tname, key);
}

int pds_close() {
    return db_close(); 
}

// Main
int main(int argc, char *argv[]) {
    FILE *cfptr;
    char test_case[500];

    if( argc != 2 ){
        fprintf(stderr, "Usage: %s testcasefile\n", argv[0]);
        exit(1);
    }

    cfptr = (FILE *) fopen(argv[1], "r");
    if(!cfptr){
        fprintf(stderr, "Error opening file %s\n", argv[1]);
        exit(1);
    }

    while(fgets(test_case, sizeof(test_case)-1, cfptr)){
        if( !strcmp(test_case,"\n") || !strcmp(test_case,"") )
            continue;
        process_line( test_case );
    }
    
    fclose(cfptr);
    return 0;
}

void process_line( char *test_case ) {
    char tname1[30], tname2[30];
    char command[15], param1[15], param2[15], info[1024];
    int hospital_id, status, rec_size, expected_status;
    Hospital testHospital;

    rec_size = sizeof(Hospital);

    sscanf(test_case, "%s", command);
    printf("Test case: %s", test_case); fflush(stdout);

    if( !strcmp(command,"CREATE") ){
        sscanf(test_case, "%s %s %s %s", command, tname1, tname2, param1);
        if( !strcmp(param1,"0") ) expected_status = SUCCESS;
        else expected_status = FAILURE;

        status = pds_create(tname1, tname2);
        
        if( status == expected_status ) TREPORT("PASS", "");
        else {
            sprintf(info,"pds_create returned status %d",status);
            TREPORT("FAIL", info);
        }
    }
    else if( !strcmp(command,"OPEN") ){
        sscanf(test_case, "%s %s %s %s", command, tname1, tname2, param1);
        if( !strcmp(param1,"0") ) expected_status = SUCCESS;
        else expected_status = FAILURE;

        status = pds_open(tname1, rec_size, tname2, rec_size);
    
        if( status == expected_status ) TREPORT("PASS", "");
        else {
            sprintf(info,"pds_open returned status %d",status);
            TREPORT("FAIL", info);
        }
    }
    else if( !strcmp(command, "STORE") ){
        sscanf(test_case, "%s %s %s %s", command, tname1, param1, param2);
        if( !strcmp(param2,"0") ) expected_status = SUCCESS;
        else expected_status = FAILURE;

        sscanf(param1, "%d", &hospital_id);
        testHospital.hospital_id = hospital_id;
        
        char name[100], address[200], email[50];
        sscanf(test_case, "%*s %*s %*s %*s %s %s %s", name, address, email);

        strcpy(testHospital.name, name);
        strcpy(testHospital.address, address);
        strcpy(testHospital.email, email);
        
        status = add_record(tname1, hospital_id, &testHospital);
        
        if( status == expected_status ) TREPORT("PASS", "");
        else {
            sprintf(info,"add_record returned status %d",status);
            TREPORT("FAIL", info);
        }
    }
    else if( !strcmp(command,"SEARCH") ){
        sscanf(test_case, "%s %s %s %s", command, tname1, param1, param2);
        
        char expected_name[100], expected_address[200], expected_email[50];
        
        if( !strcmp(param2,"0") ){
            sscanf(test_case, "%*s %*s %*s %*s %s %s %s", expected_name, expected_address, expected_email);
            expected_status = SUCCESS;
        } else {
            expected_status = REC_NOT_FOUND;
        }

        sscanf(param1, "%d", &hospital_id);
        status = search_record(tname1, hospital_id, &testHospital);
        
        if( status != expected_status ){
            sprintf(info,"search key: %d; Got status %d",hospital_id, status);
            TREPORT("FAIL", info);
        } else {
            if( expected_status == SUCCESS ){
                if (testHospital.hospital_id == hospital_id && 
                    strcmp(testHospital.name, expected_name) == 0 &&
                    strcmp(testHospital.address, expected_address) == 0 &&
                    strcmp(testHospital.email, expected_email) == 0) {
                        TREPORT("PASS", "");
                } else {
                    sprintf(info,"Data mismatch... Expected:{%d,%s,%s,%s} Got:{%d,%s,%s,%s}\n",
                        hospital_id, expected_name, expected_address, expected_email,
                        testHospital.hospital_id, testHospital.name, testHospital.address, testHospital.email);
                    TREPORT("FAIL", info);
                }
            } else {
                TREPORT("PASS", "");
            }
        }
    }
    else if( !strcmp(command,"UPDATE") ){
        sscanf(test_case, "%s %s %s %s", command, tname1, param1, param2);
        if( !strcmp(param2,"0") ) expected_status = SUCCESS;
        else expected_status = FAILURE;

        sscanf(param1, "%d", &hospital_id);
        testHospital.hospital_id = hospital_id;

        char name[100], address[200], email[50];
        sscanf(test_case, "%*s %*s %*s %*s %s %s %s", name, address, email);

        strcpy(testHospital.name, name);
        strcpy(testHospital.address, address);
        strcpy(testHospital.email, email);
        
        status = update_record(tname1, hospital_id, &testHospital);
        
        if( status == expected_status ) TREPORT("PASS", "");
        else {
            sprintf(info,"update_record returned status %d",status);
            TREPORT("FAIL", info);
        }
    }
    else if(!strcmp(command,"DELETE") ){
        sscanf(test_case, "%s %s %s %s", command, tname1, param1, param2);
        if( !strcmp(param2,"0") ) expected_status = SUCCESS;
        else expected_status = FAILURE;

        sscanf(param1, "%d", &hospital_id);
        status = delete_record(tname1, hospital_id);
        
        if( status == expected_status ) TREPORT("PASS", "");
        else {
            sprintf(info,"delete_record returned status %d",status);
            TREPORT("FAIL", info);
        }
    }
    else if(!strcmp(command,"UNDELETE") ){
        sscanf(test_case, "%s %s %s %s", command, tname1, param1, param2);
        if( !strcmp(param2,"0") ) expected_status = SUCCESS;
        else expected_status = FAILURE;

        sscanf(param1, "%d", &hospital_id);
        status = undelete_record(tname1, hospital_id);
        
        if( status == expected_status ) TREPORT("PASS", "");
        else {
            sprintf(info,"undelete_record returned status %d",status);
            TREPORT("FAIL", info);
        }
    }
    else if( !strcmp(command,"CLOSE") ){
        sscanf(test_case, "%s %s", command, param1);
        if( !strcmp(param1,"0") ) expected_status = SUCCESS;
        else expected_status = FAILURE;

        status = pds_close();

        if( status == expected_status ) TREPORT("PASS", "");
        else {
            sprintf(info,"pds_close returned status %d",status);
            TREPORT("FAIL", info);
        }
    }
}
