/*	Copyright 2019-2021 (c) Industrial Technology Research Institute (Author: Cheng-Ying Liu)
 *
 *	The subscribeModbus.c is the main program for DAQ device to collect custom sensor data.
 *	The program should be executed as a indepedent thread (i.e. add '&' in the end of command).
 *	An unsubscribe program is also provided and will forcely terminate the procress to default state.
 *	For subscribeModbus, there are two required configurations which must be set before execution.
 *	(1) DAQ Device Configuration and (2)Custom Sensor Configuration, which are seperately described as follows:
 *
 *	(1) DAQ Device Configuration:
 *		This configuration including DAQ IP and Port which are defined in IMMSNList.
 *		Note that the subscribe command should only append IMMSN instead of IP or Port,
 *		and the program will read them from IMMSNList according the IMMSN.
 *		For example, if user want to subscribe IMMSN 1, the command is as follows:
 *
 *		>./exeSubscribeModbus -IMMSN 1 &
 *
 *		After that, the DAQ IP and Port will be read from IMMSNLlist with condition IMMSN = 1.
 *		Note that the symbol "&" is to create a new thread for this procress, and the PID will also be recorded in IMMSNList.
 *
 *	(2)	Custom Sensor Configuration:
 *		This configuration including the ChannelSN and ModelSN about all custom sensors.
 *		Firstly, since different channel could connect to different types of custom sensor(e.g. cavity temperature, system pressure),
 *		therefore the manufacturing order serial number(i.e. MOSN) should provide the amount and category of each custom sensor.
 *		For example, if an IMM equipe with one system pressure sensor and 2 cavity pressure sensors, the data flow could be explained as follows,
 *
 *		IMMSN ==> MOSN 	==> system pressure ==> channel 1 ==> MOCustomSensorSN1_TableSN1 (RawData) ==> MOCustomSensorSN1 (Feature Value)
 *						==> cavity pressure ==> channel 2 ==> MOCustomSensorSN2_TableSN1 (RawData) ==> MOCustomSensorSN2 (Feature Value)
 *						==> cavity pressure ==> channel 3 ==> MOCustomSensorSN2_TableSN1 (RawData) ==> MOCustomSensorSN3 (Feature Value)
 *
 *		This program will firstly read the configuration according to manufacturing order serial number(i.e. MOSN),
 *		and then insert rawdata to rawdata table. Finally for each shot the feature value will be caculated and insert to feature value tables.
 *		It is worthy to notice that table will be created dynamically according to different situations, and this program will execute accordingly.
 *		Finally, after the feature value is caculated, the EC, QC and SPC module will judge the feature according to users' setting.
 *
 */

#include <stdio.h>
#include <mysql/mysql.h>
#include <signal.h>
#include <math.h>
#include <unistd.h>
#include <modbus.h>
#include <pthread.h>
#include "config.h"
#include "config_modbus.h"
#include "itri_injpro.h"
#include "open62541.h"

bool boolIsStopSignal = false;
unsigned int intCurrentMoldStatus = MOLD_STATUS_UNKNOW;
unsigned int intMonIDNodeIndex[3] = {0};
char charMonIDNodeIndex[3][20];
unsigned int intIMMSN;
unsigned int intMOSN;
unsigned int intRoundSN;
unsigned int intShotSN;
unsigned int intFeatureValueReadyShotSN;
unsigned int intFeatureValueDoneShotSN;
unsigned int intOPCUAVersionSN;
unsigned int intDAQChanneltoMOCustomSensorSN[MODBUS_CHANNEL_AMOUNT] = {0};
unsigned int intDAQChanneltoSensorClass[MODBUS_CHANNEL_AMOUNT] = {0};
unsigned int intDAQChanneltoSensorCategory[MODBUS_CHANNEL_AMOUNT] = {0};
double doubleMOCustomSensorConvertRatio[MODBUS_CHANNEL_AMOUNT] = {0};
struct timespec stClampedTimeStamp;

typedef struct stSensorCacheData
{
	int intCacheHeadIndex;
	int intCacheTailIndex;
	int intShotSNCacheData[MODBUS_CHANNEL_AMOUNT][MODBUS_MAX_CACHE_SIZE];
	double doubleSensorValueCacheData[MODBUS_CHANNEL_AMOUNT][MODBUS_MAX_CACHE_SIZE];
	double doubleElapsedTimeCacheData[MODBUS_CHANNEL_AMOUNT][MODBUS_MAX_CACHE_SIZE];
} stSensorCacheData;

int Modbus_GetRangebyTypeCode(uint16_t intDAQTypeCode, int *intDAQMinBound, int *intDAQMaxBound);
int Modbus_GetMoldStatus(modbus_t *modbusCon, unsigned int intModbusMoldSignalType, unsigned int intPreviousMoldStatus, unsigned int *intCurrentMoldStatus);
int Modbus_GetElapsedTime(struct timespec stStartTimeStamp, double *doubleElapsedTime);
int Modbus_ConvertRawdata(uint16_t intDAQRawData, int intDAQMinBound, int intDAQMaxBound, double *doubleConvertedDAQRawData);
void *Modbus_WriteSensorCachetoDatabase(void *ptrSensorCacheData);
int Modbus_SubscribeMonitorItem(UA_Client *OPCUAClient, MYSQL mysqlCon);
static void Modbus_CallbackState(UA_Client *OPCUAClient, UA_ClientState OPCUAClientState);
static void Modbus_CallbackSubscriptionInactivity(UA_Client *OPCUAClient, UA_UInt32 subId, void *subContext);
static void Modbus_HandlerStopSubscribeModbus(int intSign);
static void Modbus_HandlerIMMChanged(UA_Client *OPCUAClient, UA_UInt32 subId, void *subContext, UA_UInt32 monId, void *monContext, UA_DataValue *value);
void *Modbus_WriteSensorCachetoDatabase(void *ptrSensorCacheData);

