/*	Copyright 2019-2021 (c) Industrial Technology Research Institute (Author: Cheng-Ying Liu)
 *
 *	The subscribeOPCUA.c�is�the main program for the system to collect OPCUA data from OPCUA server.
 *	The program should be executed as a indepedent thread (i.e. add '&' in the end of command).
 *	An unsubscribe program is also provided and will forcely terminate the procress to default state.
 *	For The subscribeOPCUA, there are two required configurations which must be set before execution.
 *	(1) OPC UA Server Configuration and (2) OPC UA Version (including NameSpace and Node ID), which are seperately described as follows:
 *
 *	(1) OPC UA Server Configuration:
 *		This configuration including OPC UA IP and Port which are defined in IMMSNList.
 *		Note that the subscribe command should only append IMMSN instead of IP or Port,
 *		and the program will read them from IMMSNList according the IMMSN.
 *		For example, if user want to subscribe IMMSN 1, the command is as follows:
 *
 *		>./exeSubscribeOPCUA -IMMSN 1 &
 *
 *		After that, the OPC UA IP and Port will be read from IMMSNLlist with condition IMMSN = 1.
 *		Note that the symbol "&" is to create a new thread for this procress, and the PID will also be recorded in IMMSNList.
 *
 *	(2)	OPC UA Version (including NameSpace and NodeID):
 *		This configuration including the NameSpace, NodeID, Data Type and Convert Ratio.
 *		Firstly, the monitor item is decided when creating manufacturing order bases on OPC UA Version.
 *		After that, when executing subscribeOPCUA, the OPC UA client will subscribe some of OPC UA nodes(IMM Parameters),
 *		and the IMM sensor node (NameSpace and NodeID) will also be saved in an array.
 *		Note that, since the IMM parameter nodes are subscribed therefore, they new data will only be recorded when IMM parameters are changed.
 *		On the other hand, the IMM sensor data are recorded while the "Shot Complete" is changed(indicating a shot has complete).
 *
 *		It is worthy to notice that table will be created dynamically according to different situations, and this program will execute accordingly.
 *		Finally, after the feature value is caculated, the EC, QC and SPC module will judge the feature according to users' setting.
 *
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <mysql/mysql.h>
#include <unistd.h>
#include "open62541.h"
#include "itri_injpro.h"
#include "config.h"

int intIMMSN = 0;
int intMOSN = 0;
int intPiecePerShot = 0;
int intShotSN = 0;
int intInitialShotSN = 0;
int intLastInsertIMMSensorShotSN = 0;
int intExpectInsertIMMSensorShotSN = 0;
int intDiffIMMOPCUAShotSN = -1;
int intMoldClampedIMMOPCUAShotSN = -1;
int intMoldReleasedIMMOPCUAShotSN = -1;
int intShotCompleteIMMOPCUAShotSN = -1;
int intIMMSensorShotSN = -1;
int intOPCUAClientPID = 0;
int intModbusMoldSignalSource;
int intOPCUAVersionSN = 0;
int intIMMParaSN = 0;
int intSubscribeSN = 0;
int intCountAcceptCriteriaSN = 0;
int intExpectedProductVolume = 0;
int *intMOAcceptCriteriaSNTHFlag;
double *doubleMOAcceptCriteriaSNTH;
bool boolIsStopSignal = false;
int intModbusClientStatus;
bool boolIsThisShotReleased = false;
bool boolIsThisShotClamped = false;
bool boolHasThisShotReleased = false;
bool boolHasThisShotReleasing = false;
bool boolHasThisShotClamped = false;
bool boolHasThisShotClamping = false;
bool boolHasIMMParaChanged = false;
bool boolIMMParaUnstable = false;
bool boolEnableInsertIMMSensorData = false;

int intMonIDNodeIndex[OPCUA_IMMPARA_NODE_NUM] = {0};
char charMonIDNodeIndex[OPCUA_IMMPARA_NODE_NUM + 1][SMALL_STRING_SIZE];
char charOPCUAIMMSensorNodeList[OPCUA_IMMSENSOR_NODE_NUM + 1][OPCUA_META_NUM][MEDIUM_STRING_SIZE];
double doubleOPCUAConvertRatio[OPCUA_IMMPARA_NODE_NUM + OPCUA_IMMSENSOR_NODE_NUM + 1];

static void OPCUA_CallbackState(UA_Client *OPCUAClient, UA_ClientState OPCUAClientState);
static void OPCUA_CallbackSubscriptionInactivity(UA_Client *OPCUAClient, UA_UInt32 subId, void *subContext);
static void OPCUA_HandlerStopSubscribeOPCUA(int intSign);
static void OPCUA_HandlerIMMChanged(UA_Client *OPCUAClient, UA_UInt32 subId, void *subContext, UA_UInt32 monId, void *monContext, UA_DataValue *value);

int OPCUA_GetElapsedTime(struct timespec stStartTimeStamp, double *doubleElapsedTime);
int OPCUA_SubscribeMonitorItem(UA_Client *OPCUAClient, MYSQL mysqlCon);
int OPCUA_InsertPreviousIMMPara(MYSQL mysqlCon);
int OPCUA_InsertIMMSensor(MYSQL mysqlCon, UA_Client *OPCUAClient, int intShotSN);

int main(int argc, char *argv[])
{
	char charOPCUAIP[16];
	unsigned int intOPCUAPort;
	unsigned int intIMMModelSN;
	unsigned int intUserID;
	int intRetval;
	int intLogSN;
	int intOPCUAStatus = OPCUA_CLIENT_STATUS_UNKNOW;
	char charEndPointURL[MEDIUM_STRING_SIZE] = {'\0'};
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	char charTableName[MEDIUM_STRING_SIZE];
	char *charIMMParaSN;
	char *charSubscribeSN;
	bool boolHasInsertErrMsg = false;
	MYSQL mysqlCon;
	struct timespec stConnectTimeStamp;
	double doubleConnectElapsedTime;

	signal(SIGINT, OPCUA_HandlerStopSubscribeOPCUA);
	signal(SIGTERM, OPCUA_HandlerStopSubscribeOPCUA);
	signal(SIGKILL, OPCUA_HandlerStopSubscribeOPCUA);

	// Connect to MYSQL Server
	mysql_init(&mysqlCon);
	if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
	{
		fprintf(stderr, "MYSQL connection failed.\n");
		if (mysql_errno(&mysqlCon))
		{
			fprintf(stderr, "MYSQL connection error %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		}
		return EXIT_FAILURE;
	}

	// Get IMMSN from Parameters
	if (argc == 3 && strcmp(argv[1], "-IMMSN") == 0)
	{
		intIMMSN = atoi(argv[2]);
	}
	else
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to subscribe IMM due to parameter format error");
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, 0, intIMMSN, EXIT_FAILURE, charErrMsg);
		return EXIT_FAILURE;
	}

	// Select IMM Model, MOSN andOPCUA Information According to The Given IMMSN
	snprintf(charStatement, MAX_STRING_SIZE,
			 "SELECT IMMModelSN,MOSN,OPCUAVersionSN,OPCUAIP,OPCUAPort,OPCUAClientPID,ModbusMoldSignalSource,ShotSN FROM %s_%s.IMMSNList WHERE IMMSN=%d",
			 SYS_NAME, DATA_DATABASE_NAME, intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to select information where IMMSN=%d from %s_%s.IMMSNList (%d):%s",
				 intIMMSN, SYS_NAME, DATA_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, 0, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return EXIT_FAILURE;
	}

	MYSQL_RES *mysqlResult = mysql_store_result(&mysqlCon);
	MYSQL_ROW mysqlRow = mysql_fetch_row(mysqlResult);
	if (mysqlRow == NULL)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to subscribe IMM parameter due to IMMSN=%d is not found in %s_%s.IMMSNList",
				 intIMMSN, SYS_NAME, DATA_DATABASE_NAME);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, 0, charErrMsg);
		return EXIT_FAILURE;
	}

	// Get IMMModelSN
	if (mysqlRow[0] == NULL)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to select IMMModelSN of IMMSN:%d", intIMMSN);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, 0, intIMMSN, EXIT_FAILURE, charErrMsg);
		return EXIT_FAILURE;
	}
	else
		intIMMModelSN = atoi(mysqlRow[0]);

	// Get MOSN
	if (mysqlRow[1] == NULL)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to select MOSN for IMMSN:%d", intIMMSN);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, 0, intIMMSN, EXIT_FAILURE, charErrMsg);
		return EXIT_FAILURE;
	}
	else
		intMOSN = atoi(mysqlRow[1]);

	// Get OPCUAVersionSN, OPCUA IP, OPCUA Port
	if (mysqlRow[2] == NULL || mysqlRow[3] == NULL || mysqlRow[4] == NULL)
	{
		// snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to select OPCUAVersionSN for IMMSN:%d", intIMMSN);
		// SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
		// return EXIT_FAILURE;
		intOPCUAVersionSN = VIRTUAL_OPCUA_SERVER_OPCUA_VERSIONSN;
		strcpy(charOPCUAIP, VIRTUAL_OPCUA_SERVER_IP);
		intOPCUAPort = VIRTUAL_OPCUA_SERVER_PORT;
		fprintf(stderr, "[SubscribeOPCUA]OPCUAVersionSN:Localhost Virtual OPCUA Version\n");
	}
	else
	{
		intOPCUAVersionSN = atoi(mysqlRow[2]);
		fprintf(stderr, "[SubscribeOPCUA]OPCUAVersionSN:%d\n", intOPCUAVersionSN);
		strcpy(charOPCUAIP, mysqlRow[3]);
		intOPCUAPort = atoi(mysqlRow[4]);
	}
	fprintf(stderr, "[SubscribeOPCUA]OPCUA Address:%s:%d\n", charOPCUAIP, intOPCUAPort);

	// Get ModbusMoldSignalSource
	if (mysqlRow[6] == NULL)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to select ModbusMoldSignalSource for IMMSN:%d", intIMMSN);
		// SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, 0, intIMMSN, EXIT_FAILURE, charErrMsg);
		// return EXIT_FAILURE;
	}
	else
		intModbusMoldSignalSource = atoi(mysqlRow[6]);
	fprintf(stderr, "[SubscribeOPCUA]ModbusMoldSignalSource:%d\n", intModbusMoldSignalSource);

	// Get Subscribe Procress ID
	if (mysqlRow[5] != NULL)
	{
		intOPCUAClientPID = atoi(mysqlRow[5]);

		// Notify An Existing Subscribe Porcess Is Executing
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]A subscribe process PID:%d of IMMSN:%d is executing, terminating...\n", intOPCUAClientPID, intIMMSN);
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);

		// Kill the Subscribe Procress
		intRetval = kill(intOPCUAClientPID, SIGTERM);
		if (intRetval != EXIT_SUCCESS)
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to terminate subscribe process PID:%d of IMMSN:%d.\n", intOPCUAClientPID, intIMMSN);
			intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
			// mysql_close(&mysqlCon);
			// return intLogSN;
		}
		else
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]The subscribe process PID:%d of IMMSN:%d is terminated successfully\n", intOPCUAClientPID, intIMMSN);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		}

		snprintf(charStatement, MAX_STRING_SIZE, "UPDATE %s_%s.IMMSNList SET OPCUAClientStatus = %d, OPCUAClientPID = NULL WHERE IMMSN=%d",
				 SYS_NAME, DATA_DATABASE_NAME, OPCUA_CLIENT_STATUS_UNKNOW, intIMMSN);
		intRetval = mysql_query(&mysqlCon, charStatement);
		if (intRetval != EXIT_SUCCESS)
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to Update OPCUAClientPID=NULL for IMMSN=%d while terminating (%d):%s\n",
					 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
			intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
			mysql_close(&mysqlCon);
			return intLogSN;
		}

		// Get timestamp before connect to OPC UA server
		clock_gettime(CLOCK_REALTIME, &stConnectTimeStamp);

		// Wait For The OPCUAClientPID = NULL
		do
		{
			snprintf(charStatement, MAX_STRING_SIZE, "SELECT OPCUAClientPID FROM %s_%s.IMMSNList WHERE IMMSN=%d",
					 SYS_NAME, DATA_DATABASE_NAME, intIMMSN);
			intRetval = mysql_query(&mysqlCon, charStatement);
			if (intRetval == EXIT_FAILURE && boolHasInsertErrMsg == false)
			{
				snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to select OPCUAClientPID for IMMSN=%d while exiting (%d):%s\n",
						 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
				boolHasInsertErrMsg = true;
			}
			mysqlResult = mysql_store_result(&mysqlCon);
			mysqlRow = mysql_fetch_row(mysqlResult);

			OPCUA_GetElapsedTime(stConnectTimeStamp, &doubleConnectElapsedTime);

			if (mysqlRow[0] != NULL && doubleConnectElapsedTime > UA_CLIENT_TIME_OUT_SEC)
			{
				snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]OPC UA Client terminated. Fail to terminate PID:%d of IMMSN:%d (> %d)\n",
						 intOPCUAClientPID, intIMMSN, UA_CLIENT_TIME_OUT_SEC);
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
				mysql_close(&mysqlCon);
				return EXIT_FAILURE;
			}
			else if (mysqlRow[0] != NULL)
			{
				fprintf(stderr, "[SubscribeOPCUA]Check OPCUAClientPID after 1 second...\n");
				sleep(1);
			}

		} while (mysqlRow[0] != NULL);
	}
	mysql_free_result(mysqlResult);

	// Select MO Meta Information from INJPRO_Data_MO.MOSNList
	snprintf(charStatement, MAX_STRING_SIZE, "SELECT PiecePerShot FROM %s_%s_%s.MOSNList WHERE MOSN=%d",
			 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to select MO meta information from %s_%s_%s.MOSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return EXIT_FAILURE;
	}
	mysqlResult = mysql_store_result(&mysqlCon);
	mysqlRow = mysql_fetch_row(mysqlResult);
	intPiecePerShot = atoi(mysqlRow[0]);
	fprintf(stderr, "[SubscribeOPCUA]PiecePerShot:%d\n", intPiecePerShot);
	mysql_free_result(mysqlResult);

	/*****************************************************************************************
	//Select MAX AcceptCriteriaSN from INJPRO_Data_MO_[MOSN]_Info_Meta.MOAcceptCriteriaSNList
	snprintf(charStatement, MAX_STRING_SIZE,
			 "SELECT MOAcceptCriteriaSN,MOAcceptCriteriaClass,MOAcceptCriteriaMinTH,MOAcceptCriteriaMaxTH FROM %s_%s_%s_%d_Info_Meta.MOAcceptCriteriaSNList", SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to select information from %s_%s_MO_%d_Info_Meta.MOAcceptCriteriaSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return EXIT_FAILURE;
	}
	mysqlResult = mysql_store_result(&mysqlCon);
	intCountAcceptCriteriaSN = mysql_num_rows(mysqlResult);

		//intMOAcceptCriteriaSNTHFlag[N*4] where N=intCountAcceptCriteriaSN
		//intMOAcceptCriteriaSNTHFlag[N]=MOAcceptCriteriaSN
		//intMOAcceptCriteriaSNTHFlag[N+1]=AcceptCriteriaClass
		//intMOAcceptCriteriaSNTHFlag[N+2]=Min threshold is not null
		//intMOAcceptCriteriaSNTHFlag[N+3]=Max threshold is not null

	intMOAcceptCriteriaSNTHFlag = (int *)malloc(intCountAcceptCriteriaSN * ACCEPTCRATERIA_META_NUM * sizeof(int));
	doubleMOAcceptCriteriaSNTH = (double *)malloc(intCountAcceptCriteriaSN * 2 * sizeof(double));
	for (int i = 0; mysqlRow = mysql_fetch_row(mysqlResult); i++)
	{

		//Initialization
		*(intMOAcceptCriteriaSNTHFlag + i * ACCEPTCRATERIA_META_NUM + ACCEPTCRATERIA_META_SN) = atoi(mysqlRow[0]);
		*(intMOAcceptCriteriaSNTHFlag + i * ACCEPTCRATERIA_META_NUM + ACCEPTCRATERIA_META_CLASS) = atoi(mysqlRow[1]);
		*(intMOAcceptCriteriaSNTHFlag + i * ACCEPTCRATERIA_META_NUM + ACCEPTCRATERIA_META_HASMIN) = false;
		*(intMOAcceptCriteriaSNTHFlag + i * ACCEPTCRATERIA_META_NUM + ACCEPTCRATERIA_META_HASMAX) = false;

		//If AcceptCriteriaMinTH!=NULL
		if (mysqlRow[2] != NULL)
		{
			*(intMOAcceptCriteriaSNTHFlag + i * ACCEPTCRATERIA_META_NUM + ACCEPTCRATERIA_META_HASMIN) = true;
			*(doubleMOAcceptCriteriaSNTH + i * 2) = (double)atof(mysqlRow[2]);

			fprintf(stderr, "[SubscribeOPCUA]MOSN:%d MOAcceptCrateriaSN:%d has min threshold:%lf\n",
					intMOSN, atoi(mysqlRow[0]), (double)atof(mysqlRow[2]));
		}

		//If AcceptCriteriaMaxTH!=NULL
		if (mysqlRow[3] != NULL)
		{
			*(intMOAcceptCriteriaSNTHFlag + i * ACCEPTCRATERIA_META_NUM + ACCEPTCRATERIA_META_HASMAX) = true;
			*(doubleMOAcceptCriteriaSNTH + i * 2 + 1) = (double)atof(mysqlRow[3]);

			fprintf(stderr, "[SubscribeOPCUA]MOSN:%d MOAcceptCrateriaSN:%d has max threshold:%lf\n",
					intMOSN, atoi(mysqlRow[0]), (double)atof(mysqlRow[3]));
		}
	}
	mysqlRow = mysql_fetch_row(mysqlResult);
	mysql_free_result(mysqlResult);
	**********************************************************************************************************/

	// Update OPCUAClientPID and IMMLastUpdateTime to INJPRO_Data.IMMSNList
	snprintf(charStatement, MAX_STRING_SIZE, "UPDATE %s_%s.IMMSNList SET OPCUAClientPID=%d,IMMLastUpdateTime=NOW(6) WHERE IMMSN=%d",
			 SYS_NAME, DATA_DATABASE_NAME, getpid(), intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE,
				 "Fail to update OPCUAClientPID=getpid() and IMMLastUpdateTime=NOW(6) where IMMSN=%d (%d):%s",
				 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return EXIT_FAILURE;
	}

	// Check ShotSN
	intRetval = DB_SelectShotSNbyMOSN(mysqlCon, intMOSN, &intShotSN);
	if (intRetval != EXIT_SUCCESS)
	{
		fprintf(stderr, "[SubscribeOPCUA]Fail to select ShotSN for IMMSN:%d MOSN:%d\n", intIMMSN, intMOSN);
		return EXIT_FAILURE;
	}
	else
	{
		fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d resumes the MOSN:%d from ShotSN:%d\n", intIMMSN, intMOSN, intShotSN);
	}

	// Initial ShotSN Is Set According to Database
	intInitialShotSN = intShotSN;

	// Assume The Last IMM Sensor ShotSN = intInitialShotSN - 1 (Previous ShotSN)
	intLastInsertIMMSensorShotSN = intShotSN - 1;

	// Update ShotSN to INJPRO_Data.IMMSNList
	snprintf(charStatement, MAX_STRING_SIZE, "UPDATE %s_%s.IMMSNList SET ShotSN=%d, IMMLastUpdateTime=NOW(6) WHERE IMMSN=%d",
			 SYS_NAME, DATA_DATABASE_NAME, intShotSN, intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE,
				 "Fail to update ShotSN:%d to %s_%s.IMMSNList where IMMSN:%d (%d): %s",
				 intShotSN, SYS_NAME, DATA_DATABASE_NAME, intIMMSN,
				 mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return EXIT_FAILURE;
	}

	// Select MAX IMMParaSN in INJPRO_DATA_MO_[MOSN]_RawData_IMMPara.IMMParaSNList
	snprintf(charStatement, MAX_STRING_SIZE, "%s_%s_%s_%d_RawData_IMMPara.IMMParaSNList",
			 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	intRetval = DB_SelectColumnMax(mysqlCon, charStatement, "IMMParaSN", &charIMMParaSN);

	// New the next IMM parameter
	intIMMParaSN = atoi(charIMMParaSN) + 1;

	// Select MAX SubscribeSN in INJPRO_DATA_MO_[MOSN]_RawData_IMMPara.IMMParaSNList
	snprintf(charStatement, MAX_STRING_SIZE, "%s_%s_%s_%d_RawData_IMMPara.IMMParaSNList",
			 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	intRetval = DB_SelectColumnMax(mysqlCon, charStatement, "SubscribeSN", &charSubscribeSN);
	// New the next IMM parameter
	intSubscribeSN = atoi(charSubscribeSN) + 1;

	// Set Convert ratio
	intRetval = OPCUA_InitConvertRatio(mysqlCon, intOPCUAVersionSN, doubleOPCUAConvertRatio);

	// Update OPCUAClientStatus = OPCUA_CLIENT_STATUS_UNKNOW in IMMSNList
	snprintf(charStatement, MAX_STRING_SIZE,
			 "UPDATE %s_%s.IMMSNList SET OPCUAClientStatus = %d ,IMMLastUpdateTime = NOW(6) WHERE IMMSN=%d",
			 SYS_NAME, DATA_DATABASE_NAME, OPCUA_CLIENT_STATUS_UNKNOW, intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE,
				 "[SubscribeModbus]Fail to update OPCUAClientStatus = UNKNOW and IMMLastUpdateTime=NOW(6) where IMMSN=%d (%d):%s",
				 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
	}

	// Close MySQL Connection
	mysql_close(&mysqlCon);

	if (intRetval == EXIT_FAILURE)
		return EXIT_FAILURE;

	// Connect to OPC UA Server
	UA_ClientConfig config = UA_ClientConfig_default;
	config.stateCallback = OPCUA_CallbackState;
	config.subscriptionInactivityCallback = OPCUA_CallbackSubscriptionInactivity;
	UA_Client *OPCUAClient = UA_Client_new(config);

	// Get timestamp before connect to OPC UA server
	clock_gettime(CLOCK_REALTIME, &stConnectTimeStamp);

	snprintf(charEndPointURL, MEDIUM_STRING_SIZE, "opc.tcp://%s:%d", charOPCUAIP, intOPCUAPort);
	// while (boolIsStopSignal == false && (intRetval != UA_STATUSCODE_GOOD && doubleConnectElapsedTime < UA_CLIENT_TIME_OUT_SEC))
	while (boolIsStopSignal == false)
	{
		// fprintf(stderr,"[SubscribeOPCUA]Connect to %s...\n",charEndPointURL);
		// fprintf(stderr,"[SubscribeOPCUA]Connectstate=%d\n",UA_Client_getState(OPCUAClient));
		intRetval = UA_Client_connect(OPCUAClient, charEndPointURL);

		// Fail to Connect to OPC UA Server
		if (intRetval != UA_STATUSCODE_GOOD)
		{
			fprintf(stderr, "[SubscribeOPCUA]Fail to connect to %s. \n", charEndPointURL);
			// The OPC UA Status in Database is not Equal to Current Status, then Update it
			if (intOPCUAStatus != intRetval)
			{
				intOPCUAStatus = intRetval;
				// Get timestamp before reconnect to OPC UA server
				clock_gettime(CLOCK_REALTIME, &stConnectTimeStamp);
				// Connect to MYSQL Server
				mysql_init(&mysqlCon);
				// Fail to Connect to MySQL Database
				if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
				{
					fprintf(stderr, "[SubscribeOPCUA]MYSQL connection failed.\n");
					if (mysql_errno(&mysqlCon))
					{
						fprintf(stderr, "[SubscribeOPCUA]MYSQL connection error %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
					}
					// Disconnect OPC UA Client and Deliete it
					// UA_Client_disconnect(OPCUAClient);
					UA_Client_delete(OPCUAClient);
					return EXIT_SUCCESS;
				}
				// Connect to MySQL Successfully
				else
				{
					// Insert error message
					snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to subscribe IMM:%d due to OPC UA connection fail", intIMMSN);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_OPCUA_CLIENT, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);

					// Update OPCUAClientStatus = OPCUA_CLIENT_STATUS_UNKNOW in IMMSNList
					snprintf(charStatement, MAX_STRING_SIZE, "UPDATE %s_%s.IMMSNList SET OPCUAClientStatus = %d ,IMMLastUpdateTime = NOW(6) WHERE IMMSN=%d",
							 SYS_NAME, DATA_DATABASE_NAME, OPCUA_CLIENT_STATUS_UNKNOW, intIMMSN);
					intRetval = mysql_query(&mysqlCon, charStatement);

					// Fail to Insert Error Message into Database
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "[SubscribeOPCUA]Fail to update OPCUAClientStatus = OPCUA_CLIENT_STATUS_UNKNOW and IMMLastUpdateTime = NOW(6) where IMMSN=%d (%d):%s",
								 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);

						// Close MySQL Connection
						mysql_close(&mysqlCon);

						// Disconnect OPC UA Client and Deliete it
						UA_Client_disconnect(OPCUAClient);
						UA_Client_delete(OPCUAClient);
						return EXIT_SUCCESS;
					}
				}
			}
			// OPC UA Status!= UA_STATUSCODE_GOOD && intOPCUAStatus == OPC UA Status && Elapsed Time > Connect Elapsed Time
			else
			{
				// Get Elapsed Time of Connection
				OPCUA_GetElapsedTime(stConnectTimeStamp, &doubleConnectElapsedTime);

				if (doubleConnectElapsedTime > UA_CLIENT_TIME_OUT_SEC)
				{
					boolIsStopSignal = true;
				}
			}
		}
		// Successly Connect to OPC UA Server
		else
		{
			if (intOPCUAStatus != intRetval)
			{
				intOPCUAStatus = intRetval;
				// Connect to MYSQL Server
				mysql_init(&mysqlCon);
				if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
				{
					fprintf(stderr, "[SubscribeOPCUA]MYSQL connection failed.\n");
					if (mysql_errno(&mysqlCon))
					{
						fprintf(stderr, "[SubscribeOPCUA]MYSQL connection error %d: %s while connect to OPC UA server\n",
								mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
					}
					// Disconnect OPC UA Client and Deliete it
					UA_Client_disconnect(OPCUAClient);
					UA_Client_delete(OPCUAClient);
					return EXIT_SUCCESS;
				}
			}
		}
		UA_Client_runAsync(OPCUAClient, UA_CLIENT_INTERVAL);
	}

	// Connect to MYSQL Server
	mysql_init(&mysqlCon);
	if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
	{
		fprintf(stderr, "MYSQL connection failed..\n");
		if (mysql_errno(&mysqlCon))
		{
			fprintf(stderr, "MYSQL connection error %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		}
		return EXIT_FAILURE;
	}

	// Get Current OPCUAClientPID
	snprintf(charStatement, MAX_STRING_SIZE, "SELECT OPCUAClientPID FROM %s_%s.IMMSNList WHERE IMMSN=%d",
			 SYS_NAME, DATA_DATABASE_NAME, intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to select OPCUAClientPID for IMMSN=%d while exiting (%d):%s",
				 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
	}
	mysqlResult = mysql_store_result(&mysqlCon);
	mysqlRow = mysql_fetch_row(mysqlResult);

	// Check Current OPCUAClientPID == getpid()
	if (mysqlRow[0] != NULL && atoi(mysqlRow[0]) == getpid())
	{

		// Set OPCUAClientPID=NULL before exit
		snprintf(charStatement, MAX_STRING_SIZE,
				 "UPDATE %s_%s.IMMSNList SET OPCUAClientStatus = %d , OPCUAClientPID = NULL, IMMLastUpdateTime=NOW(6) WHERE IMMSN=%d",
				 SYS_NAME, DATA_DATABASE_NAME, OPCUA_CLIENT_STATUS_UNKNOW, intIMMSN);
		intRetval = mysql_query(&mysqlCon, charStatement);
		if (intRetval)
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to update OPCUAClientPID = OPCUA_CLIENT_STATUS_UNKNOW and IMMLastUpdateTime for IMMSN=%d (%d):%s",
					 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		}
	}

	// Free mysqlResult
	mysql_free_result(mysqlResult);

	// Close MySQL Connection
	mysql_close(&mysqlCon);

	// Disconnect OPC UA Client and Deliete it
	UA_Client_disconnect(OPCUAClient);
	UA_Client_delete(OPCUAClient);

	fprintf(stderr, "[SubscribeOPCUA]OPCUAClientPID=%d Terminated.\n", getpid());
	return EXIT_SUCCESS;
}

/* [OPCUA]Callback Function of OPC UA Client State.
 * If the OPCUAClientState code is UA_CLIENTSTATE_DISCONNECTED then trigger the stop sign,
 * else if the code is UA_CLIENTSTATE_SESSION, which indicates the connection has been built,
 * then the client subscribes all IMM paramater nodes.
 *
 * @param OPCUAClient to OPC UA client
 * @param OPCUAClientState to the state of UA Client */
static void OPCUA_CallbackState(UA_Client *OPCUAClient, UA_ClientState OPCUAClientState)
//[OPCUA]Callback Function of OPC UA Client State
{
	MYSQL mysqlCon;
	int intRetval;
	char charErrMsg[LONG_STRING_SIZE];
	char charStatement[MAX_STRING_SIZE];

	// Check Client State
	switch (OPCUAClientState)
	{

	case UA_CLIENTSTATE_DISCONNECTED:
	{
		// The OPCUAClient is disconnected
		fprintf(stderr, "[OPCUA_CallbackState]IMMSN:%d OPCUA OPCUAClient is disconnected\n", intIMMSN);
		// boolIsStopSignal = true;
	}
	break;

	case UA_CLIENTSTATE_SESSION:
	{
		fprintf(stderr, "[OPCUA_CallbackState]IMMSN:%d A session with the server is open\n", intIMMSN);

		// Connect to MYSQL Server
		mysql_init(&mysqlCon);
		if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
		{
			fprintf(stderr, "MYSQL connection failed.\n");
			if (mysql_errno(&mysqlCon))
			{
				fprintf(stderr, "MYSQL connection error %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
			}
			return;
		}

		// Update OPCUAClientPID and IMMLastUpdateTime to INJPRO_Data.IMMSNList
		snprintf(charStatement, MAX_STRING_SIZE,
				 "UPDATE %s_%s.IMMSNList SET OPCUAClientPID=%d,IMMLastUpdateTime=NOW(6) WHERE IMMSN=%d",
				 SYS_NAME, DATA_DATABASE_NAME, getpid(), intIMMSN);
		intRetval = mysql_query(&mysqlCon, charStatement);
		if (intRetval)
		{
			snprintf(charErrMsg, LONG_STRING_SIZE,
					 "Fail to update OPCUAClientPID and IMMLastUpdateTime for IMMSN=%d (%d):%s",
					 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
			return;
		}

		OPCUA_SubscribeMonitorItem(OPCUAClient, mysqlCon);
		mysql_close(&mysqlCon);
	}
	break;
	}
	return;
}

/* [OPCUA]Callback Function of Inactivity of Subscription.
 * This call back function is called when a subscription of the OPC UA Client is in inactivity.
 *
 * @param OPCUAClient to OPC UA client
 * @param subId to indicate a subscription
 * @param subContext to the content of the subscription */
static void OPCUA_CallbackSubscriptionInactivity(UA_Client *OPCUAClient, UA_UInt32 subId, void *subContext)
//[OPCUA]Callback Function of Inactivity of Subscription
{
	fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d OPC UA Client Inactivity for subscription %u", intIMMSN, subId);
	boolIsStopSignal = true;
}

/* [OPCUA]Handler of Stop Subscription
 *
 * @param intSign to indicate a specified sign to be done in handler */
static void OPCUA_HandlerStopSubscribeOPCUA(int intSign)
//[OPCUA]Handler of Stop Subscription
{
	fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d OPC UA Client Stop ", intIMMSN);
	boolIsStopSignal = true;
}

/* [OPCUA]Handler of the Changes from Subscribed OPC UA Node
 * This call back function is called when a monitor item is changed. In this system,
 * there are two categories of monitor item, which is (1)IMM basic signal and (2)IMM parameters.
 *
 * @param OPCUAClient to OPC UA client
 * @param subId to indicate a subscription
 * @param subContext to the content of the subscription
 * @param monId to indicate a monitor item ID
 * @param monContext to the content of the monitor item
 * @param uaDataValue to the value of monitor item */
static void OPCUA_HandlerIMMChanged(UA_Client *OPCUAClient, UA_UInt32 subId, void *subContext, UA_UInt32 monId, void *monContext, UA_DataValue *uaDataValue)
//[OPCUA]Handler of the Changes from Subscribed OPC UA Node
{

	MYSQL mysqlCon;
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	char *charIMMParaSN;
	char charIMMParaValue[OPCUA_IMMPARA_NODE_NUM][TINY_STRING_SIZE];
	unsigned int intMOCustomSensorTableSN;
	unsigned int intTechnicianUserSN;
	bool boolErrorShot = false;
	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;
	char charCommand[MAX_STRING_SIZE];
	unsigned int intRoundSN;
	unsigned int intECProgressFlag;
	unsigned int intSPCProgressFlag;
	unsigned int intQCProgressFlag;

	mysql_init(&mysqlCon);
	if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
	{
		if (mysql_errno(&mysqlCon))
			fprintf(stderr, "Fail to connect to MySql server %d: %s", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		return;
	}

	// Select Technician UserSN Where IMMSN Is Given From INJPRO_Data_IMMSNList
	/*
	snprintf(charStatement, MAX_STRING_SIZE, "SELECT TechnicianUserSN FROM %s_%s.IMMSNList WHERE IMMSN=%d",
			 SYS_NAME, DATA_DATABASE_NAME, intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[OPCUA_HandlerIMMChanged]Fail to select TechnicianUserSN where IMMSN=%d from %s_%s.IMMSNList (%d):%s",
				 intIMMSN, SYS_NAME, DATA_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		mysqlRow = mysql_fetch_row(mysqlResult);
		if (mysqlRow[0] != NULL)
		{
			intTechnicianUserSN = atoi(mysqlRow[0]);
		}
		else
		{
			intTechnicianUserSN = 0;
		}
		mysql_free_result(mysqlResult);
	}
	*/
	// fprintf(stderr,"ID:%d\n",intTechnicianUserSN);

	// If IMM Paramter Has Not Changed, then Insert a New Record of Previous IMM Parameters
	if (intMonIDNodeIndex[monId] >= OPCUA_FIRST_IMMPARA && boolHasIMMParaChanged == false)
	{
		boolHasIMMParaChanged = true;
		intRetval = OPCUA_InsertPreviousIMMPara(mysqlCon);
		if (intRetval != EXIT_SUCCESS)
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to insert previous IMM parameter for IMMSN:%d while ShotSN:%d", intIMMSN, intShotSN);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		}
	}

	// If The Current Mold Status is Mold Clamped, and The IMM Parameter Is Changed
	if (intMonIDNodeIndex[monId] >= OPCUA_FIRST_IMMPARA && boolIsThisShotClamped == true)
	{
		// snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]IMMSN:%d ShotSN:%d TechnicianUserSN:%d Parameter:%s is changed after mold clamped",
		//		 intIMMSN, intShotSN, intTechnicianUserSN, charMonIDNodeIndex[monId]);
		// SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, 0, charErrMsg);
		fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d ShotSN:%d TechnicianUserSN:%d Parameter:%s is changed after mold clamped", intIMMSN, intShotSN, intTechnicianUserSN, charMonIDNodeIndex[monId]);
		boolIMMParaUnstable = true;
	}

	// fprintf(stderr,"intMonIDNodeIndex[monId]:%d\n",intMonIDNodeIndex[monId]);

	if (intMonIDNodeIndex[monId] == DO_MOLD_CLAMPED) // Mold Clamped == true
	{
		// Mold Clamped
		if ((uaDataValue->value.type->typeIndex == UA_TYPES_BOOLEAN && *(UA_Boolean *)uaDataValue->value.data == true) ||
			(uaDataValue->value.type->typeIndex == UA_TYPES_DOUBLE && *(UA_Double *)uaDataValue->value.data == 1.0) ||
			(uaDataValue->value.type->typeIndex == UA_TYPES_STRING && *((UA_String *)(uaDataValue->value.data))->data == '1'))
		{
			fprintf(stderr, "[SubscribeOPCUA]The mold of IMMSN:%d is clamped\n", intIMMSN);
			boolIsThisShotClamped = true;
			boolIsThisShotReleased = false;
			boolHasThisShotClamped = true;

			//fprintf(stderr,"intModbusMoldSignalSource:%d\n",intModbusMoldSignalSource);
			if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_MODBUS)
			{
				// Check Modbus Status To See If It Is Ready
				intRetval = MODBUS_SelectModbusClientStatusbyIMMSN(mysqlCon, intIMMSN, &intModbusClientStatus);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to select ModbusClientStatus for IMMSN:%d while mold clamped", intIMMSN);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
				}
				else if (intModbusClientStatus != MODBUS_CLIENT_STATUS_RUNNING)
				{
					fprintf(stderr, "[SubscribeOPCUA]Waiting modbus ready while mold calmped.\n");
				}
				else
				{
					fprintf(stderr, "[SubscribeOPCUA]Modbus is ready while mold calmped.\n");

					intRetval = DB_SelectShotSNbyIMMSN(mysqlCon, intIMMSN, &intShotSN);
					if (intRetval != EXIT_SUCCESS)
					{
						fprintf(stderr, "[SubscribeOPCUA]Fail to select ShotSN by MOSN:%d for IMMSN:%d while shot complete.\n", intMOSN, intIMMSN);
						return;
					}
					intExpectInsertIMMSensorShotSN = intShotSN;
					fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d MOSN:%d ShotSN:%d has started. ExpectInsertIMMSensorShotSN=%d\n", intIMMSN, intMOSN, intShotSN, intExpectInsertIMMSensorShotSN);

					UA_Variant *readVariant = UA_Variant_new();
					intRetval = OPCUA_ReadOPCUANodeValue(mysqlCon, intIMMSN, DO_SHOT_COMPLETE, readVariant);
					if (intRetval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(readVariant))
					{
						if (readVariant->type == &UA_TYPES[UA_TYPES_FLOAT])
							intMoldClampedIMMOPCUAShotSN = (int)(*(UA_Float *)readVariant->data);
						else if (readVariant->type == &UA_TYPES[UA_TYPES_DOUBLE])
							intMoldClampedIMMOPCUAShotSN = (int)(*(UA_Double *)readVariant->data);
						else if (readVariant->type == &UA_TYPES[UA_TYPES_INT16])
							intMoldClampedIMMOPCUAShotSN = (int)(*(UA_Int16 *)readVariant->data);
						else if (readVariant->type == &UA_TYPES[UA_TYPES_UINT16])
							intMoldClampedIMMOPCUAShotSN = (int)(*(UA_UInt16 *)readVariant->data);
						else if (readVariant->type == &UA_TYPES[UA_TYPES_INT32])
							intMoldClampedIMMOPCUAShotSN = (int)(*(UA_Int32 *)readVariant->data);
						else if (readVariant->type == &UA_TYPES[UA_TYPES_UINT32])
							intMoldClampedIMMOPCUAShotSN = (int)(*(UA_UInt32 *)readVariant->data);
						else if (readVariant->type == &UA_TYPES[UA_TYPES_INT64])
							intMoldClampedIMMOPCUAShotSN = (int)(*(UA_Int64 *)readVariant->data);
						else if (readVariant->type == &UA_TYPES[UA_TYPES_UINT64])
							intMoldClampedIMMOPCUAShotSN = (int)(*(UA_UInt64 *)readVariant->data);
						else if (readVariant->type == &UA_TYPES[UA_TYPES_STRING])
							intMoldClampedIMMOPCUAShotSN = atoi(((UA_String *)(readVariant->data))->data);
						intDiffIMMOPCUAShotSN = (intShotSN - 1) - intMoldClampedIMMOPCUAShotSN;
						boolEnableInsertIMMSensorData = true;
						fprintf(stderr, "[SubscribeOPCUA][Clamped]EnableInsertIMMSensorData:%d ClampedIMMOPCUAShotSN:%d for ShotSN:%d Diff:%d\n",
								boolEnableInsertIMMSensorData, intMoldClampedIMMOPCUAShotSN, intShotSN - 1, intDiffIMMOPCUAShotSN);
					}
					UA_Variant_delete(readVariant);
				}
			}
			else if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_OPCUA)
			{
				// Set OPC UA Node Value : Mold Clamped
				OPCUA_WriteOPCUANodeValue(mysqlCon, VIRTUAL_OPCUA_SERVER_IMMSN, DO_MOLD_RELEASED, "0");
				OPCUA_WriteOPCUANodeValue(mysqlCon, VIRTUAL_OPCUA_SERVER_IMMSN, DO_MOLD_CLAMPED, "1");

				intRetval = IMM_SetIMMMoldStatus(mysqlCon, intIMMSN, MOLD_STATUS_CLAMPED);

				intExpectInsertIMMSensorShotSN = intShotSN;
				fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d MOSN:%d ShotSN:%d has started. ExpectInsertIMMSensorShotSN=%d\n", intIMMSN, intMOSN, intShotSN, intExpectInsertIMMSensorShotSN);
			}

			// If This Shot Follows Regular Steps
			if (
				boolIsThisShotClamped == true &&
				boolIsThisShotReleased == false &&
				boolHasThisShotClamping == true &&
				boolHasThisShotClamped == true &&
				boolHasThisShotReleasing == false &&
				boolHasThisShotReleased == false)
			{
				if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_MODBUS)
				{
					// If the IMMPara has been changed then IMMParaSN+=1 (Parameter Finish, Save IMMParaSN)
					if (boolHasIMMParaChanged == true)
					{
						char *charIMMParaSN;
						snprintf(charStatement, MAX_STRING_SIZE, "%s_%s_%s_%d_RawData_IMMPara.IMMParaSNList",
								 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
						intRetval = DB_SelectColumnMax(mysqlCon, charStatement, "IMMParaSN", &charIMMParaSN);
						intIMMParaSN = atoi(charIMMParaSN) + 1;
						boolHasIMMParaChanged = false;
					}

					snprintf(charStatement, MAX_STRING_SIZE,
							 "UPDATE %s_%s_%s_%d_RawData.ShotSNList SET IMMParaSN = %d, IMMParaUnstable = %d WHERE ShotSN = %d",
							 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
							 boolHasIMMParaChanged == true ? intIMMParaSN : intIMMParaSN - 1, boolIMMParaUnstable, intShotSN);

					intRetval = mysql_query(&mysqlCon, charStatement);
					// fprintf(stderr, "Q:%s\n", charStatement);
					if (intRetval != EXIT_SUCCESS)
					{
						// fprintf(stderr,"Q:%s\n",charStatement);
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "[SubscribeOPCUA]Fail to insert ShotSN to table %s_%s_%s_%d_RawData.ShotSNList (%d): %s",
								 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
						return;
					}

					fprintf(stderr, "[SubscribeOPCUA]Insert IMMParaSN:%d for ShotSN:%d\n", boolHasIMMParaChanged == true ? intIMMParaSN : intIMMParaSN - 1, intShotSN);
				}
				else if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_OPCUA)
				{

					intRetval = DB_SelectCurrentRoundSN(mysqlCon, intIMMSN, &intRoundSN);
					// fprintf(stderr, "RoundSN:%d\n", intRoundSN);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to select RoundSN for IMMSN:%d in table %s_%s.IMMSNList",
								 intIMMSN, SYS_NAME, DATA_DATABASE_NAME);
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, 0, charErrMsg);
					}

					snprintf(charStatement, MAX_STRING_SIZE,
							 "UPDATE %s_%s_%s_%d_RawData.ShotSNList SET "
							 "RoundSN = %d,	IMMParaSN = %d, MoldClampedTime = NOW(0) WHERE ShotSN = %d",
							 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
							 intRoundSN,
							 boolHasIMMParaChanged == true ? intIMMParaSN : intIMMParaSN - 1,
							 intShotSN);

					intRetval = mysql_query(&mysqlCon, charStatement);
					// fprintf(stderr,"Q:%s\n",charStatement);
					if (intRetval != EXIT_SUCCESS)
					{
						// fprintf(stderr,"Q:%s\n",charStatement);
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "[SubscribeOPCUA]Fail to insert ShotSN to table %s_%s_%s_%d_RawData.ShotSNList (%d): %s",
								 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
						return;
					}

					// Insert Default MOAcceptCriteria1 As NULL
					intRetval = MO_InsertMOAcceptCriteriaActualValue(mysqlCon, intMOSN, intShotSN, 1, "NULL");
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, MAX_STRING_SIZE,
								 "[SubscribeOPCUA]Fail to insert accept criteria actual value of ShotSN:%d MOAcceptCriteriaSN:1 into "
								 "table %s_%s_%s_%d_RawData_AcceptCriteria.MOAcceptCriteriaSN1 (%d):%s",
								 intShotSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
					}

					intRetval = MO_InsertMOAcceptCriteriaPredictValue(mysqlCon, intMOSN, intShotSN, 1, "NULL");
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, MAX_STRING_SIZE,
								 "[SubscribeOPCUA]Fail to insert accept criteria predict value of ShotSN:%d MOAcceptCriteriaSN:1 into "
								 "table %s_%s_%s_%d_RawData_AcceptCriteria.MOAcceptCriteriaSN1 (%d):%s",
								 intShotSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
					}
				}
				boolIMMParaUnstable = false;
			}
		}
		else if ((uaDataValue->value.type->typeIndex == UA_TYPES_BOOLEAN && *(UA_Boolean *)uaDataValue->value.data == false) ||
				 (uaDataValue->value.type->typeIndex == UA_TYPES_DOUBLE && *(UA_Double *)uaDataValue->value.data == 0) ||
				 (uaDataValue->value.type->typeIndex == UA_TYPES_STRING && *((UA_String *)(uaDataValue->value.data))->data == '0')) // Mold Clamped == false

		{
			if (boolHasThisShotClamping == true && boolIsThisShotReleased == false)
				fprintf(stderr, "[SubscribeOPCUA]The mold of IMMSN:%d is releasing\n", intIMMSN);

			boolIsThisShotReleased = false;
			boolIsThisShotClamped = false;
			boolHasThisShotReleasing = true;

			if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_MODBUS)
			{
				// Update ShotSN

				// intRetval = DB_SelectShotSNbyIMMSN(mysqlCon, intIMMSN, &intShotSN);
				// if (intRetval != EXIT_SUCCESS)
				//{
				//	fprintf(stderr, "[SubscribeOPCUA]Fail to update ShotSN by MOSN:%d for IMMSN:%d while mold clamped.\n", intMOSN, intIMMSN);
				// }
			}
			else if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_OPCUA)
			{
				// Set OPC UA Node Value : Mold Releasing
				OPCUA_WriteOPCUANodeValue(mysqlCon, VIRTUAL_OPCUA_SERVER_IMMSN, DO_MOLD_CLAMPED, "0");
				OPCUA_WriteOPCUANodeValue(mysqlCon, VIRTUAL_OPCUA_SERVER_IMMSN, DO_MOLD_RELEASED, "0");

				intRetval = IMM_SetIMMMoldStatus(mysqlCon, intIMMSN, MOLD_STATUS_RELEASING);
			}

			// If This Shot Follows Regular Steps
			if (
				boolIsThisShotClamped == false &&
				boolIsThisShotReleased == false &&
				boolHasThisShotClamping == true &&
				boolHasThisShotClamped == true &&
				boolHasThisShotReleasing == true &&
				boolHasThisShotReleased == false)
			{
				if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_OPCUA)
				{
					// Update ReleasingTime and ClampedCycleTime to INJPRO_DATA_MO_[MOSN]_RawData.ShotSNList
					snprintf(charStatement, MAX_STRING_SIZE,
							 "UPDATE %s_%s_%s_%d_RawData.ShotSNList SET MoldReleasingTime=NOW(6),"
							 "ClampedCycleTime=TIMESTAMPDIFF(Microsecond,MoldClampedTime,NOW(6))/1000000 WHERE ShotSN=%d",
							 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, intShotSN);
					intRetval = mysql_query(&mysqlCon, charStatement);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "Fail to update MoldReleasingTime and ClampedCycleTime of ShotSN:%d "
								 "while mold releasing to table %s_%s_%s_%d_RawData.ShotSNList (%d): %s",
								 intShotSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
						return;
					}
				}

				// intRetval=IMM_TuneIMMParaByRecom(mysqlCon,intIMMSN,intMOSN,1);
			}
		}
	}
	else if (intMonIDNodeIndex[monId] == DO_MOLD_RELEASED)
	{
		// Mold Released
		if ((uaDataValue->value.type->typeIndex == UA_TYPES_BOOLEAN && *(UA_Boolean *)uaDataValue->value.data == true) ||
			(uaDataValue->value.type->typeIndex == UA_TYPES_DOUBLE && *(UA_Double *)uaDataValue->value.data == 1.0) ||
			(uaDataValue->value.type->typeIndex == UA_TYPES_STRING && *((UA_String *)(uaDataValue->value.data))->data == '1'))
		// if (*(UA_Boolean *)uaDataValue->value.data == true || *(UA_Double *)uaDataValue->value.data == 1.0 || *((UA_String *)(uaDataValue->value.data))->data == '1')
		{
			fprintf(stderr, "[SubscribeOPCUA]The mold of IMMSN:%d is released\n", intIMMSN);
			boolIsThisShotClamped = false;
			boolIsThisShotReleased = true;
			boolHasThisShotReleased = true;

			if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_MODBUS)
			{
				// Update ShotSN

				// intRetval = DB_SelectShotSNbyIMMSN(mysqlCon, intIMMSN, &intShotSN);
				// if (intRetval != EXIT_SUCCESS)
				//{
				//	fprintf(stderr, "[SubscribeOPCUA]Fail to update ShotSN by MOSN:%d for IMMSN:%d while mold clamped.\n", intMOSN, intIMMSN);
				// }
			}
			else if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_OPCUA)
			{
				// Set OPC UA Node Value : Mold Released
				OPCUA_WriteOPCUANodeValue(mysqlCon, VIRTUAL_OPCUA_SERVER_IMMSN, DO_MOLD_CLAMPED, "0");
				OPCUA_WriteOPCUANodeValue(mysqlCon, VIRTUAL_OPCUA_SERVER_IMMSN, DO_MOLD_RELEASED, "1");

				intRetval = IMM_SetIMMMoldStatus(mysqlCon, intIMMSN, MOLD_STATUS_RELEASED);
			}

			// If This Shot Follows Regular Steps
			if (
				boolIsThisShotClamped == false &&
				boolIsThisShotReleased == true &&
				boolHasThisShotClamping == true &&
				boolHasThisShotClamped == true &&
				boolHasThisShotReleasing == true &&
				boolHasThisShotReleased == true)
			{
				if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_MODBUS)
				{

					// Check Modbus Status To See If It Is Ready
					intRetval = MODBUS_SelectModbusClientStatusbyIMMSN(mysqlCon, intIMMSN, &intModbusClientStatus);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to select ModbusClientStatus for IMMSN:%d while mold released", intIMMSN);
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
					}
					else if (intModbusClientStatus != MODBUS_CLIENT_STATUS_RUNNING)
					{
						fprintf(stderr, "[SubscribeOPCUA]Waiting Modbus Ready While Mold Released.\n");
					}
					else
					{
						fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d MOSN:%d ShotSN:%d is ready to start\n", intIMMSN, intMOSN, intShotSN + 1);

						UA_Variant *readVariant = UA_Variant_new();
						intRetval = OPCUA_ReadOPCUANodeValue(mysqlCon, intIMMSN, DO_SHOT_COMPLETE, readVariant);
						if (intRetval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(readVariant))
						{
							if (readVariant->type == &UA_TYPES[UA_TYPES_FLOAT])
								intMoldReleasedIMMOPCUAShotSN = (int)(*(UA_Float *)readVariant->data);
							else if (readVariant->type == &UA_TYPES[UA_TYPES_DOUBLE])
								intMoldReleasedIMMOPCUAShotSN = (int)(*(UA_Double *)readVariant->data);
							else if (readVariant->type == &UA_TYPES[UA_TYPES_INT16])
								intMoldReleasedIMMOPCUAShotSN = (int)(*(UA_Int16 *)readVariant->data);
							else if (readVariant->type == &UA_TYPES[UA_TYPES_UINT16])
								intMoldReleasedIMMOPCUAShotSN = (int)(*(UA_UInt16 *)readVariant->data);
							else if (readVariant->type == &UA_TYPES[UA_TYPES_INT32])
								intMoldReleasedIMMOPCUAShotSN = (int)(*(UA_Int32 *)readVariant->data);
							else if (readVariant->type == &UA_TYPES[UA_TYPES_UINT32])
								intMoldReleasedIMMOPCUAShotSN = (int)(*(UA_UInt32 *)readVariant->data);
							else if (readVariant->type == &UA_TYPES[UA_TYPES_INT64])
								intMoldReleasedIMMOPCUAShotSN = (int)(*(UA_Int64 *)readVariant->data);
							else if (readVariant->type == &UA_TYPES[UA_TYPES_UINT64])
								intMoldReleasedIMMOPCUAShotSN = (int)(*(UA_UInt64 *)readVariant->data);
							else if (readVariant->type == &UA_TYPES[UA_TYPES_STRING])
								intMoldReleasedIMMOPCUAShotSN = atoi(((UA_String *)(readVariant->data))->data);
							fprintf(stderr, "[SubscribeOPCUA]intMoldReleasedIMMOPCUAShotSN = %d\n", intMoldReleasedIMMOPCUAShotSN);
							if ((intMoldReleasedIMMOPCUAShotSN == intMoldClampedIMMOPCUAShotSN) || (intMoldReleasedIMMOPCUAShotSN - intMoldClampedIMMOPCUAShotSN == intPiecePerShot))
							{
								intDiffIMMOPCUAShotSN = (intShotSN - 1) - intMoldClampedIMMOPCUAShotSN;
								boolEnableInsertIMMSensorData = true;
								fprintf(stderr, "[SubscribeOPCUA][Released-Case1]EnableInsertIMMSensorData:%d ClampedIMMOPCUAShotSN:%d ReleasedIMMOPCUAShotSN:%d for ShotSN:%d Diff:%d\n",
										boolEnableInsertIMMSensorData, intMoldClampedIMMOPCUAShotSN, intMoldReleasedIMMOPCUAShotSN, intShotSN - 1, intDiffIMMOPCUAShotSN);
							}
							else if (intLastInsertIMMSensorShotSN == 0) // If First Shot Missing Match Then Forcelly Follow Released
							{
								boolEnableInsertIMMSensorData = false;
							}
							else
							{
								// intDiffIMMOPCUAShotSN = intDiffIMMOPCUAShotSN; //Apply use the previous diff
								boolEnableInsertIMMSensorData = true;
								fprintf(stderr, "[SubscribeOPCUA][Released-Case2]EnableInsertIMMSensorData:%d ClampedIMMOPCUAShotSN:%d ReleasedIMMOPCUAShotSN:%d for ShotSN:%d Diff:%d\n",
										boolEnableInsertIMMSensorData, intMoldClampedIMMOPCUAShotSN, intMoldReleasedIMMOPCUAShotSN, intShotSN - 1, intDiffIMMOPCUAShotSN);
							}
						}
						UA_Variant_delete(readVariant);
					}
				}
				else if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_OPCUA)
				{
					// Update the New Shot to INJPRO_DATA_MO_[MOSN]_RawData.ShotSNList
					snprintf(charStatement, MAX_STRING_SIZE,
							 "UPDATE %s_%s_%s_%d_RawData.ShotSNList SET "
							 "MoldReleasedTime=NOW(6),"
							 "CycleTime=TIMESTAMPDIFF(Microsecond,MoldClampingTime,NOW(6))/1000000, ErrorShot = %s WHERE ShotSN = %d",
							 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
							 "CASE WHEN MoldClampingTime IS NULL OR MoldClampedTime IS NULL OR MoldReleasingTime IS NULL OR MoldReleasedTime IS NULL THEN 1 ELSE 0 END",
							 intShotSN);
					// fprintf(stderr, "Q:%s\n", charStatement);
					intRetval = mysql_query(&mysqlCon, charStatement);
					if (intRetval)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "[SubscribeOPCUA]Fail to update cycle time for ShotSN:%d while mold released to table %s_%s_%s_%d_RawData.ShotSNList (%d): %s",
								 intShotSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
						// return;
					}

					// Update the New Shot to INJPRO_DATA_MO_[MOSN]_RawData.ShotSNList
					snprintf(charStatement, MAX_STRING_SIZE,
							 "UPDATE %s_%s_%s_%d_RawData.ShotSNList SET "
							 "ShotInterval=IF(%d = 1 , 0, ("
							 "SELECT TIMESTAMPDIFF(MICROSECOND,MIN(LastTwoShot.MoldReleasedTime),MAX(LastTwoShot.MoldClampingTime))/1000000 "
							 "FROM(SELECT ShotSN,MoldClampingTime,MoldReleasedTime FROM %s_%s_%s_%d_RawData.ShotSNList "
							 "WHERE ShotSN <= %d ORDER BY ShotSN DESC  LIMIT 2)AS LastTwoShot)) WHERE ShotSN=%d",
							 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
							 intShotSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, intShotSN, intShotSN);
					// fprintf(stderr, "Q:%s\n", charStatement);
					intRetval = mysql_query(&mysqlCon, charStatement);
					if (intRetval)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "[SubscribeOPCUA]Fail to update shot interval for ShotSN:%d while mold released to table %s_%s_%s_%d_RawData.ShotSNList (%d): %s",
								 intShotSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
						// return;
					}
					// Update ShotSN=ShotSN+1 in INJPRO.Data.IMMSNList
					snprintf(charStatement, MAX_STRING_SIZE,
							 "UPDATE %s_%s.IMMSNList SET ShotSN=%d, IMMLastUpdateTime=NOW(6) WHERE IMMSN=%d",
							 SYS_NAME, DATA_DATABASE_NAME, ++intShotSN, intIMMSN);

					// intRetval = mysql_query(&mysqlCon, charStatement);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to update ShotSN in table %s_%s.IMMSNList (%d): %s",
								 SYS_NAME, DATA_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
						return;
					}
					fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d MOSN:%d ShotSN:%d is ready to start\n", intIMMSN, intMOSN, intShotSN);
				}
				boolHasThisShotReleased = false;
			}
						
			boolIsThisShotReleased = false;
			boolIsThisShotClamped = false;
			boolHasThisShotReleased = false;
			boolHasThisShotClamping = false;
			boolHasThisShotClamped = false;
			boolHasThisShotReleasing = false;			
		}
		else if (
			(uaDataValue->value.type->typeIndex == UA_TYPES_BOOLEAN && *(UA_Boolean *)uaDataValue->value.data == false) ||
			(uaDataValue->value.type->typeIndex == UA_TYPES_DOUBLE && *(UA_Double *)uaDataValue->value.data == 0.0) ||
			(uaDataValue->value.type->typeIndex == UA_TYPES_STRING && *((UA_String *)(uaDataValue->value.data))->data == '0'))
		{
			if (boolHasThisShotReleased == false && boolIsThisShotClamped == false)
				fprintf(stderr, "[SubscribeOPCUA]The mold of IMMSN:%d is clamping\n", intIMMSN);
			boolIsThisShotReleased = false;
			boolIsThisShotClamped = false;
			boolHasThisShotClamping = true;

			if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_MODBUS)
			{
				/*
				//Check Modbus Status To See If It Is Ready
				intRetval = MODBUS_SelectModbusClientStatusbyIMMSN(mysqlCon, intIMMSN, &intModbusClientStatus);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to select ModbusClientStatus for IMMSN:%d while mold clamped", intIMMSN);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
				}
				else if (intModbusClientStatus != MODBUS_CLIENT_STATUS_RUNNING)
				{
					//fprintf(stderr, "[SubscribeOPCUA]Waiting Modbus Ready While Mold Calmping.\n");
				}
				else
				{
					intRetval = DB_SelectShotSNbyIMMSN(mysqlCon, intIMMSN, &intShotSN);
					if (intRetval != EXIT_SUCCESS)
					{
						fprintf(stderr, "[SubscribeOPCUA]Fail to select ShotSN by MOSN:%d for IMMSN:%d while shot complete.\n", intMOSN, intIMMSN);
						return;
					}
					fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d MOSN:%d ShotSN:%d has started. ExpectInsertIMMSensorShotSN=%d\n", intIMMSN, intMOSN, intShotSN,intExpectInsertIMMSensorShotSN);
					intExpectInsertIMMSensorShotSN = intShotSN;
				}*/
			}
			else if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_OPCUA)
			{
				// Set OPC UA Node Value : Mold Clamping
				OPCUA_WriteOPCUANodeValue(mysqlCon, VIRTUAL_OPCUA_SERVER_IMMSN, DO_MOLD_CLAMPED, "0");
				OPCUA_WriteOPCUANodeValue(mysqlCon, VIRTUAL_OPCUA_SERVER_IMMSN, DO_MOLD_RELEASED, "0");

				// Update DB MoldStatus
				intRetval = IMM_SetIMMMoldStatus(mysqlCon, intIMMSN, MOLD_STATUS_CLAMPING);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to update MoldStatus for IMMSN:%d in table %s_%s.IMMSNList",
							 intIMMSN, SYS_NAME, DATA_DATABASE_NAME);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, 0, charErrMsg);
				}

				/*
				intExpectInsertIMMSensorShotSN = intShotSN;
				fprintf(stderr,"intExpectInsertIMMSensorShotSN = %d\n",intExpectInsertIMMSensorShotSN);
				*/
			}

			// If This Shot Follows Regular Steps
			if (
				boolIsThisShotClamped == false &&
				boolIsThisShotReleased == false &&
				boolHasThisShotClamping == true &&
				boolHasThisShotClamped == false &&
				boolHasThisShotReleasing == false &&
				boolHasThisShotReleased == false)
			{
				if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_OPCUA)
				{
					snprintf(charStatement, MAX_STRING_SIZE,
							 "INSERT INTO %s_%s_%s_%d_RawData.ShotSNList "
							 "(ShotSN,	MoldClampingTime,	MOCustomSensorTableSN,	ErrorShot) VALUE "
							 "(%d,		NOW(6),				%d,						%d)",
							 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
							 intShotSN,
							 (intShotSN - 1) / SHOT_NUM_PER_TABLE + 1,
							 ERROR_SHOT_IMCOMPLETE);

					intRetval = mysql_query(&mysqlCon, charStatement);
					// fprintf(stderr,"Q:%s\n",charStatement);
					if (intRetval != EXIT_SUCCESS)
					{
						// fprintf(stderr,"Q:%s\n",charStatement);
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "[SubscribeOPCUA]Fail to insert ShotSN to table %s_%s_%s_%d_RawData.ShotSNList (%d): %s",
								 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
						return;
					}

					/*
					snprintf(charStatement, MAX_STRING_SIZE,
							 "INSERT INTO %s_%s_%s_%d_RawData.ShotSNList "
							 "(ShotSN,	RoundSN, 	MoldClampingTime,	IMMParaSN,	MOCustomSensorTableSN,	IMMParaUnstable,	ErrorShot) VALUE "
							 "(%d,		%d,			NOW(6),				%d,			%d,						%d,					%d)",
							 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
							 intShotSN, intRoundSN,
							 boolHasIMMParaChanged == true ? intIMMParaSN : intIMMParaSN - 1,
							 (intShotSN - 1) / SHOT_NUM_PER_TABLE + 1,
							 boolIMMParaUnstable, ERROR_SHOT_IMCOMPLETE);

					intRetval = mysql_query(&mysqlCon, charStatement);
					//fprintf(stderr,"Q:%s\n",charStatement);
					if (intRetval != EXIT_SUCCESS)
					{
						//fprintf(stderr,"Q:%s\n",charStatement);
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "[SubscribeOPCUA]Fail to insert ShotSN to table %s_%s_%s_%d_RawData.ShotSNList (%d): %s",
								 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
						return;
					}
					*/
				}
			}
		}
	}

	// If the Change Caused by Basic Signal (Not IMM Parameter)
	if (intMonIDNodeIndex[monId] < OPCUA_FIRST_IMMPARA)
	{
		// If Shot Complete Signal Is Detected
		if (intMonIDNodeIndex[monId] == DO_SHOT_COMPLETE)
		{
			if (uaDataValue->value.type->typeIndex == UA_TYPES_FLOAT)
			{
				intShotCompleteIMMOPCUAShotSN = (int)(*(UA_Float *)uaDataValue->value.data);
				fprintf(stderr, "[SubscribeOPCUA]intShotCompleteIMMOPCUAShotSN = %d\n", (int)(*(UA_Float *)uaDataValue->value.data));
			}
			else if (uaDataValue->value.type->typeIndex == UA_TYPES_DOUBLE)
			{
				intShotCompleteIMMOPCUAShotSN = (int)(*(UA_Double *)uaDataValue->value.data);
				fprintf(stderr, "[SubscribeOPCUA]intShotCompleteIMMOPCUAShotSN = %d\n", (int)(*(UA_Double *)uaDataValue->value.data));
			}
			else if (uaDataValue->value.type->typeIndex == UA_TYPES_INT16)
			{
				intShotCompleteIMMOPCUAShotSN = (int)(*(UA_Int16 *)uaDataValue->value.data);
				fprintf(stderr, "[SubscribeOPCUA]intShotCompleteIMMOPCUAShotSN = %d\n", (int)(*(UA_Int16 *)uaDataValue->value.data));
			}
			else if (uaDataValue->value.type->typeIndex == UA_TYPES_UINT16)
			{
				intShotCompleteIMMOPCUAShotSN = (int)(*(UA_UInt16 *)uaDataValue->value.data);
				fprintf(stderr, "[SubscribeOPCUA]intShotCompleteIMMOPCUAShotSN = %d\n", (int)(*(UA_UInt16 *)uaDataValue->value.data));
			}
			else if (uaDataValue->value.type->typeIndex == UA_TYPES_INT32)
			{
				intShotCompleteIMMOPCUAShotSN = (int)(*(UA_Int32 *)uaDataValue->value.data);
				fprintf(stderr, "[SubscribeOPCUA]intShotCompleteIMMOPCUAShotSN = %d\n", (int)(*(UA_Int32 *)uaDataValue->value.data));
			}
			else if (uaDataValue->value.type->typeIndex == UA_TYPES_UINT32)
			{
				intShotCompleteIMMOPCUAShotSN = (int)(*(UA_UInt32 *)uaDataValue->value.data);
				fprintf(stderr, "[SubscribeOPCUA]intShotCompleteIMMOPCUAShotSN = %d\n", (int)(*(UA_UInt32 *)uaDataValue->value.data));
			}
			else if (uaDataValue->value.type->typeIndex == UA_TYPES_INT64)
			{
				intShotCompleteIMMOPCUAShotSN = (int)(*(UA_Int64 *)uaDataValue->value.data);
				fprintf(stderr, "[SubscribeOPCUA]intShotCompleteIMMOPCUAShotSN = %d\n", (int)(*(UA_Int64 *)uaDataValue->value.data));
			}
			else if (uaDataValue->value.type->typeIndex == UA_TYPES_UINT64)
			{
				intShotCompleteIMMOPCUAShotSN = (int)(*(UA_UInt64 *)uaDataValue->value.data);
				fprintf(stderr, "[SubscribeOPCUA]intShotCompleteIMMOPCUAShotSN = %d\n", (int)(*(UA_UInt64 *)uaDataValue->value.data));
			}
			else if (uaDataValue->value.type->typeIndex == UA_TYPES_STRING)
			{
				intShotCompleteIMMOPCUAShotSN = atoi(((UA_String *)(uaDataValue->value.data))->data);
				fprintf(stderr, "[SubscribeOPCUA]intShotCompleteIMMOPCUAShotSN = %d\n", atoi(((UA_String *)(uaDataValue->value.data))->data));
			}
			intIMMSensorShotSN = intShotCompleteIMMOPCUAShotSN + intDiffIMMOPCUAShotSN;

			fprintf(stderr, "boolEnableInsertIMMSensorData:%d intIMMSensorShotSN:%d intShotCompleteIMMOPCUAShotSN:%d intDiffIMMOPCUAShotSN:%d intInitialShotSN:%d intExpectInsertIMMSensorShotSN:%d intLastInsertIMMSensorShotSN=%d\n",
					boolEnableInsertIMMSensorData, intIMMSensorShotSN, intShotCompleteIMMOPCUAShotSN, intDiffIMMOPCUAShotSN, intInitialShotSN, intExpectInsertIMMSensorShotSN, intLastInsertIMMSensorShotSN);
			if (boolEnableInsertIMMSensorData == true && intIMMSensorShotSN > 0 && intIMMSensorShotSN > intLastInsertIMMSensorShotSN)
			{
#ifdef ENABLE_IMM_SENSOR
				// LonSo
				// if( intIMMSensorShotSN == intLastInsertIMMSensorShotSN)
				//	intIMMSensorShotSN += 1;
				// else if (intIMMSensorShotSN > intExpectInsertIMMSensorShotSN)
				//	intIMMSensorShotSN = intExpectInsertIMMSensorShotSN;

				fprintf(stderr, "[SubscribeOPCUA]Insert OPC UA IMM sensor data for ShotSN:%d\n", intIMMSensorShotSN);
				intRetval = OPCUA_InsertIMMSensor(mysqlCon, OPCUAClient, intIMMSensorShotSN);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[SubscribeOPCUA]Fail to insert IMM sensor feature value for ShotSN:%d \n", intIMMSensorShotSN);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
				}
				else
				{
					// Write IMMOPCUAShotSN to Database
					snprintf(charStatement, MAX_STRING_SIZE,
							 "UPDATE %s_%s_%s_%d_RawData.ShotSNList SET IMMOPCUAShotSN = %d WHERE ShotSN = %d",
							 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, intShotCompleteIMMOPCUAShotSN, intIMMSensorShotSN);

					intRetval = mysql_query(&mysqlCon, charStatement);
					// fprintf(stderr, "Q:%s\n", charStatement);
					if (intRetval != EXIT_SUCCESS)
					{
						// fprintf(stderr,"Q:%s\n",charStatement);
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "[SubscribeOPCUA]Fail to insert IMMOPCUAShotSN to table %s_%s_%s_%d_RawData.ShotSNList (%d): %s",
								 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
					}
					intLastInsertIMMSensorShotSN = intIMMSensorShotSN;
				}
				// boolEnableInsertIMMSensorData = false;
#endif
				// Trigger EC_PROGRESS_FLAG_MODBUS_READY
				/* EC is disable for OPC UA now
				intRetval = EC_TriggerECProgressFlag(mysqlCon, intMOSN, intShotSN, EC_PROGRESS_FLAG_MODBUS_READY);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[Modbus_WriteSensorCachetoDatabase]Fail to trigger EC modbus ready flag for ShotSN:%d MOSN:%d\n", i, intMOSN);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
					//intFeatureValueDoneShotSN = i;
				}*/

				// Trigger SPC_PROGRESS_FLAG_OPCUA_READY
				intRetval = SPC_TriggerSPCProgressFlag(mysqlCon, intMOSN, intIMMSensorShotSN, SPC_PROGRESS_FLAG_OPCUA_READY);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[SubscribeOPCUA]Fail to trigger SoftSensor feature value for ShotSN:%d \n", intIMMSensorShotSN);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
				}

				// Trigger QC_PROGRESS_FLAG_OPCUA_READY
				intRetval = QC_TriggerQCProgressFlag(mysqlCon, intMOSN, intIMMSensorShotSN, QC_PROGRESS_FLAG_OPCUA_READY);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[SubscribeOPCUA]Fail to trigger SoftSensor feature value for ShotSN:%d \n", intIMMSensorShotSN);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
				}

				// Get ECProgressFlag, if OPCUA and Modbus Data are Ready then Execute Empty Shot Control
				intRetval = EC_SelectECProgressFlag(mysqlCon, intMOSN, intIMMSensorShotSN, &intECProgressFlag);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[SubscribeOPCUA]Fail to select EC modbus ready for ShotSN:%d MOSN:%d\n", intIMMSensorShotSN, intMOSN);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
				}
				else if ((intECProgressFlag & EC_PROGRESS_FLAG_MODBUS_READY) == false &&
						 (intECProgressFlag & EC_PROGRESS_FLAG_OPCUA_READY) == false &&
						 (intECProgressFlag & EC_PROGRESS_FLAG_FINISHED) == true)
				{
					intRetval = EC_CheckEmptyShot(mysqlCon, intMOSN, intIMMSensorShotSN);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to insert EC result for ShotSN:%d \n", intIMMSensorShotSN);
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
					}
					else
					{
						// Trigger SPC_FLAG_SPC_FINISH If The Alarm Is Successfully Inserted
						intRetval = EC_TriggerECProgressFlag(mysqlCon, intMOSN, intIMMSensorShotSN, EC_PROGRESS_FLAG_FINISHED);
						if (intRetval != EXIT_SUCCESS)
						{
							snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to trigger EC finished for ShotSN:%d MOSN:%d\n", intIMMSensorShotSN, intMOSN);
							SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
						}
						else
						{
							fprintf(stderr, "[SubscribeOPCUA]EC of MOSN:%d ShotSN:%d has caculated successfully.\n", intMOSN, intIMMSensorShotSN);
						}
					}
				}

				// Get SPCProgressFlag, if OPCUA and Modbus Data are Ready then Execute SPC
				intRetval = SPC_SelectSPCProgressFlag(mysqlCon, intMOSN, intIMMSensorShotSN, &intSPCProgressFlag);

				// fprintf(stderr, "SPCProgressFlag:%d \n", intSPCProgressFlag);
				// fprintf(stderr, "intSPCProgressFlag & SPC_PROGRESS_FLAG_MODBUS_READY:%d\n", intSPCProgressFlag & SPC_PROGRESS_FLAG_MODBUS_READY);
				// fprintf(stderr, "intSPCProgressFlag & SPC_PROGRESS_FLAG_OPCUA_READY:%d\n", intSPCProgressFlag & SPC_PROGRESS_FLAG_OPCUA_READY);
				// fprintf(stderr, "intSPCProgressFlag & SPC_PROGRESS_FLAG_FINISHED:%d\n", intSPCProgressFlag & SPC_PROGRESS_FLAG_FINISHED);
				// fprintf(stderr, "Result:%d\n", ((intSPCProgressFlag & SPC_PROGRESS_FLAG_FINISHED) == true));

				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[SubscribeOPCUA]Fail to select SPCProgressFlag for ShotSN:%d \n", intIMMSensorShotSN);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
				}

				// If Features of Both Modbus and OPC UA Are Ready, Then Insert SPC Alarm
				else if ((intSPCProgressFlag & SPC_PROGRESS_FLAG_MODBUS_READY) == false &&
						 (intSPCProgressFlag & SPC_PROGRESS_FLAG_OPCUA_READY) == false &&
						 (intSPCProgressFlag & SPC_PROGRESS_FLAG_FINISHED) == true)
				{
					intRetval = SPC_InsertSPCAlarm(mysqlCon, intMOSN, intIMMSensorShotSN);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to insert SPC Alarm for ShotSN:%d \n", intIMMSensorShotSN);
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
						// return;
					}
					else
					{
						// Trigger SPC_FLAG_SPC_FINISH If The Alarm Is Successfully Inserted
						intRetval = SPC_TriggerSPCProgressFlag(mysqlCon, intMOSN, intIMMSensorShotSN, SPC_PROGRESS_FLAG_FINISHED);
						if (intRetval != EXIT_SUCCESS)
						{
							snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to trigger SoftSensor feature value for ShotSN:%d \n", intIMMSensorShotSN);
							SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
							// return;
						}
						else
						{
							fprintf(stderr, "[SubscribeOPCUA]SPC of MOSN:%d ShotSN:%d has caculated successfully.\n", intMOSN, intIMMSensorShotSN);
						}
					}
				}

				// Get QCFlag, if OPCUA and Modbus Data are Ready then Execute Quality Control
				intRetval = QC_SelectQCProgressFlag(mysqlCon, intMOSN, intIMMSensorShotSN, &intQCProgressFlag);
				// fprintf(stderr, "QCFlag:%d QCFlag&QC_PROGRESS_FLAG_FINISHED:%d Result:%d\n", intQCProgressFlag, intQCProgressFlag & QC_PROGRESS_FLAG_FINISHED, (intQCProgressFlag & QC_PROGRESS_FLAG_MODBUS_READY));
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[SubscribeOPCUA]Fail to select QCFlag for ShotSN:%d \n", intIMMSensorShotSN);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
				}
				// If Features of Both Modbus and OPC UA Are Ready, Then Insert SPC Alarm
				else if (
					(intQCProgressFlag & QC_PROGRESS_FLAG_MODBUS_READY) == false &&
					(intQCProgressFlag & QC_PROGRESS_FLAG_OPCUA_READY) == false &&
					(intQCProgressFlag & QC_PROGRESS_FLAG_FINISHED) == true)
				{
					// fprintf(stderr, "QC ing\n");
					intRetval = QC_InsertQCAlarm(mysqlCon, intMOSN, intIMMSensorShotSN);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to insert SPC Quality Control Alarm for ShotSN:%d \n", intIMMSensorShotSN);
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
					}
					else
					{
						// Trigger QC_PROGRESS_FLAG_QC_FINISH If The Alarm Is Successfully Inserted
						intRetval = QC_TriggerQCProgressFlag(mysqlCon, intMOSN, intIMMSensorShotSN, QC_PROGRESS_FLAG_FINISHED);
						if (intRetval != EXIT_SUCCESS)
						{
							snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to trigger SoftSensor feature value for ShotSN:%d \n", intIMMSensorShotSN);
							SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
						}
						else
						{
							fprintf(stderr, "[SubscribeOPCUA]QC of MOSN:%d ShotSN:%d has caculated successfully.\n", intMOSN, intIMMSensorShotSN);
						}
					}
				}

				// bermuda: Decide to Use Predict or Actual
				// Update Info
				intRetval = MO_UpdateMOSNListAfterShot(mysqlCon, intMOSN, YIELD_RATE_USE_ACTUAL);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[SubscribeOPCUA]Fail to update information (MO_UpdateMOSNListAfterShot) for ShotSN:%d \n", intIMMSensorShotSN);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
					// return EXIT_FAILURE;
				}
			}
		}

		// Display Value of Change Node
		if (uaDataValue->value.type->typeIndex == UA_TYPES_DATETIME)
		{
			UA_DateTimeStruct t = UA_DateTime_toStruct(*(UA_DateTime *)uaDataValue->value.data);
			fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d IMM Info [SubID:%u MonID:%3d NodeName:%-20s:%4d-%02d-%02d %02d:%02d:%02d]\n",
					intIMMSN, subId, monId, charMonIDNodeIndex[monId], t.year, t.month, t.day, t.hour, t.min, t.sec);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_BOOLEAN)
		{
			fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d IMM Info [SubID:%u MonID:%3d NodeName:%-20s:%d]\n",
					intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Boolean *)uaDataValue->value.data);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_FLOAT)
		{
			fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d IMM Info [SubID:%u MonID:%3d NodeName:%-20s:%f]\n",
					intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Float *)uaDataValue->value.data);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_DOUBLE)
		{
			fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d IMM Info [SubID:%u MonID:%3d NodeName:%-20s:%f]\n",
					intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Double *)uaDataValue->value.data);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_INT16)
		{
			fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d IMM Info [SubID:%u MonID:%3d NodeName:%-20s:%d]\n",
					intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Int16 *)uaDataValue->value.data);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_UINT16)
		{
			fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d IMM Info [SubID:%u MonID:%3d NodeName:%-20s:%d]\n",
					intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_UInt16 *)uaDataValue->value.data);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_INT32)
		{
			fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d IMM Info [SubID:%u MonID:%3d NodeName:%-20s:%d]\n",
					intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Int32 *)uaDataValue->value.data);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_UINT32)
		{
			fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d IMM Info [SubID:%u MonID:%3d NodeName:%-20s:%d]\n",
					intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_UInt32 *)uaDataValue->value.data);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_INT64)
		{
			fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d IMM Info [SubID:%u MonID:%3d NodeName:%-20s:%ld]\n",
					intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Int64 *)uaDataValue->value.data);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_UINT64)
		{
			fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d IMM Info [SubID:%u MonID:%3d NodeName:%-20s:%ld]\n",
					intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Int64 *)uaDataValue->value.data);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_STRING)
		{
			fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d IMM Info [SubID:%u MonID:%3d NodeName:%-20s:%s]\n",
					intIMMSN, subId, monId, charMonIDNodeIndex[monId], ((UA_String *)(uaDataValue->value.data))->data);
		}
	}
	// If Changed Caused by IMM Parameters
	else if (intMonIDNodeIndex[monId] >= OPCUA_FIRST_IMMPARA)
	{
		if (uaDataValue->value.type->typeIndex == UA_TYPES_BOOLEAN)
		{
			// Converted IMM Para Value According ConvertedRatio and Sace As String For Insertion
			if (doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]] == 1.0)
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%d'", (*(UA_Boolean *)uaDataValue->value.data));
			else
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%lf'", (double)(*(UA_Boolean *)uaDataValue->value.data) * doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]]);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_FLOAT)
		{
			// Converted IMM Para Value According ConvertedRatio and Sace As String For Insertion
			if (doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]] == 1.0)
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%lf'", *(UA_Float *)uaDataValue->value.data);
			else
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%lf'", (double)(*(UA_Float *)uaDataValue->value.data) * doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]]);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_DOUBLE)
		{
			// Converted IMM Para Value According ConvertedRatio and Sace As String For Insertion
			if (doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]] == 1.0)
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%lf'", *(UA_Double *)uaDataValue->value.data);
			else
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%lf'", (double)(*(UA_Double *)uaDataValue->value.data) * doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]]);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_INT16)
		{
			// Converted IMM Para Value According ConvertedRatio and Sace As String For Insertion
			if (doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]] == 1.0)
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%d'", *(UA_Int16 *)uaDataValue->value.data);
			else
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%lf'", (double)(*(UA_Int16 *)uaDataValue->value.data) * doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]]);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_UINT16)
		{
			// Converted IMM Para Value According ConvertedRatio and Sace As String For Insertion
			if (doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]] == 1.0)
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%d'", *(UA_UInt16 *)uaDataValue->value.data);
			else
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%lf'", (double)(*(UA_UInt16 *)uaDataValue->value.data) * doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]]);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_INT32)
		{
			// Converted IMM Para Value According ConvertedRatio and Sace As String For Insertion
			if (doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]] == 1.0)
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%d'", *(UA_Int32 *)uaDataValue->value.data);
			else
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%lf'", (double)(*(UA_Int32 *)uaDataValue->value.data) * doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]]);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_UINT32)
		{
			// Converted IMM Para Value According ConvertedRatio and Sace As String For Insertion
			if (doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]] == 1.0)
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%d'", *(UA_UInt32 *)uaDataValue->value.data);
			else
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%lf'", (double)(*(UA_UInt32 *)uaDataValue->value.data) * doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]]);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_INT64)
		{
			// Converted IMM Para Value According ConvertedRatio and Sace As String For Insertion
			if (doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]] == 1.0)
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%ld'", *(UA_Int64 *)uaDataValue->value.data);
			else
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%lf'", (double)(*(UA_Int64 *)uaDataValue->value.data) * doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]]);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_UINT64)
		{
			// Converted IMM Para Value According ConvertedRatio and Sace As String For Insertion
			if (doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]] == 1.0)
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%ld'", *(UA_UInt64 *)uaDataValue->value.data);
			else
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%lf'", (double)(*(UA_UInt64 *)uaDataValue->value.data) * doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]]);
		}
		else if (uaDataValue->value.type->typeIndex == UA_TYPES_STRING)
		{
			// Converted IMM Para Value According ConvertedRatio and Sace As String For Insertion
			if (doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]] == 1.0)
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%s'", ((UA_String *)(uaDataValue->value.data))->data);
			else
				snprintf(charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], TINY_STRING_SIZE, "'%lf'", atof(((UA_String *)(uaDataValue->value.data))->data) * doubleOPCUAConvertRatio[intMonIDNodeIndex[monId]]);
		}
		else
		{
			fprintf(stderr, "[SubscribeOPCUA]Unsupport OPC UA Node Type:%d\n", uaDataValue->value.type->typeIndex);
			return;
		}

		// Display IMM Parameter Detail
		fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d IMM Parameter [SubID:%u MonID:%3d NodeName:%-25s:%s]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1]);

		// Insert/Update The IMM Parameter to Database
		snprintf(charStatement, MAX_STRING_SIZE,
				 "INSERT INTO %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList (IMMParaSN,SubscribeSN, TechnicianUserSN,IMMParaLastUpdateTime,%s) VALUE (%d,%d,%d,NOW(6),%s) "
				 "ON DUPLICATE KEY UPDATE %s=%s,TechnicianUserSN=%d,IMMParaLastUpdateTime=NOW(6)",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, charMonIDNodeIndex[monId],
				 intIMMParaSN, intSubscribeSN, intTechnicianUserSN,
				 charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1],
				 charMonIDNodeIndex[monId], charIMMParaValue[intMonIDNodeIndex[monId] - OPCUA_FIRST_IMMPARA + 1], intTechnicianUserSN);
		// fprintf(stderr,"[TEST]%s\n",charStatement);
		intRetval = mysql_query(&mysqlCon, charStatement);
		if (intRetval != EXIT_SUCCESS)
		{
			// fprintf(stderr, "Q:%s\n", charStatement);
			snprintf(charErrMsg, LONG_STRING_SIZE,
					 "[SubscribeOPCUA]Fail to insert/update %s in table %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList (%d): %s",
					 charMonIDNodeIndex[monId], SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
			return;
		}
	}

	/*
	if (intMonIDNodeIndex[monId] == DO_MOLD_CLAMPED ||
		intMonIDNodeIndex[monId] == DO_MOLD_RELEASED)
	{
		fprintf(stderr, "IS-CLAMPED:%d IS-RELEASED:%d HAS-CLAMPING:%d HAS-CLAMPED:%d HAS-RELEASING:%d HAS-RELEASED:%d\n",
				boolIsThisShotClamped, boolIsThisShotReleased,
				boolHasThisShotClamping, boolHasThisShotClamped,
				boolHasThisShotReleasing, boolHasThisShotReleased);
	}*/

	// Update INJPRO_Data.IMMSNList Set IMMLastUpdateTime as NOW Where IMMSN Is Given
	snprintf(charStatement, MAX_STRING_SIZE, "UPDATE %s_%s.IMMSNList SET IMMLastUpdateTime=NOW(6) WHERE IMMSN=%d",
			 SYS_NAME, DATA_DATABASE_NAME, intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE,
				 "Fail to update IMMLastUpdateTime in table %s_%s.IMMSNList for IMMSN:%d (%d): %s",
				 SYS_NAME, DATA_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return;
	}

	mysql_close(&mysqlCon);
	return;
}

