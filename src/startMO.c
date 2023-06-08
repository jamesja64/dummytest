#include <stdio.h>
#include <mysql/mysql.h>
#include "config.h"
#include "itri_injpro.h"

int main(int argc, char *argv[])
{
	unsigned int	intMOSN,intIMMSN;
	int		intRetval;
	MYSQL		mysqlCon;


	//deleteMO -MOSN X -IMMSN Y
        if(argc==5){
                if(strcmp(argv[1],"-MOSN")==0){
                        intMOSN=atoi(argv[2]);
                }else
                        return EXIT_FAILURE;
                if(strcmp(argv[3],"-IMMSN")==0){
                        intIMMSN=atoi(argv[4]);
                }else
                        return EXIT_FAILURE;
        }

	mysql_init(&mysqlCon);
	if(!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD,NULL,0,NULL,CLIENT_FOUND_ROWS))
	{
		if (mysql_errno(&mysqlCon)){
			fprintf(stderr, "Fail to connect to MySql server %d: %s\n",mysql_errno(&mysqlCon),mysql_error(&mysqlCon));
		}
		return EXIT_FAILURE;
	}
	intRetval=MO_StartMOonIMM(mysqlCon,intMOSN,intIMMSN);
	return intRetval;
}