int main(int argc, char *argv[])
{
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	unsigned int intLogSN;

	unsigned int intModbusMoldSignalSource;
	unsigned int intModbusMoldSignalType;
	int intModbusClientPID;
	unsigned int intPreviousMoldStatus = MOLD_STATUS_UNKNOW;
	char charStatement[MAX_STRING_SIZE];

	char charOPCUAIP[16];
	unsigned int intOPCUAPort;
	char charEndPointURL[MEDIUM_STRING_SIZE] = {'\0'};
	bool boolHasInsertErrMsg = false;
	bool boolHasElapsedTimeStarted = false;
	bool boolHasMoldStatusReady = false;
	bool boolHasThisShotReleased = false;
	bool boolHasThisShotReleasing = false;
	bool boolHasThisShotClamped = false;
	bool boolHasThisShotClamping = false;
	bool boolHasThisShotExceedMaxCycleTime = false;

	modbus_t *modbusCon;
	char charModbusIP[16];
	unsigned int intModbusPort;
	struct timeval stModbusTimeOut;

	unsigned int intMOCustomSensorCount = 0;
	unsigned int intDAQChannelAmount;
	int *intDAQBound;
	uint16_t intDAQTypeCode[125];
	uint16_t intPreviousDAQRawData[MODBUS_MAX_READ_REGISTERS];
	uint16_t intDAQRawData[MODBUS_MAX_READ_REGISTERS];
	double doubleConvertedDAQRawData;
	int intDAQRawDataSize;

	double doubleElapsedTime;

	struct timespec stConnectTimeStamp;
	double doubleConnectElapsedTime;

	MYSQL mysqlCon;
	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	UA_ClientConfig config;

	stSensorCacheData varSensorCacheData;
	pthread_t threadWriteSensorCachetoDatabase;

	signal(SIGINT, Modbus_HandlerStopSubscribeModbus);
	signal(SIGTERM, Modbus_HandlerStopSubscribeModbus);
	signal(SIGKILL, Modbus_HandlerStopSubscribeModbus);

	varSensorCacheData.intCacheHeadIndex = 0;
	varSensorCacheData.intCacheTailIndex = MODBUS_MAX_CACHE_SIZE - 1;

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

	// exeSubscribeModbus -IMMSN [1]
	if (argc == 3)
	{
		if (strcmp(argv[1], "-IMMSN") == 0)
		{
			intIMMSN = atoi(argv[2]);
		}
		else
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to subscribe Modbus due to parameter format error");
			intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, 0, intIMMSN, EXIT_FAILURE, charErrMsg);
			return intLogSN;
		}
	}

	fprintf(stderr, "Subscribe Modbus for IMMSN:%d\n", intIMMSN);

	// Get OPCUAVersionSN, ModbusIP, ModbusPort, ModbusMoldSignalType and ShotSN According to The Given IMMSN
	snprintf(charStatement, MAX_STRING_SIZE, "SELECT MOSN,ModbusMoldSignalSource,ModbusMoldSignalType,OPCUAVersionSN,OPCUAIP,OPCUAPort,ModbusIP,ModbusPort,ModbusClientPID FROM %s_%s.IMMSNList WHERE IMMSN=%d", SYS_NAME, DATA_DATABASE_NAME, intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to select Modbus information where IMMSN=%d from %s_%s.IMMSNList (%d):%s",
				 intIMMSN, SYS_NAME, DATA_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, 0, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return intLogSN;
	}

	mysqlResult = mysql_store_result(&mysqlCon);
	mysqlRow = mysql_fetch_row(mysqlResult);
	if (mysqlRow == NULL)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to subscribe Modbus due to IMMSN=%d is not found in %s_%s.IMMSNList",
				 intIMMSN, SYS_NAME, DATA_DATABASE_NAME);
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, 0, charErrMsg);
		return intLogSN;
	}

	// Get MOSN
	if (mysqlRow[0] == NULL)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to subscribe Modbus due to no MOSN for IMMSN:%d", intIMMSN);
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, 0, intIMMSN, EXIT_FAILURE, charErrMsg);
		return intLogSN;
	}
	else
	{
		intMOSN = atoi(mysqlRow[0]);
		fprintf(stderr, "[SubscribeModbus]MOSN:%d\n", intMOSN);
	}

	// Get ModbusMoldSignalSource
	if (mysqlRow[1] == NULL)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to subscribe Modbus due to no mold source for IMMSN:%d", intIMMSN);
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
		return intLogSN;
	}
	else
	{
		intModbusMoldSignalSource = atoi(mysqlRow[1]);
		fprintf(stderr, "[SubscribeModbus]Modbus Mold Signal Source:%d (1:MODBUS / 2:OPCUA)\n", intModbusMoldSignalSource);
	}

	// Get ModbusMoldSignalType
	if (mysqlRow[2] == NULL)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to subscribe Modbus due to no mold signaltype for IMMSN:%d", intIMMSN);
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
		return intLogSN;
	}
	else
	{
		intModbusMoldSignalType = atoi(mysqlRow[2]);
		fprintf(stderr, "[SubscribeModbus]Mold Signal Type:%d (1:singnal loop / 2:multiple loop)\n", intModbusMoldSignalType);
	}

	// Get OPCUAVersionSN
	if (mysqlRow[3] == NULL || mysqlRow[4] == NULL || mysqlRow[5] == NULL)
	{
		// snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to subscribe Modbus due to no OPCUAVersionSN for IMMSN:%d", intIMMSN);
		// intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
		// return intLogSN;

		intOPCUAVersionSN = VIRTUAL_OPCUA_SERVER_OPCUA_VERSIONSN;
		fprintf(stderr, "[SubscribeOPCUA]OPCUAVersionSN:Localhost Virtual OPCUA Version\n");
		strcpy(charOPCUAIP, VIRTUAL_OPCUA_SERVER_IP);
		intOPCUAPort = VIRTUAL_OPCUA_SERVER_PORT;
	}
	else
	{
		intOPCUAVersionSN = atoi(mysqlRow[3]);
		fprintf(stderr, "[SubscribeModbus]OPCUAVersionSN:%d\n", intOPCUAVersionSN);
		strcpy(charOPCUAIP, mysqlRow[4]);
		intOPCUAPort = atoi(mysqlRow[5]);
	}

	fprintf(stderr, "[SubscribeModbus]OPCUA Address:%s:%d\n", charOPCUAIP, intOPCUAPort);

	// Get Modbus IP
	if (mysqlRow[6] == NULL)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to subscribe Modbus due to no Modbus IP of IMMSN:%d", intIMMSN);
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
		return intLogSN;
	}
	else
	{
		strcpy(charModbusIP, mysqlRow[6]);
	}

	// Get Modbus Port
	if (mysqlRow[7] == NULL)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to subscribe Modbus due to no Modbus port for IMMSN:%d", intIMMSN);
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
		return intLogSN;
	}
	else
	{
		intModbusPort = atoi(mysqlRow[7]);
	}
	fprintf(stderr, "[SubscribeModbus]Modbus Address:%s:%d\n", charModbusIP, intModbusPort);

	// Get ModbusClientPID and If ModbusClientPID != NULL (A Subscribe Modbus Client Is Executing or Anomoly Exit)
	if (mysqlRow[8] != NULL)
	{
		intModbusClientPID = atoi(mysqlRow[8]);

		// Notify An Existing Subscribe Porcess Is Executing
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]A subscribe process PID:%d of IMMSN:%d is executing", intModbusClientPID, intIMMSN);
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
		// mysql_free_result(mysqlResult);
		// return intLogSN;

		// Kill the Subscribe Procress
		intRetval = kill(intModbusClientPID, SIGTERM);
		// Sleep 1 sec for waiting another procress.
		sleep(1);
		if (intRetval != EXIT_SUCCESS)
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to kill subscribe process PID:%d of IMMSN:%d.", intModbusClientPID, intIMMSN);
			intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
			// mysql_close(&mysqlCon);
			// return intLogSN;
		}
		else // Kill Procress Successfully
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]The subscribe process PID:%d of IMMSN:%d is terminated", intModbusClientPID, intIMMSN);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);

			clock_gettime(CLOCK_REALTIME, &stConnectTimeStamp);
			// Wait For The ModbusClientPID = NULL
			do
			{
				snprintf(charStatement, MAX_STRING_SIZE, "SELECT ModbusClientPID FROM %s_%s.IMMSNList WHERE IMMSN=%d",
						 SYS_NAME, DATA_DATABASE_NAME, intIMMSN);
				intRetval = mysql_query(&mysqlCon, charStatement);
				if (intRetval != EXIT_SUCCESS && boolHasInsertErrMsg == false)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to select SubscribeModbus for IMMSN=%d while exiting (%d):%s",
							 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
					mysql_free_result(mysqlResult);
					mysql_close(&mysqlCon);
					return EXIT_FAILURE;
				}
				mysqlResult = mysql_store_result(&mysqlCon);
				mysqlRow = mysql_fetch_row(mysqlResult);
				sleep(1);
				Modbus_GetElapsedTime(stConnectTimeStamp, &doubleConnectElapsedTime);

				if (doubleConnectElapsedTime > 3)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Subscribtion terminated fail due to wait for more than 3 secs. ModbusClientPID !=NULL. PID:%d of IMMSN:%d",
							 intModbusClientPID, intIMMSN);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
				}
			} while (mysqlRow[0] != NULL && doubleConnectElapsedTime < 3);
		}
	}
	mysql_free_result(mysqlResult);

	// Get timestamp before connect to OPC UA server
	// clock_gettime(CLOCK_REALTIME, &stConnectTimeStamp);

	// Update ModbusClientPID, ModbusClientStatus and IMMLastUpdateTime to INJPRO_Data.IMMSNList
	intModbusClientPID = getpid();
	snprintf(charStatement, MAX_STRING_SIZE,
			 "UPDATE %s_%s.IMMSNList SET ModbusClientStatus=%d, ModbusClientPID=%d, IMMLastUpdateTime = NOW(6) WHERE IMMSN=%d",
			 SYS_NAME, DATA_DATABASE_NAME, MODBUS_CLIENT_STATUS_UNKNOW, intModbusClientPID, intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE,
				 "[SubscribeModbus]Fail to update ModbusClientStatus = UNKNOW, ModbusClientPID = %d and IMMLastUpdateTime = NOW(6) where IMMSN=%d (%d):%s",
				 intModbusClientPID, intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return intLogSN;
	}

	// Get MOCustomSensorSN and MOCustomSensorChannelSN
	snprintf(charStatement, MAX_STRING_SIZE, "SELECT MOCustomSensorSN,MOCustomSensorChannelSN,MOCustomSensorConvertRatio FROM %s_%s_%s_%d_Info_Meta.MOCustomSensorSNList",
			 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to select MOCustomSensorSN and MOCustomSensorChannelSN from %s_%s_%s_%d_Info_Meta.MOCustomSensorSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return intLogSN;
	}
	mysqlResult = mysql_store_result(&mysqlCon);
	for (int i = 0; (mysqlRow = mysql_fetch_row(mysqlResult)) != NULL && i < MODBUS_CHANNEL_AMOUNT; i++)
	{
		intDAQChanneltoMOCustomSensorSN[atoi(mysqlRow[1])] = atoi(mysqlRow[0]);
		doubleMOCustomSensorConvertRatio[atoi(mysqlRow[0])] = atof(mysqlRow[2]);
		intMOCustomSensorCount++;
	}
	mysql_free_result(mysqlResult);

	// Get ShotSN
	intRetval = DB_SelectShotSNbyMOSN(mysqlCon, intMOSN, &intShotSN);
	if (intRetval != EXIT_SUCCESS)
	{
		fprintf(stderr, "[SubscribeModbus]Fail to select ShotSN for IMMSN:%d MOSN:%d\n", intIMMSN, intMOSN);
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeOPCUA]Fail to select ShotSN for IMMSN:%d MOSN:%d\n", intIMMSN, intMOSN);
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return intLogSN;
	}
	else
	{
		fprintf(stderr, "[SubscribeModbus]ShotSN:%d\n", intShotSN);
	}

	// Set intFeatureValueDoneShotSN and intFeatureValueReadyShotSN
	intFeatureValueDoneShotSN = intShotSN - 1;
	intFeatureValueReadyShotSN = intShotSN - 1;

	// Get Custom Sensor Category
	for (int i = 0; i < MODBUS_CHANNEL_AMOUNT; i++)
	{
		if (intDAQChanneltoMOCustomSensorSN[i] != 0)
		{
			intRetval = DB_SelectMOCustomSensorClassAndCategorybyMOCustomSensorSN(mysqlCon, intMOSN, intDAQChanneltoMOCustomSensorSN[i], &intDAQChanneltoSensorClass[i], &intDAQChanneltoSensorCategory[i]);
			if (intRetval != EXIT_SUCCESS)
			{
				snprintf(charErrMsg, LONG_STRING_SIZE,
						 "[SubscribeModbus]Fail to select mold sensor class of MOSN:%d MOCustomSensorSN:%d from %s_%s.SensorModelIndex and %s_%s_%s_%d_Info_Meta.MOCustomSensorSNList (%d):%s",
						 intMOSN, intDAQChanneltoMOCustomSensorSN[i], SYS_NAME, INDEX_DATABASE_NAME, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
				intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
				return intLogSN;
			}
		}
	}

	// Display Custom Sensor Channel #, CustomerSensorSN and SensorClass
	for (int i = 0; i < MODBUS_CHANNEL_AMOUNT; i++)
	{
		fprintf(stderr, "[SubscribeModbus]Channel:%d MOCustomSensorSN:%d SensorClass:%d SensorCategory:%d\n", i, intDAQChanneltoMOCustomSensorSN[i], intDAQChanneltoSensorClass[i], intDAQChanneltoSensorCategory[i]);
	}

	// Connect to Modbus device
	modbusCon = modbus_new_tcp(charModbusIP, intModbusPort);
	if (modbusCon == NULL)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to connect Modbus device for IMMSN:%d", intIMMSN);
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);

		// Update ModbusClientStatus and IMMLastUpdateTime to INJPRO_Data.IMMSNList
		intModbusClientPID = getpid();
		snprintf(charStatement, MAX_STRING_SIZE,
				 "UPDATE %s_%s.IMMSNList SET ModbusClientStatus = %d,ModbusClientPID=%d, IMMLastUpdateTime=NOW(6) WHERE IMMSN=%d",
				 SYS_NAME, DATA_DATABASE_NAME, MODBUS_CLIENT_STATUS_UNKNOW, intModbusClientPID, intIMMSN);
		intRetval = mysql_query(&mysqlCon, charStatement);
		if (intRetval != EXIT_SUCCESS)
		{
			snprintf(charErrMsg, LONG_STRING_SIZE,
					 "[SubscribeModbus]Fail to update ModbusClientStatus = MODBUS_CLIENT_STATUS_UNKNOW, ModbusClientPID = %d and IMMLastUpdateTime = NOW(6) where IMMSN=%d (%d):%s",
					 intModbusClientPID, intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
			intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
			return intLogSN;
		}

		// modbus_free(modbusCon);
		// mysql_close(&mysqlCon);
		boolIsStopSignal = true;
		return intLogSN;
	}

	// Set Modbus Device ID
	modbus_set_slave(modbusCon, MODBUS_DEVICE_ID);

	// Set Modbus Debug Mode
	modbus_set_debug(modbusCon, 0);

	// SetModbus Timeout
	stModbusTimeOut.tv_sec = MODBUS_TIMEOUT_SEC;
	stModbusTimeOut.tv_usec = 0;
	modbus_get_byte_timeout(modbusCon, &stModbusTimeOut);
	stModbusTimeOut.tv_sec = MODBUS_TIMEOUT_SEC;
	stModbusTimeOut.tv_usec = 0;
	modbus_set_response_timeout(modbusCon, &stModbusTimeOut);

	// Connect to DAQ SD
	if (intRetval = modbus_connect(modbusCon) == -1)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to connect DAQ SD for IMMSN:%d (%s)", intIMMSN, modbus_strerror(intRetval != EXIT_SUCCESS));
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
		boolIsStopSignal = true;
		// modbus_free(modbusCon);
		// mysql_close(&mysqlCon);
		// return intLogSN;
	}

	// Get Type Code and Range of Each DAQ Channel
	intDAQChannelAmount = modbus_read_registers(modbusCon, COMMAND_AI_TYPECODE_ADDRESS, COMMAND_AI_TYPECODE_AMOUNT, intDAQTypeCode);
	if (intDAQChannelAmount <= 0)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to get range of each channel for DAQ of IMMSN:%d (%s)", intIMMSN, modbus_strerror(intRetval != EXIT_SUCCESS));
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
		boolIsStopSignal = true;
		// return intLogSN;
	}

	// Change ModbusClientStatus = RUNNING After Connect to DAQ Device
	/*
	snprintf(charStatement, MAX_STRING_SIZE,
			 "UPDATE %s_%s.IMMSNList SET ModbusClientStatus = %d, IMMLastUpdateTime = NOW(6) WHERE IMMSN = %d",
			 SYS_NAME, DATA_DATABASE_NAME, MODBUS_CLIENT_STATUS_RUNNING, intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE,
				 "[SubscribeModbus]Fail to update ModbusClientStatus = RUNNING and IMMLastUpdateTime = NOW(6) where IMMSN=%d (%d):%s",
				 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return intLogSN;
	}*/

	intDAQBound = (int *)malloc(intDAQChannelAmount * 2 * sizeof(int));
	for (int i = 0; i < intDAQChannelAmount && boolIsStopSignal != true; i++)
	{
		intRetval = Modbus_GetRangebyTypeCode(intDAQTypeCode[i], intDAQBound + i * 2, intDAQBound + i * 2 + 1);
		if (intRetval != EXIT_SUCCESS)
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to get Max/Min range of DAQ for TypeCode:%x", intDAQTypeCode[i]);
			intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
			boolIsStopSignal = true;
			// return intLogSN;
		}
		else
			fprintf(stderr, "[SubscribeModbus]Channel:%d TypeCode:%x Maxmal Voltage:%d Minimal Voltage:%d\n", i, intDAQTypeCode[i], *(intDAQBound + i * 2 + 1), *(intDAQBound + i * 2));
	}

	if (boolIsStopSignal != true)
	{
		pthread_create(&threadWriteSensorCachetoDatabase, NULL, Modbus_WriteSensorCachetoDatabase, (void *)&varSensorCacheData);
		fprintf(stderr, "[SubscribeModbus]Modbus_WriteSensorCachetoDatabase has been activated. Modbus now is ready\n");

		// Update ModbusClientStatus = MODBUS_CLIENT_STATUS_WAITING in IMMSNList
		snprintf(charStatement, MAX_STRING_SIZE, "UPDATE %s_%s.IMMSNList SET ModbusClientStatus = %d ,IMMLastUpdateTime = NOW(6) WHERE IMMSN=%d",
				 SYS_NAME, DATA_DATABASE_NAME, MODBUS_CLIENT_STATUS_WAITING, intIMMSN);
		intRetval = mysql_query(&mysqlCon, charStatement);
		if (intRetval != EXIT_SUCCESS)
		{
			snprintf(charErrMsg, LONG_STRING_SIZE,
					 "[SubscribeModbus]Fail to update ModbusClientStatus = MODBUS_CLIENT_STATUS_WAITING and IMMLastUpdateTime = NOW(6) where IMMSN = %d (%d):%s",
					 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
			intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
			return intLogSN;
		}
	}
	// Close MySQL Connection Before Main Loop
	mysql_close(&mysqlCon);

	// Main Loop IMM (ModbusMoldSignalSource=Modbus)
	while (boolIsStopSignal == false && intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_MODBUS)
	{
		// fprintf(stderr,"[SubscribeModbus]Subscribe Modbus on an IMM without OPC UA...\n");
		// Get Current Mold Status
		intRetval = Modbus_GetMoldStatus(modbusCon, intModbusMoldSignalType, intPreviousMoldStatus, &intCurrentMoldStatus);
		if (intRetval != EXIT_SUCCESS)
		{

			snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to get mold status ofIMMSN:%d\n", intIMMSN);
			intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);

			boolIsStopSignal = true;
			continue;
			// return intLogSN;
			// If Mold Status is Changed
		}
		else if (intCurrentMoldStatus != intPreviousMoldStatus)
		{
			// fprintf(stderr, "MoldStatus:%d->%d\n", intPreviousMoldStatus, intCurrentMoldStatus);

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

			// Mold Released
			if (intCurrentMoldStatus == MOLD_STATUS_RELEASED)
			{
				fprintf(stderr, "[SubscribeModbus]IMMSN:%d MoldStatus:Released\n", intIMMSN);
				doubleElapsedTime = 0;
				if (boolHasMoldStatusReady == false)

				{
					boolHasMoldStatusReady = true;
					// Update ModbusClientStatus = MODBUS_CLIENT_STATUS_RUNNING in IMMSNList
					snprintf(charStatement, MAX_STRING_SIZE, "UPDATE %s_%s.IMMSNList SET ModbusClientStatus = %d ,IMMLastUpdateTime = NOW(6) WHERE IMMSN=%d",
							 SYS_NAME, DATA_DATABASE_NAME, MODBUS_CLIENT_STATUS_RUNNING, intIMMSN);
					intRetval = mysql_query(&mysqlCon, charStatement);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "[SubscribeModbus]Fail to update ModbusClientStatus = RUNNING and IMMLastUpdateTime = NOW(6) where IMMSN = %d (%d):%s",
								 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
						return intLogSN;
					}
				}

				// Set OPC UA Node Value : Mold Released
				OPCUA_WriteOPCUANodeValue(mysqlCon, VIRTUAL_OPCUA_SERVER_IMMSN, DO_MOLD_CLAMPED, "0");
				OPCUA_WriteOPCUANodeValue(mysqlCon, VIRTUAL_OPCUA_SERVER_IMMSN, DO_MOLD_RELEASED, "1");

				// If ModBus Mold Signal Is Modbus, Then Record The Mold Status
				if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_MODBUS)
				{
					// Update DB MoldStatus As Released
					intRetval = IMM_SetIMMMoldStatus(mysqlCon, intIMMSN, MOLD_STATUS_RELEASED);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to update MoldStatus:Released for IMMSN:%d in table %s_%s.IMMSNList",
								 intIMMSN, SYS_NAME, DATA_DATABASE_NAME);
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
					}
				}
				// If This Shot Follows Regular Steps
				if (boolHasThisShotReleased == true &&
					boolHasThisShotClamping == true &&
					boolHasThisShotClamped == true &&
					boolHasThisShotReleasing == true)
				{
					// Update ShotSN
					// intRetval = DB_SelectShotSNbyMOSN(mysqlCon, intMOSN, &intShotSN);
					// if (intRetval != EXIT_SUCCESS)
					//{
					//	fprintf(stderr, "[SubscribeModbus]Fail to select ShotSN for IMMSN:%d MOSN:%d\n", intIMMSN, intMOSN);
					// }

					snprintf(charStatement, MAX_STRING_SIZE,
							 "UPDATE %s_%s_%s_%d_RawData.ShotSNList SET "
							 "MoldReleasedTime = NOW(6),"
							 "CycleTime=TIMESTAMPDIFF(Microsecond,MoldClampingTime,NOW(6))/1000000, "
							 "MoldBasedCycleTime=TIMESTAMPDIFF(Microsecond,MoldClampingTime,NOW(6))/1000000, "
							 "ErrorShot = %s WHERE ShotSN = %d",
							 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
							 "CASE WHEN MoldClampingTime IS NULL OR MoldClampedTime IS NULL OR MoldReleasingTime IS NULL OR MoldReleasedTime IS NULL THEN 1 ELSE 0 END",
							 intShotSN);
					// fprintf(stderr, "Q:%s\n", charStatement);
					intRetval = mysql_query(&mysqlCon, charStatement);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "[SubscribeModbus]Fail to update cycle time for ShotSN:%d while mold released to table %s_%s_%s_%d_RawData.ShotSNList (%d): %s",
								 intShotSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
						return EXIT_FAILURE;
					}

					// Update the New Shot to INJPRO_DATA_MO_[MOSN]_RawData.ShotSNList
					snprintf(charStatement, MAX_STRING_SIZE,
							 "UPDATE %s_%s_%s_%d_RawData.ShotSNList SET "
							 "MoldShotInterval=IF(%d = 1 , 0, ("
							 "SELECT TIMESTAMPDIFF(MICROSECOND,MIN(LastTwoShot.MoldReleasedTime),MAX(LastTwoShot.MoldClampingTime))/1000000 "
							 "FROM(SELECT ShotSN,MoldClampingTime,MoldReleasedTime FROM %s_%s_%s_%d_RawData.ShotSNList "
							 "WHERE ShotSN <= %d ORDER BY ShotSN DESC  LIMIT 2)AS LastTwoShot)),"
							 "MoldCompleteCycleTime = MoldBasedCycleTime + MoldShotInterval WHERE ShotSN = %d",
							 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
							 intShotSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, intShotSN, intShotSN);
					// fprintf(stderr, "Q:%s\n", charStatement);
					intRetval = mysql_query(&mysqlCon, charStatement);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "[SubscribeModbus]Fail to update shot interval for ShotSN:%d while mold released to table %s_%s_%s_%d_RawData.ShotSNList (%d): %s",
								 intShotSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
					}

					//Update Mold Action Feature Values
					intRetval = DB_InsertMoldActionFeatureValue(mysqlCon, intMOSN, intShotSN);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "[SubscribeModbus]Fail to update mold action feature vlaues for ShotSN:%d of MOSN:%d (%d): %s",
								 intShotSN, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
					}

					// Update ShotSN=ShotSN+1 in INJPRO.Data.IMMSNList
					snprintf(charStatement, MAX_STRING_SIZE,
							 "UPDATE %s_%s.IMMSNList SET ShotSN=%d, IMMLastUpdateTime=NOW(6) WHERE IMMSN=%d",
							 SYS_NAME, DATA_DATABASE_NAME, ++intShotSN, intIMMSN);

					// fprintf(stderr, "Released Final ShotSN:%d\n", intShotSN);
					intRetval = mysql_query(&mysqlCon, charStatement);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to update ShotSN in table %s_%s.IMMSNList (%d): %s",
								 SYS_NAME, DATA_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
						//return EXIT_FAILURE;
					}
					else
					{
						// Update VIRTUAL OPCUA Server
						intRetval = MO_UpdateVirtualOPCUAServer(mysqlCon, intMOSN, intShotSN, -1);
						if (intRetval != EXIT_SUCCESS)
						{
							snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to update info to virtual OPCUA Server(IMM.RealTimeShotSN).\n");
							SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
						}
					}

					boolHasThisShotReleased = false;
				}
				boolHasThisShotReleased = true;
				boolHasThisShotClamping = false;
				boolHasThisShotClamped = false;
				boolHasThisShotReleasing = false;
				boolHasElapsedTimeStarted = false;
			}
			else if (intCurrentMoldStatus == MOLD_STATUS_CLAMPING)
			// Mold Clamping
			{
				fprintf(stderr, "[SubscribeModbus]IMMSN:%d MoldStatus:Clamping\n", intIMMSN);

				// Set OPC UA Node Value : Mold Clamping
				OPCUA_WriteOPCUANodeValue(mysqlCon, VIRTUAL_OPCUA_SERVER_IMMSN, DO_MOLD_CLAMPED, "0");
				OPCUA_WriteOPCUANodeValue(mysqlCon, VIRTUAL_OPCUA_SERVER_IMMSN, DO_MOLD_RELEASED, "0");

				// Set boolHasThisShotClamping = true
				boolHasThisShotClamping = true;

				// If ModBus Mold Signal Is Modbus, Then Record The Mold Status
				if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_MODBUS)
				{
					// Update DB MoldStatus As Clamping
					intRetval = IMM_SetIMMMoldStatus(mysqlCon, intIMMSN, MOLD_STATUS_CLAMPING);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to update MoldStatus:Clamping for IMMSN:%d in table %s_%s.IMMSNList",
								 intIMMSN, SYS_NAME, DATA_DATABASE_NAME);
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
						return EXIT_FAILURE;
					}
				}

				// If This Shot Follows Regular Steps
				if (boolHasThisShotReleased == true &&
					boolHasThisShotClamping == true &&
					boolHasThisShotClamped == false &&
					boolHasThisShotReleasing == false)
				{
					intRetval = DB_SelectCurrentRoundSN(mysqlCon, intIMMSN, &intRoundSN);
					// fprintf(stderr, "RoundSN:%d\n", intRoundSN);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to select RoundSN for IMMSN:%d in table %s_%s.IMMSNList",
								 intIMMSN, SYS_NAME, DATA_DATABASE_NAME);
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, 0, charErrMsg);
					}

					snprintf(charStatement, MAX_STRING_SIZE,
							 "INSERT INTO %s_%s_%s_%d_RawData.ShotSNList "
							 "(ShotSN,	RoundSN,	MoldClampingTime,	IMMParaSN,	MOCustomSensorTableSN,	IMMParaUnstable,	ErrorShot) VALUE "
							 "(%d,		%d,			NOW(6),				NULL,		%d,						NULL,				%d)",
							 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
							 intShotSN, intRoundSN,
							 (intShotSN - 1) / SHOT_NUM_PER_TABLE + 1,
							 ERROR_SHOT_IMCOMPLETE);

					intRetval = mysql_query(&mysqlCon, charStatement);
					// fprintf(stderr,"Q:%s\n",charStatement);
					if (intRetval != EXIT_SUCCESS)
					{
						// fprintf(stderr,"Q:%s\n",charStatement);
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "[SubscribeModbus]Fail to insert ShotSN to table %s_%s_%s_%d_RawData.ShotSNList (%d): %s",
								 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
						// return EXIT_FAILURE;
					}

					// Insert Default MOAcceptCriteria1 As NULL
					intRetval = MO_InsertMOAcceptCriteriaActualValue(mysqlCon, intMOSN, intShotSN, 1, "0");
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, MAX_STRING_SIZE,
								 "[SubscribeOPCUA]Fail to insert accept criteria actual value of ShotSN:%d MOAcceptCriteriaSN:1 into "
								 "table %s_%s_%s_%d_RawData_AcceptCriteria.MOAcceptCriteriaSN1 (%d):%s",
								 intShotSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
					}

					intRetval = MO_InsertMOAcceptCriteriaPredictValue(mysqlCon, intMOSN, intShotSN, 1, "0");
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, MAX_STRING_SIZE,
								 "[SubscribeOPCUA]Fail to insert accept criteria predict value of ShotSN:%d MOAcceptCriteriaSN:1 into "
								 "table %s_%s_%s_%d_RawData_AcceptCriteria.MOAcceptCriteriaSN1 (%d):%s",
								 intShotSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
					}

					// Insert Default Predict/Actual Bool of MOAcceptCriteria1 As NULL
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, MAX_STRING_SIZE,
								 "[SubscribeModbus]Fail to insert accept criteria actual value of ShotSN:%d MOAcceptCriteriaSN:1 into "
								 "table %s_%s_%s_%d_RawData_AcceptCriteria.MOAcceptCriteriaSN1 (%d):%s",
								 intShotSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
						// return EXIT_FAILURE;
					}
				}
			}

			else if (intCurrentMoldStatus == MOLD_STATUS_CLAMPED) // Mold Clamped
			{
				fprintf(stderr, "[SubscribeModbus]IMMSN:%d MoldStatus:Clamped\n", intIMMSN);

				// Set OPC UA Node Value : Mold Clamped
				OPCUA_WriteOPCUANodeValue(mysqlCon, VIRTUAL_OPCUA_SERVER_IMMSN, DO_MOLD_RELEASED, "0");
				OPCUA_WriteOPCUANodeValue(mysqlCon, VIRTUAL_OPCUA_SERVER_IMMSN, DO_MOLD_CLAMPED, "1");

				// Set boolHasThisShotClamped = true
				boolHasThisShotClamped = true;

				// If ModBus Mold Signal Is Modbus, Then Record The Mold Status
				if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_MODBUS)
				{
					// Update DB MoldStatus As Clamped
					intRetval = IMM_SetIMMMoldStatus(mysqlCon, intIMMSN, MOLD_STATUS_CLAMPED);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to update MoldStatus:Clamped for IMMSN:%d in table %s_%s.IMMSNList",
								 intIMMSN, SYS_NAME, DATA_DATABASE_NAME);
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
						return EXIT_FAILURE;
					}
				}

				// If This Shot Follows Regular Steps
				if (boolHasThisShotReleased == true &&
					boolHasThisShotClamping == true &&
					boolHasThisShotClamped == true &&
					boolHasThisShotReleasing == false)
				{
					// Update ClampedTime to INJPRO_DATA_MO_[MOSN]_RawData.ShotSNList
					snprintf(charStatement, MAX_STRING_SIZE,
							 "UPDATE %s_%s_%s_%d_RawData.ShotSNList SET MoldClampedTime = NOW(6) WHERE ShotSN = %d",
							 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, intShotSN);
					intRetval = mysql_query(&mysqlCon, charStatement);
					if (intRetval)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "[SubscribeModbus]Fail to update MoldClampedTime of ShotSN:%d while mold clamped to table %s_%s_%s_%d_RawData.ShotSNList (%d): %s",
								 intShotSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
						return EXIT_FAILURE;
					}

					// Start Mold Clamped Timer
					if (boolHasElapsedTimeStarted == false)
					{
						// Set Clamping  ElapsedTime Stamp
						clock_gettime(CLOCK_REALTIME, &stClampedTimeStamp);
						boolHasElapsedTimeStarted = true;
					}
				}
			}
			else if (intCurrentMoldStatus == MOLD_STATUS_RELEASING) // Mold Releasing

			{
				fprintf(stderr, "[SubscribeModbus]IMMSN:%d MoldStatus:Releasing\n", intIMMSN);

				// Set OPC UA Node Value : Mold Releasing
				OPCUA_WriteOPCUANodeValue(mysqlCon, VIRTUAL_OPCUA_SERVER_IMMSN, DO_MOLD_CLAMPED, "0");
				OPCUA_WriteOPCUANodeValue(mysqlCon, VIRTUAL_OPCUA_SERVER_IMMSN, DO_MOLD_RELEASED, "0");

				// Set boolHasThisShotReleasing = true
				boolHasThisShotReleasing = true;

				// If ModBus Mold Signal Is Modbus, Then Record The Mold Status
				if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_MODBUS)
				{
					// Update DB MoldStatus As Releasing
					intRetval = IMM_SetIMMMoldStatus(mysqlCon, intIMMSN, MOLD_STATUS_RELEASING);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to update MoldStatus:Releasing for IMMSN:%d in table %s_%s.IMMSNList",
								 intIMMSN, SYS_NAME, DATA_DATABASE_NAME);
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
					}
				}

				// If This Shot Follows Regular Steps
				if (boolHasThisShotReleased == true &&
					boolHasThisShotClamping == true &&
					boolHasThisShotClamped == true &&
					boolHasThisShotReleasing == true)

				{
					// Update ClampedTime to INJPRO_DATA_MO_[MOSN]_RawData.ShotSNList
					snprintf(charStatement, MAX_STRING_SIZE,
							 "UPDATE %s_%s_%s_%d_RawData.ShotSNList SET "
							 "MoldClampedCycleTime = TIMESTAMPDIFF(Microsecond,MoldClampedTime,NOW(6))/1000000,"
							 "MoldReleasingTime = NOW(6) WHERE ShotSN = %d",
							 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, intShotSN);
					intRetval = mysql_query(&mysqlCon, charStatement);
					if (intRetval)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "[SubscribeModbus]Fail to update ClampedTime of ShotSN:%d while mold clamped to table %s_%s_%s_%d_RawData.ShotSNList (%d): %s",
								 intShotSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
						// return EXIT_FAILURE;
					}
				}
			}
			else if (intCurrentMoldStatus == MOLD_STATUS_UNKNOW) // Mold Status Unknow
			{
				fprintf(stderr, "[SubscribeModbus]IMMSN:%d MoldStatus:Unknow\n", intIMMSN);

				// If ModBus Mold Signal Is Modbus, Then Record The Mold Status
				if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_MODBUS)
				{
					// Update DB MoldStatus As Clamped
					intRetval = IMM_SetIMMMoldStatus(mysqlCon, intIMMSN, MOLD_STATUS_UNKNOW);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to update MoldStatus:Unknow for IMMSN:%d in table %s_%s.IMMSNList",
								 intIMMSN, SYS_NAME, DATA_DATABASE_NAME);
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
						// return EXIT_FAILURE;
					}
				}
				usleep(MODBUS_SAMPLE_RATE);
			}
			intPreviousMoldStatus = intCurrentMoldStatus;
			// Close MySQL Connection After Write OPC UA Node
			mysql_close(&mysqlCon);
		}

		// Write Mold Sensor Data to Database
		if (intCurrentMoldStatus == MOLD_STATUS_CLAMPED &&
			boolHasThisShotReleased == true &&
			boolHasThisShotClamping == true &&
			boolHasThisShotClamped == true &&
			boolHasThisShotReleasing == false &&
			doubleElapsedTime <= MODBUS_MAX_CYCLE_TIME)
		{
			// fprintf(stderr, "[SubscribeModbus]IMMSN:%d MoldStatus:Clamped\n", intIMMSN);

			// Get Timestamp before Fetch Rawdata
			clock_gettime(CLOCK_REALTIME, &stConnectTimeStamp);
			boolHasThisShotExceedMaxCycleTime = false;
			do
			{
				// fprintf(stderr, "[SubscribeModbus]Get RawData from DAQ.\n");
				// Get Rawdata from DAQ via Modbus command
				intDAQRawDataSize = modbus_read_registers(modbusCon, COMMAND_AI_VALUE_ADDRESS, COMMAND_AI_VALUE_AMOUNT, intDAQRawData);

				if (intDAQRawDataSize < 0)
				{
					// Connect to MYSQL Server
					mysql_init(&mysqlCon);
					if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
					{
						fprintf(stderr, "[SubscribeModbus]MYSQL connection failed.\n");
						if (mysql_errno(&mysqlCon))
						{
							fprintf(stderr, "[SubscribeModbus]MYSQL connection error %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						}
						return EXIT_FAILURE;
					}
					snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to get rawdata from DAQ of IMMSN:%d. Reconnect.", intIMMSN);
					intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);

					// Close MySQL Connection After Write Log
					mysql_close(&mysqlCon);

					modbus_close(modbusCon);
					usleep(MODBUS_SAMPLE_RATE);
					modbus_connect(modbusCon);
					// If the Rawdata Is not Completely Fetched
				}
				else if (intDAQRawDataSize != COMMAND_AI_VALUE_AMOUNT)
				{
					// Connect to MYSQL Server
					mysql_init(&mysqlCon);
					if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
					{
						fprintf(stderr, "[SubscribeModbus]MYSQL connection failed.\n");
						if (mysql_errno(&mysqlCon))
						{
							fprintf(stderr, "[SubscribeModbus]MYSQL connection error %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
						}
						return EXIT_FAILURE;
					}
					snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]DAQ size %d error (expected %d) IMMSN:%d.", intDAQRawDataSize, COMMAND_AI_VALUE_AMOUNT, intIMMSN);
					intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
					// Close MySQL Connection After Write Log
					mysql_close(&mysqlCon);
				}
				Modbus_GetElapsedTime(stConnectTimeStamp, &doubleConnectElapsedTime);
			} while ((intDAQRawDataSize != COMMAND_AI_VALUE_AMOUNT || modbusCon == NULL) && doubleConnectElapsedTime < MODBUS_CONNECT_INTEVAL);

			// fprintf(stderr, "[SubscribeModbus]Finishing Getting RawData.\n");

			// Terminate if Re-Connect > MODBUS_CONNECT_INTEVAL
			if (doubleConnectElapsedTime >= MODBUS_CONNECT_INTEVAL)
			{
				// Connect to MYSQL Server
				mysql_init(&mysqlCon);
				if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
				{
					fprintf(stderr, "[SubscribeModbus]MYSQL connection failed.\n");
					if (mysql_errno(&mysqlCon))
					{
						fprintf(stderr, "[SubscribeModbus]MYSQL connection error %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
					}
					return EXIT_FAILURE;
				}
				snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]DAQ size %d error (expected %d) IMMSN:%d ElapsedTime:%lf",
						 intDAQRawDataSize, COMMAND_AI_VALUE_AMOUNT, intIMMSN, doubleConnectElapsedTime);
				intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
				// Close MySQL Connection After Write Log
				mysql_close(&mysqlCon);
				return EXIT_FAILURE;
			}

			// fprintf(stderr, "[SubscribeModbus]DAQ Alive.\n");
			// Get Elapsed Time
			Modbus_GetElapsedTime(stClampedTimeStamp, &doubleElapsedTime);
			// fprintf(stderr, "DAQSize:%d ElapsedTime:%lf\n", intDAQRawDataSize, doubleElapsedTime);

			// Convert RawData to Voltage
			for (int i = 0; i < intDAQRawDataSize; i++)
			{
				// fprintf(stderr, "[SubscribeModbus]intDAQRawDataSize:%d\n", intDAQRawDataSize);
				// If the Channel is Enable
				if (intDAQChanneltoMOCustomSensorSN[i] != 0)
				{
					Modbus_ConvertRawdata(intDAQRawData[i], *(intDAQBound + i * 2), *(intDAQBound + i * 2 + 1), &doubleConvertedDAQRawData);

#ifdef DEBUG_MODE_SUBSCRIBE_MODBUS
					fprintf(stderr,
							"[SubscribeModbus]ShotSN:%d Channel:%d MOCustomSensorSN:%d ElapsedTime:%lf Voltage:%u Converted:%lf\n",
							intShotSN, i, intDAQChanneltoMOCustomSensorSN[i], doubleElapsedTime, intDAQRawData[i], doubleConvertedDAQRawData);
#endif

					// Write Converted RawData to Cache
					varSensorCacheData.intShotSNCacheData[i][varSensorCacheData.intCacheHeadIndex] = intShotSN;
					varSensorCacheData.doubleSensorValueCacheData[i][varSensorCacheData.intCacheHeadIndex] = doubleConvertedDAQRawData;
					varSensorCacheData.doubleElapsedTimeCacheData[i][varSensorCacheData.intCacheHeadIndex] = doubleElapsedTime;
				}
			}

			// fprintf(stderr, "[SubscribeModbus]Before intCacheHeadIndex++\n");
			// intCacheHeadIndex++
			if ((varSensorCacheData.intCacheHeadIndex + 1 == MODBUS_MAX_CACHE_SIZE ? 0 : varSensorCacheData.intCacheHeadIndex + 1) == varSensorCacheData.intCacheTailIndex)
			{
				fprintf(stderr, "[SubscribeModbus]Sensor data cache full!\n");

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
				snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]IMMSN:%d ShotSN:%d Sensor data cache full!", intIMMSN, intShotSN);
				intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
				// Close MySQL Connection After Write Log
				mysql_close(&mysqlCon);
			}
			else
			{
				varSensorCacheData.intCacheHeadIndex = varSensorCacheData.intCacheHeadIndex + 1 == MODBUS_MAX_CACHE_SIZE ? 0 : varSensorCacheData.intCacheHeadIndex + 1;
			}
			// fprintf(stderr, "[SubscribeModbus]CacheHeadIndex += 1 Finished.\n");
			usleep(MODBUS_SAMPLE_RATE);
		}
		else if (doubleElapsedTime > MODBUS_MAX_CYCLE_TIME && boolHasThisShotExceedMaxCycleTime == false)
		{
			boolHasThisShotExceedMaxCycleTime = true;

			// Update the New Shot to INJPRO_DATA_MO_[MOSN]_RawData.ShotSNList
			snprintf(charStatement, MAX_STRING_SIZE,
					 "UPDATE %s_%s_%s_%d_RawData.ShotSNList SET ErrorShot = %d WHERE ShotSN = %d",
					 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, ERROR_SHOT_EXCEED_MAX_CYCLE_TIME, intShotSN);
			// fprintf(stderr, "Q:%s\n", charStatement);
			intRetval = mysql_query(&mysqlCon, charStatement);
			if (intRetval)
			{
				snprintf(charErrMsg, LONG_STRING_SIZE,
						 "[SubscribeModbus]Fail to update ErrorShot = 1 ShotSN:%d while ElapsedTime > %d to table %s_%s_%s_%d_RawData.ShotSNList (%d): %s",
						 intShotSN, MODBUS_MAX_CYCLE_TIME, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
				// return;
			}
		}
		// else
		//{
		//	fprintf(stderr, "[SubscribeModbus]Skip Insert MoldStatus:%d HasReleased:%d HasClamping:%d HasClamped:%d Has Releasing:%d ElapsedTime:%d\n",
		//		intCurrentMoldStatus, boolHasThisShotReleased, boolHasThisShotClamping, boolHasThisShotClamped, boolHasThisShotReleasing, doubleElapsedTime);
		// }
	} // End Main Loop

	// IMM + OPC UA
	if (intOPCUAVersionSN != VIRTUAL_OPCUA_SERVER_OPCUA_VERSIONSN && intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_OPCUA)
	{
		fprintf(stderr, "[SubscribeModbus]Subscribe Modbus on an IMM with OPC UA...\n");

		// Config the OPC UA Connector
		config = UA_ClientConfig_default;
		config.stateCallback = Modbus_CallbackState;
		config.subscriptionInactivityCallback = Modbus_CallbackSubscriptionInactivity;
		UA_Client *OPCUAClient = UA_Client_new(config);
		snprintf(charEndPointURL, MEDIUM_STRING_SIZE, "opc.tcp://%s:%d", charOPCUAIP, intOPCUAPort);
		fprintf(stderr, "[SubscribeModbus]Connect via OPC UA to %s\n", charEndPointURL);

		// Main Loop
		while (boolIsStopSignal == false)
		{

			// Connect to OPC UA Server
			clock_gettime(CLOCK_REALTIME, &stConnectTimeStamp);
			do
			{
				intRetval = UA_Client_connect(OPCUAClient, charEndPointURL);
				Modbus_GetElapsedTime(stConnectTimeStamp, &doubleConnectElapsedTime);
				if (intRetval < 0)
					usleep(MODBUS_SAMPLE_RATE);
			} while (intRetval < 0 && doubleConnectElapsedTime < MODBUS_CONNECT_INTEVAL);

			// If Connecting Time Is Too Long
			if (doubleConnectElapsedTime > MODBUS_CONNECT_INTEVAL)
			{
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
				snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to get mold status of IMMSN:%d", intIMMSN);
				intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
				// Close MySQL Connection After Write Log
				mysql_close(&mysqlCon);
				boolIsStopSignal = true;
			}

			if (intCurrentMoldStatus == MOLD_STATUS_RELEASED)
			{
				if (boolHasThisShotClamped == true)
				{
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

					// Update ShotSN
					intRetval = DB_SelectShotSNbyMOSN(mysqlCon, intMOSN, &intShotSN);
					if (intRetval != EXIT_SUCCESS)
					{
						fprintf(stderr, "[SubscribeModbus]Fail to select ShotSN by MOSN:%d for IMMSN:%d\n", intMOSN, intIMMSN);
					}

					// Close MySQL Connection After Write Log
					mysql_close(&mysqlCon);
					boolHasThisShotClamped = false;
				}
			}
			else if (intCurrentMoldStatus == MOLD_STATUS_CLAMPED)
			{
				boolHasThisShotClamped = true;

				// Get Timestamp before Fetch Rawdata
				clock_gettime(CLOCK_REALTIME, &stConnectTimeStamp);

				do
				{
					// Get Rawdata from DAQ via Modbus command
					intDAQRawDataSize = modbus_read_registers(modbusCon, COMMAND_AI_VALUE_ADDRESS, COMMAND_AI_VALUE_AMOUNT, intDAQRawData);
					if (intDAQRawDataSize < 0)
					{
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
						snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to get rawdata from DAQ of IMMSN:%d. Reconnect.", intIMMSN);
						intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
						// Close MySQL Connection After Write Log
						mysql_close(&mysqlCon);
						snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Closing Modbus\n");
						modbus_close(modbusCon);
						usleep(MODBUS_SAMPLE_RATE);
						snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Modbus Closed and Reconnecting...\n");
						intRetval = modbus_connect(modbusCon);
						snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Modbus Reconnected\n");
						// If the Rawdata Is not Completely Fetched
					}
					else if (intDAQRawDataSize > 0 && intDAQRawDataSize != COMMAND_AI_VALUE_AMOUNT)
					{
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
						snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]DAQ size %d error (expected %d) IMMSN:%d.", intDAQRawDataSize, COMMAND_AI_VALUE_AMOUNT, intIMMSN);
						intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
						// Close MySQL Connection After Write Log
						mysql_close(&mysqlCon);
						Modbus_GetElapsedTime(stConnectTimeStamp, &doubleConnectElapsedTime);
						fprintf(stderr, "Reconnect Elapsed Time :%lf\n", doubleElapsedTime);
					}
				} while ((intDAQRawDataSize != COMMAND_AI_VALUE_AMOUNT || modbusCon == NULL) && doubleConnectElapsedTime < MODBUS_CONNECT_INTEVAL);

				// Terminate if Re-Connect > MODBUS_CONNECT_INTEVAL
				if (intDAQRawDataSize != COMMAND_AI_VALUE_AMOUNT)
				{
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
					snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]DAQ size %d error (expected %d) IMMSN:%d ElapsedTime:%lf",
							 intDAQRawDataSize, COMMAND_AI_VALUE_AMOUNT, intIMMSN, doubleConnectElapsedTime);
					intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_MODBUS, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
					// Close MySQL Connection After Write Log
					mysql_close(&mysqlCon);
					return EXIT_FAILURE;
				}

				// Get Elapsed Time
				Modbus_GetElapsedTime(stClampedTimeStamp, &doubleElapsedTime);
				// fprintf(stderr,"DAQSize:%d ElapsedTime:%lf\n",intDAQRawDataSize,doubleElapsedTime);

				// Convert RawData to Voltage
				for (int i = 0; i < intDAQRawDataSize; i++)
				{
					// If the Channel is Enable
					if (intDAQChanneltoMOCustomSensorSN[i] != 0)
					{
						Modbus_ConvertRawdata(intDAQRawData[i], *(intDAQBound + i * 2), *(intDAQBound + i * 2 + 1), &doubleConvertedDAQRawData);

#ifdef DEBUG_MODE_SUBSCRIBE_MODBUS
						fprintf(stderr,
								"[SubscribeModbus]ShotSN:%d Channel:%d MOCustomSensorSN:%d ElapsedTime:%lf Voltage:%u Converted:%lf\n",
								intShotSN, i, intDAQChanneltoMOCustomSensorSN[i], doubleElapsedTime, intDAQRawData[i], doubleConvertedDAQRawData);
#endif

						/*
						if(intDAQChanneltoMOCustomSensorSN[i]>=1 && intDAQChanneltoMOCustomSensorSN[i]<=3){
							doubleConvertedDAQRawData*=5.0;
						}*/

						// Write Converted RawData to Cache
						varSensorCacheData.intShotSNCacheData[i][varSensorCacheData.intCacheHeadIndex] = intShotSN;
						varSensorCacheData.doubleSensorValueCacheData[i][varSensorCacheData.intCacheHeadIndex] = doubleConvertedDAQRawData;
						varSensorCacheData.doubleElapsedTimeCacheData[i][varSensorCacheData.intCacheHeadIndex] = doubleElapsedTime;

						fprintf(stderr, "[SubscribeModbus]Write Converted RawData to Cache Finished.\n");
					}
				}

				// intCacheHeadIndex++
				if ((varSensorCacheData.intCacheHeadIndex + 1 == MODBUS_MAX_CACHE_SIZE ? 0 : varSensorCacheData.intCacheHeadIndex + 1) == varSensorCacheData.intCacheTailIndex)
				{
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
					snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]IMMSN:%d ShotSN:%d Sensor data cache full!", intIMMSN, intShotSN);
					intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_MODBUS, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
					// Close MySQL Connection After Write Log
					mysql_close(&mysqlCon);
				}
				else
				{
					varSensorCacheData.intCacheHeadIndex = varSensorCacheData.intCacheHeadIndex + 1 == MODBUS_MAX_CACHE_SIZE ? 0 : varSensorCacheData.intCacheHeadIndex + 1;
				}
				usleep(MODBUS_SAMPLE_RATE);
			}
			UA_Client_runAsync(OPCUAClient, UA_CLIENT_INTERVAL);
		}
		// Close
		UA_Client_delete(OPCUAClient);
	} // End if Mold signal type is from OPC UA

	/*
	else{
		snprintf(charErrMsg,LONG_STRING_SIZE,"[SubscribeModbus]Incorrect mold signal type (%d) is given for IMMSN:%d",intModbusMoldSignalType,intIMMSN);
		intLogSN=SYS_InsertSysErrMsg(mysqlCon,ERRCLASS_IMM,intMOSN,intIMMSN,EXIT_FAILURE,charErrMsg);
		modbus_free(modbusCon);
		mysql_close(&mysqlCon);
		return intLogSN;
	}*/

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

	// Get Current ModbusClientPID
	snprintf(charStatement, MAX_STRING_SIZE, "SELECT ModbusClientPID FROM %s_%s.IMMSNList WHERE IMMSN=%d",
			 SYS_NAME, DATA_DATABASE_NAME, intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to select ModbusClientPID for IMMSN=%d while exiting (%d):%s",
				 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_MODBUS, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
	}

	mysqlResult = mysql_store_result(&mysqlCon);
	mysqlRow = mysql_fetch_row(mysqlResult);

	// Wait the thread
	pthread_join(threadWriteSensorCachetoDatabase, NULL);

	// Close Modbus Connection
	modbus_free(modbusCon);

	// Check Current SubscribeIMMPID == getpid()
	if (mysqlRow[0] != NULL && atoi(mysqlRow[0]) == getpid())
	{
		snprintf(charStatement, MAX_STRING_SIZE, "UPDATE %s_%s.IMMSNList SET ModbusClientStatus = %d, ModbusClientPID = NULL, IMMLastUpdateTime = NOW(6) WHERE IMMSN=%d",
				 SYS_NAME, DATA_DATABASE_NAME, MODBUS_CLIENT_STATUS_UNKNOW, intIMMSN);
		// fprintf(stderr, "[Bermuda]:%s\n", charStatement);
		intRetval = mysql_query(&mysqlCon, charStatement);
		if (intRetval != EXIT_SUCCESS)
		{
			snprintf(charErrMsg, LONG_STRING_SIZE,
					 "[SubscribeModbus]Fail to update ModbusClientStatus = UNKNOW, ModbusClientPID = NULL and IMMLastUpdateTime = NOW(6) for IMMSN = %d (%d):%s",
					 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_MODBUS, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		}
	}

	// If Mold Signal Source Is From Modbus, Then Set Mold Status As UNKNOW While Leaving
	if (intModbusMoldSignalSource == MODBUS_MOLD_SIGNAL_SOURCE_MODBUS)
		intRetval = IMM_SetIMMMoldStatus(mysqlCon, intIMMSN, MOLD_STATUS_UNKNOW);

	// Wait For The ModbusClientPID = NULL
	do
	{
		snprintf(charStatement, MAX_STRING_SIZE, "SELECT ModbusClientPID FROM %s_%s.IMMSNList WHERE IMMSN=%d",
				 SYS_NAME, DATA_DATABASE_NAME, intIMMSN);
		intRetval = mysql_query(&mysqlCon, charStatement);
		if (intRetval && boolHasInsertErrMsg == false)
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to select SubscribeModbus for IMMSN=%d while exiting (%d):%s",
					 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
			boolHasInsertErrMsg = true;
		}
		mysqlResult = mysql_store_result(&mysqlCon);
		mysqlRow = mysql_fetch_row(mysqlResult);
		usleep(MODBUS_SAMPLE_RATE);
		Modbus_GetElapsedTime(stConnectTimeStamp, &doubleConnectElapsedTime);

		if (doubleConnectElapsedTime > MODBUS_CLIENT_TIME_OUT_SEC)
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Subscribtion terminated due to Modbus connection timeout. PID:%d of IMMSN:%d",
					 intModbusClientPID, intIMMSN);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		}

		// Update ModbusClientStatus, ModbusClientPID and IMMLastUpdateTime to INJPRO_Data.IMMSNList
		intModbusClientPID = getpid();
		snprintf(charStatement, MAX_STRING_SIZE,
				 "UPDATE %s_%s.IMMSNList SET ModbusClientStatus = %d, ModbusClientPID = NULL, IMMLastUpdateTime = NOW(6) WHERE IMMSN=%d",
				 SYS_NAME, DATA_DATABASE_NAME, MODBUS_CLIENT_STATUS_UNKNOW, intIMMSN);
		intRetval = mysql_query(&mysqlCon, charStatement);
		if (intRetval != EXIT_SUCCESS)
		{
			snprintf(charErrMsg, LONG_STRING_SIZE,
					 "[SubscribeModbus]Fail to update ModbusClientStatus = MODBUS_CLIENT_STATUS_UNKNOW ModbusClientPID = NULL and IMMLastUpdateTime = NOW(6) where IMMSN=%d (%d):%s",
					 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
			intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
			return intLogSN;
		}

		// Close MySQL Connection
		mysql_close(&mysqlCon);
		return EXIT_FAILURE;
	} while (mysqlRow[0] != NULL);

	// Close MySQL Connection
	mysql_close(&mysqlCon);

	fprintf(stderr, "[SubscribeModbus]ModbusClientPID=%d Terminated.\n", getpid());

	return EXIT_SUCCESS;
}

