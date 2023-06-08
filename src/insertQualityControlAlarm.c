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

	//insertQualityControlAlarm -MOSN [MOSN]
	if (argc == 6)
	{
		if (strcmp(argv[1], "-MOSN") == 0)
		{
			intMOSN = atoi(argv[2]);
		}
		else
		{
			fprintf(stderr, "Format Error:insertQualityControlAlarm -MOSN [MOSN]\n");
			return EXIT_FAILURE;
		}

		if (strcmp(argv[3], "-ShotSN") == 0)
		{
			intShotSN_Start = atoi(argv[4]);
			intShotSN_End = atoi(argv[5]);
		}
		else
		{
			fprintf(stderr, "Format Error:insertQualityControlAlarm -MOSN [MOSN]\n");
			return EXIT_FAILURE;
		}
	}

	fprintf(stderr, "Insert SPC Quality Control Alarm  for MOSN:%d \n", intMOSN);

	mysql_init(&mysqlCon);
	if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
	{
		if (mysql_errno(&mysqlCon))
		{
			fprintf(stderr, "Fail to connect to MySql server %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		}
		return EXIT_FAILURE;
	}

	for (int i = intShotSN_Start; i <= intShotSN_End; i += 1){
		//fprintf(stderr,"SPC Quality Control Alarm ShotSN:%d\n",i);
		intRetval = QC_InsertQCAlarm(mysqlCon, intMOSN, i);
		if (intRetval != EXIT_SUCCESS)
		{
			fprintf(stderr, "Fail to insert SPC Quality Control Alarm  for MOSN:%d ShotSN:%d\n", intMOSN,i);
		}
	}
	mysql_close(&mysqlCon);
	return intRetval;
}
