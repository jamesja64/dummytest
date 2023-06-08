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

    for (int intMOSN = intMaxMOSN; intMOSN >= 1; intMOSN--)
    {
        int iOK = EXIT_SUCCESS;
        snprintf(charStatement, MAX_STRING_SIZE,
                 "ALTER TABLE %s_%s_%s_%d_RawData.ShotSNList "
                 "ADD COLUMN MoldClampedCycleTime DOUBLE UNSIGNED DEFAULT NULL AFTER MoldReleasedTime,"
                 "ADD COLUMN MoldBasedCycleTime DOUBLE UNSIGNED DEFAULT NULL AFTER MoldClampedCycleTime,"
                 "ADD COLUMN MoldShotInterval DOUBLE UNSIGNED DEFAULT NULL AFTER MoldBasedCycleTime,"
                 "ADD COLUMN MoldCompleteCycleTime DOUBLE UNSIGNED DEFAULT NULL AFTER MoldShotInterval",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            //fprintf(stderr, "MOSN:%d Fail to add 4 mold action feature values on table ShotSNList(%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "CREATE TABLE %s_%s_%s_%d_RawData_FeatureValue.SoftSensor ("
                 "ShotSN					BIGINT UNSIGNED NOT NULL,"
                 "PRIMARY KEY (ShotSN),"
                 "CONSTRAINT %d_FeatureValue_SoftSensor_ShotSN FOREIGN KEY (ShotSN) REFERENCES %s_%s_%s_%d_RawData.ShotSNList(ShotSN) ON UPDATE CASCADE)"
                 "ENGINE=InnoDB",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
                 intMOSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            //fprintf(stderr, "MOSN:%d Fail to create table SoftSensor(%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "ALTER TABLE %s_%s_%s_%d_RawData_FeatureValue.SoftSensor "
                 "ADD COLUMN MoldClampedCycleTime   DOUBLE UNSIGNED DEFAULT NULL AFTER ShotSN,"
                 "ADD COLUMN MoldBasedCycleTime     DOUBLE UNSIGNED DEFAULT NULL AFTER MoldClampedCycleTime,"
                 "ADD COLUMN MoldShotInterval       DOUBLE UNSIGNED DEFAULT NULL AFTER MoldBasedCycleTime,"
                 "ADD COLUMN MoldCompleteCycleTime  DOUBLE UNSIGNED DEFAULT NULL AFTER MoldShotInterval",                 
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            fprintf(stderr, "MOSN:%d Fail to add 4 mold action feature values on table SoftSensor (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "ALTER TABLE %s_%s_%s_%d_RawData_FeatureValue.SoftSensor "                 
                 "ADD COLUMN DiffMaxPressure				DOUBLE DEFAULT NULL AFTER MoldCompleteCycleTime,"
                 "ADD COLUMN DiffIncreasePressureTime		DOUBLE DEFAULT NULL AFTER DiffMaxPressure,"
                 "ADD COLUMN DiffMoldReleasePressure		DOUBLE DEFAULT NULL AFTER DiffIncreasePressureTime",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            fprintf(stderr, "MOSN:%d Fail to add 4 mold action feature values on table SoftSensor (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "UPDATE %s_%s_%s_%d_RawData.ShotSNList SET MoldBasedCycleTime = CycleTime",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr, "Q:%s\n", charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            //fprintf(stderr, "MOSN:%d Fail to set MoldBasedCycleTime = CycleTime(%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "UPDATE %s_%s_%s_%d_RawData.ShotSNList SET MoldShotInterval = ShotInterval",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr, "Q:%s\n", charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            //fprintf(stderr, "MOSN:%d Fail to set MoldShotInterval = ShotInterval (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "UPDATE %s_%s_%s_%d_RawData.ShotSNList SET MoldClampedCycleTime = TIMESTAMPDIFF(Microsecond,MoldClampedTime,MoldReleasingTime)/1000000, MoldCompleteCycleTime = MoldBasedCycleTime + MoldShotInterval",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr, "Q:%s\n", charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            //fprintf(stderr, "MOSN:%d Fail to set MoldShotInterval = ShotInterval (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }         

        snprintf(charStatement, MAX_STRING_SIZE,
                 "ALTER TABLE %s_%s_%s_%d_RawData_Alarm.QualityControlAlarmLog "                 
                 "ADD COLUMN QualityControlUCL		DOUBLE DEFAULT NULL AFTER QualityControlAlarm,"
                 "ADD COLUMN QualityControlLCL      DOUBLE DEFAULT NULL AFTER QualityControlUCL,"
                 "ADD COLUMN QualityControlXValue   DOUBLE DEFAULT NULL AFTER QualityControlLCL,"
                 "ADD COLUMN QualityControlXDiff    DOUBLE DEFAULT NULL AFTER QualityControlXValue,"
                 "ADD COLUMN QualityControlAlarmTime     DATETIME(6) DEFAULT NULL AFTER QualityControlXDiff",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            fprintf(stderr, "MOSN:%d Fail to QCAlarm 5 related columns (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "UPDATE %s_%s_%s_%d_RawData_Alarm.QualityControlAlarmLog SET QualityControlXDiff = QualityControlAlarmDiff",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr, "Q:%s\n", charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            //fprintf(stderr, "MOSN:%d Fail to set MoldShotInterval = ShotInterval (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }   

        snprintf(charStatement, MAX_STRING_SIZE,
                 "ALTER TABLE %s_%s_%s_%d_RawData_Alarm.QualityControlAlarmLog DROP QualityControlAlarmDiff",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            fprintf(stderr, "MOSN:%d Fail to drop 5 QualityControlAlarmXDiff (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "ALTER TABLE %s_%s_%s_%d_RawData_Alarm.SPCAlarmLog DROP SPCRule1XBarValue",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            fprintf(stderr, "MOSN:%d Fail to drop SPCRule1XBarValue (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "ALTER TABLE %s_%s_%s_%d_RawData_Alarm.SPCAlarmLog ADD COLUMN SPCRUCL DOUBLE DEFAULT NULL AFTER SPCRAlarm",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            fprintf(stderr, "MOSN:%d Fail to add SPCRUCL (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "ALTER TABLE %s_%s_%s_%d_RawData_Alarm.SPCAlarmLog ADD COLUMN SPCRLCL DOUBLE DEFAULT NULL AFTER SPCRUCL",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            fprintf(stderr, "MOSN:%d Fail to add SPCRLCL (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "ALTER TABLE %s_%s_%s_%d_RawData_Alarm.SPCAlarmLog ADD COLUMN SPCRValue DOUBLE DEFAULT NULL AFTER SPCRLCL",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            fprintf(stderr, "MOSN:%d Fail to add SPCRValue (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "ALTER TABLE %s_%s_%s_%d_RawData_Alarm.SPCAlarmLog ADD COLUMN SPCRDiff DOUBLE DEFAULT NULL AFTER SPCRValue",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            fprintf(stderr, "MOSN:%d Fail to add SPCRDiff (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "ALTER TABLE %s_%s_%s_%d_RawData_Alarm.SPCAlarmLog ADD COLUMN SPCRule1UCL DOUBLE DEFAULT NULL AFTER SPCRule1Alarm",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            fprintf(stderr, "MOSN:%d Fail to add SPCRule1UCL (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "ALTER TABLE %s_%s_%s_%d_RawData_Alarm.SPCAlarmLog ADD COLUMN SPCRule1LCL DOUBLE DEFAULT NULL AFTER SPCRule1UCL",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            fprintf(stderr, "MOSN:%d Fail to add SPCRule1LCL (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "ALTER TABLE %s_%s_%s_%d_RawData_Alarm.SPCAlarmLog ADD COLUMN SPCRule1XBarValue DOUBLE DEFAULT NULL AFTER SPCRule1LCL",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            fprintf(stderr, "MOSN:%d Fail to add SPCRule1XBarValue (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "ALTER TABLE %s_%s_%s_%d_RawData_Alarm.SPCAlarmLog ADD COLUMN SPCRule1XBarDiff DOUBLE DEFAULT NULL AFTER SPCRule1XBarValue",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            fprintf(stderr, "MOSN:%d Fail to add SPCRule1XBarDiff (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "ALTER TABLE %s_%s_%s_%d_RawData_Alarm.SPCAlarmLog ADD COLUMN SPCAlarmTime DATETIME(6) DEFAULT NULL",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            fprintf(stderr, "MOSN:%d Fail to add SPCAlarmTime (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }

        snprintf(charStatement, MAX_STRING_SIZE,
                 "UPDATE %s_%s_%s_%d_RawData_Alarm.SPCAlarmLog SET SPCRDiff = SPCRule1RDiff",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr, "Q:%s\n", charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            //fprintf(stderr, "MOSN:%d Fail to set MoldShotInterval = ShotInterval (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }  

        snprintf(charStatement, MAX_STRING_SIZE,
                 "ALTER TABLE %s_%s_%s_%d_RawData_Alarm.SPCAlarmLog DROP SPCRule1RDiff",
                 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
        //fprintf(stderr,"Q:%s\n",charStatement);
        intRetval = mysql_query(&mysqlCon, charStatement);
        if (intRetval != EXIT_SUCCESS)
        {
            fprintf(stderr, "MOSN:%d Fail to drop 5 QualityControlAlarmXDiff (%d):%s\n", intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            iOK = EXIT_FAILURE;
        }  
    }

    mysql_close(&mysqlCon);

    return EXIT_SUCCESS;
}