/* [OPCUA]Get Elapsed Time from Start Time Stamp
 * Caculate the elapsed time wchih starts from the given time stamp
 *
 * @param stStartTimeStamp to indicate the start time stamp
 * @param doubleElapsedTime to indicate the elapsed time which starts from the given time stamp
 * @return to indicate if the function call is success (EXIT_SUCCESS) or failure (EXIT_FAILURE) */
int OPCUA_GetElapsedTime(struct timespec stStartTimeStamp, double *doubleElapsedTime)
//[OPCUA]Get Elapsed Time from Start Time Stamp
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

/* [OPCUA]Subscribe OPC UA Node (According IMMSN, OPC UA Version, etc.)
 * This function is to subscribe all monitor items according to OPC UA Version defined in database.
 *
 * @param OPCUAClient to OPC UA client
 * @param mysqlCon to MySQL connector
 * @return to indicate if the function call is success (EXIT_SUCCESS) or failure (EXIT_FAILURE) */
int OPCUA_SubscribeMonitorItem(UA_Client *OPCUAClient, MYSQL mysqlCon)
//[OPCUA]Subscribe OPC UA Node (According IMMSN, OPC UA Version, etc.)
{
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	unsigned int intLogSN;
	// int	intOPCUAVersionSN;
	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
	request.requestedPublishingInterval = 1000;
	// request.requestedPublishingInterval=3;
	request.requestedMaxKeepAliveCount = 1000;
	// request.maxNotificationsPerPublish=1;
	// request.publishingEnabled=false;

	UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(OPCUAClient, request, NULL, NULL, NULL);
	if (response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
	{
#ifdef DEBUG_MODE_SUBSCRIBE_OPCUA

		fprintf(stderr, "[OPCUA_SubscribeMonitorItem]Create subscription succeeded, id %u\n", response.subscriptionId);
#endif
		// Update OPCUAClientStatus = OPCUA_CLIENT_STATUS_WAITING in IMMSNList
		snprintf(charStatement, MAX_STRING_SIZE, "UPDATE %s_%s.IMMSNList SET OPCUAClientStatus = %d ,IMMLastUpdateTime = NOW(6) WHERE IMMSN=%d",
				 SYS_NAME, DATA_DATABASE_NAME, OPCUA_CLIENT_STATUS_WAITING, intIMMSN);
		intRetval = mysql_query(&mysqlCon, charStatement);
		if (intRetval != EXIT_SUCCESS)
		{
			snprintf(charErrMsg, LONG_STRING_SIZE,
					 "[SubscribeModbus]Fail to update OPCUAClientStatus = OPCUA_CLIENT_STATUS_WAITING and IMMLastUpdateTime=NOW(6) where IMMSN=%d (%d):%s",
					 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
			boolIsStopSignal = true;
		}
	}
	else
	{
#ifdef DEBUG_MODE_SUBSCRIBE_OPCUA

		fprintf(stderr, "[OPCUA_SubscribeMonitorItem]Create subscription failed\n");
		return EXIT_FAILURE;
#endif
	}

	UA_MonitoredItemCreateRequest monRequest;
	UA_MonitoredItemCreateResult monResponse;

	mysql_init(&mysqlCon);
	if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
	{
		if (mysql_errno(&mysqlCon))
			fprintf(stderr, "Fail to connect to MySql server %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		return EXIT_FAILURE;
	}

	/*
	snprintf(charStatement,MAX_STRING_SIZE,"SELECT OPCUAVersionSN FROM %s_%s.IMMSNList WHERE IMMSN=%d",
	 SYS_NAME,DATA_DATABASE_NAME,intIMMSN);
	intRetval=mysql_query(&mysqlCon,charStatement);
	if(intRetval){
		snprintf(charErrMsg,LONG_STRING_SIZE,"[SubscribeOPCUA]Fail to select OPCUAVersionSN from table %s_%s.IMMSNList (%d): %s",
		 SYS_NAME,DATA_DATABASE_NAME,mysql_errno(&mysqlCon),mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon,ERRCLASS_IMM,intMOSN,intIMMSN,mysql_errno(&mysqlCon),charErrMsg);
		return EXIT_FAILURE;
	}

	MYSQL_RES *mysqlResult=mysql_store_result(&mysqlCon);
	MYSQL_ROW mysqlRow=mysql_fetch_row(mysqlResult);
	mysql_free_result(mysqlResult);
	intOPCUAVersionSN=atoi(mysqlRow[0]);
	*/

	// Select Node NameSpace and ID Where OPCUAVersionSN Is Given
	snprintf(charStatement, MAX_STRING_SIZE,
			 "SELECT * FROM %s_%s.OPCUAVersionIndex WHERE OPCUAVersionSN=%d", SYS_NAME, INDEX_DATABASE_NAME, intOPCUAVersionSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to select OPCUAVersionSN from table %s_%s.IMMSNList (%d): %s",
				 SYS_NAME, DATA_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return intLogSN;
	}
	mysqlResult = mysql_store_result(&mysqlCon);
	mysqlRow = mysql_fetch_row(mysqlResult);

	// Monitor All IMM Parameters from 1 to OPCUA_IMMPARA_NODE_NUM
	for (int i = 1; i <= OPCUA_IMMPARA_NODE_NUM; i++)
	{
		if (mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NS] != NULL &&
			(strcmp(mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_IDENTIFIER_TYPE], "i") == 0 ||
			 strcmp(mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_IDENTIFIER_TYPE], "s") == 0))
		{
			if (strcmp(mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_IDENTIFIER_TYPE], "i") == 0)
			{
				monRequest = UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(
					atoi(mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NS]),
					atoi(mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_ID])));
			}
			else
			{
				monRequest = UA_MonitoredItemCreateRequest_default(UA_NODEID_STRING(
					atoi(mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NS]),
					mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_ID]));
			}

			monResponse = UA_Client_MonitoredItems_createDataChange(OPCUAClient, response.subscriptionId,
																	UA_TIMESTAMPSTORETURN_BOTH, monRequest, NULL, OPCUA_HandlerIMMChanged, NULL);
			fprintf(stderr, "[SubscribeOPCUA]i=%d IMMSN:%d is trying to monitor %s[%s:%s], id %u\n", i, intIMMSN,
					mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NAME],
					mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NS],
					mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_ID], monResponse.monitoredItemId);

			if (monResponse.statusCode == UA_STATUSCODE_GOOD)
			{
				/*
				fprintf(stderr, "[SubscribeOPCUA]IMMSN:%d is monitoring [NS:%2s ID:%-65s Name:%-20s], id %u\n", intIMMSN,
						mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NS],
						mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_ID],
						mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NAME], monResponse.monitoredItemId);
				*/
				strcpy(charMonIDNodeIndex[monResponse.monitoredItemId], mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NAME]);
				intMonIDNodeIndex[monResponse.monitoredItemId] = i;
			}
			else
			{
				snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]IMMSN:%d fails to monitor [%s:%s], id %u", intIMMSN,
						 mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NS],
						 mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_ID], monResponse.monitoredItemId);
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
				boolIsStopSignal = true;
			}
		}
	}

	// Save All IMM Sensor Node ID to charOPCUAIMMSensorNodeList
	for (int i = OPCUA_IMMPARA_NODE_NUM + 1; i <= OPCUA_IMMSENSOR_NODE_NUM + OPCUA_IMMPARA_NODE_NUM; i++)
	{
		strcpy(charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NAME],
			   mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NAME]);
		if (mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NS] != NULL)
		{
			strcpy(charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_IDENTIFIER_TYPE],
				   mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_IDENTIFIER_TYPE]);
			strcpy(charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NS],
				   mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NS]);
			strcpy(charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_ID],
				   mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_ID]);

			fprintf(stderr, "[SubscribeOPCUA]i=%d IMMSN:%d is copying IMMSensor node information %s(Type:%s) [%s:%s]\n",
					i, intIMMSN,
					charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NAME],
					charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_IDENTIFIER_TYPE],
					charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NS],
					charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_ID]);
		}
	}

	// Update OPCUAClientStatus = MODBUS_STATUS_RUNNING in IMMSNList
	snprintf(charStatement, MAX_STRING_SIZE, "UPDATE %s_%s.IMMSNList SET OPCUAClientStatus = %d ,IMMLastUpdateTime = NOW(6) WHERE IMMSN=%d",
			 SYS_NAME, DATA_DATABASE_NAME, OPCUA_CLIENT_STATUS_RUNNING, intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE,
				 "[SubscribeModbus]Fail to update OPCUAClientStatus = OPCUAClientStatus and IMMLastUpdateTime=NOW(6) where IMMSN=%d (%d):%s",
				 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		boolIsStopSignal = true;
	}
	mysql_free_result(mysqlResult);

	return EXIT_SUCCESS;
}

