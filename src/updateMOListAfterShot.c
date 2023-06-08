#include <stdio.h>
#include <mysql/mysql.h>
#include "config.h"
#include "itri_injpro.h"

int main(int argc, char *argv[])
{
	unsigned int intMOSN;
	unsigned int intShotSN;
	unsigned int intMOAcceptCriteriaSN;
	char charPredictValue[TINY_STRING_SIZE];
	MYSQL mysqlCon;
	unsigned int intRetval;
	bool boolUseActual;

	//updateMOSNListAfterShot -MOSN [MOSN]
	if (argc == 3)
	{
		if (strcmp(argv[1], "-MOSN") == 0)
		{
			intMOSN = atoi(argv[2]);
		}
		else
		{
			fprintf(stderr, "Format Error:updateMOSNListAfterShot -MOSN [MOSN]\n");
			return EXIT_FAILURE;
		}
	}

	//fprintf(stderr, "Update MOSNList After Shot for MOSN:%d \n", intMOSN);

	mysql_init(&mysqlCon);
	if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
	{
		if (mysql_errno(&mysqlCon))
		{
			fprintf(stderr, "Fail to connect to MySql server %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		}
		return EXIT_FAILURE;
	}
	boolUseActual = true;
	intRetval = MO_UpdateMOSNListAfterShot(mysqlCon, intMOSN, boolUseActual);
	if (intRetval != EXIT_SUCCESS)
	{
		fprintf(stderr, "Fail to update MOSNList:%d after shot\n", intMOSN);
	}
	mysql_close(&mysqlCon);
	return intRetval;
}