/* [Modbus]Get Max and Min voltage bound for A Specified Channel.
 * Based on the configuration of each channel, a type code is indicate the various max and min voltage bounds.
 *
 * @param OPCUAClient to OPC UA client
 * @param mysqlCon to MySQL connector
 * @return to indicate if the function call is success (EXIT_SUCCESS) or failure (EXIT_FAILURE) */
int Modbus_GetRangebyTypeCode(uint16_t intDAQTypeCode, int *intDAQMinBound, int *intDAQMaxBound)
{
	switch (intDAQTypeCode)
	{
	case 0x0103: // mV
		*intDAQMaxBound = TYPECODE_0103_UP;
		*intDAQMinBound = TYPECODE_0103_LOW;
		break;
	case 0x0104: // mV
		*intDAQMaxBound = TYPECODE_0104_UP;
		*intDAQMinBound = TYPECODE_0104_LOW;
		break;
	case 0x0105: // mV
		*intDAQMaxBound = TYPECODE_0105_UP;
		*intDAQMinBound = TYPECODE_0105_LOW;
		break;
	case 0x0106: // mV
		*intDAQMaxBound = TYPECODE_0106_UP;
		*intDAQMinBound = TYPECODE_0106_LOW;
		break;
	case 0x0140: // V
		*intDAQMaxBound = TYPECODE_0140_UP;
		*intDAQMinBound = TYPECODE_0140_LOW;
		break;
	case 0x0142: // V
		*intDAQMaxBound = TYPECODE_0142_UP;
		*intDAQMinBound = TYPECODE_0142_LOW;
		break;
	case 0x0143: // V
		*intDAQMaxBound = TYPECODE_0143_UP;
		*intDAQMinBound = TYPECODE_0143_LOW;
		break;
	case 0x0145: // V
		*intDAQMaxBound = TYPECODE_0145_UP;
		*intDAQMinBound = TYPECODE_0145_LOW;
		break;
	case 0x0147: // V
		*intDAQMaxBound = TYPECODE_0147_UP;
		*intDAQMinBound = TYPECODE_0147_LOW;
		break;
	case 0x0148: // V
		*intDAQMaxBound = TYPECODE_0148_UP;
		*intDAQMinBound = TYPECODE_0148_LOW;
		break;
	case 0x0181: // mA
		*intDAQMaxBound = TYPECODE_0181_UP;
		*intDAQMinBound = TYPECODE_0181_LOW;
		break;
	case 0x1080: // mA
		*intDAQMaxBound = TYPECODE_1080_UP;
		*intDAQMinBound = TYPECODE_1080_LOW;
		break;
	case 0x1082: // mA
		*intDAQMaxBound = TYPECODE_1082_UP;
		*intDAQMinBound = TYPECODE_1082_LOW;
		break;
	default: // unknow type code
		*intDAQMaxBound = TYPECODE_UNKNOW_UP;
		*intDAQMinBound = TYPECODE_UNKNOW_LOW;
		break;
	}
	if (*intDAQMaxBound == 0 && *intDAQMinBound == 0)
		return EXIT_FAILURE;
	else
		return EXIT_SUCCESS;
}