/* [OPCUA]Copy and Insert Previous IMM Parameters as A New Record
 * This function is to copy the curren IMM parameters and then insert to a new record(row).
 * It is used to indicate a new IMM parameter change is detected.
 *
 * @param mysqlCon to MySQL connector
 * @return to indicate if the function call is success (EXIT_SUCCESS) or failure (EXIT_FAILURE) */
int OPCUA_InsertPreviousIMMPara(MYSQL mysqlCon)
//[OPCUA]Copy and Insert Previous IMM Parameters as A New Record
{
	char charStatement[MAX_STRING_SIZE];
	int intRetval;
	char charErrMsg[LONG_STRING_SIZE];
	char *charIMMParaSN;
	unsigned int intTechnicianUserSN;
	unsigned int intLogSN;
	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	// Select Technician UserSN Where IMMSN Is Given From INJPRO_Data_IMMSNList
	snprintf(charStatement, MAX_STRING_SIZE, "SELECT TechnicianUserSN FROM %s_%s.IMMSNList WHERE IMMSN=%d",
			 SYS_NAME, DATA_DATABASE_NAME, intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[OPCUA_InsertPreviousIMMPara]Fail to select TechnicianUserSN where IMMSN=%d from %s_%s.IMMSNList (%d):%s",
				 intIMMSN, SYS_NAME, DATA_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return intLogSN;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		mysqlRow = mysql_fetch_row(mysqlResult);
		if (mysqlRow[0] != NULL)
		{
			intTechnicianUserSN = atoi(mysqlRow[0]);
		}
		else
		{
			intTechnicianUserSN = 0;
		}
		mysql_free_result(mysqlResult);
	}

	// Select Last IMMPara from INJPRO_DATA_MO_[MOSN]_RawData_IMMPara.IMMParaSNList
	snprintf(charStatement, MAX_STRING_SIZE,
			 "INSERT INTO %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList "
			 "(	IMMParaSN,				SubscribeSN,				TechnicianUserSN,			IMMParaLastUpdateTime,	"
			 "InjectStage,				HoldStage,					PlasStage,				"
			 "MeltTemperature,			"
			 "PlastificationVolume1,	PlastificationVolume2,		PlastificationVolume3,		PlastificationVolume4,		PlastificationVolume5,"
			 "InjectionPressure1,		InjectionPressure2,			InjectionPressure3,			InjectionPressure4,			InjectionPressure5,"
			 "InjectionSpeed1,			InjectionSpeed2,			InjectionSpeed3,			InjectionSpeed4,			InjectionSpeed5,"
			 "VPChangeOverPosition,		VPChangeOverTime,			PackingPressure,			"
			 "HoldingPressure1,			HoldingPressure2,			HoldingPressure3,			HoldingPressure4,			HoldingPressure5,"
			 "PackingTime,				"
			 "HoldingTime1,				HoldingTime2,				HoldingTime3,				HoldingTime4,				HoldingTime5,"
			 "CoolingTime,				"
			 "ScrewRPM1,				ScrewRPM2,					ScrewRPM3,					ScrewRPM4,					ScrewRPM5,"
			 "BackPressure1,			BackPressure2,				BackPressure3,				BackPressure4,	 			BackPressure5,"
			 "MoldTemperature	"
			 ") SELECT "
			 "%d,						%d,							%d,							NOW(6),"
			 "InjectStage,				HoldStage, 					PlasStage,					"
			 "MeltTemperature,			"
			 "PlastificationVolume1,	PlastificationVolume2,		PlastificationVolume3,		PlastificationVolume4,		PlastificationVolume5,"
			 "InjectionPressure1,		InjectionPressure2,			InjectionPressure3,			InjectionPressure4,			InjectionPressure5,"
			 "InjectionSpeed1,			InjectionSpeed2,			InjectionSpeed3,			InjectionSpeed4,			InjectionSpeed5,"
			 "VPChangeOverPosition,		VPChangeOverTime,			PackingPressure,			"
			 "HoldingPressure1,			HoldingPressure2,			HoldingPressure3,			HoldingPressure4,			HoldingPressure5,"
			 "PackingTime,				"
			 "HoldingTime1,				HoldingTime2,				HoldingTime3,				HoldingTime4,				HoldingTime5,"
			 "CoolingTime,		"
			 "ScrewRPM1,				ScrewRPM2,					ScrewRPM3,					ScrewRPM4,					ScrewRPM5,"
			 "BackPressure1,			BackPressure2,				BackPressure3,				BackPressure4,				BackPressure5,"
			 "MoldTemperature	"
			 "FROM %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList WHERE IMMParaSN=%d",
			 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
			 intIMMParaSN, intSubscribeSN, intTechnicianUserSN,
			 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, intIMMParaSN - 1);

	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE,
				 "[OPCUA_InsertPreviousIMMPara]Fail to select/insert previous IMMPara:%d row from table %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList (%d): %s",
				 intIMMParaSN - 1, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return intLogSN;
	}
	return EXIT_SUCCESS;
}

