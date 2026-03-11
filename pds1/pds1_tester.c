#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "pds.h" // Modify this if your filename is different

typedef struct Hospital {
	int hospital_id;
	char name[100];
	char address[200];
	char email[50];
} Hospital;

#define TREPORT(a1,a2) printf("Status: %s - %s\n\n", a1, a2); fflush(stdout);

void process_line( char *test_case );

// PDS function calls
int pds_create(char* repo_name)
{
	return create_db(repo_name); // Replace with your function 
}

int pds_open(char* repo_name, int rec_size)
{ 
	return open_db(repo_name, rec_size); // Replace with your function 
}

int add_record(int key, void* record)
{
	return store_db(key, record); // Replace with your function 
}

int search_record(int key, void* record)
{
	return get_db(key, record); // Replace with your function 
}

int update_record(int key, void* record)
{
	return update_db(key, record); // Replace with your function 
}

int delete_record(int key)
{
	return delete_db(key); // Replace with your function 
}

int pds_close()
{
	return close_db(); // Replace with your function 
}


// Main
int main(int argc, char *argv[])
{
	FILE *cfptr;
	char test_case[500];

	if( argc != 2 ){
		fprintf(stderr, "Usage: %s testcasefile\n", argv[0]);
		exit(1);
	}

	cfptr = (FILE *) fopen(argv[1], "r");
	while(fgets(test_case, sizeof(test_case)-1, cfptr)){
		// printf("line:%s",test_case);
		if( !strcmp(test_case,"\n") || !strcmp(test_case,"") )
			continue;
		process_line( test_case );
	}
}

void process_line( char *test_case )
{
	char dbname[30];
	char command[15], param1[15], param2[15], info[1024];
	int hospital_id, status, rec_size, expected_status;
	Hospital testHospital;

	rec_size = sizeof(Hospital);

	sscanf(test_case, "%s%s%s", command, param1, param2);
	printf("Test case: %s", test_case); fflush(stdout);

	if( !strcmp(command,"CREATE") ){
		strcpy(dbname, param1);
		if( !strcmp(param2,"0") )
			expected_status = SUCCESS;
		else
			expected_status = FAILURE;

		status = pds_create(dbname);
		
		if( status == expected_status ){
			TREPORT("PASS", "");
		}
		else{
			sprintf(info,"pds_create returned status %d",status);
			TREPORT("FAIL", info);
		}
	}
	else if( !strcmp(command,"OPEN") ){
		strcpy(dbname, param1);
		if( !strcmp(param2,"0") )
			expected_status = SUCCESS;
		else
			expected_status = FAILURE;

		status = pds_open(dbname, rec_size) ;
	
		if( status == expected_status ){
			TREPORT("PASS", "");
		}
		else{
			sprintf(info,"pds_open returned status %d",status);
			TREPORT("FAIL", info);
		}
	}
	else if( !strcmp(command, "STORE") ){
		if( !strcmp(param2,"0") )
			expected_status = SUCCESS;
		else
			expected_status = FAILURE;

		sscanf(param1, "%d", &hospital_id);
		testHospital.hospital_id = hospital_id;

		char name[100];
		char address[200];
		char email[50];
		sscanf(test_case, "%s%s%s%s%s%s", command, param1, param2, name, address, email);

		strcpy(testHospital.name, name);
		strcpy(testHospital.address, address);
		strcpy(testHospital.email, email);
		
		status = add_record(hospital_id, &testHospital);
		
		if( status == expected_status ){
			TREPORT("PASS", "");
		}
		else{
			sprintf(info,"add_record returned status %d",status);
			TREPORT("FAIL", info);
		}
	}
	else if( !strcmp(command,"SEARCH") ){
		char expected_name[100];
		char expected_address[200];
		char expected_email[50];
		if( !strcmp(param2,"0") )
		{
			sscanf(test_case, "%s%s%s%s%s%s", command, param1, param2, expected_name, expected_address, expected_email);
			expected_status = SUCCESS;
		}
		else
			expected_status = REC_NOT_FOUND;

		sscanf(param1, "%d", &hospital_id);
		testHospital.hospital_id =  -1;
		
		status = search_record(hospital_id, &testHospital);
		
		if( status != expected_status ){
			sprintf(info,"search key: %d; Got status %d",hospital_id, status);
			TREPORT("FAIL", info);
		}
		else{
			// Check if the retrieved values match
			if( expected_status == SUCCESS ){
				if (testHospital.hospital_id == hospital_id && 
					strcmp(testHospital.name, expected_name) == 0 &&
					strcmp(testHospital.address, expected_address) == 0 &&
					strcmp(testHospital.email, expected_email) == 0
				){
						TREPORT("PASS", "");
				}
				else{
					sprintf(info,"Hospital data not matching... Expected:{%d,%s,%s,%s} Got:{%d,%s,%s,%s}\n",
						hospital_id, expected_name, expected_address, expected_email,
						testHospital.hospital_id, testHospital.name, testHospital.address, testHospital.email
					);
					TREPORT("FAIL", info);
				}
			}
			else
				TREPORT("PASS", "");
		}
	}
	else if( !strcmp(command,"UPDATE") ){
		if( !strcmp(param2,"0") )
			expected_status = SUCCESS;
		else
			expected_status = FAILURE;

		sscanf(param1, "%d", &hospital_id);
		testHospital.hospital_id = hospital_id;

		char name[100];
		char address[200];
		char email[50];

		sscanf(test_case, "%s%s%s%s%s%s", command, param1, param2, name, address, email);

		strcpy(testHospital.name, name);
		strcpy(testHospital.address, address);
		strcpy(testHospital.email, email);
		
		status = update_record(hospital_id, &testHospital);
		
		if( status == expected_status ){
			TREPORT("PASS", "");
		}
		else{
			sprintf(info,"update_record returned status %d",status);
			TREPORT("FAIL", info);
		}
	}
	else if(!strcmp(command,"DELETE") ){
		if( !strcmp(param2,"0") )
			expected_status = SUCCESS;
		else
			expected_status = FAILURE;

		sscanf(param1, "%d", &hospital_id);

		status = delete_record(hospital_id);
		
		if( status == expected_status ){
			TREPORT("PASS", "");
		}
		else{
			sprintf(info,"delete_record returned status %d",status);
			TREPORT("FAIL", info);
		}
	}
	else if( !strcmp(command,"CLOSE") ){
		if( !strcmp(param1,"0") )
			expected_status = SUCCESS;
		else
			expected_status = FAILURE;

		status = pds_close();

		if( status == expected_status ){
			TREPORT("PASS", "");
		}
		else{
			sprintf(info,"pds_close returned status %d",status);
			TREPORT("FAIL", info);
		}
	}
}


