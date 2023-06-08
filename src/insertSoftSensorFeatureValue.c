#include <stdio.h>
#include <mysql/mysql.h>
#include "config.h"
#include "itri_injpro.h"

int main(int argc, char *argv[])
{
	unsigned int intMOSN;
	unsigned int intShotSN_Start, intShotSN_End;
	unsigned int intNGCustomSensorSN, intFGCustomSensorSN;
	int intRetval;
	double *doubleSensorData;
	unsigned int intSensorDataSize;
	MYSQL mysqlCon;
	char charNGCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE];
	char charFGCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE];

	doubleSensorData=NULL;

	//exeInsertSPCFeatureValue -MOSN [1] -ShotSN [2] [3] -CustomSensorSN [4] [5]
	if (argc == 9)
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
			intNGCustomSensorSN = atoi(argv[7]);
			intFGCustomSensorSN = atoi(argv[8]);
		}
	}
	else
	{
		fprintf(stderr, "Parameter format error! exeInsertSPCFeatureValue -MOSN [1] -ShotSN [2] [3] -CustomSensorSN [4] [5]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "MOSN:%d ShotSN:%d %d CustomSensorSN:%d %d\n", intMOSN, intShotSN_Start, intShotSN_End, intNGCustomSensorSN, intFGCustomSensorSN);
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
		intRetval = DB_SelectMOCustomSensorRawData(mysqlCon, intMOSN, i, intNGCustomSensorSN, &doubleSensorData, &intSensorDataSize);
		if (intRetval != EXIT_SUCCESS)
		{
			fprintf(stderr, "Fail to select mold pressure sensor data for ShotSN:%d CustomSensorSN:%d\n", i, intNGCustomSensorSN);
			//free(doubleSensorData);
			continue;
			//return EXIT_FAILURE;
		}
		intRetval = DB_GetCavityPressureSensorFeatureValue(mysqlCon, intMOSN, doubleSensorData, intSensorDataSize, charNGCustomSensorFeatureValue);
		if (intRetval != EXIT_SUCCESS)
		{
			fprintf(stderr, "Fail to select mold pressure sensor feature value for ShotSN:%d CustomSensorSN:%d\n", i, intNGCustomSensorSN);
			//free(doubleSensorData);
			continue;
			//return EXIT_FAILURE;
		}
		//free(doubleSensorData);

		/*
		fprintf(stderr,"[NG]ShotSN:%d MaxPressureTime:[%s] MaxPressure:[%s] SumStage1:[%s] AvgStage1Slope:[%s] AvgStage2Slope:[%s] MoldReleasePressure:[%s]\n",i,
		 charNGCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_FEATURE_MAX_PRESSURE_TIME],
		 charNGCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_FEATURE_MAX_PRESSURE],
		 charNGCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_FEATURE_STAGE1_PRESSURE_SUM],
		 charNGCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_FEATURE_AVG_STAGE1_PRESSURESLOPE],
		 charNGCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_FEATURE_AVG_STAGE2_PRESSURESLOPE],
		 charNGCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_FEATURE_RELEASED_PRESSURE]);
		*/
		

		intRetval = DB_SelectMOCustomSensorRawData(mysqlCon, intMOSN, i, intFGCustomSensorSN, &doubleSensorData, &intSensorDataSize);
		if (intRetval != EXIT_SUCCESS)
		{
			fprintf(stderr, "Fail to select mold pressure sensor data for ShotSN:%d CustomSensorSN:%d\n", i, intNGCustomSensorSN);
			//free(doubleSensorData);
			continue;
			//return EXIT_FAILURE;
		}
		intRetval = DB_GetCavityPressureSensorFeatureValue(mysqlCon, intMOSN, doubleSensorData, intSensorDataSize, charFGCustomSensorFeatureValue);
		if (intRetval != EXIT_SUCCESS)
		{
			fprintf(stderr, "Fail to select mold pressure sensor feature value for ShotSN:%d CustomSensorSN:%d\n", i, intNGCustomSensorSN);
			//free(doubleSensorData);
			continue;
			//return EXIT_FAILURE;
		}
		//free(doubleSensorData);

		/*
		fprintf(stderr,"[FG]ShotSN:%d MaxPressureTime:[%s] MaxPressure:[%s] SumStage1:[%s] AvgStage1Slope:[%s] AvgStage2Slope:[%s] MoldReleasePressure:[%s]\n",i,
		 charFGCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_FEATURE_MAX_PRESSURE_TIME],
		 charFGCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_FEATURE_MAX_PRESSURE],
		 charFGCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_FEATURE_STAGE1_PRESSURE_SUM],
		 charFGCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_FEATURE_AVG_STAGE1_PRESSURESLOPE],
		 charFGCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_FEATURE_AVG_STAGE2_PRESSURESLOPE],
		 charFGCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_FEATURE_RELEASED_PRESSURE]);
		*/

		intRetval = DB_InsertCavityPressureSensorSoftSensorFeatureValue(mysqlCon, intMOSN, i, charNGCustomSensorFeatureValue, charFGCustomSensorFeatureValue);
		if (intRetval != EXIT_SUCCESS)
		{
			fprintf(stderr, "Fail to insert SPC feature value for ShotSN:%d \n", i);
			continue;
			//return EXIT_FAILURE;
		}
		
		intRetval = DB_InsertMoldActionFeatureValue(mysqlCon, intMOSN, i);
		if (intRetval != EXIT_SUCCESS)
		{
			fprintf(stderr, "Fail to insert mold action feature value for ShotSN:%d \n", i);
			continue;
			//return EXIT_FAILURE;
		}
	}

	mysql_close(&mysqlCon);
	return intRetval;
}