/* [OPCUA]Insert IMM Sensor Data (via OPC UA) of A Specified Shot
 * This function is to insert the current IMM sensor data to the given shot serial number(ShotSN).
 *
 * @param mysqlCon to MySQL connector
 * @param OPCUAClient to OPC UA client
 * @param intShotSN to indicate the shot serial number in database
 * @return EXIT_FAILURE to indicate if the function call is success (EXIT_SUCCESS) or failure (EXIT_FAILURE) */
int OPCUA_InsertIMMSensor(MYSQL mysqlCon, UA_Client *OPCUAClient, int intShotSN)
//[OPCUA]Insert IMM Sensor Data (via OPC UA) of A Specified Shot
{
	char charStatement[MAX_STRING_SIZE];
	unsigned int intLogSN;
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	char charIMMSensorValue[OPCUA_IMMSENSOR_NODE_NUM + 1][TINY_STRING_SIZE];

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;
	unsigned int intMonitorItemSN;
	unsigned int intMonitorItemSensorSN;
	unsigned int intSPCSampleSize;
	double *doubleXBarValue;
	double *doubleRValue;

	// UA_ClientConfig config_local = UA_ClientConfig_default;
	// UA_Client *client_local = UA_Client_new(config_local);

	// intRetval = UA_Client_connect(client_local, "opc.tcp://10.10.150.3:4842");
	// intRetval = UA_Client_connect(client_local, "opc.tcp://127.0.0.1:9528");

	for (int i = OPCUA_FIRST_IMMSENSOR; i <= OPCUA_IMMSENSOR_NODE_NUM + OPCUA_IMMPARA_NODE_NUM; i++)
	{
		if (charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NS] != NULL)
		{

			// fprintf(stderr,"[SubscribeOPCUA]%d-%s-%s-%s-%s\n",i - OPCUA_IMMPARA_NODE_NUM,
			// charOPCUAIMMSensorNodeList[i-OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NAME],
			// charOPCUAIMMSensorNodeList[i-OPCUA_IMMPARA_NODE_NUM][OPCUA_META_IDENTIFIER_TYPE],
			// charOPCUAIMMSensorNodeList[i-OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NS],
			// charOPCUAIMMSensorNodeList[i-OPCUA_IMMPARA_NODE_NUM][OPCUA_META_ID]);

			// fprintf(stderr, "!!!Index:%d Converted Ratio:%lf\n", i, doubleOPCUAConvertRatio[i]);
			snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "%s", "NULL");

			UA_Variant *readVariant = UA_Variant_new();
			UA_Variant *convertedVariant = UA_Variant_new();

			if (strcmp(charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_IDENTIFIER_TYPE], "i") == 0)
			{
				intRetval = UA_Client_readValueAttribute(OPCUAClient,
														 UA_NODEID_NUMERIC(atoi(charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NS]),
																		   atoi(charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_ID])),
														 readVariant);
			}
			else if (strcmp(charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_IDENTIFIER_TYPE], "s") == 0)
			{
				intRetval = UA_Client_readValueAttribute(OPCUAClient,
														 UA_NODEID_STRING(atoi(charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NS]),
																		  charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_ID]),
														 readVariant);
			}

			if (intRetval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(readVariant))
			{
				if (readVariant->type == &UA_TYPES[UA_TYPES_BOOLEAN])
				{

					if (doubleOPCUAConvertRatio[i] == 1.0 || doubleOPCUAConvertRatio[i] == 0)
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%d'", *(UA_Boolean *)readVariant->data);
					else
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%lf'", (double)(*(UA_Boolean *)readVariant->data) * doubleOPCUAConvertRatio[i]);

#ifdef DEBUG_MODE_SUBSCRIBE_OPCUA
					printf("[OPCUA_InsertIMMSensor]Type:Boolean IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%d ConvertRatio:%lf ConvertedValue:%s\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NAME],
						   *(UA_Boolean *)readVariant->data, doubleOPCUAConvertRatio[i], charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM]);
#endif
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_FLOAT])
				{
					if (doubleOPCUAConvertRatio[i] == 1.0)
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%lf'", *(UA_Float *)readVariant->data);
					else
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%lf'", (double)(*(UA_Float *)readVariant->data) * doubleOPCUAConvertRatio[i]);

#ifdef DEBUG_MODE_SUBSCRIBE_OPCUA
					printf("[OPCUA_InsertIMMSensor]Type:Float IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%f ConvertRatio:%lf ConvertedValue:%s\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NAME],
						   *(UA_Float *)readVariant->data, doubleOPCUAConvertRatio[i], charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM]);