/* [Modbus]Get Mold Status via DAQ Based on Previous Mold Status and Mold Signal Type.
 * There are two types of mold signal types including: (1)Single Loop and (2)Multiple Loop.
 * (1)Single Loop IMM (also known as ING mode)
 * 					Released	Clamping	Clamped		Releasing
 * 		DI[0]		0			1			0			0
 * 		DI[1]		0			0			0			1
 *
 * (2)Multiple Loop IMM (also known as ED mode)
 * 					Released	Clamping	Clamped		Releasing
 * 		DI[0]		0			0			1			0
 * 		DI[1]		1			0			0			0
 * Note that previous mold state is required in order to identify current mold status.
 *
 * @param stStartTimeStamp to indicate the start time stamp
 * @param doubleElapsedTime to indicate the elapsed time which starts from the given time stamp
 * @return to indicate if the function call is success (EXIT_SUCCESS) or failure (EXIT_FAILURE) */
int Modbus_GetMoldStatus(modbus_t *modbusCon, unsigned int intModbusMoldSignalType, unsigned int intPreviousMoldStatus, unsigned int *intCurrentMoldStatus)
{
	int intRetval;
	uint8_t intDI[2] = {0};

	struct timespec stConnectTimeStamp;
	double doubleConnectElapsedTime;

	// Get Timestamp before Fetch Mold Status
	clock_gettime(CLOCK_REALTIME, &stConnectTimeStamp);
	do
	{
		intRetval = modbus_read_bits(modbusCon, COMMAND_MOLD_STATUS_ADDRESS, COMMAND_MOLD_STATUS_AMOUNT, intDI);
		if (intRetval < 0)
		{
			Modbus_GetElapsedTime(stConnectTimeStamp, &doubleConnectElapsedTime);
			fprintf(stderr, "[Modbus_GetMoldStatus] Fail to Get Mold status From DI0 and DI1. (ElapsedTime:%lf)\n", doubleConnectElapsedTime);
			usleep(MODBUS_SAMPLE_RATE);
			// Close Modbus Connection
			// modbus_close(modbusCon);
			// modbus_connect(modbusCon);
		}

	} while (intRetval < 0 && doubleConnectElapsedTime < MODBUS_CONNECT_INTEVAL);

	if (intRetval <= 0 || doubleConnectElapsedTime > MODBUS_CONNECT_INTEVAL)
	{
		// bermuda
		fprintf(stderr, "[Modbus_GetMoldStatus] Fail to Get Mold status From DI0 and DI1. (ElapsedTime:%lf > MODBUS_CONNECT_INTEVAL)\n",
				doubleConnectElapsedTime);
		return EXIT_FAILURE;
	}

	// fprintf(stderr,"DI0:%d DI1:%d\n",intDI[0],intDI[1]);

	// Single Loop IMM
	//		Released	Clamping	Clamped		Releasing
	// DI[0]		0		1			0			0
	// DI[1]		0		0			0			1

	if (intModbusMoldSignalType == 1)
	{
		if (intDI[0] == 1 && intDI[1] == 1)
		{
			// fprintf(stderr, "[Modbus_GetMoldStatus]Both Mold Clamping and Mold Releasing signals are high.\n");
			*intCurrentMoldStatus = MOLD_STATUS_UNKNOW;
		}
		else if (intDI[0] == 1)
		{
			*intCurrentMoldStatus = MOLD_STATUS_CLAMPING;
		}
		else if (intDI[1] == 1)
		{
			*intCurrentMoldStatus = MOLD_STATUS_RELEASING;
		}
		else
		{
			if (intPreviousMoldStatus == MOLD_STATUS_RELEASING)
				*intCurrentMoldStatus = MOLD_STATUS_RELEASED;
			else if (intPreviousMoldStatus == MOLD_STATUS_CLAMPING)
				*intCurrentMoldStatus = MOLD_STATUS_CLAMPED;
			else if (intPreviousMoldStatus == MOLD_STATUS_UNKNOW)
				*intCurrentMoldStatus = MOLD_STATUS_UNKNOW;
		}
	}

	// Multiple Loop IMM
	//		Released	Clamping	Clamped		Releasing
	// DI[0]		0		0			1			0
	// DI[1]		1		0			0			0
	else if (intModbusMoldSignalType == 2)
	{
		if (intDI[0] == 1 && intDI[1] == 1)
		{
			// fprintf(stderr, "[Modbus_GetMoldStatus]Both Mold Clamping and Mold Releasing signals are high.\n");
			*intCurrentMoldStatus = MOLD_STATUS_UNKNOW;
		}
		else if (intDI[0] == 1)
		{
			*intCurrentMoldStatus = MOLD_STATUS_CLAMPED;
		}
		else if (intDI[1] == 1)
		{
			*intCurrentMoldStatus = MOLD_STATUS_RELEASED;
		}
		else
		{
			if (intPreviousMoldStatus == MOLD_STATUS_RELEASED)
				*intCurrentMoldStatus = MOLD_STATUS_CLAMPING;
			else if (intPreviousMoldStatus == MOLD_STATUS_CLAMPED)
				*intCurrentMoldStatus = MOLD_STATUS_RELEASING;
			else if (intPreviousMoldStatus == MOLD_STATUS_UNKNOW)
				*intCurrentMoldStatus = MOLD_STATUS_UNKNOW;
		}
	}
	return EXIT_SUCCESS;
}

