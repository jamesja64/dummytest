#include <stdio.h>
#include <mysql/mysql.h>
#include "config.h"
#include "itri_injpro.h"

int main(int argc, char *argv[])
{
	unsigned int intMOSN;
	unsigned int intShotSN;
	int intRetval;
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	char charCustomSN[TINY_STRING_SIZE];
	MYSQL mysqlCon;

	//deleteShot -MOSN X -ShotSN X Y
	if (argc == 6)
	{
		if (strcmp(argv[1], "-MOSN") == 0)
		{
			intMOSN = atoi(argv[2]);
		}
		else
			return EXIT_FAILURE;

		if (strcmp(argv[3], "-ShotSN") == 0)
		{
			intShotSN = atoi(argv[4]);
			strncpy(charCustomSN, argv[5], TINY_STRING_SIZE);
		}
	}
	else
		return EXIT_FAILURE;

	mysql_init(&mysqlCon);
	if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
	{
		if (mysql_errno(&mysqlCon))
		{
			fprintf(stderr, "Fail to connect to MySql server %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		}
		return EXIT_FAILURE;
	}

	intRetval = DB_UpdateCustomSN(mysqlCon, intMOSN, intShotSN, charCustomSN);

	return intRetval;
}
