#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <mysql/mysql.h>
#include "config.h"
#include "itri_injpro.h"

int main(int argc, char *argv[])
{
	MYSQL	mysqlCon;
	char	charStatement[MAX_STRING_SIZE];
	char	charErrMsg[MEDIUM_STRING_SIZE];
	int	intMOSN;
	int	intIMMSN;
	int	intRetval;
	int	intOPCUAClientPID;


	//Connect to MYSQL Server
	mysql_init(&mysqlCon);
	if(!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD,NULL,0,NULL,CLIENT_FOUND_ROWS)){
		fprintf(stderr, "MYSQL connection failed.\n");
		if (mysql_errno(&mysqlCon)){
			fprintf(stderr, "MYSQL connection error %d: %s\n",mysql_errno(&mysqlCon),mysql_error(&mysqlCon));
		}
		return EXIT_FAILURE;
	}

	//Get IMMSN from Parameters
	if(argc==3 && strcmp(argv[1],"-IMMSN")==0){
		intIMMSN=atoi(argv[2]);
	}else{
		snprintf(charErrMsg,MEDIUM_STRING_SIZE,"Fail to unsubscribe IMM due to parameter format error");
		SYS_InsertSysErrMsg(mysqlCon,ERRCLASS_MYSQL,0,intIMMSN,EXIT_FAILURE,charErrMsg);
		return EXIT_FAILURE;
	}

	//Test
	IMM_UnsubscribeOPCUA(mysqlCon,intIMMSN);
	mysql_close(&mysqlCon);
	return EXIT_SUCCESS;
}