#endif
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_DOUBLE])
				{

					if (doubleOPCUAConvertRatio[i] == 1.0)
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%lf'", *(UA_Double *)readVariant->data);
					else
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%lf'", (double)(*(UA_Double *)readVariant->data) * doubleOPCUAConvertRatio[i]);

#ifdef DEBUG_MODE_SUBSCRIBE_OPCUA
					printf("[OPCUA_InsertIMMSensor]Type:Double IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%f ConvertRatio:%lf ConvertedValue:%s\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NAME],
						   *(UA_Double *)readVariant->data, doubleOPCUAConvertRatio[i], charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM]);
#endif
					// fprintf(stderr, "I am double!!! Index:%d  Ratio:%lf ConvertedValue:%s\n", i, doubleOPCUAConvertRatio[i], charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM]);
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_INT16])
				{
					if (doubleOPCUAConvertRatio[i] == 1.0)
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%d'", *(UA_Int16 *)readVariant->data);
					else
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%lf'", (double)(*(UA_Int16 *)readVariant->data) * doubleOPCUAConvertRatio[i]);

#ifdef DEBUG_MODE_SUBSCRIBE_OPCUA
					printf("[OPCUA_InsertIMMSensor]Type:Int16 IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%d ConvertRatio:%lf ConvertedValue:%s\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NAME],
						   *(UA_Int16 *)readVariant->data, doubleOPCUAConvertRatio[i], charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM]);
