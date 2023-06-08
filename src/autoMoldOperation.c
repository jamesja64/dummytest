#include <stdio.h>
#include <mysql/mysql.h>
#include <signal.h>
#include <unistd.h>
//#include "config.h"
#include "itri_injpro.h"

bool boolIsStopSignal = false;

static void handler_AutoMoldOperation(int intSign)
{
	boolIsStopSignal = true;
}

int main(int argc, char *argv[])
{
	unsigned int intIMMSN;
	int intRetval;
	MYSQL mysqlCon;

	signal(SIGINT, handler_AutoMoldOperation);
	signal(SIGTERM, handler_AutoMoldOperation);

	if ((argc == 3) && (strcmp(argv[1], "-IMMSN") == 0))
		intIMMSN = atoi(argv[2]);
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

	intRetval = OPCUA_WriteOPCUANodeValue(mysqlCon, intIMMSN, DO_MOLD_RELEASED, "1");
	if (intRetval != EXIT_SUCCESS)
	{
		fprintf(stderr, "Fail to initialize OPC UA server to Mold Released\n");
		return EXIT_FAILURE;
	}

	while (boolIsStopSignal == false)
	{
		OPCUA_WriteOPCUANodeValue(mysqlCon, intIMMSN, DO_MOLD_RELEASED, "1");
		fprintf(stderr, "Set IMMSN:%d as Mold Released.\n", intIMMSN);
		sleep(3);
		OPCUA_WriteOPCUANodeValue(mysqlCon, intIMMSN, DO_MOLD_RELEASED, "0");
		fprintf(stderr, "Set IMMSN:%d as Mold Clamping.\n", intIMMSN);
		sleep(3);
		OPCUA_WriteOPCUANodeValue(mysqlCon, intIMMSN, DO_MOLD_CLAMPED, "1");
		fprintf(stderr, "Set IMMSN:%d as Mold Clamped.\n", intIMMSN);
		sleep(3);
		OPCUA_WriteOPCUANodeValue(mysqlCon, intIMMSN, DO_MOLD_CLAMPED, "0");
		fprintf(stderr, "Set IMMSN:%d as Mold Releasing.\n", intIMMSN);
		sleep(3);
	}
	return EXIT_SUCCESS;
}
