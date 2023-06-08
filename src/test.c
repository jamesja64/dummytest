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
    double *doubleXBar;
    char charMonitorItemDBName[TINY_STRING_SIZE];
    char charMonitorItemDisplayName[TINY_STRING_SIZE];

    unsigned int intIMMSN;
    unsigned int intMOSN;
    unsigned int intShotSN;
    unsigned int intMonitorItemSN;
    bool boolSPCRule[8] = {0};
    unsigned int intShotSNStart;
    unsigned int intShotSNEnd;
    unsigned int intRoundSN;
    bool boolUseActual;
    bool boolSPCQC;
    unsigned int intSPCRuleAlarmSN, intSPCQCAlarmSN;
    bool boolSPCRuleAlarmRead, boolSPCQCAlarmRead;
    bool boolintMonitorItemSNEnable;
    unsigned int intSPCRuleSampleSize, intSPCRuleGroupSize;
    double doubleSPCRuleCLRatio;
    char charOPCUAIMMNodeList[OPCUA_IMMPARA_NODE_NUM + OPCUA_IMMSENSOR_NODE_NUM + 1][OPCUA_META_NUM][MEDIUM_STRING_SIZE];
    struct tm tmExpectedStartTime;
	struct tm tmExpectedEndTime;

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

    intMOSN = 4;
    for (int i = 1; i <= 316; i++)
        intRetval = DB_InsertMoldActionFeatureValue(mysqlCon, intMOSN, i);
    return EXIT_SUCCESS;