#endif
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_UINT16])
				{
					if (doubleOPCUAConvertRatio[i] == 1.0)
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%d'", *(UA_UInt16 *)readVariant->data);
					else
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%lf'", (double)(*(UA_UInt16 *)readVariant->data) * doubleOPCUAConvertRatio[i]);

#ifdef DEBUG_MODE_SUBSCRIBE_OPCUA
					printf("[OPCUA_InsertIMMSensor]Type:UInt16 IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%d ConvertRatio:%lf ConvertedValue:%s\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NAME],
						   *(UA_UInt16 *)readVariant->data, doubleOPCUAConvertRatio[i], charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM]);
#endif
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_INT32])
				{
					if (doubleOPCUAConvertRatio[i] == 1.0)
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%d'", *(UA_Int32 *)readVariant->data);
					else
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%lf'", (double)(*(UA_Int32 *)readVariant->data) * doubleOPCUAConvertRatio[i]);

#ifdef DEBUG_MODE_SUBSCRIBE_OPCUA
					printf("[OPCUA_InsertIMMSensor]Type:Int32 IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%d ConvertRatio:%lf ConvertedValue:%s\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NAME],
						   *(UA_Int32 *)readVariant->data, doubleOPCUAConvertRatio[i], charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM]);
#endif
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_UINT32])
				{
					if (doubleOPCUAConvertRatio[i] == 1.0)
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%d'", *(UA_UInt32 *)readVariant->data);
					else
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%lf'", (double)(*(UA_UInt32 *)readVariant->data) * doubleOPCUAConvertRatio[i]);
#ifdef DEBUG_MODE_SUBSCRIBE_OPCUA

					printf("[OPCUA_InsertIMMSensor]Type:UInt32 IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%d ConvertRatio:%lf ConvertedValue:%s\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NAME],
						   *(UA_UInt32 *)readVariant->data, doubleOPCUAConvertRatio[i], charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM]);
