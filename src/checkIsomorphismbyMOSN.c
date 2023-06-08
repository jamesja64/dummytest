#include <stdio.h>
#include <mysql/mysql.h>
#include "config.h"
#include "itri_injpro.h"

int main(int argc, char *argv[])
{
	unsigned int intMOSN1, intMOSN2;

	MYSQL mysqlCon;
	unsigned int intRetval;
	bool boolIsomorphism;

	//updateMOSNListAfterShot -MOSN [MOSN]
	if (argc == 4)
	{
		if (strcmp(argv[1], "-MOSN") == 0)
		{
			intMOSN1 = atoi(argv[2]);
			intMOSN2 = atoi(argv[3]);
		}
		else
		{
			fprintf(stderr, "Format Error:checkIsomorphismbyMOSN -MOSN [MOSN] [MOSN]\n");
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

	intRetval = DB_CheckIsomorphismbyMOSN(mysqlCon, intMOSN1, intMOSN2, &boolIsomorphism);
	if (intRetval != EXIT_SUCCESS)
	{
		fprintf(stderr, "Fail to check Isomorphism of MOSN1:%d and MOSN2:%d\n", intMOSN1, intMOSN2);
	}
	mysql_close(&mysqlCon);
	return intRetval;
}