/* [Modbus]Get Elapsed Time from Start Time Stamp
 * Caculate the elapsed time wchih starts from the given time stamp
 *
 * @param stStartTimeStamp to indicate the start time stamp
 * @param doubleElapsedTime to indicate the elapsed time which starts from the given time stamp
 * @return to indicate if the function call is success (EXIT_SUCCESS) or failure (EXIT_FAILURE) */
int Modbus_GetElapsedTime(struct timespec stStartTimeStamp, double *doubleElapsedTime)
//[Modbus]Get Elapsed Time from Start Time Stamp
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

/* [Modbus]Convert analog raw data collected via DAQ to voltage based on Max and Min voltage bound.
 * For each channel, there are Maximal and Minimal voltage bound which can be configed manually.
 * To obtain the voltage, converting the raw data via this conifguration is required.
 *
 * @param OPCUAClient to OPC UA client
 * @param mysqlCon to MySQL connector
 * @return to indicate if the function call is success (EXIT_SUCCESS) or failure (EXIT_FAILURE) */
int Modbus_ConvertRawdata(uint16_t intDAQRawData, int intDAQMinBound, int intDAQMaxBound, double *doubleConvertedDAQRawData)
//[Modbus]Convert analog raw data collected via DAQ to voltage based on Max and Min voltage bound
{

	double doubleDAQResolutionRatio;
	doubleDAQResolutionRatio = pow(2.0, MODBUS_AI_RESOLUTION) - 1.0;
	*doubleConvertedDAQRawData = intDAQRawData / doubleDAQResolutionRatio * (intDAQMaxBound - intDAQMinBound) + intDAQMinBound;
	if (*doubleConvertedDAQRawData > 0)
	{
		*doubleConvertedDAQRawData = (int)(*doubleConvertedDAQRawData * pow((double)(10), (double)(MODBUS_ROUND_DIGITAL)) + 0.5) / pow((double)(10), (double)(MODBUS_ROUND_DIGITAL)) / 1.0;
	}
	else if (*doubleConvertedDAQRawData < 0)
	{
		*doubleConvertedDAQRawData = (int)(*doubleConvertedDAQRawData * pow((double)(10), (double)(MODBUS_ROUND_DIGITAL)) - 0.5) / pow((double)(10), (double)(MODBUS_ROUND_DIGITAL)) / 1.0;
	}

	return EXIT_SUCCESS;
}

