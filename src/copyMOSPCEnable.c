#include <stdio.h>
#include <mysql/mysql.h>
#include "config.h"
#include "itri_injpro.h"

int main(int argc, char *argv[])
{
    unsigned int intSourceMOSN, intDestinationMOSN;
    int intRetval;
    double *doubleSensorData;
    unsigned int intSensorDataSize;
    MYSQL mysqlCon;
    char charNGMoldSensorFeatureValue[CAVITY_PRESSURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE];
    char charFGMoldSensorFeatureValue[CAVITY_PRESSURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE];

    //analysisShot -MOSN A -ShotSN B
    if (argc == 4)
    {
        if (strcmp(argv[1], "-MOSN") == 0)
        {
            intSourceMOSN = atoi(argv[2]);
            intDestinationMOSN = atoi(argv[3]);
        }
        else
            return EXIT_FAILURE;
    }

    fprintf(stderr, "SourceMOSN:%d DestinationMoSN:%d \n", intSourceMOSN, intDestinationMOSN);
    mysql_init(&mysqlCon);
    if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
    {
        if (mysql_errno(&mysqlCon))
        {
            fprintf(stderr, "Fail to connect to MySql server %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
        }
        return EXIT_FAILURE;
    }
    SPC_CopySPCEnable(mysqlCon, intSourceMOSN, intDestinationMOSN);

    mysql_close(&mysqlCon);
    return intRetval;
}
