#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include "itri_injpro.h"
#include "open62541.h"
#include "config.h"

int main(int argc, char *argv[])
{
    MYSQL mysqlCon;
    int intRetval;
    unsigned int intMaxMOSN;
    char charErrMsg[LONG_STRING_SIZE];
    unsigned int intLogSN;
    char charStatement[MAX_STRING_SIZE];
    char *charMaxMOSN;

    

    fprintf(stderr, "%-55s", "Connecting to MySQL Server...");
    mysql_init(&mysqlCon);
    if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
    {
        fprintf(stderr, "[\033[31mFail\033[m]\n");
        if (mysql_errno(&mysqlCon))
        {
            fprintf(stderr, "Fail to connect to MySql server %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
        }
        return EXIT_FAILURE;
    }
    else
    {
        fprintf(stderr, "[\033[32mOK\033[m]\n");
    }

    snprintf(charStatement, MAX_STRING_SIZE, "%s_%s_%s.MOSNList", SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME);
    intRetval = DB_SelectColumnMax(mysqlCon, charStatement, "MOSN", &charMaxMOSN);
    intMaxMOSN = atoi(charMaxMOSN);
    fprintf(stderr, "Max MOSN:%d\n", intMaxMOSN);

    for (int intMOSN = 1; intMOSN <= intMaxMOSN; intMOSN++)
    {
        int iOK = EXIT_SUCCESS;
        snprintf(charStatement, MAX_STRING_SIZE,
                 "ALTER TABLE %s_%s_%s_%d_RawData.ShotSNList DROP CycleTime",SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            fprintf(stderr, "MOSN:%d Fail to drop column CycleTime on table ShotSNList(%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "ALTER TABLE %s_%s_%s_%d_RawData.ShotSNList DROP ShotInterval",SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            fprintf(stderr, "MOSN:%d Fail to drop column ShotInterval on table ShotSNList(%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }
    }

    mysql_close(&mysqlCon);

    return EXIT_SUCCESS;
}
