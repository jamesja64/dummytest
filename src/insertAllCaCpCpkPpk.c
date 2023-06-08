#include <stdio.h>
#include <mysql/mysql.h>
#include "config.h"
#include "itri_injpro.h"

int main(int argc, char *argv[])
{
	unsigned int intMOSN;
	unsigned int intShotSN;
	unsigned int intShotSN_Start;
	unsigned int intShotSN_End;
	char charPredictValue[TINY_STRING_SIZE];
	MYSQL mysqlCon;
	unsigned int intRetval;
	bool boolUseActual;

	//InsertAllCaCpCpkPpk -MOSN [MOSN]
	if (argc == 6)
	{
		if (strcmp(argv[1], "-MOSN") == 0)
		{
			intMOSN = atoi(argv[2]);
		}
		else
		{
			fprintf(stderr, "Format Error:insertAllCaCpCpkPpk -MOSN [MOSN]\n");
			return EXIT_FAILURE;
		}

		if (strcmp(argv[3], "-ShotSN") == 0)
		{
			intShotSN_Start = atoi(argv[4]);
			intShotSN_End = atoi(argv[5]);
		}
		else
		{
			fprintf(stderr, "Format Error:insertAllCaCpCpkPpk -MOSN [MOSN]\n");
			return EXIT_FAILURE;
		}
	}

	fprintf(stderr, "Insert All Ca,Cp,Cpk,Ppk for MOSN:%d From Shot:%d-%d\n", intMOSN, intShotSN_Start, intShotSN_End);

	mysql_init(&mysqlCon);
	if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
	{
		if (mysql_errno(&mysqlCon))
		{
			fprintf(stderr, "Fail to connect to MySql server %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		}
		return EXIT_FAILURE;
	}

	intRetval = DB_InsertAllCaCpCpkPpk(mysqlCon, intMOSN, intShotSN_Start, intShotSN_End);
	if (intRetval != EXIT_SUCCESS)
	{
		fprintf(stderr, "Fail to insert Ca,Cp,Cpk and Ppk for MOSN:%d ShotSN:%d-%d\n", intMOSN, intShotSN_Start, intShotSN_End);
	}

	mysql_close(&mysqlCon);
	return intRetval;
}
