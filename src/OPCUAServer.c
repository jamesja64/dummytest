#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <mysql/mysql.h>
#include "open62541.h"
#include "itri_injpro.h"
#include "config.h"

UA_Boolean boolRunningSignal = true;

int WatchDog_GetElapsedTime(struct timespec stStartTimeStamp, double *doubleElapsedTime);
static void handler_StopOPCUAServer(int intSignal);
static void OPCUA_CreateOPCUANodes(UA_Server *server);
void *OPCUA_WatchDog(void *ptrArg);

int main(void)
{
    pthread_t threadWatchDog;
    pthread_create(&threadWatchDog, NULL, OPCUA_WatchDog, NULL);

    signal(SIGINT, handler_StopOPCUAServer);
    signal(SIGTERM, handler_StopOPCUAServer);
    signal(SIGKILL, handler_StopOPCUAServer);

    UA_ServerConfig *UA_Config = UA_ServerConfig_new_minimal(9527, NULL);
    UA_ServerConfig_set_customHostname(UA_Config, UA_String_fromChars("127.0.0.1"));
    UA_Server *UA_Server = UA_Server_new(UA_Config);

    OPCUA_CreateOPCUANodes(UA_Server);

    /* Run the server */
    UA_StatusCode retval = UA_Server_run(UA_Server, &boolRunningSignal);
    UA_Server_delete(UA_Server);
    UA_ServerConfig_delete(UA_Config);
    pthread_join(threadWatchDog, NULL);

    return (int)retval;
}

static void handler_StopOPCUAServer(int intSignal)
{
    boolRunningSignal = false;
}