/* [Modbus]Subscribe OPC UA Node (According IMMSN, OPC UA Version, etc.)
 * This function is to subscribe only "MoldClamped" and "MoldReleased" according to OPC UA Version defined in database.
 *
 * @param OPCUAClient to OPC UA client
 * @param mysqlCon to MySQL connector
 * @return to indicate if the function call is success (EXIT_SUCCESS) or failure (EXIT_FAILURE) */
int Modbus_SubscribeMonitorItem(UA_Client *OPCUAClient, MYSQL mysqlCon)
//[Modbus]Subscribe OPC UA Node (According IMMSN, OPC UA Version, etc.)
{

	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
	request.requestedPublishingInterval = 1000;
	request.requestedMaxKeepAliveCount = 1000;

	UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(OPCUAClient, request, NULL, NULL, NULL);
	if (response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
	{
		fprintf(stderr, "[Modbus_SubscribeMonitorItem]Create subscription succeeded, id %u\n", response.subscriptionId);
	}
	else
	{
		fprintf(stderr, "[Modbus_SubscribeMonitorItem]Create subscription failed\n");
	}

	UA_MonitoredItemCreateRequest monRequest;
	UA_MonitoredItemCreateResult monResponse;

	// Select Node NameSpace and ID Where OPCUAVersionSN Is Given
	snprintf(charStatement, MAX_STRING_SIZE,
			 "SELECT * FROM %s_%s.OPCUAVersionIndex WHERE OPCUAVersionSN=%d", SYS_NAME, INDEX_DATABASE_NAME, intOPCUAVersionSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[Modbus_SubscribeMonitorItem]Fail to select OPCUAVersionSN from table %s_%s.IMMSNList (%d): %s",
				 SYS_NAME, DATA_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return EXIT_FAILURE;
	}

	mysqlResult = mysql_store_result(&mysqlCon);
	mysqlRow = mysql_fetch_row(mysqlResult);

	for (int i = DO_MOLD_CLAMPED; i <= DO_MOLD_RELEASED; i++)
	{
		if (mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NS] != NULL &&
			(strcmp(mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_IDENTIFIER_TYPE], "i") == 0 ||
			 strcmp(mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_IDENTIFIER_TYPE], "s") == 0))
		{
			fprintf(stderr, "[Modbus_SubscribeMonitorItem]i=%d IMMSN:%d is trying to monitor %s[%s:%s], id %u\n", i, intIMMSN,
					mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NAME],
					mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NS],
					mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_ID], monResponse.monitoredItemId);
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
																	UA_TIMESTAMPSTORETURN_BOTH, monRequest, NULL, Modbus_HandlerIMMChanged, NULL);
			if (monResponse.statusCode == UA_STATUSCODE_GOOD)
			{
				fprintf(stderr, "[Modbus_SubscribeMonitorItem]IMMSN:%d is monitoring Name:%-20s [NS:%2s ID:%s], id %u\n", intIMMSN,
						mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NAME],
						mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NS],
						mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_ID],
						monResponse.monitoredItemId);
				strcpy(charMonIDNodeIndex[monResponse.monitoredItemId], mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NAME]);
				intMonIDNodeIndex[monResponse.monitoredItemId] = i;
			}
			else
			{
				snprintf(charErrMsg, LONG_STRING_SIZE, "[Modbus_SubscribeMonitorItem]IMMSN:%d fails to monitor [%s:%s], id %u", intIMMSN,
						 mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NS],
						 mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_ID], monResponse.monitoredItemId);
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
			}
		}
	}

	mysql_free_result(mysqlResult);
	return EXIT_SUCCESS;
}

/* [Modbus]Callback Function of OPC UA Client State.
 * If the OPCUAClientState code is UA_CLIENTSTATE_DISCONNECTED then trigger the stop sign,
 * else if the code is UA_CLIENTSTATE_SESSION, which indicates the connection has been built,
 * then the client subscribes all IMM paramater nodes.
 *
 * @param OPCUAClient to OPC UA client
 * @param OPCUAClientState to the state of UA Client */
static void Modbus_CallbackState(UA_Client *OPCUAClient, UA_ClientState OPCUAClientState)
//[Modbus]Callback Function of OPC UA Client State
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
		// The client is disconnected
		fprintf(stderr, "[Modbus_CallbackState]IMMSN:%d OPCUA client is disconnected\n", intIMMSN);
	}
	break;
	case UA_CLIENTSTATE_SESSION:
	{
		fprintf(stderr, "[Modbus_CallbackState]IMMSN:%d A session with the server is open\n", intIMMSN);

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

		// Update OPCUAStatus=RUNNING,SubscribePID and IMMLastUpdateTime to INJPRO_Data.IMMSNList
		snprintf(charStatement, MAX_STRING_SIZE,
				 "UPDATE %s_%s.IMMSNList SET ModbusClientPID = %d,IMMLastUpdateTime = NOW(6) WHERE IMMSN=%d",
				 SYS_NAME, DATA_DATABASE_NAME, getpid(), intIMMSN);
		intRetval = mysql_query(&mysqlCon, charStatement);
		if (intRetval != EXIT_SUCCESS)
		{
			snprintf(charErrMsg, LONG_STRING_SIZE,
					 "[Modbus_CallbackState]Fail to update ModbusClientPID and IMMLastUpdateTime for IMMSN=%d (%d):%s",
					 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
			return;
		}
		Modbus_SubscribeMonitorItem(OPCUAClient, mysqlCon);
		mysql_close(&mysqlCon);
	}
	break;
	}
	return;
}

/* [Modbus]Callback Function of Inactivity of Subscription.
 * This call back function is called when a subscription of the OPC UA Client is in inactivity.
 *
 * @param OPCUAClient to OPC UA client
 * @param subId to indicate a subscription
 * @param subContext to the content of the subscription */
static void Modbus_CallbackSubscriptionInactivity(UA_Client *OPCUAClient, UA_UInt32 subId, void *subContext)
//[Modbus]Callback Function of Inactivity of Subscription
{
	fprintf(stderr, "[Modbus_CallbackSubscriptionInactivity]Inactivity for subscription %u", subId);
}

/* [Modbus]Handler of Stop Subscription
 *
 * @param intSign to indicate a specified sign to be done in handler */