#endif
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_INT64])
				{
					if (doubleOPCUAConvertRatio[i] == 1.0)
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%ld'", *(UA_Int64 *)readVariant->data);
					else
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%lf'", (double)(*(UA_Int64 *)readVariant->data) * doubleOPCUAConvertRatio[i]);

#ifdef DEBUG_MODE_SUBSCRIBE_OPCUA
					printf("[OPCUA_InsertIMMSensor]Type:Int64 IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%ld ConvertRatio:%lf ConvertedValue:%s\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NAME],
						   *(UA_Int64 *)readVariant->data, doubleOPCUAConvertRatio[i], charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM]);
					// *(UA_Int64*)readVariant->data);
#endif
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_UINT64])
				{
					if (doubleOPCUAConvertRatio[i] == 1.0)
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%ld'", *(UA_UInt64 *)readVariant->data);
					else
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%lf'", (double)(*(UA_UInt64 *)readVariant->data) * doubleOPCUAConvertRatio[i]);

#ifdef DEBUG_MODE_SUBSCRIBE_OPCUA
					printf("[OPCUA_InsertIMMSensor]Type:UInt64 IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%ld ConvertRatio:%lf ConvertedValue:%s\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NAME],
						   *(UA_UInt64 *)readVariant->data, doubleOPCUAConvertRatio[i], charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM]);
#endif
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_STRING])
				{
					if (doubleOPCUAConvertRatio[i] == 1.0)
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%s'", ((UA_String *)(readVariant->data))->data);
					else
						snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM], TINY_STRING_SIZE, "'%lf'", atof(((UA_String *)(readVariant->data))->data) * doubleOPCUAConvertRatio[i]);

					// #ifdef DEBUG_MODE_SUBSCRIBE_OPCUA

					printf("[OPCUA_InsertIMMSensor]Type:String IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%s ConvertRatio:%lf ConvertedValue:%s\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NAME],
						   ((UA_String *)(readVariant->data))->data, doubleOPCUAConvertRatio[i], charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM]);
					//(char *)((UA_String *)readVariant->data));
					// #endif
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_DATETIME])
				{
#ifdef DEBUG_MODE_SUBSCRIBE_OPCUA

					UA_DateTimeStruct t = UA_DateTime_toStruct(*(UA_DateTime *)readVariant->data);
					printf("[OPCUA_InsertIMMSensor]Type:DateTime IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%4d-%02d-%02d %02d:%02d:%02d\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM][OPCUA_META_NAME],
						   t.year, t.month, t.day, t.hour, t.min, t.sec);
#endif
					// snprintf(charIMMSensorValue[i-OPCUA_IMMPARA_NODE_NUM-1],TINY_STRING_SIZE,"%d",*(UA_Int16*)readVariant->data);
				}
			}
			UA_Variant_delete(readVariant);
			UA_Variant_delete(convertedVariant);
		}
	}

	// Disconnect OPC UA Client and Deliete it
	// UA_Client_disconnect(client_local);
	// UA_Client_delete(client_local);

	// bermuda
#ifdef CARBON_EMISSION_MODE_OFF
	snprintf(charStatement, MAX_STRING_SIZE,
			 "INSERT INTO %s_%s_%s_%d_RawData_FeatureValue.IMMSensorSNList "
			 "("
			 "ShotSN,                   	CycleTime,						MoldClampingTime,			MoldReleasingTime,"
			 "InjectionTime,            	VPChangeOverDelayTime,			HoldingTime,				PlastificationTime,"
			 "VPChangeOverPosition,			CusionVolume,					EndHoldingPosition,			EndPlastificationPosition,"
			 "Nozzle1Temperature,			Nozzle2Temperature,				Nozzle3Temperature,			Nozzle4Temperature,"
			 "Nozzle5Temperature,			IMMOilTemperature,				FallMaterialTemperature,	MaxInjectionPressure,"
			 "VPChangeOverPressure,			SumInjectionPressure,			SumHoldingPressure,			MaxCavityNearGatePressure,"
			 "MaxCavityFarGatePressure,		SumCavityPressure,				SumMoldClampingForce,		MaxMoldClampingForce,"
			 "MaxBackPressure,				DelayPlastificationTime,		InMoldCoolingTemperature,	OutMoldCoolingTemperature,"
			 "MoldCoolingVolume,			MoldReleasingPosition2,			MoldReleasingPosition3,		MoldReleasingPosition4,"
			 "MoldReleasingPosition5,		ScrewSuckBackPosition,			ScrewRPM,					PartWeight,"
			 "GateClosingTime,				MeltTemperature,				MoldTemperature"
			 ") VALUE ("
			 "%d,							%s,								%s,							%s,"
			 "%s,							%s,								%s,							%s,"
			 "%s,							%s,								%s,							%s,"
			 "%s,							%s,								%s,							%s,"
			 "%s,							%s,								%s,							%s,"
			 "%s,							%s,								%s,							%s,"
			 "%s,							%s,								%s,							%s,"
			 "%s,							%s,								%s,							%s,"
			 "%s,							%s,								%s,							%s,"
			 "%s,							%s,								%s,							%s,"
			 "%s,							%s,								%s"
			 ")",
			 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
			 intShotSN,
			 charIMMSensorValue[IMMSENSOR_CYCLE_TIME - OPCUA_IMMPARA_NODE_NUM],					  // 54
			 charIMMSensorValue[IMMSENSOR_MOLD_CLAMPING_TIME - OPCUA_IMMPARA_NODE_NUM],			  // 55
			 charIMMSensorValue[IMMSENSOR_MOLD_RELEASING_TIME - OPCUA_IMMPARA_NODE_NUM],		  // 56
			 charIMMSensorValue[IMMSENSOR_INJECTION_TIME - OPCUA_IMMPARA_NODE_NUM],				  // 57
			 charIMMSensorValue[IMMSENSOR_VPCHANGEOVER_DELAY_TIME - OPCUA_IMMPARA_NODE_NUM],	  // 58
			 charIMMSensorValue[IMMSENSOR_HOLDING_TIME - OPCUA_IMMPARA_NODE_NUM],				  // 59
			 charIMMSensorValue[IMMSENSOR_PLASTIFICATION_TIME - OPCUA_IMMPARA_NODE_NUM],		  // 60
			 charIMMSensorValue[IMMSENSOR_VPCHANGEOVER_POSITION - OPCUA_IMMPARA_NODE_NUM],		  // 61
			 charIMMSensorValue[IMMSENSOR_CUSION_VOLUME - OPCUA_IMMPARA_NODE_NUM],				  // 62
			 charIMMSensorValue[IMMSENSOR_END_HOLDING_POSITION - OPCUA_IMMPARA_NODE_NUM],		  // 63
			 charIMMSensorValue[IMMSENSOR_END_PLASTIFICATION_POSITION - OPCUA_IMMPARA_NODE_NUM],  // 64
			 charIMMSensorValue[IMMSENSOR_NOZZLE1_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],		  // 65
			 charIMMSensorValue[IMMSENSOR_NOZZLE2_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],		  // 66
			 charIMMSensorValue[IMMSENSOR_NOZZLE3_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],		  // 67
			 charIMMSensorValue[IMMSENSOR_NOZZLE4_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],		  // 68
			 charIMMSensorValue[IMMSENSOR_NOZZLE5_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],		  // 69
			 charIMMSensorValue[IMMSENSOR_IMM_OIL_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],		  // 70
			 charIMMSensorValue[IMMSENSOR_FALL_MATERIAL_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],	  // 71
			 charIMMSensorValue[IMMSENSOR_MAX_INJECTION_PRESSURE - OPCUA_IMMPARA_NODE_NUM],		  // 72
			 charIMMSensorValue[IMMSENSOR_MAX_VPCHANGEOVER_PRESSURE - OPCUA_IMMPARA_NODE_NUM],	  // 73
			 charIMMSensorValue[IMMSENSOR_SUM_INJECTION_PRESSURE - OPCUA_IMMPARA_NODE_NUM],		  // 74
			 charIMMSensorValue[IMMSENSOR_SUM_HOLDING_PRESSURE - OPCUA_IMMPARA_NODE_NUM],		  // 75
			 charIMMSensorValue[IMMSENSOR_MAX_CAVITYNEARGATE_PRESSURE - OPCUA_IMMPARA_NODE_NUM],  // 76
			 charIMMSensorValue[IMMSENSOR_MAX_CAVITYFARGATE_PRESSURE - OPCUA_IMMPARA_NODE_NUM],	  // 77
			 charIMMSensorValue[IMMSENSOR_SUM_CAVITY_PRESSURE - OPCUA_IMMPARA_NODE_NUM],		  // 78
			 charIMMSensorValue[IMMSENSOR_SUM_MOLD_CLAMPING_FORCE - OPCUA_IMMPARA_NODE_NUM],	  // 79
			 charIMMSensorValue[IMMSENSOR_MAX_MOLD_CLAMPING_FORCE - OPCUA_IMMPARA_NODE_NUM],	  // 80
			 charIMMSensorValue[IMMSENSOR_MAX_BACK_PRESSURE - OPCUA_IMMPARA_NODE_NUM],			  // 81
			 charIMMSensorValue[IMMSENSOR_DELAY_PLASTIFICATION_TIME - OPCUA_IMMPARA_NODE_NUM],	  // 82
			 charIMMSensorValue[IMMSENSOR_IN_MOLD_COOLING_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],  // 83
			 charIMMSensorValue[IMMSENSOR_OUT_MOLD_COOLING_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM], // 84
			 charIMMSensorValue[IMMSENSOR_MOLD_COOLING_VOLUME - OPCUA_IMMPARA_NODE_NUM],		  // 85
			 charIMMSensorValue[IMMSENSOR_MOLD_RELEASING_POSITION_2 - OPCUA_IMMPARA_NODE_NUM],	  // 86
			 charIMMSensorValue[IMMSENSOR_MOLD_RELEASING_POSITION_3 - OPCUA_IMMPARA_NODE_NUM],	  // 87
			 charIMMSensorValue[IMMSENSOR_MOLD_RELEASING_POSITION_4 - OPCUA_IMMPARA_NODE_NUM],	  // 88
			 charIMMSensorValue[IMMSENSOR_MOLD_RELEASING_POSITION_5 - OPCUA_IMMPARA_NODE_NUM],	  // 89
			 charIMMSensorValue[IMMSENSOR_SCREW_SUCK_BACK_POSITION - OPCUA_IMMPARA_NODE_NUM],	  // 90
			 charIMMSensorValue[IMMSENSOR_SCREW_RPM - OPCUA_IMMPARA_NODE_NUM],					  // 91
			 charIMMSensorValue[IMMSENSOR_PART_WEIGHT - OPCUA_IMMPARA_NODE_NUM],				  // 92
			 charIMMSensorValue[IMMSENSOR_GATE_CLOSING_TIME - OPCUA_IMMPARA_NODE_NUM],			  // 93
			 charIMMSensorValue[IMMSENSOR_MELT_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],			  // 94
			 charIMMSensorValue[IMMSENSOR_MOLD_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM]);			  // 95