void *OPCUA_WatchDog(void *ptrArg)
{
    int intRetval;
    struct timespec stStartTimeStamp;
    clock_gettime(CLOCK_REALTIME, &stStartTimeStamp);

    while (boolRunningSignal)
    {
        double doubleElapsedTime;
        intRetval = WatchDog_GetElapsedTime(stStartTimeStamp, &doubleElapsedTime);
        if (doubleElapsedTime < WATCHDOG_CHECK_INTEVAL)
        {
            continue;
        }
        clock_gettime(CLOCK_REALTIME, &stStartTimeStamp);

        unsigned int intIMMSN;
        // Connect to MYSQL Server

        MYSQL mysqlCon;
        unsigned int intLogSN;
        char charErrMsg[LONG_STRING_SIZE];

        unsigned int intOPCUAClientPID, intModbusClientPID;

        mysql_init(&mysqlCon);
        if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
        {
            fprintf(stderr, "[OPCUAServer-WatchDog]MYSQL connection failed.\n");
            if (mysql_errno(&mysqlCon))
            {
                fprintf(stderr, "[OPCUAServer-WatchDog]MYSQL connection error %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
            }
            boolRunningSignal = false;
        }
        // MySQL Connect Successfully
        else
        {
            intRetval = DB_SelectIMMSNofPrimaryIMM(mysqlCon, &intIMMSN);

            // Fail to Select PrimaryIMMSN
            if (intRetval != EXIT_SUCCESS)
            {
                snprintf(charErrMsg, LONG_STRING_SIZE, "[OPCUAServer-WatchDog]Fail to select primary IMMSN (%d):%s",
                         mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
                intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_MYSQL, 0, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
                boolRunningSignal = false;
                continue;
            }

            // Select OPCUAClientPID and ModbusClientPID
            intRetval = DB_SelectPIDbyIMMSN(mysqlCon, intIMMSN, &intOPCUAClientPID, &intModbusClientPID);

            // Fail to Select OPCUAClientPID and ModbusClientPID
            if (intRetval != EXIT_SUCCESS)
            {
                snprintf(charErrMsg, LONG_STRING_SIZE, "[OPCUAServer-WatchDog]Fail to select OPCUAClientPID and ModbusClientPID for IMMSN:%d (%d):%s",
                         intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
                intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_MYSQL, 0, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
                boolRunningSignal = false;
                continue;
            }
            // Succssfully Select OPCUAClientPID and ModbusClientPID
            else
            {
                if (intOPCUAClientPID != 0)
                {
                    int intOPCUAClientStatus = kill(intOPCUAClientPID, 0);
                    fprintf(stderr, "OPCUAClientPID:%d Status:%d\n", intOPCUAClientPID, intOPCUAClientStatus);
                    // If OPCUA Client Status = -1 (Terminated)
                    if (intOPCUAClientStatus == -1)
                    {
                        char charStatement[MAX_STRING_SIZE];
                        // Update OPCUAClientStatus = OPCUA_CLIENT_STATUS_UNKNOW in IMMSNList
                        snprintf(charStatement, MAX_STRING_SIZE,
                                 "UPDATE %s_%s.IMMSNList SET OPCUAClientPID = NULL, OPCUAClientStatus = %d, IMMLastUpdateTime = NOW(6) WHERE IMMSN = %d",
                                 SYS_NAME, DATA_DATABASE_NAME, OPCUA_CLIENT_STATUS_UNKNOW, intIMMSN);
                        intRetval = mysql_query(&mysqlCon, charStatement);
                        if (intRetval != EXIT_SUCCESS)
                        {
                            snprintf(charErrMsg, LONG_STRING_SIZE,
                                     "[OPCUAServer-WatchDog]Fail to update OPCUAClientStatus = UNKNOW, OPCUAClientPID = NULL and IMMLastUpdateTime = NOW(6) where IMMSN = %d (%d):%s",
                                     intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
                            SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, 0, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
                        }
                    }
                }

                if (intModbusClientPID != 0)
                {
                    int ModbusClientStatus = kill(intModbusClientPID, 0);
                    fprintf(stderr, "ModbusClientPID:%d Status:%d\n", intModbusClientPID, intModbusClientPID);

                    // If Modbus Client Status = -1 (Terminated)
                    if (ModbusClientStatus == -1)
                    {
                        char charStatement[MAX_STRING_SIZE];
                        // Update ModbusClientStatus = MODBUS_CLIENT_STATUS_UNKNOW in IMMSNList
                        snprintf(charStatement, MAX_STRING_SIZE,
                                 "UPDATE %s_%s.IMMSNList SET ModbusClientPID = NULL, ModbusClientStatus = %d, IMMLastUpdateTime = NOW(6) WHERE IMMSN = %d",
                                 SYS_NAME, DATA_DATABASE_NAME, MODBUS_CLIENT_STATUS_UNKNOW, intIMMSN);
                        intRetval = mysql_query(&mysqlCon, charStatement);
                        if (intRetval != EXIT_SUCCESS)
                        {
                            snprintf(charErrMsg, LONG_STRING_SIZE,
                                     "[OPCUAServer-WatchDog]Fail to update ModbusClientStatus = UNKNO, ModbusClientPID = NULL and IMMLastUpdateTime = NOW(6) where IMMSN = %d (%d):%s",
                                     intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
                            SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, 0, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
                        }
                    }
                }
            }
            mysql_close(&mysqlCon);
        }
    }
}

int WatchDog_GetElapsedTime(struct timespec stStartTimeStamp, double *doubleElapsedTime)
//[WatchDog]Get Elapsed Time from Start Time Stamp
{
    struct timespec stDiffElapsedTime;
    struct timespec stCurrentTimeStamp;

    clock_gettime(CLOCK_REALTIME, &stCurrentTimeStamp);
    if (stCurrentTimeStamp.tv_nsec - stStartTimeStamp.tv_nsec < 0)
    {
        stDiffElapsedTime.tv_sec = stCurrentTimeStamp.tv_sec - stStartTimeStamp.tv_sec - 1;
        stDiffElapsedTime.tv_nsec = stCurrentTimeStamp.tv_nsec - stStartTimeStamp.tv_nsec + 1000000000;
    }
    else
    {
        stDiffElapsedTime.tv_sec = stCurrentTimeStamp.tv_sec - stStartTimeStamp.tv_sec;
        stDiffElapsedTime.tv_nsec = stCurrentTimeStamp.tv_nsec - stStartTimeStamp.tv_nsec;
    }
    *doubleElapsedTime = stDiffElapsedTime.tv_sec + stDiffElapsedTime.tv_nsec / 1000000000.0;

    return EXIT_SUCCESS;
}

static void OPCUA_CreateOPCUANodes(UA_Server *server)
{

    UA_NodeId IdIMM; /* get the nodeid assigned by the server */
    UA_NodeId IdMO;  /* get the nodeid assigned by the server */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_VariableAttributes mnAttr = UA_VariableAttributes_default;

    // Object:IMM
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "IMM");
    UA_Server_addObjectNode(server, UA_NODEID_NULL,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "IMM"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                            oAttr, NULL, &IdIMM);

    // Data:IMM.IMMSN
    UA_Int32 int32IMMSN = 1;
    mnAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&mnAttr.value, &int32IMMSN, &UA_TYPES[UA_TYPES_INT32]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "IMM.IMMSN");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "IMM.IMMSN"), IdIMM,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "IMM.IMMSN"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    // Data:IMM.MOSN
    UA_Int32 int32MOSN = 0;
    mnAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&mnAttr.value, &int32MOSN, &UA_TYPES[UA_TYPES_INT32]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "IMM.MOSN");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "IMM.MOSN"), IdIMM,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "IMM.MOSN"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    // Data:MO.MOID    
    UA_String strMOID = UA_STRING("");
    mnAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&mnAttr.value, &strMOID, &UA_TYPES[UA_TYPES_STRING]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "IMM.MOID");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "IMM.MOID"), IdIMM,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "IMM.MOID"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);                              

    // Data:IMM.MoldStatus
    UA_Int32 int32MoldStatus = 0;
    mnAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&mnAttr.value, &int32MoldStatus, &UA_TYPES[UA_TYPES_INT32]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "IMM.MoldStatus");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "IMM.MoldStatus"), IdIMM,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "IMM.MoldStatus"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    // Data:IMM.MoldReleased
    UA_Boolean boolMoldReleased = 1;
    mnAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&mnAttr.value, &boolMoldReleased, &UA_TYPES[UA_TYPES_BOOLEAN]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "IMM.MoldReleased");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "IMM.MoldReleased"), IdIMM,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "IMM.MoldReleased"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    // Data:IMM.MoldClamped
    UA_Boolean boolMoldClamped = 0;
    mnAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&mnAttr.value, &boolMoldClamped, &UA_TYPES[UA_TYPES_BOOLEAN]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "IMM.MoldClamped");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "IMM.MoldClamped"), IdIMM,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "IMM.MoldClamped"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    // Data:IMM.RoundSN
    UA_UInt64 int64RoundSN = 0;
    mnAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&mnAttr.value, &int64RoundSN, &UA_TYPES[UA_TYPES_INT64]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "IMM.RoundSN");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "IMM.RoundSN"), IdIMM,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "IMM.RoundSN"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    // Data:IMM.RealTimeShotSN
    UA_UInt32 intRealTimeShotSN = 0;
    mnAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&mnAttr.value, &intRealTimeShotSN, &UA_TYPES[UA_TYPES_INT32]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "IMM.RealTimeShotSN");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "IMM.RealTimeShotSN"), IdIMM,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "IMM.RealTimeShotSN"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    // Data:IMM.IMMSensorDataReadyShotSN
    UA_UInt32 intIMMSensorDataReadyShotSN = 0;
    mnAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&mnAttr.value, &intIMMSensorDataReadyShotSN, &UA_TYPES[UA_TYPES_INT32]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "IMM.IMMSensorDataReadyShotSN");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "IMM.IMMSensorDataReadyShotSN"), IdIMM,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "IMM.IMMSensorDataReadyShotSN"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);                              

    // Data:IMM.CycleTime
    // UA_String CycleTime = UA_STRING("123.45");
    UA_Double CycleTime = 123.45;
    mnAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&mnAttr.value, &CycleTime, &UA_TYPES[UA_TYPES_DOUBLE]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "IMM.CycleTime");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "IMM.CycleTime"), IdIMM,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "IMM.CycleTime"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    // Data:IMM.myString
    UA_String CoolingTime = UA_STRING("246.45");
    mnAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&mnAttr.value, &CoolingTime, &UA_TYPES[UA_TYPES_STRING]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "IMM.CoolingTime");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "IMM.CoolingTime"), IdIMM,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "IMM.CoolingTime"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    // Object:MO
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "MO");
    UA_Server_addObjectNode(server, UA_NODEID_NULL,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "MO"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                            oAttr, NULL, &IdMO);

    // Data:MO.MOSN
    //UA_Int32 int32MOSN = 5;
    mnAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&mnAttr.value, &int32MOSN, &UA_TYPES[UA_TYPES_INT32]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "MO.MOSN");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "MO.MOSN"), IdMO,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "MO.MOSN"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    /*
    UA_Float	floatValue;
    UA_Int64	int64Value=9527;
    UA_Int32	int32Value=9528;
    UA_Boolean	boolValue=false;
    time_t      t;
    srand((unsigned) time(&t));

    UA_Variant_setScalar(&mnAttr.value, &boolValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Mold1.sv_bMoldClosed");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Mold1.sv_bMoldClosed"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Mold1.sv_bMoldClosed"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    boolValue=true;
    UA_Variant_setScalar(&mnAttr.value, &boolValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Mold1.sv_bMoldOpen");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Mold1.sv_bMoldOpen"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Mold1.sv_bMoldOpen"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    boolValue=true;
    UA_Variant_setScalar(&mnAttr.value, &boolValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.OperationMode1.do_AlarmLamp");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.OperationMode1.do_AlarmLamp"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.OperationMode1.do_AlarmLamp"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    boolValue=false;
    UA_Variant_setScalar(&mnAttr.value, &boolValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.OperationMode1.do_StoppedLamp");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.OperationMode1.do_StoppedLamp"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.OperationMode1.do_StoppedLamp"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    boolValue=false;
    UA_Variant_setScalar(&mnAttr.value, &boolValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.OperationMode1.do_boolStopFlagLamp");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.OperationMode1.do_boolStopFlagLamp"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.OperationMode1.do_boolStopFlagLamp"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);


    int64Value=3;
    UA_Variant_setScalar(&mnAttr.value, &int64Value, &UA_TYPES[UA_TYPES_INT64]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_InjectProfVis.Profile.iNoOfPoints");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_InjectProfVis.Profile.iNoOfPoints"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_InjectProfVis.Profile.iNoOfPoints"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    int64Value=2;
    UA_Variant_setScalar(&mnAttr.value, &int64Value, &UA_TYPES[UA_TYPES_INT64]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_HoldProfVis.Profile.iNoOfPoints");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_HoldProfVis.Profile.iNoOfPoints"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_HoldProfVis.Profile.iNoOfPoints"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);
    int64Value=4;
    UA_Variant_setScalar(&mnAttr.value, &int64Value, &UA_TYPES[UA_TYPES_INT64]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_PlastProfVis.Profile.iNoOfPoints");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_PlastProfVis.Profile.iNoOfPoints"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_PlastProfVis.Profile.iNoOfPoints"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);


    int32Value=rand()%5000;
    UA_Variant_setScalar(&mnAttr.value, &int32Value, &UA_TYPES[UA_TYPES_INT32]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.system.sv_OperationMode");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.system.sv_OperationMode"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.system.sv_OperationMode"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    boolValue=false;
    UA_Variant_setScalar(&mnAttr.value, &boolValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Motor1.sv_bMotorStarted");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Motor1.sv_bMotorStarted"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Motor1.sv_bMotorStarted"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    boolValue=false;
    UA_Variant_setScalar(&mnAttr.value, &boolValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.HeatingNozzle1.sv_bHeatingOn");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.HeatingNozzle1.sv_bHeatingOn"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.HeatingNozzle1.sv_bHeatingOn"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Mold1.sv_rMoldIntegralPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Mold1.sv_rMoldIntegralPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Mold1.sv_rMoldIntegralPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);



    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.HeatingNozzle1.sv_ZoneRetain1.rSetValVis");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.HeatingNozzle1.sv_ZoneRetain1.rSetValVis"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.HeatingNozzle1.sv_ZoneRetain1.rSetValVis"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_PlastProfVis.Profile.Points[1].rStartPos");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[1].rStartPos"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[1].rStartPos"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_PlastProfVis.Profile.Points[2].rStartPos");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[2].rStartPos"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[2].rStartPos"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_PlastProfVis.Profile.Points[3].rStartPos");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[3].rStartPos"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[3].rStartPos"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_PlastProfVis.Profile.Points[4].rStartPos");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[4].rStartPos"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[4].rStartPos"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_PlastProfVis.Profile.Points[5].rStartPos");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[5].rStartPos"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[5].rStartPos"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);



    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_InjectProfVis.Profile.Points[1].rPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[1].rPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[1].rPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_InjectProfVis.Profile.Points[2].rPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[2].rPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[2].rPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_InjectProfVis.Profile.Points[3].rPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[3].rPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[3].rPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_InjectProfVis.Profile.Points[4].rPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[4].rPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[4].rPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_InjectProfVis.Profile.Points[5].rPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[5].rPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[5].rPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);


    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_InjectProfVis.Profile.Points[1].rVelocity");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[1].rVelocity"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[1].rVelocity"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_InjectProfVis.Profile.Points[2].rVelocity");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[2].rVelocity"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[2].rVelocity"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_InjectProfVis.Profile.Points[3].rVelocity");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[3].rVelocity"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[3].rVelocity"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_InjectProfVis.Profile.Points[4].rVelocity");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[4].rVelocity"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[4].rVelocity"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_InjectProfVis.Profile.Points[5].rVelocity");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[5].rVelocity"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_InjectProfVis.Profile.Points[5].rVelocity"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    int64Value=rand()%5000;
    UA_Variant_setScalar(&mnAttr.value, &int64Value, &UA_TYPES[UA_TYPES_INT64]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_InjectTimesAct.dActMoveTime");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_InjectTimesAct.dActMoveTime"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_InjectTimesAct.dActMoveTime"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);


    int64Value=rand()%5000;
    UA_Variant_setScalar(&mnAttr.value, &int64Value, &UA_TYPES[UA_TYPES_INT64]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_HoldProfVis.Profile.Points[1].rStartPos");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[1].rStartPos"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[1].rStartPos"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    int64Value=rand()%5000;
    UA_Variant_setScalar(&mnAttr.value, &int64Value, &UA_TYPES[UA_TYPES_INT64]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_HoldProfVis.Profile.Points[2].rStartPos");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[2].rStartPos"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[2].rStartPos"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    int64Value=rand()%5000;
    UA_Variant_setScalar(&mnAttr.value, &int64Value, &UA_TYPES[UA_TYPES_INT64]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_HoldProfVis.Profile.Points[3].rStartPos");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[3].rStartPos"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[3].rStartPos"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    int64Value=rand()%5000;
    UA_Variant_setScalar(&mnAttr.value, &int64Value, &UA_TYPES[UA_TYPES_INT64]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_HoldProfVis.Profile.Points[4].rStartPos");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[4].rStartPos"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[4].rStartPos"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    int64Value=rand()%5000;
    UA_Variant_setScalar(&mnAttr.value, &int64Value, &UA_TYPES[UA_TYPES_INT64]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_HoldProfVis.Profile.Points[5].rStartPos");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[5].rStartPos"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[5].rStartPos"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);


    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_HoldProfVis.Profile.Points[1].rPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[1].rPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[1].rPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);
    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_HoldProfVis.Profile.Points[2].rPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[2].rPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[2].rPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_HoldProfVis.Profile.Points[3].rPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[3].rPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[3].rPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);
    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_HoldProfVis.Profile.Points[4].rPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[4].rPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[4].rPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);
    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_HoldProfVis.Profile.Points[5].rPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[5].rPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_HoldProfVis.Profile.Points[5].rPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    int64Value=rand()%5000;
    UA_Variant_setScalar(&mnAttr.value, &int64Value, &UA_TYPES[UA_TYPES_INT64]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.CoolingTime1.sv_dCoolingTime");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.CoolingTime1.sv_dCoolingTime"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.CoolingTime1.sv_dCoolingTime"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_PlastProfVis.Profile.Points[1].rRotation");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[1].rRotation"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[1].rRotation"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_PlastProfVis.Profile.Points[2].rRotation");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[2].rRotation"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[2].rRotation"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_PlastProfVis.Profile.Points[3].rRotation");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[3].rRotation"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[3].rRotation"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_PlastProfVis.Profile.Points[4].rRotation");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[4].rRotation"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[4].rRotation"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_PlastProfVis.Profile.Points[5].rRotation");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[5].rRotation"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[5].rRotation"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);


    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_PlastProfVis.Profile.Points[1].rBackPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[1].rBackPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[1].rBackPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_PlastProfVis.Profile.Points[2].rBackPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[2].rBackPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[2].rBackPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_PlastProfVis.Profile.Points[3].rBackPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[3].rBackPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[3].rBackPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);
    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_PlastProfVis.Profile.Points[4].rBackPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[4].rBackPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[4].rBackPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);
    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_PlastProfVis.Profile.Points[5].rBackPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[5].rBackPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_PlastProfVis.Profile.Points[5].rBackPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);


    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.ShotVolume");
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1,2018), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.ShotVolume"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_CutOffParams.rPositionThreshold");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_CutOffParams.rPositionThreshold"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_CutOffParams.rPositionThreshold"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_CutOffParams.dTimeThreshold");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_CutOffParams.dTimeThreshold"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_CutOffParams.dTimeThreshold"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    int64Value=rand()%5000;
    UA_Variant_setScalar(&mnAttr.value, &int64Value, &UA_TYPES[UA_TYPES_INT64]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.system.sv_dCycleTime");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.system.sv_dCycleTime"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.system.sv_dCycleTime"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    int64Value=rand()%5000;
    UA_Variant_setScalar(&mnAttr.value, &int64Value, &UA_TYPES[UA_TYPES_INT64]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Mold1.sv_dActCloseTime");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Mold1.sv_dActCloseTime"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Mold1.sv_dActCloseTime"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    int64Value=rand()%5000;
    UA_Variant_setScalar(&mnAttr.value, &int64Value, &UA_TYPES[UA_TYPES_INT64]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Mold1.sv_dActOpenTime");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Mold1.sv_dActOpenTime"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Mold1.sv_dActOpenTime"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    int64Value=rand()%5000;
    UA_Variant_setScalar(&mnAttr.value, &int64Value, &UA_TYPES[UA_TYPES_INT64]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_InjectTimesAct.dActMoveTime");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_InjectTimesAct.dActMoveTime"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_InjectTimesAct.dActMoveTime"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    int64Value=rand()%5000;
    UA_Variant_setScalar(&mnAttr.value, &int64Value, &UA_TYPES[UA_TYPES_INT64]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_dActHoldTime");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_dActHoldTime"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_dActHoldTime"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_rCutOffPosition");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_rCutOffPosition"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_rCutOffPosition"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_rCushion");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_rCushion"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_rCushion"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.HeatingNozzle1.ti_InTemp1");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.HeatingNozzle1.ti_InTemp1"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.HeatingNozzle1.ti_InTemp1"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.OilMaintenance1.ti_OilTemp");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.OilMaintenance1.ti_OilTemp"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.OilMaintenance1.ti_OilTemp"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.HeatingNozzle1.ti_HopperTemp");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.HeatingNozzle1.ti_HopperTemp"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.HeatingNozzle1.ti_HopperTemp"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_rInjPeakPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_rInjPeakPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_rInjPeakPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Mold1.sv_rMoldIntegralPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Mold1.sv_rMoldIntegralPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Mold1.sv_rMoldIntegralPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_rActPressure");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_rActPressure"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_rActPressure"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.HeatingNozzle1.ti_InTemp2");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.HeatingNozzle1.ti_InTemp2"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.HeatingNozzle1.ti_InTemp2"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.HeatingNozzle1.ti_InTemp3");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.HeatingNozzle1.ti_InTemp3"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.HeatingNozzle1.ti_InTemp3"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.HeatingNozzle1.ti_InTemp4");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.HeatingNozzle1.ti_InTemp4"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.HeatingNozzle1.ti_InTemp4"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.HeatingNozzle1.ti_InTemp5");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.HeatingNozzle1.ti_InTemp5"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.HeatingNozzle1.ti_InTemp5"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.HeatingNozzle1.ti_InTemp6");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.HeatingNozzle1.ti_InTemp6"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.HeatingNozzle1.ti_InTemp6"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    int64Value=rand()%5000;
    UA_Variant_setScalar(&mnAttr.value, &int64Value, &UA_TYPES[UA_TYPES_INT64]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_PlastTimesAct.dActDelayTime");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_PlastTimesAct.dActDelayTime"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_PlastTimesAct.dActDelayTime"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Mold1.sv_MoldBwdProfVis.Profile.Points[2].rStartPos");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Mold1.sv_MoldBwdProfVis.Profile.Points[2].rStartPos"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Mold1.sv_MoldBwdProfVis.Profile.Points[2].rStartPos"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);

    floatValue=(double)rand()/RAND_MAX+rand()%1000;
    UA_Variant_setScalar(&mnAttr.value, &floatValue, &UA_TYPES[UA_TYPES_FLOAT]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "APPL.Injection1.sv_ScrewDecPositionAbs");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "APPL.Injection1.sv_ScrewDecPositionAbs"), APPLId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "APPL.Injection1.sv_ScrewDecPositionAbs"),
                              UA_NODEID_NULL, mnAttr, NULL, NULL);
    */
}