static void Modbus_HandlerStopSubscribeModbus(int intSign)
// [Modbus]Handler of Stop Subscription
{
	MYSQL mysqlCon;
	int intRetval;
	char charErrMsg[LONG_STRING_SIZE];
	char charStatement[MAX_STRING_SIZE];

	boolIsStopSignal = true;
	// Connect to MYSQL Server
	mysql_init(&mysqlCon);
	if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
	{
		fprintf(stderr, "MYSQL connection failed.\n");
		if (mysql_errno(&mysqlCon))
		{
			fprintf(stderr, "MYSQL connection error %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		}
	}

	// Update ModbusClientPID, ModbusClientStatus and IMMLastUpdateTime to INJPRO_Data.IMMSNList
	snprintf(charStatement, MAX_STRING_SIZE,
			 "UPDATE %s_%s.IMMSNList SET ModbusClientStatus = %d, ModbusClientPID = NULL, IMMLastUpdateTime = NOW(6) WHERE IMMSN = %d",
			 SYS_NAME, DATA_DATABASE_NAME, MODBUS_CLIENT_STATUS_WAITING, intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE,
				 "[SubscribeModbus]Fail to update ModbusClientStatus = FAILED and IMMLastUpdateTime = NOW(6) where IMMSN=%d (%d):%s",
				 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return;
	}
	// Close MySQL Connection
	mysql_close(&mysqlCon);
}

/* [Modbus]Handler of the Changes from Subscribed OPC UA Node
 * This call back function is called while mold clamping, clapmed, releasing and released.
 * So far only three types including UA_Boolean, UA_Double and UA_String are support.
 *
 * @param OPCUAClient to OPC UA client
 * @param subId to indicate a subscription
 * @param subContext to the content of the subscription
 * @param monId to indicate a monitor item ID
 * @param monContext to the content of the monitor item
 * @param uaDataValue to the value of monitor item */
static void Modbus_HandlerIMMChanged(UA_Client *OPCUAClient, UA_UInt32 subId, void *subContext, UA_UInt32 monId, void *monContext, UA_DataValue *value)
//[Modbus]Handler of the Changes from Subscribed OPC UA Node
{

	MYSQL mysqlCon;
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	unsigned int intLogSN;

	if (intMonIDNodeIndex[monId] == DO_MOLD_CLAMPED)
	{
		if ((value->value.type->typeIndex == UA_TYPES_BOOLEAN && *(UA_Boolean *)value->value.data == true) ||
			(value->value.type->typeIndex == UA_TYPES_DOUBLE && *(UA_Double *)value->value.data == 1.0) ||
			(value->value.type->typeIndex == UA_TYPES_STRING && *((UA_String *)(value->value.data))->data == '1'))
		{
			intCurrentMoldStatus = MOLD_STATUS_CLAMPED;
		}
		else if ((value->value.type->typeIndex == UA_TYPES_BOOLEAN && *(UA_Boolean *)value->value.data == false) ||
				 (value->value.type->typeIndex == UA_TYPES_DOUBLE && *(UA_Double *)value->value.data == 0) ||
				 (value->value.type->typeIndex == UA_TYPES_STRING && *((UA_String *)(value->value.data))->data == '0'))
			intCurrentMoldStatus = MOLD_STATUS_RELEASING;
	}
	else if (intMonIDNodeIndex[monId] == DO_MOLD_RELEASED)
	{
		if ((value->value.type->typeIndex == UA_TYPES_BOOLEAN && *(UA_Boolean *)value->value.data == true) ||
			(value->value.type->typeIndex == UA_TYPES_DOUBLE && *(UA_Double *)value->value.data == 1.0) ||
			(value->value.type->typeIndex == UA_TYPES_STRING && *((UA_String *)(value->value.data))->data == '1'))
		{
			intCurrentMoldStatus = MOLD_STATUS_RELEASED;
		}
		else if (
			(value->value.type->typeIndex == UA_TYPES_BOOLEAN && *(UA_Boolean *)value->value.data == false) ||
			(value->value.type->typeIndex == UA_TYPES_DOUBLE && *(UA_Double *)value->value.data == 0.0) ||
			(value->value.type->typeIndex == UA_TYPES_STRING && *((UA_String *)(value->value.data))->data == '0'))
		{
			intCurrentMoldStatus = MOLD_STATUS_CLAMPING;
			// Set Clamping Time Stamp
			clock_gettime(CLOCK_REALTIME, &stClampedTimeStamp);
		}

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

		// Renew ShotSN
		// intRetval = DB_SelectShotSNbyMOSN(mysqlCon, intMOSN, &intShotSN);
		// if (intRetval != EXIT_SUCCESS)
		//{
		//	snprintf(charErrMsg, LONG_STRING_SIZE, "[Modbus_HandlerIMMChanged]Fail to renew ShotSN while mold released for IMMSN:%d", intIMMSN);
		//	intLogSN = SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
		//	return;
		// }
		mysql_close(&mysqlCon);
	}

	if (value->value.type->typeIndex == UA_TYPES_DATETIME)
	{
		UA_DateTimeStruct t = UA_DateTime_toStruct(*(UA_DateTime *)value->value.data);
		fprintf(stderr, "[Modbus_HandlerIMMChanged]IMMSN:%d IMM [SubID:%d MonID:%3d NodeName:%-20s:%4d-%02d-%02d %02d:%02d:%02d]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], t.year, t.month, t.day, t.hour, t.min, t.sec);
	}
	else if (value->value.type->typeIndex == UA_TYPES_BOOLEAN)
	{
		fprintf(stderr, "[Modbus_HandlerIMMChanged]IMMSN:%d IMM [SubID:%d MonID:%3d NodeName:%-20s:%d]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Boolean *)value->value.data);
	}
	else if (value->value.type->typeIndex == UA_TYPES_FLOAT)
	{
		fprintf(stderr, "[Modbus_HandlerIMMChanged]IMMSN:%d IMM [SubID:%d MonID:%3d NodeName:%-20s:%f]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Float *)value->value.data);
	}
	else if (value->value.type->typeIndex == UA_TYPES_DOUBLE)
	{
		fprintf(stderr, "[Modbus_HandlerIMMChanged]IMMSN:%d IMM [SubID:%d MonID:%3d NodeName:%-20s:%f]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Double *)value->value.data);
	}
	else if (value->value.type->typeIndex == UA_TYPES_INT16)
	{
		fprintf(stderr, "[Modbus_HandlerIMMChanged]IMMSN:%d IMM [SubID:%d MonID:%3d NodeName:%-20s:%d]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Int16 *)value->value.data);
	}
	else if (value->value.type->typeIndex == UA_TYPES_UINT16)
	{
		fprintf(stderr, "[Modbus_HandlerIMMChanged]IMMSN:%d IMM [SubID:%d MonID:%3d NodeName:%-20s:%d]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_UInt16 *)value->value.data);
	}
	else if (value->value.type->typeIndex == UA_TYPES_INT32)
	{
		fprintf(stderr, "[Modbus_HandlerIMMChanged]IMMSN:%d IMM [SubID:%d MonID:%3d NodeName:%-20s:%d]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Int32 *)value->value.data);
	}
	else if (value->value.type->typeIndex == UA_TYPES_UINT32)
	{
		fprintf(stderr, "[Modbus_HandlerIMMChanged]IMMSN:%d IMM [SubID:%d MonID:%3d NodeName:%-20s:%d]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_UInt32 *)value->value.data);
	}
	else if (value->value.type->typeIndex == UA_TYPES_INT64)
	{
		fprintf(stderr, "[Modbus_HandlerIMMChanged]IMMSN:%d IMM [SubID:%d MonID:%3d NodeName:%-20s:%ld]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Int64 *)value->value.data);
	}
	else if (value->value.type->typeIndex == UA_TYPES_UINT64)
	{
		fprintf(stderr, "[Modbus_HandlerIMMChanged]IMMSN:%d IMM [SubID:%d MonID:%3d NodeName:%-20s:%ld]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Int64 *)value->value.data);
	}
	return;
}

/* [Modbus]Write Cache Raw Data to Database.
 * First, the raw data will be inserted into database as primary task.
 * After that, for each shot, the feature values will be caculated based on two categories of raw data including:
 * (1)IMM system pressure and (2)Cavity pressure. Finally, the EC, QC and SPC judgement will be peformed.
 * Note that this is a function which should be called in a thread so that the raw data collection will not be delay.
 *
 * @param ptrSensorCacheData to a pointer of raw data */
void *Modbus_WriteSensorCachetoDatabase(void *ptrSensorCacheData)
//[Modbus] Write Cache Raw Data to Database
{
	stSensorCacheData *varSensorCacheData = (stSensorCacheData *)ptrSensorCacheData;
	char charErrMsg[LONG_STRING_SIZE];
	char charStatement[MAX_STRING_SIZE];
	int intHead, intTail;
	int intRetval;

	// bool boolCheckEmptyShot;
	bool boolSoftSensorReady;

	MYSQL mysqlCon;
	double *doubleSensorData;
	unsigned int intSensorDataSize;
	char charIMMSysPressureSensorFeatureValue[IMM_SYSPRESSURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE];
	char charCavityPressureSensorFeatureValue[CAVITY_PRESSURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE];
	char charCavityTemperatureSensorFeatureValue[CAVITY_TEMPERATURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE];

	char charNGMOCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE];
	char charFGMOCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE];

	unsigned int intCavityPressureSensorCount;
	unsigned int intECProgressFlag;
	unsigned int intSPCProgressFlag;
	unsigned int intQCProgressFlag;

	// Initial doubleSensorData Data Pointer
	doubleSensorData = NULL;

	while (boolIsStopSignal == false)
	{

		// Connect to MYSQL Server
		mysql_init(&mysqlCon);
		if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
		{
			fprintf(stderr, "MYSQL connection failed.\n");
			if (mysql_errno(&mysqlCon))
			{
				fprintf(stderr, "MYSQL connection error %d: %s\n", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
			}
			pthread_exit(NULL);
		}

		// varSensorCacheData->intCacheHeadIndex : First one ready to be written
		// varSensorCacheData->intCacheTailIndex : Last one already inserted
		// intHead : Last one ready to be inserted
		// intTail : First one ready to be inserted

		// If Head = intCacheHeadIndex - 1 (Last one ready to be inserted)
		intHead = varSensorCacheData->intCacheHeadIndex - 1 < 0 ? MODBUS_MAX_CACHE_SIZE - 1 : varSensorCacheData->intCacheHeadIndex - 1;
		// If Tail = intCacheTailIndex + 1 (First one ready to be inserted)
		intTail = varSensorCacheData->intCacheTailIndex + 1 == MODBUS_MAX_CACHE_SIZE ? 0 : varSensorCacheData->intCacheTailIndex + 1;

		// Empty: First one ready to be inserted(intTail) == First one ready to be written(varSensorCacheData->intCacheHeadIndex))
		if (intTail == varSensorCacheData->intCacheHeadIndex)
		{
#ifdef DEBUG_MODE_SUBSCRIBE_MODBUS

			fprintf(stderr, "[Modbus_WriteSensorCachetoDatabase]The Cache is empty (Mold Status:%d)\n", intCurrentMoldStatus);
#endif
			// If Mold Status Is Releasing or Released and cache is empty then intFeatureValueReadyShotSN is the last one
			intFeatureValueReadyShotSN = intShotSN - 1;
		}
		else if (intHead >= intTail)
		{
			for (int i = 0; i < MODBUS_CHANNEL_AMOUNT; i++)
			{
				if (intDAQChanneltoMOCustomSensorSN[i] != 0)
				{
#ifdef MODBUS_INSERT_ENABLE
					intRetval = DB_InsertMOCustomSensorDataBatch(mysqlCon,															   // Mysql Connector
																 intMOSN,															   // MOSN
																 varSensorCacheData->intShotSNCacheData[i],							   // ShotSN
																 intDAQChanneltoMOCustomSensorSN[i],								   // MOCustomSensorSN
																 varSensorCacheData->doubleElapsedTimeCacheData[i],					   // ElapsedTime
																 varSensorCacheData->doubleSensorValueCacheData[i],					   // Converted Voltage
																 doubleMOCustomSensorConvertRatio[intDAQChanneltoMOCustomSensorSN[i]], // Converted Ratio
																 intTail, intHead);													   // Queue Tail and Head
#endif

					// Set intFeatureValueReadyShotSN = ShotSN in Cache -1
					intFeatureValueReadyShotSN = varSensorCacheData->intShotSNCacheData[i][intHead] - 1;
				}
			}
		}
		else if (intTail > intHead)
		{
			for (int i = 0; i < MODBUS_CHANNEL_AMOUNT; i++)
			{
				if (intDAQChanneltoMOCustomSensorSN[i] != 0)
				{
#ifdef MODBUS_INSERT_ENABLE
					intRetval = DB_InsertMOCustomSensorDataBatch(mysqlCon,															   // Mysql Connector
																 intMOSN,															   // MOSN
																 varSensorCacheData->intShotSNCacheData[i],							   // ShotSN
																 intDAQChanneltoMOCustomSensorSN[i],								   // MOCustomSensorSN
																 varSensorCacheData->doubleElapsedTimeCacheData[i],					   // ElapsedTime
																 varSensorCacheData->doubleSensorValueCacheData[i],					   // Converted Voltage
																 doubleMOCustomSensorConvertRatio[intDAQChanneltoMOCustomSensorSN[i]], // Converted Ratio
																 intTail, MODBUS_MAX_CACHE_SIZE - 1);								   // Queue Tail and Head

					intRetval = DB_InsertMOCustomSensorDataBatch(mysqlCon,															   // Mysql Connector
																 intMOSN,															   // MOSN
																 varSensorCacheData->intShotSNCacheData[i],							   // ShotSN
																 intDAQChanneltoMOCustomSensorSN[i],								   // MOCustomSensorSN
																 varSensorCacheData->doubleElapsedTimeCacheData[i],					   // ElapsedTime
																 varSensorCacheData->doubleSensorValueCacheData[i],					   // Converted Voltage
																 doubleMOCustomSensorConvertRatio[intDAQChanneltoMOCustomSensorSN[i]], // Converted Ratio
																 0, intHead);														   // Queue Tail and Head
#endif

					// Set intFeatureValueReadyShotSN = ShotSN in Cache -1
					intFeatureValueReadyShotSN = varSensorCacheData->intShotSNCacheData[i][intHead] - 1;
				}
			}
		}

		// Last one already inserted = Last one ready to be inserted
		varSensorCacheData->intCacheTailIndex = intHead;

#ifdef ENABLE_CUSTOM_SENSOR_FEATURE_VALUE
		fprintf(stderr, "intFeatureValueDoneShotSN=%d intFeatureValueReadyShotSN=%d ShotSN:%d\n", intFeatureValueDoneShotSN, intFeatureValueReadyShotSN, intShotSN);
		// Caculate feature value for all cache Shot
		for (int i = intFeatureValueDoneShotSN + 1; i <= intFeatureValueReadyShotSN && i != 0; i++)
		{
			// The flag to specify if the empty shot has been checked
			// boolCheckEmptyShot = false;

			// The flag to indecate if features values of soft sensor should be caculated. (default is true)
			boolSoftSensorReady = true;

			// Initialize the Mold Pressure Count as Zero
			intCavityPressureSensorCount = 0;

			// Caculate Feature for Each Channel
			for (int j = 0; j < MODBUS_CHANNEL_AMOUNT; j++)
			{

				// Check if this channel has mold sensor
				if (intDAQChanneltoMOCustomSensorSN[j] == 0)
					continue;

				// Fetch mold sensor rawdata
				intRetval = DB_SelectMOCustomSensorRawData(mysqlCon, intMOSN, i, intDAQChanneltoMOCustomSensorSN[j], &doubleSensorData, &intSensorDataSize);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[Modbus_WriteSensorCachetoDatabase]Fail to fetch mold sensor rawdata for MOSN:%d ShotSN:%d MOCustomSensorSN:%d while caculating feature value",
							 intMOSN, i, intDAQChanneltoMOCustomSensorSN[j]);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
					intFeatureValueDoneShotSN = i;
					continue;
				}
				// If Class = IMM and Category = Pressure (System Pressure)
				if (intDAQChanneltoSensorClass[j] == CUSTOM_SENSOR_CLASS_IMM && intDAQChanneltoSensorCategory[j] == CUSTOM_SENSOR_CATEGORY_PRESSURE)
				{
					fprintf(stderr, "[Modbus_WriteSensorCachetoDatabase]Channel:%d Class:%d Category:%d \n", j, intDAQChanneltoSensorClass[j], intDAQChanneltoSensorCategory[j]);

					intRetval = DB_GetIMMSysPressureSensorFeatureValue(mysqlCon, intMOSN, doubleSensorData, intSensorDataSize, charIMMSysPressureSensorFeatureValue);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "[Modbus_WriteSensorCachetoDatabase]Fail to caculate IMM pressure feature value for MOSN:%d ShotSN:%d MOCustomSensorSN:%d  while caculating feature value",
								 intMOSN, i, intDAQChanneltoMOCustomSensorSN[j]);
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
					}
					else
					{
						// Insert IMM system pressure feature value
						intRetval = DB_InsertIMMSysPressureSensorFeatureValue(mysqlCon, intMOSN, i, intDAQChanneltoMOCustomSensorSN[j], charIMMSysPressureSensorFeatureValue);
						if (intRetval != EXIT_SUCCESS)
						{
							snprintf(charErrMsg, LONG_STRING_SIZE,
									 "[Modbus_WriteSensorCachetoDatabase]Fail to insert IMM system pressure feature value for MOSN:%d ShotSN:%d MOCustomSensorSN:%d while caculating feature value",
									 intMOSN, i, intDAQChanneltoMOCustomSensorSN[j]);
							SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
						}
					}
				}
				// If Class = Cavity and Category = Pressure and
				else if (intDAQChanneltoSensorClass[j] == CUSTOM_SENSOR_CLASS_CAVITY && intDAQChanneltoSensorCategory[j] == CUSTOM_SENSOR_CATEGORY_PRESSURE)
				{
					// fprintf(stderr,"Channel:%d Category:%d Pressure\n",j,intDAQChanneltoSensorClass[j]);
					intCavityPressureSensorCount++;

					// Caculate mold pressure sensor feature value
					intRetval = DB_GetCavityPressureSensorFeatureValue(mysqlCon, intMOSN, doubleSensorData, intSensorDataSize, charCavityPressureSensorFeatureValue);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "[Modbus_WriteSensorCachetoDatabase]Fail to caculate mold pressure feature value for MOSN:%d ShotSN:%d MOCustomSensorSN:%d  while caculating feature value",
								 intMOSN, i, intDAQChanneltoMOCustomSensorSN[j]);
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);

						// Cancle Feature Value Caculation of Soft Sensor
						boolSoftSensorReady = false;
					}
					else
					{
						// If CavityPressureSensorCount == 1, then Copy Feature to Near Gate
						if (intCavityPressureSensorCount == 1)
						{
							for (int k = 1; k <= CAVITY_PRESSURE_SENSOR_SINGLE_FEATURE_NUM; k++)
							{
								strcpy(charNGMOCustomSensorFeatureValue[k], charCavityPressureSensorFeatureValue[k]);
							}
						}
						// Else if CavityPressureSensorCount == 2, then Copy Feature to Far Gate
						else if (intCavityPressureSensorCount == 2)
						{
							for (int k = 1; k <= CAVITY_PRESSURE_SENSOR_SINGLE_FEATURE_NUM; k++)
							{
								strcpy(charFGMOCustomSensorFeatureValue[k], charCavityPressureSensorFeatureValue[k]);
							}
						}

						// Insert mold pressure feature value
						intRetval = DB_InsertCavityPressureSensorFeatureValue(mysqlCon, intMOSN, i, intDAQChanneltoMOCustomSensorSN[j], charCavityPressureSensorFeatureValue);
						if (intRetval != EXIT_SUCCESS)
						{
							snprintf(charErrMsg, LONG_STRING_SIZE,
									 "[Modbus_WriteSensorCachetoDatabase]Fail to insert mold pressure feature value for MOSN:%d ShotSN:%d MOCustomSensorSN:%d while caculating feature value",
									 intMOSN, i, intDAQChanneltoMOCustomSensorSN[j]);
							SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);

							// Cancle Feature Value Caculation of Soft Sensor
							boolSoftSensorReady = false;
						}
					}
				}
				else if (intDAQChanneltoSensorClass[j] == CUSTOM_SENSOR_CLASS_CAVITY && intDAQChanneltoSensorCategory[j] == CUSTOM_SENSOR_CATEGORY_TEMPERATURE)
				{
					// fprintf(stderr, "Channel:%d Category:%d Temperature\n", i, intDAQChanneltoSensorClass[j]);

					// Caculate mold temperature sensor feature value
					intRetval = DB_GetCavityTemperatureSensorFeatureValue(mysqlCon, intMOSN, doubleSensorData, intSensorDataSize, charCavityTemperatureSensorFeatureValue);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE,
								 "[Modbus_WriteSensorCachetoDatabase]Fail to caculate mold temperature feature value for MOSN:%d ShotSN:%d MOCustomSensorSN:%d  while caculating feature value",
								 intMOSN, i, intDAQChanneltoMOCustomSensorSN[j]);
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);

						// Cancle Feature Value Caculation of Soft Sensor
						boolSoftSensorReady = false;
					}
					else
					{
						// Insert mold pressure feature value
						intRetval = DB_InsertCavityTemperatureSensorFeatureValue(mysqlCon, intMOSN, i, intDAQChanneltoMOCustomSensorSN[j], charCavityTemperatureSensorFeatureValue);
						if (intRetval != EXIT_SUCCESS)
						{
							snprintf(charErrMsg, LONG_STRING_SIZE,
									 "[Modbus_WriteSensorCachetoDatabase]Fail to insert mold temperature feature value for MOSN:%d ShotSN:%d MOCustomSensorSN:%d while caculating feature value",
									 intMOSN, i, intDAQChanneltoMOCustomSensorSN[j]);
							SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);

							// Cancle Feature Value Caculation of Soft Sensor
							boolSoftSensorReady = false;
						}
					}
				}
			}

			// SoftSensor
			if (boolSoftSensorReady == true && intCavityPressureSensorCount >= 2)
			{
				// Select near gate mold pressure rawdata
				/*
				intRetval = DB_SelectMOCustomSensorRawData(mysqlCon, intMOSN, i, 1, &doubleSensorData, &intSensorDataSize);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[Modbus_WriteSensorCachetoDatabase]Fail to select near gate mold pressure sensor data for ShotSN:%d MOCustomSensorSN:%d\n", i, 2);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
					intFeatureValueDoneShotSN = i;
					continue;
				}

				//Caculate near gate mold pressure sensor SPC feature value
				intRetval = DB_GetCavityPressureSensorFeatureValue(mysqlCon, intMOSN, doubleSensorData, intSensorDataSize, charNGMOCustomSensorFeatureValue);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[Modbus_WriteSensorCachetoDatabase]Fail to select near gate mold pressure sensor feature value for ShotSN:%d MOCustomSensorSN:%d\n", i, 2);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
					intFeatureValueDoneShotSN = i;
					continue;
				}
				//free(doubleSensorData);

				//Select far gate mold pressure rawdata
				intRetval = DB_SelectMOCustomSensorRawData(mysqlCon, intMOSN, i, 2, &doubleSensorData, &intSensorDataSize);
				if (intRetval != EXIT_SUCCESS)
				{

					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[Modbus_WriteSensorCachetoDatabase]Fail to select far gate mold pressure sensor data for ShotSN:%d MOCustomSensorSN:%d\n", i, 1);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
					intFeatureValueDoneShotSN = i;
					continue;
				}

				//Caculate far gate mold pressure sensor SPC feature value
				intRetval = DB_GetCavityPressureSensorFeatureValue(mysqlCon, intMOSN, doubleSensorData, intSensorDataSize, charFGMOCustomSensorFeatureValue);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[Modbus_WriteSensorCachetoDatabase]Fail to caculate far gate mold pressure sensor feature value for ShotSN:%d MOCustomSensorSN:%d\n", i, 1);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
					intFeatureValueDoneShotSN = i;
					continue;
				}
				*/

				// Insert SPC feature value for ShotSN:i
				intRetval = DB_InsertCavityPressureSensorSoftSensorFeatureValue(mysqlCon, intMOSN, i, charNGMOCustomSensorFeatureValue, charFGMOCustomSensorFeatureValue);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[Modbus_WriteSensorCachetoDatabase]Fail to insert SoftSensor feature value for ShotSN:%d \n", i);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
					intFeatureValueDoneShotSN = i;
					continue;
				}

				fprintf(stderr, "[Modbus_WriteSensorCachetoDatabase]Mold Pressure/Temperature/Soft Feature Value of MOSN:%d ShotSN:%d has caculated successfully.\n", intMOSN, i);
			}
			else
			{
				fprintf(stderr, "[Modbus_WriteSensorCachetoDatabase]P-SensorCount:%d No softsensor feature value will be caculated\n", intCavityPressureSensorCount);
			}

			if (intOPCUAVersionSN == VIRTUAL_OPCUA_SERVER_OPCUA_VERSIONSN)
			{
				// Trigger EC_PROGRESS_FLAG_MODBUS_READY
				/* EC is disable for OPC UA now
				intRetval = EC_TriggerECProgressFlag(mysqlCon, intMOSN, i, EC_PROGRESS_FLAG_MODBUS_READY);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[Modbus_WriteSensorCachetoDatabase]Fail to trigger EC OPCUA ready flag for ShotSN:%d \n", i);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
					//intFeatureValueDoneShotSN = i;
				}*/

				// Trigger SPC_PROGRESS_FLAG_OPCUA_READY
				intRetval = SPC_TriggerSPCProgressFlag(mysqlCon, intMOSN, i, SPC_PROGRESS_FLAG_OPCUA_READY);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[Modbus_WriteSensorCachetoDatabase]Fail to trigger SPC OPCUA ready flag for ShotSN:%d \n", i);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
				}

				// Trigger QC_PROGRESS_FLAG_OPCUA_READY
				intRetval = QC_TriggerQCProgressFlag(mysqlCon, intMOSN, i, QC_PROGRESS_FLAG_OPCUA_READY);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE,
							 "[Modbus_WriteSensorCachetoDatabase]Fail to trigger QC OPCUA ready flag for ShotSN:%d \n", i);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
				}
			}

			// Trigger EC_PROGRESS_FLAG_MODBUS_READY
			intRetval = EC_TriggerECProgressFlag(mysqlCon, intMOSN, i, EC_PROGRESS_FLAG_MODBUS_READY);
			if (intRetval != EXIT_SUCCESS)
			{
				snprintf(charErrMsg, LONG_STRING_SIZE,
						 "[Modbus_WriteSensorCachetoDatabase]Fail to trigger EC modbus ready flag for ShotSN:%d MOSN:%d\n", i, intMOSN);
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
				// intFeatureValueDoneShotSN = i;
			}

			// Trigger SPC_PROGRESS_FLAG_MODBUS_READY
			intRetval = SPC_TriggerSPCProgressFlag(mysqlCon, intMOSN, i, SPC_PROGRESS_FLAG_MODBUS_READY);
			if (intRetval != EXIT_SUCCESS)
			{
				snprintf(charErrMsg, LONG_STRING_SIZE,
						 "[Modbus_WriteSensorCachetoDatabase]Fail to trigger SPC modbus ready flag for ShotSN:%d MOSN:%d\n", i, intMOSN);
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
				// intFeatureValueDoneShotSN = i;
			}

			// Trigger QC_PROGRESS_FLAG_MODBUS_READY
			intRetval = QC_TriggerQCProgressFlag(mysqlCon, intMOSN, i, QC_PROGRESS_FLAG_MODBUS_READY);
			if (intRetval != EXIT_SUCCESS)
			{
				snprintf(charErrMsg, LONG_STRING_SIZE,
						 "[Modbus_WriteSensorCachetoDatabase]Fail to trigger QC modbus ready flag for ShotSN:%d MOSN:%d\n", i, intMOSN);
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
				// intFeatureValueDoneShotSN = i;
			}

			// Get ECProgressFlag
			intRetval = EC_SelectECProgressFlag(mysqlCon, intMOSN, i, &intECProgressFlag);
			if (intRetval != EXIT_SUCCESS)
			{
				snprintf(charErrMsg, LONG_STRING_SIZE,
						 "[Modbus_WriteSensorCachetoDatabase]Fail to select EC modbus ready for ShotSN:%d MOSN:%d\n", i, intMOSN);
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
				intFeatureValueDoneShotSN = i;
				continue;
			}
			else if ((intECProgressFlag & EC_PROGRESS_FLAG_MODBUS_READY) == false &&
					 (intECProgressFlag & EC_PROGRESS_FLAG_OPCUA_READY) == false &&
					 (intECProgressFlag & EC_PROGRESS_FLAG_FINISHED) == true)
			{
				intRetval = EC_CheckEmptyShot(mysqlCon, intMOSN, i);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE, "[Modbus_WriteSensorCachetoDatabase]Fail to insert EC result for ShotSN:%d \n", i);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
				}
				else
				{
					// Trigger SPC_FLAG_SPC_FINISH If The Alarm Is Successfully Inserted
					intRetval = EC_TriggerECProgressFlag(mysqlCon, intMOSN, i, EC_PROGRESS_FLAG_FINISHED);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE, "[Modbus_WriteSensorCachetoDatabase]Fail to trigger EC finished for ShotSN:%d MOSN:%d\n", i, intMOSN);
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
					}
					else
					{
						fprintf(stderr, "[Modbus_WriteSensorCachetoDatabase]EC of MOSN:%d ShotSN:%d has caculated successfully.\n", intMOSN, i);
					}
				}
			}

			// Get SPCProgressFlag
			intRetval = SPC_SelectSPCProgressFlag(mysqlCon, intMOSN, i, &intSPCProgressFlag);

			// fprintf(stderr, "SPCProgressFlag:%d \n", intSPCProgressFlag);
			// fprintf(stderr, "intSPCProgressFlag & SPC_PROGRESS_FLAG_MODBUS_READY:%d\n", intSPCProgressFlag & SPC_PROGRESS_FLAG_MODBUS_READY);
			// fprintf(stderr, "intSPCProgressFlag & SPC_PROGRESS_FLAG_OPCUA_READY:%d\n", intSPCProgressFlag & SPC_PROGRESS_FLAG_OPCUA_READY);
			// fprintf(stderr, "intSPCProgressFlag & SPC_PROGRESS_FLAG_FINISHED:%d\n", intSPCProgressFlag & SPC_PROGRESS_FLAG_FINISHED);
			// fprintf(stderr, "Result:%d\n", ((intSPCProgressFlag & SPC_PROGRESS_FLAG_FINISHED) == true));

			if (intRetval != EXIT_SUCCESS)
			{
				snprintf(charErrMsg, LONG_STRING_SIZE,
						 "[Modbus_WriteSensorCachetoDatabase]Fail to select SPCProgressFlag for ShotSN:%d MOSN:%d\n", i, intMOSN);
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
				intFeatureValueDoneShotSN = i;
				continue;
			}

			// If Features of Both Modbus and OPC UA Are Ready, Then Insert SPC Alarm
			else if ((intSPCProgressFlag & SPC_PROGRESS_FLAG_MODBUS_READY) == false &&
					 (intSPCProgressFlag & SPC_PROGRESS_FLAG_OPCUA_READY) == false &&
					 (intSPCProgressFlag & SPC_PROGRESS_FLAG_FINISHED) == true)
			{
				intRetval = SPC_InsertSPCAlarm(mysqlCon, intMOSN, i);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE, "[Modbus_WriteSensorCachetoDatabase]Fail to insert SPC Alarm for ShotSN:%d \n", i);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
				}
				else
				{
					// Trigger SPC_FLAG_SPC_FINISH If The Alarm Is Successfully Inserted
					intRetval = SPC_TriggerSPCProgressFlag(mysqlCon, intMOSN, i, SPC_PROGRESS_FLAG_FINISHED);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE, "[Modbus_WriteSensorCachetoDatabase]Fail to trigger SoftSensor feature value for ShotSN:%d \n", i);
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
					}
					else
					{
						fprintf(stderr, "[Modbus_WriteSensorCachetoDatabase]SPC of MOSN:%d ShotSN:%d has caculated successfully.\n", intMOSN, i);
					}
				}
			}

			// Get QCCFlag
			intRetval = QC_SelectQCProgressFlag(mysqlCon, intMOSN, i, &intQCProgressFlag);
			if (intRetval != EXIT_SUCCESS)
			{
				snprintf(charErrMsg, LONG_STRING_SIZE,
						 "[Modbus_WriteSensorCachetoDatabase]Fail to select QCProgressFlag for ShotSN:%d \n", i);
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
				intFeatureValueDoneShotSN = i;
				continue;
			}

			// If Features of Both Modbus and OPC UA Are Ready, Then Insert SPC Alarm
			else if (
				(intQCProgressFlag & QC_PROGRESS_FLAG_MODBUS_READY) == false &&
				(intQCProgressFlag & QC_PROGRESS_FLAG_OPCUA_READY) == false &&
				(intQCProgressFlag & QC_PROGRESS_FLAG_FINISHED) == true)
			{
				intRetval = QC_InsertQCAlarm(mysqlCon, intMOSN, i);
				if (intRetval != EXIT_SUCCESS)
				{
					snprintf(charErrMsg, LONG_STRING_SIZE, "[Modbus_WriteSensorCachetoDatabase]Fail to insert SPC Quality Control Alarm for ShotSN:%d \n", i);
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
				}
				else
				{
					// Trigger QC_PROGRESS_FLAG_QC_FINISH If The Alarm Is Successfully Inserted
					intRetval = QC_TriggerQCProgressFlag(mysqlCon, intMOSN, i, QC_PROGRESS_FLAG_FINISHED);
					if (intRetval != EXIT_SUCCESS)
					{
						snprintf(charErrMsg, LONG_STRING_SIZE, "[Modbus_WriteSensorCachetoDatabase]Fail to trigger SoftSensor feature value for ShotSN:%d \n", i);
						SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
					}
					else
					{
						fprintf(stderr, "[Modbus_WriteSensorCachetoDatabase]QC of MOSN:%d ShotSN:%d has caculated successfully.\n", intMOSN, i);
					}
				}
			}

			// bermuda: Decide to Use Predict or Actual
			// Update Info
			intRetval = MO_UpdateMOSNListAfterShot(mysqlCon, intMOSN, YIELD_RATE_USE_ACTUAL);
			if (intRetval != EXIT_SUCCESS)
			{
				snprintf(charErrMsg, LONG_STRING_SIZE,
						 "[SubscribeOPCUA]Fail to update information (MO_UpdateMOSNListAfterShot) for ShotSN:%d \n", intShotSN);
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_DB, intMOSN, intIMMSN, 0, charErrMsg);
				// return EXIT_FAILURE;
			}

			// Update VIRTUAL OPCUA Server
			intRetval = MO_UpdateVirtualOPCUAServer(mysqlCon, intMOSN, -1, i);
			if (intRetval != EXIT_SUCCESS)
			{
				snprintf(charErrMsg, LONG_STRING_SIZE, "[SubscribeModbus]Fail to update info to virtual OPCUA Server(IMM.IMMSensorDataReadyShotSN).\n");
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
			}
			// Set intFeatureValueDoneShotSN = This ShotSN
			intFeatureValueDoneShotSN = i;
		}

		// Close MySQL Connection
		mysql_close(&mysqlCon);
		// usleep(MODBUS_SAMPLE_RATE*2.5);
		sleep(1);
	}
#endif
	if (doubleSensorData != NULL)
		free(doubleSensorData);
	fprintf(stderr, "[Modbus_WriteSensorCachetoDatabase]Thread Modbus_WriteSensorCachetoDatabase Terminated.\n");
	pthread_exit(NULL);
}