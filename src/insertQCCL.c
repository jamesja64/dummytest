#include <stdio.h>
#include <mysql/mysql.h>
#include "config.h"
#include "itri_injpro.h"

int main(int argc, char *argv[])
{
	unsigned int	intMOSN;
	unsigned int	intShotSN_Start,intShotSN_End;
	unsigned int	intNGCustomSensorSN,intFGCustomSensorSN;
	int		intRetval;
	double		*doubleSensorData;
	unsigned int	intSensorDataSize;
	MYSQL		mysqlCon;
	char		charNGMoldSensorFeatureValue[CAVITY_PRESSURE_SENSOR_SINGLE_FEATURE_NUM+1][TINY_STRING_SIZE];
	char		charFGMoldSensorFeatureValue[CAVITY_PRESSURE_SENSOR_SINGLE_FEATURE_NUM+1][TINY_STRING_SIZE];

        //analysisShot -MOSN A -ShotSN B 
        if(argc==6){
                if(strcmp(argv[1],"-MOSN")==0){
                        intMOSN=atoi(argv[2]);
                }else
                        return EXIT_FAILURE;
                if(strcmp(argv[3],"-ShotSN")==0){
                        intShotSN_Start=atoi(argv[4]);
						intShotSN_End=atoi(argv[5]);
                }else
                        return EXIT_FAILURE;
        }


	fprintf(stderr,"MOSN:%d ShotSN:%d \n",intMOSN, intShotSN_End);
	mysql_init(&mysqlCon);
	if(!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD,NULL,0,NULL,CLIENT_FOUND_ROWS))
	{
		if (mysql_errno(&mysqlCon)){
			fprintf(stderr, "Fail to connect to MySql server %d: %s\n",mysql_errno(&mysqlCon),mysql_error(&mysqlCon));
		}
		return EXIT_FAILURE;
	}

	//intRetval=INDEX_InsertSPCCL(mysqlCon,intMOSN,intShotSN_Start,intShotSN_End);

	//intRetval = SPC_InsertAllSPCCL(mysqlCon, intMOSN, intShotSN_End);
	intRetval = QC_InsertAllQCCL(mysqlCon, intMOSN, intShotSN_Start, intShotSN_End);

	mysql_close(&mysqlCon);
	return intRetval;
}