/*
    tmExpectedStartTime.tm_year = 2020;
	tmExpectedStartTime.tm_mon = 5;
	tmExpectedStartTime.tm_mday = 14;
	tmExpectedStartTime.tm_hour = 8;
	tmExpectedStartTime.tm_min = 0;
	tmExpectedStartTime.tm_sec = 0;

	tmExpectedEndTime.tm_year = 2020;
	tmExpectedEndTime.tm_mon = 5;
	tmExpectedEndTime.tm_mday = 14;
	tmExpectedEndTime.tm_hour = 16;
	tmExpectedEndTime.tm_min = 30;
	tmExpectedEndTime.tm_sec = 0;
    MO_UpdateMO(mysqlCon,6,"Ginz-TestMO",9,2,2,2,tmExpectedStartTime, tmExpectedEndTime,1,10000);
    */
    //MO_UpdateMO(mysqlCon,6,"Bermuda-NO",9,1,1,1,tmExpectedStartTime, tmExpectedEndTime,2,19999);
    //DB_UpdateCarbonEmission(mysqlCon,4);
    return EXIT_SUCCESS;

    /*
    intIMMSN = 1;
    intRetval = IMM_AddRoundSN(mysqlCon, intIMMSN);
    if (intRetval != EXIT_SUCCESS)
        fprintf(stderr, "Fail to add RoundSN on IMMSN :%d [\033[31mFail\033[m]\n", intIMMSN);
    else
        fprintf(stderr, "Add RoundSN on IMMSN :%d successfully[\033[32mOK\033[m]\n", intIMMSN);
        */

    /*
    intMOSN = 4;
    intSPCRuleAlarmSN = 1;
    boolSPCRuleAlarmRead = 0;
    intRetval = DB_UpdateSPCRuleAlarmRead(mysqlCon, intMOSN, intSPCRuleAlarmSN, boolSPCRuleAlarmRead);
    if (intRetval != EXIT_SUCCESS)
        fprintf(stderr, "Fail to set MOSN:%d intSPCRuleAlarmSN:%d Read:%d [\033[31mFail\033[m]\n", intMOSN,intSPCRuleAlarmSN, boolSPCRuleAlarmRead);
    else
        fprintf(stderr, "Set MOSN:%d intSPCRuleAlarmSN:%d Read:%d successfully[\033[32mOK\033[m]\n", intMOSN,intSPCRuleAlarmSN, boolSPCRuleAlarmRead);
    
    intSPCQCAlarmSN=1;
    boolSPCQCAlarmRead=1;
    DB_UpdateSPCQCAlarmRead(mysqlCon, intMOSN, intSPCQCAlarmSN, boolSPCQCAlarmRead);
    if (intRetval != EXIT_SUCCESS)
        fprintf(stderr, "Fail to set MOSN:%d intSPCQCAlarmSN:%d Read:%d [\033[31mFail\033[m]\n", intMOSN,intSPCQCAlarmSN, boolSPCQCAlarmRead);
    else
        fprintf(stderr, "Set MOSN:%d intSPCQCAlarmSN:%d Read:%d successfully[\033[32mOK\033[m]\n", intMOSN,intSPCQCAlarmSN, boolSPCQCAlarmRead);
        */

    /*
    intRetval = IMM_SetAutoOPCUAControl(mysqlCon, intIMMSN, false);
    if (intRetval != EXIT_SUCCESS)
        fprintf(stderr, "Fail to set AutoOPCUAControl function on IMMSN :%d [\033[31mFail\033[m]\n",intIMMSN);
    else
        fprintf(stderr, "Set AutoOPCUAControl function on IMMSN :%d successfully[\033[32mOK\033[m]\n",intIMMSN);
    */

    /*
    fprintf(stderr, "%-55s", "Inserting Index OPCUA Version:FCS-CT-80e-OPCUA...");
    DEMO_SetFCSCT80EOPCUAVersion(charOPCUAIMMNodeList);
    intRetval = INDEX_InsertOPCUAVersion(mysqlCon, "FCS-CT-80e", charOPCUAIMMNodeList);
    if (intRetval == EXIT_SUCCESS)
        fprintf(stderr, "[\033[32mOK\033[m]\n");
    else
    {
        fprintf(stderr, "[\033[31mFail\033[m]\n");
        return EXIT_FAILURE;
    }
    */
    //SPC_InsertAllCaCpCpkPpk(mysqlCon,1,1,20);

    //return EXIT_SUCCESS;

    //intMOSN = 4;
    //intIMMSN = 2;
    //intShotSN = 1;
    //intMonitorItemSN = 1;
    //intShotSNStart = 100;
    //intShotSNEnd = 300;

    //intRetval =  MO_ExportMO(mysqlCon, intMOSN);

    //intRetval = MO_DeleteShot(mysqlCon, intMOSN, intShotSNStart, intShotSNEnd);

    //intRetval = SPC_InsertAllSPCRuleCL(mysqlCon, intMOSN, intShotSNStart);

    //boolSPCQC = 1;

    intMOSN = 1;
    intMonitorItemSN = 1;
    intSPCRuleSampleSize = 2;
    intSPCRuleGroupSize = 10;
    doubleSPCRuleCLRatio = 3;
    boolintMonitorItemSNEnable = 1;
    boolSPCRule[0] = 1;
    boolSPCRule[1] = 1;
    boolSPCRule[2] = 1;
    boolSPCRule[3] = 1;
    boolSPCRule[4] = 1;
    boolSPCRule[5] = 1;
    boolSPCRule[6] = 1;
    boolSPCRule[7] = 1;

    
    //fprintf(stderr,"3&1=%d 3&2=%d\n",3&1,3&2);

    //intRetval = SPC_ConfigMonitorItemSN(mysqlCon, intMOSN, intMonitorItemSN, intSPCRuleSampleSize, intSPCRuleGroupSize, doubleSPCRuleCLRatio, boolSPCRule);

    //intRetval = DB_GetMonitorItemDBName(mysqlCon, intMOSN, intMonitorItemSN, char *charMonitorItemDBName);
    //intRetval = DB_GetMonitorItemDisplayName( mysqlCon,  intMOSN,  intMonitorItemSN, char *charMonitorItemDisplayName);

    //intRetval = SPC_InsertAllQCCL(mysqlCon, intMOSN, intShotSNStart, intShotSNEnd);
    //intRetval = SPC_InsertQCCL(mysqlCon, intMOSN, intMonitorItemSN, intShotSNStart, intShotSNEnd);

    //intRetval = DB_InsertMOAcceptCriteriaPredictValue(mysqlCon, intMOSN, intShotSN, 1, "1");

    //IMM_AddRoundSN(mysqlCon,intIMMSN);

    /*
    for (int i = 0; i < 40; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            OPCUA_WriteOPCUANodeValue(mysqlCon, intIMMSN, IMMPARA_DI_MOLD_RELEASED, "0");
            sleep(1);
            OPCUA_WriteOPCUANodeValue(mysqlCon, intIMMSN, IMMPARA_DI_MOLD_CLAMPED, "1");
            sleep(1);
            OPCUA_WriteOPCUANodeValue(mysqlCon, intIMMSN, IMMPARA_DI_MOLD_CLAMPED, "0");
            sleep(1);
            OPCUA_WriteOPCUANodeValue(mysqlCon, intIMMSN, IMMPARA_DI_MOLD_RELEASED, "1");
            sleep(1);
        }
        //IMM_AddIMMRoundSN(mysqlCon,intIMMSN);
    }*/

    //boolUseActual = false;
    //intRetval = DB_UpdateMOListAfterShot(mysqlCon, intMOSN, boolUseActual);

    //intRetval =  DB_SelectRoundSNbyShotSN(mysqlCon, intMOSN,  intShotSN,  &intRoundSN);
    //fprintf(stderr,"ShotSN:%d RoundSN:%d \n",intShotSN,intRoundSN);

    //  IMM_SelectRoundSN(mysqlCon,intIMMSN,&intRoundSN);
    //fprintf(stderr,"RoundSN:%d\n",intRoundSN);

    //for (int i = 1; i <= 162; i += 1)

    //intRetval = SPC_InsertSPCRuleAlarm(mysqlCon, intMOSN, i);

    //SPC_SelectXBarValue(mysqlCon,5, 36, 10, 10, &doubleXBar);
    //for(int i=0;i<10;i++)
    //    fprintf(stderr,"XBar[%d]=%lf\n",i,*(doubleXBar+i));
    //intRetval = SPC_InsertAllSPCRuleCL(mysqlCon, intMOSN, intShotSNEnd);
    //intRetval = SPC_InsertAllSPCQCCL(mysqlCon, intMOSN, intShotSNStart, intShotSNEnd);
    //for(int i=24;i<=59;i++)
    //intRetval = SPC_InsertSPCCL(mysqlCon, 5, 1, 40);

    //for(int i=100;i<=300;i+=1)
    //intRetval = SPC_InsertSPCRuleAlarm(mysqlCon, 5, i);

    //for(int i=1;i<=10;i++){
    //intRetval = SPC_InsertSPCRuleAlarm(mysqlCon, 5, i);
    //}

    //intRetval = SPC_InsertSPCRuleAlarm(mysqlCon, 5, 1);

    //intRetval=SPC_SelectXBarValue(mysqlCon,5,59,124,&doubleXBar);
    //fprintf(stderr,"XBar:%lf\n",doubleXBar);
    //

    
    //fprintf(stderr,"MonitorItemName:%s\n",charMonitorItemSNName);
    mysql_close(&mysqlCon);

    return EXIT_SUCCESS;
}
