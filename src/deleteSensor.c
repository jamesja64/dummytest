#include <stdio.h>
#include <mysql/mysql.h>
#include "config.h"
#include "itri_injpro.h"

int main(int argc, char *argv[])
{
	unsigned int	intCutomSensorModelSN;
	int		intRetval;
	MYSQL		mysqlCon;

	if(argc==2)
		intCutomSensorModelSN=atoi(argv[1]);
	else
		return EXIT_FAILURE;

	mysql_init(&mysqlCon);
	if(!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD,NULL,0,NULL,CLIENT_FOUND_ROWS))
	{
		if (mysql_errno(&mysqlCon)){
			fprintf(stderr, "Fail to connect to MySql server %d: %s\n",mysql_errno(&mysqlCon),mysql_error(&mysqlCon));
		}
		return EXIT_FAILURE;
	}
	intRetval=INDEX_DeleteCustomSensorModel(mysqlCon,intCutomSensorModelSN);
	return intRetval;
}

