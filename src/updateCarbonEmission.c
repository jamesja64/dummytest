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
        if(argc==3){
                if(strcmp(argv[1],"-MOSN")==0){
                        intMOSN=atoi(argv[2]);
                }else
                        return EXIT_FAILURE;                
        }else
                        return EXIT_FAILURE;


	fprintf(stderr,"Update Carbon Emission MOSN:%d \n",intMOSN);
	mysql_init(&mysqlCon);
	if(!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD,NULL,0,NULL,CLIENT_FOUND_ROWS))
	{
		if (mysql_errno(&mysqlCon)){
			fprintf(stderr, "Fail to connect to MySql server %d: %s\n",mysql_errno(&mysqlCon),mysql_error(&mysqlCon));
		}
		return EXIT_FAILURE;
	}
	intRetval = DB_UpdateCarbonEmission(mysqlCon,intMOSN);
	mysql_close(&mysqlCon);
	return intRetval;
}

