#include <stdio.h>
#include <mysql/mysql.h>
#include "config.h"
#include "itri_injpro.h"

int main(int argc, char *argv[])
{
	unsigned int intMOSN;
	unsigned int intShotSN_Start, intShotSN_End;
	unsigned int intMOCustomSensorSN;
	int intRetval;
	double *doubleSensorData;
	unsigned int intSensorDataSize;
	MYSQL mysqlCon;
	char charIMMSysPressureSensorFeatureValue[IMM_SYSPRESSURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE];

	doubleSensorData = NULL;

	//exeInsertCavityPressureFeatureValue -MOSN [2] -ShotSN [4]  [5] -MOCustomSensorSN [7]
	if (argc == 8)
	{
		if (strcmp(argv[1], "-MOSN") == 0)
		{
			intMOSN = atoi(argv[2]);
		}
		else
			return EXIT_FAILURE;
		if (strcmp(argv[3], "-ShotSN") == 0)
		{
			intShotSN_Start = atoi(argv[4]);
			intShotSN_End = atoi(argv[5]);
		}
		else
			return EXIT_FAILURE;
		if (strcmp(argv[6], "-MOCustomSensorSN") == 0)
		{
			intMOCustomSensorSN = atoi(argv[7]);
		}
	}

	fprintf(stderr, "MOSN:%d ShotSN:%d %d MOCustomSensorSN:%d\n", intMOSN, intShotSN_Start, intShotSN_End, intMOCustomSensorSN);
	mysql_init(&mysqlCon);
	if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
	{
		if (mysql_errno(&mysqlCon))
		{
			fprintf(stderr, "Fail to connect to MySql server %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		}
		return EXIT_FAILURE;
	}

	for (int i = intShotSN_Start; i <= intShotSN_End; i++)
	{
		fprintf(stderr, "ShotSN:%d\n", i);
		intRetval = DB_SelectMOCustomSensorRawData(mysqlCon, intMOSN, i, intMOCustomSensorSN, &doubleSensorData, &intSensorDataSize);

		if (intRetval != EXIT_SUCCESS)
		{
			fprintf(stderr, "Fail to select IMM system pressure data for shotsn:%d\n", i);
			//continue;
			//return EXIT_FAILURE;
		}

		//fprintf(stderr,"Data[0]:%lf\n",doubleSensorData[0]);

		intRetval = DB_GetIMMSysPressureSensorFeatureValue(mysqlCon, intMOSN, doubleSensorData, intSensorDataSize, charIMMSysPressureSensorFeatureValue);
		if (intRetval != EXIT_SUCCESS)
		{

			fprintf(stderr, "Fail to caculate IMM system pressure feature value for shotsn:%d\n", i);
			//free(doubleSensorData);
			continue;
			//return EXIT_FAILURE;
		}
		//free(doubleSensorData);

		intRetval = DB_InsertIMMSysPressureSensorFeatureValue(mysqlCon, intMOSN, i, intMOCustomSensorSN, charIMMSysPressureSensorFeatureValue);
		if (intRetval != EXIT_SUCCESS)
		{
			fprintf(stderr, "Fail to insert to MySql server %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
			continue;
			//return EXIT_FAILURE;
		}
	}

	mysql_close(&mysqlCon);
	return intRetval;
}