#endif

#ifdef CARBON_EMISSION_MODE_ON
	snprintf(charStatement, MAX_STRING_SIZE,
			 "INSERT INTO %s_%s_%s_%d_RawData_FeatureValue.IMMSensorSNList "
			 "("
			 "ShotSN,                   	CycleTime,						MoldClampingTime,			MoldReleasingTime,"
			 "InjectionTime,            	VPChangeOverDelayTime,			HoldingTime,				PlastificationTime,"
			 "VPChangeOverPosition,			CusionVolume,					EndHoldingPosition,			EndPlastificationPosition,"
			 "Nozzle1Temperature,			Nozzle2Temperature,				Nozzle3Temperature,			Nozzle4Temperature,"
			 "Nozzle5Temperature,			IMMOilTemperature,				FallMaterialTemperature,	MaxInjectionPressure,"
			 "VPChangeOverPressure,			SumInjectionPressure,			SumHoldingPressure,			MaxCavityNearGatePressure,"
			 "MaxCavityFarGatePressure,		SumCavityPressure,				SumMoldClampingForce,		MaxMoldClampingForce,"
			 "MaxBackPressure,				DelayPlastificationTime,		InMoldCoolingTemperature,	OutMoldCoolingTemperature,"
			 "MoldCoolingVolume,			MoldReleasingPosition2,			MoldReleasingPosition3,		MoldReleasingPosition4,"
			 "MoldReleasingPosition5,		ScrewSuckBackPosition,			ScrewRPM,					PartWeight,"
			 "GateClosingTime,				MeltTemperature,				MoldTemperature"
			 ") VALUE ("
			 "%d,							%s,								%s,							%s,"
			 "%s,							%s,								%s,							%s,"
			 "%s,							%s,								%s,							%s,"
			 "%s,							%s,								%s,							%s,"
			 "%s,							%s,								%s,							%s,"
			 "%s,							%s,								%s,							%s,"
			 "%s,							%s,								%s,							%s,"
			 "%s,							%s,								%s,							%s,"
			 "%s,							%s,								%s,							%s,"
			 "%s,							%s,								%s,							%s*0.601+13.23*3.26/1000.0,"
			 "%s,							%s,								%s"
			 ")",
			 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
			 intShotSN,
			 charIMMSensorValue[IMMSENSOR_CYCLE_TIME - OPCUA_IMMPARA_NODE_NUM],					  // 54
			 charIMMSensorValue[IMMSENSOR_MOLD_CLAMPING_TIME - OPCUA_IMMPARA_NODE_NUM],			  // 55
			 charIMMSensorValue[IMMSENSOR_MOLD_RELEASING_TIME - OPCUA_IMMPARA_NODE_NUM],		  // 56
			 charIMMSensorValue[IMMSENSOR_INJECTION_TIME - OPCUA_IMMPARA_NODE_NUM],				  // 57
			 charIMMSensorValue[IMMSENSOR_VPCHANGEOVER_DELAY_TIME - OPCUA_IMMPARA_NODE_NUM],	  // 58
			 charIMMSensorValue[IMMSENSOR_HOLDING_TIME - OPCUA_IMMPARA_NODE_NUM],				  // 59
			 charIMMSensorValue[IMMSENSOR_PLASTIFICATION_TIME - OPCUA_IMMPARA_NODE_NUM],		  // 60
			 charIMMSensorValue[IMMSENSOR_VPCHANGEOVER_POSITION - OPCUA_IMMPARA_NODE_NUM],		  // 61
			 charIMMSensorValue[IMMSENSOR_CUSION_VOLUME - OPCUA_IMMPARA_NODE_NUM],				  // 62
			 charIMMSensorValue[IMMSENSOR_END_HOLDING_POSITION - OPCUA_IMMPARA_NODE_NUM],		  // 63
			 charIMMSensorValue[IMMSENSOR_END_PLASTIFICATION_POSITION - OPCUA_IMMPARA_NODE_NUM],  // 64
			 charIMMSensorValue[IMMSENSOR_NOZZLE1_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],		  // 65
			 charIMMSensorValue[IMMSENSOR_NOZZLE2_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],		  // 66
			 charIMMSensorValue[IMMSENSOR_NOZZLE3_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],		  // 67
			 charIMMSensorValue[IMMSENSOR_NOZZLE4_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],		  // 68
			 charIMMSensorValue[IMMSENSOR_NOZZLE5_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],		  // 69
			 charIMMSensorValue[IMMSENSOR_IMM_OIL_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],		  // 70
			 charIMMSensorValue[IMMSENSOR_FALL_MATERIAL_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],	  // 71
			 charIMMSensorValue[IMMSENSOR_MAX_INJECTION_PRESSURE - OPCUA_IMMPARA_NODE_NUM],		  // 72
			 charIMMSensorValue[IMMSENSOR_MAX_VPCHANGEOVER_PRESSURE - OPCUA_IMMPARA_NODE_NUM],	  // 73
			 charIMMSensorValue[IMMSENSOR_SUM_INJECTION_PRESSURE - OPCUA_IMMPARA_NODE_NUM],		  // 74
			 charIMMSensorValue[IMMSENSOR_SUM_HOLDING_PRESSURE - OPCUA_IMMPARA_NODE_NUM],		  // 75
			 charIMMSensorValue[IMMSENSOR_MAX_CAVITYNEARGATE_PRESSURE - OPCUA_IMMPARA_NODE_NUM],  // 76
			 charIMMSensorValue[IMMSENSOR_MAX_CAVITYFARGATE_PRESSURE - OPCUA_IMMPARA_NODE_NUM],	  // 77
			 charIMMSensorValue[IMMSENSOR_SUM_CAVITY_PRESSURE - OPCUA_IMMPARA_NODE_NUM],		  // 78
			 charIMMSensorValue[IMMSENSOR_SUM_MOLD_CLAMPING_FORCE - OPCUA_IMMPARA_NODE_NUM],	  // 79
			 charIMMSensorValue[IMMSENSOR_MAX_MOLD_CLAMPING_FORCE - OPCUA_IMMPARA_NODE_NUM],	  // 80
			 charIMMSensorValue[IMMSENSOR_MAX_BACK_PRESSURE - OPCUA_IMMPARA_NODE_NUM],			  // 81
			 charIMMSensorValue[IMMSENSOR_DELAY_PLASTIFICATION_TIME - OPCUA_IMMPARA_NODE_NUM],	  // 82
			 charIMMSensorValue[IMMSENSOR_IN_MOLD_COOLING_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],  // 83
			 charIMMSensorValue[IMMSENSOR_OUT_MOLD_COOLING_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM], // 84
			 charIMMSensorValue[IMMSENSOR_MOLD_COOLING_VOLUME - OPCUA_IMMPARA_NODE_NUM],		  // 85
			 charIMMSensorValue[IMMSENSOR_MOLD_RELEASING_POSITION_2 - OPCUA_IMMPARA_NODE_NUM],	  // 86
			 charIMMSensorValue[IMMSENSOR_MOLD_RELEASING_POSITION_3 - OPCUA_IMMPARA_NODE_NUM],	  // 87
			 charIMMSensorValue[IMMSENSOR_MOLD_RELEASING_POSITION_4 - OPCUA_IMMPARA_NODE_NUM],	  // 88
			 charIMMSensorValue[IMMSENSOR_MOLD_RELEASING_POSITION_5 - OPCUA_IMMPARA_NODE_NUM],	  // 89
			 charIMMSensorValue[IMMSENSOR_SCREW_SUCK_BACK_POSITION - OPCUA_IMMPARA_NODE_NUM],	  // 90
			 charIMMSensorValue[IMMSENSOR_SCREW_RPM - OPCUA_IMMPARA_NODE_NUM],					  // 91
			 charIMMSensorValue[IMMSENSOR_MELT_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],			  // 92
			 charIMMSensorValue[IMMSENSOR_GATE_CLOSING_TIME - OPCUA_IMMPARA_NODE_NUM],			  // 93
			 charIMMSensorValue[IMMSENSOR_MELT_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM],			  // 94
			 charIMMSensorValue[IMMSENSOR_MOLD_TEMPERATURE - OPCUA_IMMPARA_NODE_NUM]);			  // 95
#endif

	//fprintf(stderr, "Q:%s\n", charStatement);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE,
				 "[OPCUA_InsertIMMSensor]Fail to insertIMM sensor data to table %s_%s_%s_%d_RawData_FeatureValue.IMMSensorSNList for ShotSN:%d (%d): %s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, intShotSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		fprintf(stderr, "Error Sended\n");
		return EXIT_FAILURE;
	}

	
#ifdef CARBON_EMISSION_MODE_ON
	intRetval = DB_UpdateCarbonEmission(mysqlCon, intMOSN);
	if (intRetval != EXIT_SUCCESS)
	{
		// fprintf(stderr,"Q:%s\n",charStatement);
		snprintf(charErrMsg, LONG_STRING_SIZE,
				 "[OPCUA_InsertIMMSensor]Fail to update carbon emission related information for MOSN:%d in table %s_%s_%s.MOSNList (%d): %s",
				 intMOSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
	}
	else
	{
		fprintf(stderr, "[OPCUA_InsertIMMSensor]Insert Carbon Emission feature value for ShotSN:%d successfully.\n", intIMMSensorShotSN);
	}

#endif
	// Select MonitorItemSN for Each Feature Value
	snprintf(charStatement, MAX_STRING_SIZE,
			 "SELECT MonitorItemSN,MonitorItemSensorSN,SPCSampleSize FROM %s_%s_%s_%d_Info_Config.MonitorItemSNList WHERE MonitorItemClass = %d",
			 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, MONITOR_ITEM_CLASS_OPCUA);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE,
				 "[OPCUA_InsertIMMSensor]Fail to select MonitorItemSN,MonitorItemIndex and SPCSampleSize for MonitorItemClass = OPCUA(%d) from %s_%s_%s_%d_Info_Config.MonitorItemSNList (%d):%s",
				 MONITOR_ITEM_CLASS_MODBUS, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, 0, mysql_errno(&mysqlCon), charErrMsg);
		return intLogSN;
	}
	mysqlResult = mysql_store_result(&mysqlCon);
	if (mysqlResult == NULL)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE,
				 "[OPCUA_InsertIMMSensor]No result of MonitorItemSN, MonitorItemIndex and SPCSampleSize for MonitorItemClass = OPCUA(%d) from %s_%s_%s_%d_Info_Config.MonitorItemSNList",
				 MONITOR_ITEM_CLASS_MODBUS, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, 0, mysql_errno(&mysqlCon), charErrMsg);
		return intLogSN;
	}	

	// For All MonitorItemSN Monitor Item
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (mysqlRow[0] != NULL && mysqlRow[1] != NULL && mysqlRow[2] != NULL)
		{
			intMonitorItemSN = atoi(mysqlRow[0]);
			intMonitorItemSensorSN = atoi(mysqlRow[1]);
			intSPCSampleSize = atoi(mysqlRow[2]);

			//fprintf(stderr, "ShotSN:%d MonitorItemSN:%d MonitorItemSensorSN:%d SPCSampleSize:%d\n",
			//intShotSN, intMonitorItemSN, intMonitorItemSensorSN, intSPCSampleSize);

			// If A Group IS Completed
			if (intShotSN % intSPCSampleSize == 0)
			{
				// Fetch XBar Value
				intRetval = SPC_SelectSPCXBarValue(mysqlCon, intMOSN, intMonitorItemSN, intShotSN, SPC_RULE1_POINT_SIZE, &doubleXBarValue);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[OPCUA_InsertIMMSensor]Fail to select XBar Value for MOSN:%d ShotSN:%d PointSize:%d",
							 intMOSN, intShotSN, SPC_RULE1_POINT_SIZE);
					intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, 0, 0, charErrMsg);
					//mysql_free_result(mysqlResult);
					return intLogSN;
				}

				// Fetch R Value
				intRetval = SPC_SelectRValue(mysqlCon, intMOSN, intMonitorItemSN, intShotSN, SPC_RULE1_POINT_SIZE, &doubleRValue);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[OPCUA_InsertIMMSensor]Fail to select R Value for MOSN:%d ShotSN:%d PointSize:%d",
							 intMOSN, intShotSN, SPC_RULE1_POINT_SIZE);
					intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, 0, 0, charErrMsg);
					//mysql_free_result(mysqlResult);
					return intLogSN;
				}
				snprintf(charStatement, MAX_STRING_SIZE,
						 "INSERT INTO %s_%s_%s_%d_RawData_ChartData.MonitorItemSN%d"
						 "(ShotSN, GroupSN, XValue, XBarValue, RValue) VALUE (%d, %d, %s, %lf, %lf)",
						 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, intMonitorItemSN,
						 intShotSN, intShotSN / intSPCSampleSize,
						 charIMMSensorValue[intMonitorItemSensorSN - OPCUA_IMMPARA_NODE_NUM],
						 *doubleXBarValue, *doubleRValue);
			}
			else
			{
				snprintf(charStatement, MAX_STRING_SIZE,
						 "INSERT INTO %s_%s_%s_%d_RawData_ChartData.MonitorItemSN%d"
						 "(ShotSN, GroupSN, XValue, XBarValue, RValue) VALUE (%d, NULL, %s, NULL, NULL)",
						 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, intMonitorItemSN,
						 intShotSN, charIMMSensorValue[intMonitorItemSensorSN - OPCUA_IMMPARA_NODE_NUM]);
			}

			intRetval = mysql_query(&mysqlCon, charStatement);
			//fprintf(stderr,"Q:%s\n",charStatement);
			if (intRetval != EXIT_SUCCESS)
			{
				// fprintf(stderr,"Q:%s\n",charStatement);
				snprintf(charErrMsg, MAX_STRING_SIZE,
						 "[OPCUA_InsertIMMSensor]Fail to insert OPCUA feature value of "
						 "ShotSN:%d into table %s_%s_%s_%d_RawData_ChartData.MonitorItemSN%d (%d):%s",
						 intShotSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, intMonitorItemSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
				intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, 0, mysql_errno(&mysqlCon), charErrMsg);
				//mysql_free_result(mysqlResult);
				return intLogSN;
			}
		}
	}
	mysql_free_result(mysqlResult);

	return EXIT_SUCCESS;
}