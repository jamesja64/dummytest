#include <stdio.h>
#include <mysql/mysql.h>
#include "config.h"
#include "itri_injpro.h"

int main(int argc, char *argv[])
{

	unsigned int	intIMMSN;
	unsigned int	intUserID=70012;
	int		intRetval;
	MYSQL		mysqlCon;
	char		charIMMParaValue[OPCUA_IMMPARA_NODE_NUM+1][TINY_STRING_SIZE];



        //exeTuneIMM -IMMSN X
        if(argc==3){
                if(strcmp(argv[1],"-IMMSN")==0){
                        intIMMSN=atoi(argv[2]);
                }else
                        return EXIT_FAILURE;
        }

	//Connect to MySql Server
	mysql_init(&mysqlCon);
	if(!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD,NULL,0,NULL,CLIENT_FOUND_ROWS))
	{
		if (mysql_errno(&mysqlCon)){
			fprintf(stderr, "Fail to connect to MySql server %d: %s\n",mysql_errno(&mysqlCon),mysql_error(&mysqlCon));
		}
		return EXIT_FAILURE;
	}

	fprintf(stderr,"IMMSN:%d Tunning IMM parameter\n",intIMMSN);
	DEMO_SetIMMParaValue(charIMMParaValue);
	IMM_TuneIMMPara(mysqlCon,intIMMSN,intUserID,charIMMParaValue);
	mysql_close(&mysqlCon);
	return intRetval;
}

