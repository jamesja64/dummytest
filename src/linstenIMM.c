#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <mysql/mysql.h>
#include <unistd.h>
#include "open62541.h"
#include "itri_posdss.h"
#include "config.h"

int intIMMSN = 0;
int intMOSN = 0;
int intShotSN = 0;
int intSubscribePID = 0;
int intOPCUAVersionSN = 0;
int intIMMParaSN = 0;
int intCountAcceptCriteriaSN = 0;
int intExpectedProdVolume = 0;
int *intMOAcceptCriteriaSNTHFlag;
double *doubleMOAcceptCriteriaSNTH;
bool boolIsStopSignal = false;
bool boolIsMoldClamped = false;
bool boolHasMoldClamped = false;
bool boolHasIMMParaChanged = false;
bool boolHasIMMParaChangedAfterMoldClamped = false;
int intMonIDNodeIndex[OPCUA_IMMPARA_NODE_NUM] = {0};
char charMonIDNodeIndex[OPCUA_IMMPARA_NODE_NUM][20];
char charOPCUAIMMSensorNodeList[OPCUA_IMMSENSOR_NODE_NUM + 1][OPCUA_META_NUM][MEDIUM_STRING_SIZE];

static void handler_StopSubscribeIMM(int intSign)
{
	boolIsStopSignal = true;
}

static void
subscriptionInactivityCallback(UA_Client *client, UA_UInt32 subId, void *subContext)
{
	fprintf(stderr, "Inactivity for subscription %u", subId);
}

static void inactivityCallback(UA_Client *client)
{
	fprintf(stderr, "Server Inactivity");
}

int DB_InsertIMMSensor(MYSQL mysqlCon, UA_Client *client, int intShotSN)
{
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[MEDIUM_STRING_SIZE];
	int intRetval;
	char charIMMSensorValue[OPCUA_IMMSENSOR_NODE_NUM][TINY_STRING_SIZE];

	for (int i = OPCUA_IMMPARA_NODE_NUM + 1; i <= OPCUA_IMMSENSOR_NODE_NUM + OPCUA_IMMPARA_NODE_NUM; i++)
	{
		if (charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NS] != NULL)
		{

			/*
			fprintf(stderr,"%s-%s-%s-%s\n",
			 charOPCUAIMMSensorNodeList[i-OPCUA_IMMPARA_NODE_NUM-1][OPCUA_META_NAME],
			 charOPCUAIMMSensorNodeList[i-OPCUA_IMMPARA_NODE_NUM-1][OPCUA_META_IDENTIFIER_TYPE],
			 charOPCUAIMMSensorNodeList[i-OPCUA_IMMPARA_NODE_NUM-1][OPCUA_META_NS],
			 charOPCUAIMMSensorNodeList[i-OPCUA_IMMPARA_NODE_NUM-1][OPCUA_META_ID]);
			*/

			snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM - 1], TINY_STRING_SIZE, "%s", "NULL");

			UA_Variant *readVariant = UA_Variant_new();

			if (strcmp(charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_IDENTIFIER_TYPE], "i") == 0)
			{
				intRetval = UA_Client_readValueAttribute(client,
														 UA_NODEID_NUMERIC(atoi(charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NS]),
																		   atoi(charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_ID])),
														 readVariant);
			}
			else if (strcmp(charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_IDENTIFIER_TYPE], "s") == 0)
			{
				intRetval = UA_Client_readValueAttribute(client,
														 UA_NODEID_STRING(atoi(charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NS]),
																		  charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_ID]),
														 readVariant);
			}

			if (intRetval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(readVariant))
			{
				if (readVariant->type == &UA_TYPES[UA_TYPES_BOOLEAN])
				{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
					printf("IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%d\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NAME],
						   *(UA_Boolean *)readVariant->data);
#endif
					snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM - 1], TINY_STRING_SIZE, "'%d'", *(UA_Boolean *)readVariant->data);
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_FLOAT])
				{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
					printf("IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%f\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NAME],
						   *(UA_Float *)readVariant->data);
#endif
					snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM - 1], TINY_STRING_SIZE, "'%f'", *(UA_Float *)readVariant->data);
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_DOUBLE])
				{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
					printf("IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%f\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NAME],
						   *(UA_Double *)readVariant->data);
#endif
					snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM - 1], TINY_STRING_SIZE, "'%f'", *(UA_Double *)readVariant->data);
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_INT16])
				{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
					printf("IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%d\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NAME],
						   *(UA_Int16 *)readVariant->data);
#endif
					snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM - 1], TINY_STRING_SIZE, "'%d'", *(UA_Int16 *)readVariant->data);
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_UINT16])
				{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
					printf("IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%d\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NAME],
						   *(UA_UInt16 *)readVariant->data);
#endif
					snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM - 1], TINY_STRING_SIZE, "'%d'", *(UA_UInt16 *)readVariant->data);
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_INT32])
				{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
					printf("IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%d\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NAME],
						   *(UA_Int32 *)readVariant->data);
#endif
					snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM - 1], TINY_STRING_SIZE, "'%d'", *(UA_Int32 *)readVariant->data);
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_UINT32])
				{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
					printf("IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%d\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NAME],
						   *(UA_UInt32 *)readVariant->data);
#endif
					snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM - 1], TINY_STRING_SIZE, "%d", *(UA_UInt32 *)readVariant->data);
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_INT64])
				{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
					printf("IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%ld\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NAME],
						   *(UA_Int64 *)readVariant->data);
#endif
					snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM - 1], TINY_STRING_SIZE, "'%ld'", *(UA_Int64 *)readVariant->data);
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_UINT64])
				{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
					printf("IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%ld\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NAME],
						   *(UA_UInt64 *)readVariant->data);
#endif
					snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM - 1], TINY_STRING_SIZE, "'%ld'", *(UA_UInt64 *)readVariant->data);
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_STRING])
				{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
					printf("IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%s\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NAME],
						   (char *)((UA_String *)readVariant->data));
#endif
					snprintf(charIMMSensorValue[i - OPCUA_IMMPARA_NODE_NUM - 1], TINY_STRING_SIZE, "'%s'", (char *)((UA_String *)readVariant->data));
				}
				else if (readVariant->type == &UA_TYPES[UA_TYPES_DATETIME])
				{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
					UA_DateTimeStruct t = UA_DateTime_toStruct(*(UA_DateTime *)readVariant->data);
					printf("IMMSN:%d IMM Sensor[NS:%2s ID:%-60s Name:%-30s]:%4d-%02d-%02d %02d:%02d:%02d\n", intIMMSN,
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NS],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_ID],
						   charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NAME],
						   t.year, t.month, t.day, t.hour, t.min, t.sec);
#endif
					//snprintf(charIMMSensorValue[i-OPCUA_IMMPARA_NODE_NUM-1],TINY_STRING_SIZE,"%d",*(UA_Int16*)readVariant->data);
				}
			}
		}
	}

	snprintf(charStatement, MAX_STRING_SIZE, "INSERT INTO %s_%s_%s_%d_RawData_Sensor_IMM.IMMSensorSNList "
											 "(ShotSN,CycleTime,MoldClampingTime,MoldReleasingTime,InjectionTime,VPDelayTime,HoldingTime,FillingTime,VPPosition,EndHoldingPosition,"
											 "EndFillingPosition,Nozzle1Temperature,IMMOilTemperature,FallMaterialTemperature,MaxInjectionPressure,MaxVPPressure,SumInjectionPressure,"
											 "SumHoldingPressure,MaxCavityNearGatePressure,MaxCavityFarGatePressure,SumCavityPressure,SumMoldClampingForce,MaxMoldClampingForce,"
											 "BackPressure,Nozzle2Temperature,Nozzle3Temperature,Nozzle4Temperature,Nozzle5Temperature,Nozzle6Temperature,DelayFillingTime,"
											 "InMoldCoolingTemperature,OutMoldCoolingTemperature,MoldCoolingVolume,MoldReleasingPosition,ScrewSuckBackPosition,PartWeight,GateClosingTime,"
											 "MeltTemperature,MoldTemperature) VALUE "
											 "(%d,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)",
			 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, intShotSN,
			 charIMMSensorValue[0], charIMMSensorValue[1], charIMMSensorValue[2], charIMMSensorValue[3], charIMMSensorValue[4],
			 charIMMSensorValue[5], charIMMSensorValue[6], charIMMSensorValue[7], charIMMSensorValue[8], charIMMSensorValue[19],
			 charIMMSensorValue[10], charIMMSensorValue[11], charIMMSensorValue[12], charIMMSensorValue[13], charIMMSensorValue[14],
			 charIMMSensorValue[15], charIMMSensorValue[16], charIMMSensorValue[17], charIMMSensorValue[18], charIMMSensorValue[19],
			 charIMMSensorValue[20], charIMMSensorValue[21], charIMMSensorValue[22], charIMMSensorValue[23], charIMMSensorValue[24],
			 charIMMSensorValue[25], charIMMSensorValue[26], charIMMSensorValue[27], charIMMSensorValue[28], charIMMSensorValue[29],
			 charIMMSensorValue[30], charIMMSensorValue[31], charIMMSensorValue[32], charIMMSensorValue[33], charIMMSensorValue[34],
			 charIMMSensorValue[35], charIMMSensorValue[36], charIMMSensorValue[37]);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE,
				 "Fail to insertIMM sensor data to table %s_%s_%s_%d_RawData_Sensor_IMM.IMMSensorSNList for Shot:%d (%d): %s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, intShotSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int DB_InsertPreviousIMMPara(MYSQL mysqlCon)
{
	char charStatement[MAX_STRING_SIZE];
	int intRetval;
	char charErrMsg[MEDIUM_STRING_SIZE];
	char *charIMMParaSN;

	/*
	snprintf(charStatement,MAX_STRING_SIZE,"%s_%s_%s_%d_RawData_IMMPara.IMMParaSNList",
	 SYS_NAME,DATA_DATABASE_NAME,MO_DATABASE_NAME,intMOSN);
	intRetval=DB_SelectColumnMax(mysqlCon,charStatement,"IMMParaSN",&charIMMParaSN);
	intIMMParaSN=atoi(charIMMParaSN)+1;
	*/

	//Select Last IMMPara from POSDSS_DATA_MO_[MOSN]_RawData_IMMPara.IMMParaSNList
	snprintf(charStatement, MAX_STRING_SIZE, "INSERT INTO %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList "
											 "(IMMParaSN,RecomIMMParaSN,MeltTemperature,ShotVolume1,ShotVolume2,ShotVolume3,ShotVolume4,ShotVolume5,"
											 "InjectionPressure1,InjectionPressure2,InjectionPressure3,InjectionPressure4,InjectionPressure5,"
											 "InjectionSpeed1,InjectionSpeed2,InjectionSpeed3,InjectionSpeed4,InjectionSpeed5,VPPosition,VPTime,"
											 "PackingPressure,HoldingPressure1,HoldingPressure2,HoldingPressure3,HoldingPressure4,HoldingPressure5,"
											 "PackingTime,HoldingTime1,HoldingTime2,HoldingTime3,HoldingTime4,HoldingTime5,CoolingTime,"
											 "ScrewRPM1,ScrewRPM2,ScrewRPM3,ScrewRPM4,ScrewRPM5,BackPressure1,BackPressure2,BackPressure3,BackPressure4,"
											 "BackPressure5,MoldTemperature) SELECT "
											 "%d,NULL,MeltTemperature,ShotVolume1,ShotVolume2,ShotVolume3,ShotVolume4,ShotVolume5,"
											 "InjectionPressure1,InjectionPressure2,InjectionPressure3,InjectionPressure4,InjectionPressure5,"
											 "InjectionSpeed1,InjectionSpeed2,InjectionSpeed3,InjectionSpeed4,InjectionSpeed5,VPPosition,VPTime,"
											 "PackingPressure,HoldingPressure1,HoldingPressure2,HoldingPressure3,HoldingPressure4,HoldingPressure5,"
											 "PackingTime,HoldingTime1,HoldingTime2,HoldingTime3,HoldingTime4,HoldingTime5,CoolingTime,"
											 "ScrewRPM1,ScrewRPM2,ScrewRPM3,ScrewRPM4,ScrewRPM5,BackPressure1,BackPressure2,BackPressure3,BackPressure4,"
											 "BackPressure5,MoldTemperature "
											 "FROM %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList WHERE IMMParaSN=%d",
			 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
			 intIMMParaSN,
			 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, intIMMParaSN - 1);

	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE,
				 "Fail to select/insert previous IMMPara:%d row from table %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList (%d): %s",
				 intIMMParaSN - 1, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

static void handler_IMMChanged(UA_Client *client, UA_UInt32 subId, void *subContext,
							   UA_UInt32 monId, void *monContext, UA_DataValue *value)
{

	MYSQL mysqlCon;
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[MEDIUM_STRING_SIZE];
	int intRetval;
	int intErrShot;
	char *charIMMParaSN;

	mysql_init(&mysqlCon);
	if (!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD, NULL, 0, NULL, CLIENT_FOUND_ROWS))
	{
		if (mysql_errno(&mysqlCon))
			fprintf(stderr, "Fail to connect to MySql server %d: %s", mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		return;
	}

	if (intMonIDNodeIndex[monId] != IMMPARA_DI_MOLD_CLAMPED && intMonIDNodeIndex[monId] != IMMPARA_DI_MOLD_RELEASED && boolHasIMMParaChanged == false)
	{
		boolHasIMMParaChanged = true;
		DB_InsertPreviousIMMPara(mysqlCon);
	}

	if (intMonIDNodeIndex[monId] != IMMPARA_DI_MOLD_CLAMPED && intMonIDNodeIndex[monId] != IMMPARA_DI_MOLD_RELEASED && boolIsMoldClamped == true)
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]IMMSN:%d ShotSN:%d Parameter:%s is changed after mold clamped", intIMMSN, intShotSN, charMonIDNodeIndex[monId]);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		boolHasIMMParaChangedAfterMoldClamped = true;
	}

	switch (intMonIDNodeIndex[monId])
	{
		/*
		case IMMSTATE_CURRENT_TIME:{
			#ifdef DEBUG_MODE_SUBSCRIBEIMM_CURRENT_TIME
			UA_DateTimeStruct t=UA_DateTime_toStruct(*(UA_DateTime*)value->value.data);
			fprintf(stderr,"[SubID:%d MonID:%3d]NodeName:%-20s:%4d-%02d-%02d %02d:%02d:%02d\n",
			 subId,monId,charMonIDNodeIndex[monId],t.year,t.month,t.day,t.hour+SYS_CONFIG_TIMEOFFSET,t.min,t.sec);
			#endif
		}break;*/

	case IMMPARA_DI_MOLD_CLAMPED:
	{
		if (*(UA_Boolean *)value->value.data == 1)
		{
			boolIsMoldClamped = true;
			boolHasMoldClamped = true;

			//Insert a New Shot to POSDSS_DATA_MO_[MOSN]_RawData.ShotSNList
			snprintf(charStatement, MAX_STRING_SIZE,
					 "INSERT INTO %s_%s_%s_%d_RawData.ShotSNList "
					 "(ShotSN,StartTime,IMMParaSN,MOSensorTableSN,ErrShot) VALUE "
					 //TODO File Split
					 "(%d,NOW(),%d,1,%d)",
					 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME,
					 intMOSN, intShotSN, boolHasIMMParaChanged == true ? intIMMParaSN : intIMMParaSN - 1, ERRSHOT_NO_END);
			intRetval = mysql_query(&mysqlCon, charStatement);
			if (intRetval)
			{
				snprintf(charErrMsg, MEDIUM_STRING_SIZE,
						 "Fail to insert new ShotSN:%d while mold clamped to table %s_%s_%s_%d_RawData.ShotSNList (%d): %s",
						 intShotSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
						 mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
				return;
			}
		}
		else
		{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
//fprintf(stderr,"The mold of IMMSN:%d is releasing\n",intIMMSN);
#endif
			boolIsMoldClamped = false;
			if (boolHasMoldClamped)
			{
				//Record IMM Sensor
				DB_InsertIMMSensor(mysqlCon, client, intShotSN);
				//Performance Accept Criteria Inference
				//intRetval=NN_AcceptCriteriaInference(mysqlCon,intMOSN,intIMMSN,intExpectedProdVolume,intCountAcceptCriteriaSN,intShotSN,
				// intMOAcceptCriteriaSNTHFlag,doubleMOAcceptCriteriaSNTH);
			}
		}
	}
	break;

	case IMMPARA_DI_MOLD_RELEASED:
	{
		if (*(UA_Boolean *)value->value.data == 1)
		{
			//If the mold is released and has been clamped (A complete shot)
			if (boolHasMoldClamped)
			{
				boolHasMoldClamped = false;

				//Update ShotSN=ShotSN+1 in POSDSS.Data.IMMList
				/*
					snprintf(charStatement,MAX_STRING_SIZE,"UPDATE %s_%s.IMMList SET ShotSN=%d, IMMLastUpdateTime=NOW() WHERE IMMSN=%d",
					 SYS_NAME,DATA_DATABASE_NAME,intShotSN+1,intIMMSN);
					intRetval=mysql_query(&mysqlCon,charStatement);
					if(intRetval){
					snprintf(charErrMsg,MEDIUM_STRING_SIZE,"[SubscribeIMM]Fail to update ShotSN in table %s_%s.IMMList (%d): %s",
						 SYS_NAME,DATA_DATABASE_NAME,mysql_errno(&mysqlCon),mysql_error(&mysqlCon));
					SYS_InsertSysErrMsg(mysqlCon,ERRCLASS_IMM,intMOSN,intIMMSN,mysql_errno(&mysqlCon),charErrMsg);
					return;
					}*/

				if (boolHasIMMParaChanged == true)
				{
					char *charIMMParaSN;
					snprintf(charStatement, MAX_STRING_SIZE, "%s_%s_%s_%d_RawData_IMMPara.IMMParaSNList",
							 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
					intRetval = DB_SelectColumnMax(mysqlCon, charStatement, "IMMParaSN", &charIMMParaSN);
					intIMMParaSN = atoi(charIMMParaSN) + 1;
					boolHasIMMParaChanged = false;
				}

				if (boolHasIMMParaChangedAfterMoldClamped == true)
				{
					intErrShot = ERRSHOT_IMMPARA;
					boolHasIMMParaChangedAfterMoldClamped = false;
				}
				else
					intErrShot = ERRSHOT_NONE;

				//Update the New Shot to POSDSS_DATA_MO_[MOSN]_RawData.ShotSNList
				snprintf(charStatement, MAX_STRING_SIZE,
						 "UPDATE %s_%s_%s_%d_RawData.ShotSNList SET EndTime=NOW(),"
						 "CycleTime=TIMESTAMPDIFF(Second,StartTime,NOW()),ErrShot=%d WHERE ShotSN=%d",
						 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME,
						 intMOSN, intErrShot, intShotSN);
				intRetval = mysql_query(&mysqlCon, charStatement);
				if (intRetval)
				{
					snprintf(charErrMsg, MEDIUM_STRING_SIZE,
							 "Fail to update ShotSN:%d while mold releaseded to table %s_%s_%s_%d_RawData.ShotSNList (%d): %s",
							 intShotSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
					return;
				}

				//Performance Accept Criteria Inference
				//intRetval=NN_AcceptCriteriaInference(mysqlCon,intMOSN,intIMMSN,intExpectedProdVolume,intCountAcceptCriteriaSN,intShotSN,
				// intMOAcceptCriteriaSNTHFlag,doubleMOAcceptCriteriaSNTH);

				//Update ShotSN=ShotSN+1 in POSDSS.Data.IMMList
				snprintf(charStatement, MAX_STRING_SIZE,
						 "UPDATE %s_%s.IMMList SET ShotSN=%d, MOProgress=%lf, YieldRate=AcceptPart/(AcceptPart+RejectPart+1), "
						 "IMMLastUpdateTime=NOW() WHERE IMMSN=%d",
						 SYS_NAME, DATA_DATABASE_NAME, intShotSN + 1, ((double)intShotSN / (double)intExpectedProdVolume), intIMMSN);
				intShotSN++;
				fprintf(stderr, "[ShotSN:%d]\n", intShotSN);
				intRetval = mysql_query(&mysqlCon, charStatement);
				if (intRetval)
				{
					snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Fail to update ShotSN in table %s_%s.IMMList (%d): %s",
							 SYS_NAME, DATA_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
					return;
				}
			}
		}
		else if (*(UA_Boolean *)value->value.data == 0)
		{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
//fprintf(stderr,"The mold of IMMSN:%d is releasing\n",intIMMSN);
#endif
		}
	}
	break;

	case IMMPARA_DI_ALARM_LAMP:
	case IMMPARA_DI_STOPPED_LAMP:
	case IMMPARA_DI_RUNNING_LAMP:
	case IMMPARA_DI_MOTOR_START:
	case IMMPARA_DI_OPERATION_MODE:
	case IMMPARA_DI_HEAT_ON:
		break;

	default:
	{
		if (value->value.type->typeIndex == UA_TYPES_FLOAT)
		{
			snprintf(charStatement, MAX_STRING_SIZE,
					 "INSERT INTO %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList (IMMParaSN,%s) VALUE (%d,%f) "
					 "ON DUPLICATE KEY UPDATE %s=%f",
					 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, charMonIDNodeIndex[monId],
					 intIMMParaSN, *(UA_Float *)value->value.data, charMonIDNodeIndex[monId], *(UA_Float *)value->value.data);
		}
		else if (value->value.type->typeIndex == UA_TYPES_DOUBLE)
		{
			snprintf(charStatement, MAX_STRING_SIZE,
					 "INSERT INTO %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList (IMMParaSN,%s) VALUE (%d,%f) "
					 "ON DUPLICATE KEY UPDATE %s=%f",
					 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, charMonIDNodeIndex[monId],
					 intIMMParaSN, *(UA_Double *)value->value.data, charMonIDNodeIndex[monId], *(UA_Double *)value->value.data);
		}
		else if (value->value.type->typeIndex == UA_TYPES_INT16)
		{
			snprintf(charStatement, MAX_STRING_SIZE,
					 "INSERT INTO %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList (IMMParaSN,%s) VALUE (%d,%d) "
					 "ON DUPLICATE KEY UPDATE %s=%d",
					 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, charMonIDNodeIndex[monId],
					 intIMMParaSN, *(UA_Int16 *)value->value.data, charMonIDNodeIndex[monId], *(UA_Int16 *)value->value.data);
		}
		else if (value->value.type->typeIndex == UA_TYPES_UINT16)
		{
			snprintf(charStatement, MAX_STRING_SIZE,
					 "INSERT INTO %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList (IMMParaSN,%s) VALUE (%d,%d) "
					 "ON DUPLICATE KEY UPDATE %s=%d",
					 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, charMonIDNodeIndex[monId],
					 intIMMParaSN, *(UA_UInt16 *)value->value.data, charMonIDNodeIndex[monId], *(UA_UInt16 *)value->value.data);
		}
		else if (value->value.type->typeIndex == UA_TYPES_INT32)
		{
			snprintf(charStatement, MAX_STRING_SIZE,
					 "INSERT INTO %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList (IMMParaSN,%s) VALUE (%d,%d) "
					 "ON DUPLICATE KEY UPDATE %s=%d",
					 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, charMonIDNodeIndex[monId],
					 intIMMParaSN, *(UA_Int32 *)value->value.data, charMonIDNodeIndex[monId], *(UA_Int32 *)value->value.data);
		}
		else if (value->value.type->typeIndex == UA_TYPES_UINT32)
		{
			snprintf(charStatement, MAX_STRING_SIZE,
					 "INSERT INTO %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList (IMMParaSN,%s) VALUE (%d,%d) "
					 "ON DUPLICATE KEY UPDATE %s=%d",
					 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, charMonIDNodeIndex[monId],
					 intIMMParaSN, *(UA_UInt32 *)value->value.data, charMonIDNodeIndex[monId], *(UA_UInt32 *)value->value.data);
		}
		else if (value->value.type->typeIndex == UA_TYPES_INT64)
		{
			snprintf(charStatement, MAX_STRING_SIZE,
					 "INSERT INTO %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList (IMMParaSN,%s) VALUE (%d,%ld) "
					 "ON DUPLICATE KEY UPDATE %s=%ld",
					 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, charMonIDNodeIndex[monId],
					 intIMMParaSN, *(UA_Int64 *)value->value.data, charMonIDNodeIndex[monId], *(UA_Int64 *)value->value.data);
		}
		else if (value->value.type->typeIndex == UA_TYPES_UINT64)
		{
			snprintf(charStatement, MAX_STRING_SIZE,
					 "INSERT INTO %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList (IMMParaSN,%s) VALUE (%d,%ld) "
					 "ON DUPLICATE KEY UPDATE %s=%ld",
					 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, charMonIDNodeIndex[monId],
					 intIMMParaSN, *(UA_UInt64 *)value->value.data, charMonIDNodeIndex[monId], *(UA_UInt64 *)value->value.data);
		}
		intRetval = mysql_query(&mysqlCon, charStatement);
		if (intRetval)
		{
			snprintf(charErrMsg, MEDIUM_STRING_SIZE,
					 "Fail to insert/update %s in table %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList (%d): %s",
					 charMonIDNodeIndex[monId], SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
			return;
		}
	}
	}

	//Update POSDSS_Data.IMMList Set IMMLastUpdateTime as NOW Where IMMSN Is Given
	snprintf(charStatement, MAX_STRING_SIZE, "UPDATE %s_%s.IMMList SET IMMLastUpdateTime=NOW() WHERE IMMSN=%d",
			 SYS_NAME, DATA_DATABASE_NAME, intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE,
				 "Fail to update IMMLastUpdateTime in table %s_%s.IMMList for IMMSN:%d (%d): %s",
				 SYS_NAME, DATA_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return;
	}

#ifdef DEBUG_MODE_SUBSCRIBEIMM_MONITOR_ITEM
	if (value->value.type->typeIndex == UA_TYPES_DATETIME)
	{
		UA_DateTimeStruct t = UA_DateTime_toStruct(*(UA_DateTime *)value->value.data);
		fprintf(stderr, "IMMSN:%d IMM Parameter [SubID:%d MonID:%3d NodeName:%-20s:%4d-%02d-%02d %02d:%02d:%02d]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], t.year, t.month, t.day, t.hour, t.min, t.sec);
	}
	else if (value->value.type->typeIndex == UA_TYPES_BOOLEAN)
	{
		fprintf(stderr, "IMMSN:%d IMM Parameter [SubID:%d MonID:%3d NodeName:%-20s:%d]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Boolean *)value->value.data);
	}
	else if (value->value.type->typeIndex == UA_TYPES_FLOAT)
	{
		fprintf(stderr, "IMMSN:%d IMM Parameter [SubID:%d MonID:%3d NodeName:%-20s:%f]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Float *)value->value.data);
	}
	else if (value->value.type->typeIndex == UA_TYPES_DOUBLE)
	{
		fprintf(stderr, "IMMSN:%d IMM Parameter [SubID:%d MonID:%3d NodeName:%-20s:%f]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Double *)value->value.data);
	}
	else if (value->value.type->typeIndex == UA_TYPES_INT16)
	{
		fprintf(stderr, "IMMSN:%d IMM Parameter [SubID:%d MonID:%3d NodeName:%-20s:%d]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Int16 *)value->value.data);
	}
	else if (value->value.type->typeIndex == UA_TYPES_UINT16)
	{
		fprintf(stderr, "IMMSN:%d IMM Parameter [SubID:%d MonID:%3d NodeName:%-20s:%d]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_UInt16 *)value->value.data);
	}
	else if (value->value.type->typeIndex == UA_TYPES_INT32)
	{
		fprintf(stderr, "IMMSN:%d IMM Parameter [SubID:%d MonID:%3d NodeName:%-20s:%d]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Int32 *)value->value.data);
	}
	else if (value->value.type->typeIndex == UA_TYPES_UINT32)
	{
		fprintf(stderr, "IMMSN:%d IMM Parameter [SubID:%d MonID:%3d NodeName:%-20s:%d]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_UInt32 *)value->value.data);
	}
	else if (value->value.type->typeIndex == UA_TYPES_INT64)
	{
		fprintf(stderr, "IMMSN:%d IMM Parameter [SubID:%d MonID:%3d NodeName:%-20s:%ld]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Int64 *)value->value.data);
	}
	else if (value->value.type->typeIndex == UA_TYPES_UINT64)
	{
		fprintf(stderr, "IMMSN:%d IMM Parameter [SubID:%d MonID:%3d NodeName:%-20s:%ld]\n",
				intIMMSN, subId, monId, charMonIDNodeIndex[monId], *(UA_Int64 *)value->value.data);
	}
#endif

	mysql_close(&mysqlCon);
	return;
}

int subscribeIMM(UA_Client *client, MYSQL mysqlCon)
{

	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[MEDIUM_STRING_SIZE];
	int intRetval;
	//int	intOPCUAVersionSN;

	UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
	request.requestedPublishingInterval = 1000;
	//request.requestedPublishingInterval=3;
	//request.requestedMaxKeepAliveCount=1;
	//request.maxNotificationsPerPublish=1;
	//request.publishingEnabled=false;

	UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
	if (response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
	{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
		fprintf(stderr, "Create subscription succeeded, id %u\n", response.subscriptionId);
#endif
	}
	else
	{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
		fprintf(stderr, "Create subscription failed\n");
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
	snprintf(charStatement,MAX_STRING_SIZE,"SELECT OPCUAVersionSN FROM %s_%s.IMMList WHERE IMMSN=%d",
	 SYS_NAME,DATA_DATABASE_NAME,intIMMSN);
	intRetval=mysql_query(&mysqlCon,charStatement);
	if(intRetval){
		snprintf(charErrMsg,MEDIUM_STRING_SIZE,"[SubscribeIMM]Fail to select OPCUAVersionSN from table %s_%s.IMMList (%d): %s",
		 SYS_NAME,DATA_DATABASE_NAME,mysql_errno(&mysqlCon),mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon,ERRCLASS_IMM,intMOSN,intIMMSN,mysql_errno(&mysqlCon),charErrMsg);
		return EXIT_FAILURE;
	}

	MYSQL_RES *mysqlResult=mysql_store_result(&mysqlCon);
	MYSQL_ROW mysqlRow=mysql_fetch_row(mysqlResult);
	mysql_free_result(mysqlResult);
	intOPCUAVersionSN=atoi(mysqlRow[0]);
	*/

	//Select Node NameSpace and ID Where OPCUAVersionSN Is Given
	snprintf(charStatement, MAX_STRING_SIZE,
			 "SELECT * FROM %s_%s.OPCUAVersionIndex WHERE OPCUAVersionSN=%d", SYS_NAME, INDEX_DATABASE_NAME, intOPCUAVersionSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Fail to select OPCUAVersionSN from table %s_%s.IMMList (%d): %s",
				 SYS_NAME, DATA_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return EXIT_FAILURE;
	}

	MYSQL_RES *mysqlResult = mysql_store_result(&mysqlCon);
	MYSQL_ROW mysqlRow = mysql_fetch_row(mysqlResult);
	for (int i = 1; i <= OPCUA_IMMPARA_NODE_NUM; i++)
	{
		if (mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NS] != NULL &&
			(strcmp(mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_IDENTIFIER_TYPE], "i") == 0 ||
			 strcmp(mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_IDENTIFIER_TYPE], "s") == 0))
		{
			//fprintf(stderr,"i=%d IMMSN:%d is trying to monitor %s[%s:%s], id %u\n",i,intIMMSN,
			// mysqlRow[OPCUA_VERSION_META_NUM+(i-1)*OPCUA_META_NUM+OPCUA_META_NAME],
			// mysqlRow[OPCUA_VERSION_META_NUM+(i-1)*OPCUA_META_NUM+OPCUA_META_NS],
			// mysqlRow[OPCUA_VERSION_META_NUM+(i-1)*OPCUA_META_NUM+OPCUA_META_ID],monResponse.monitoredItemId);
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

			monResponse = UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
																	UA_TIMESTAMPSTORETURN_BOTH, monRequest, NULL, handler_IMMChanged, NULL);
			if (monResponse.statusCode == UA_STATUSCODE_GOOD)
			{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
				fprintf(stderr, "IMMSN:%d is monitoring [NS:%2s ID:%-65s Name:%-20s], id %u\n", intIMMSN,
						mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NS],
						mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_ID],
						mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NAME], monResponse.monitoredItemId);
#endif
				strcpy(charMonIDNodeIndex[monResponse.monitoredItemId], mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NAME]);
				intMonIDNodeIndex[monResponse.monitoredItemId] = i;
			}
			else
			{
				snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]IMMSN:%d fails to monitor [%s:%s], id %u", intIMMSN,
						 mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NS],
						 mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_ID], monResponse.monitoredItemId);
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
			}
		}
	}

	for (int i = OPCUA_IMMPARA_NODE_NUM + 1; i <= OPCUA_IMMSENSOR_NODE_NUM + OPCUA_IMMPARA_NODE_NUM; i++)
	{
		strcpy(charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NAME],
			   mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NAME]);
		if (mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NS] != NULL)
		{
			strcpy(charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_IDENTIFIER_TYPE],
				   mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_IDENTIFIER_TYPE]);
			strcpy(charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_NS],
				   mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_NS]);
			strcpy(charOPCUAIMMSensorNodeList[i - OPCUA_IMMPARA_NODE_NUM - 1][OPCUA_META_ID],
				   mysqlRow[OPCUA_VERSION_META_NUM + (i - 1) * OPCUA_META_NUM + OPCUA_META_ID]);
		}
	}
	mysql_free_result(mysqlResult);

#ifdef DEBUG_MODE_SUBSCRIBEIMM_CURRENT_TIME
	monRequest = UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, 2258));
	monResponse = UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
															UA_TIMESTAMPSTORETURN_BOTH, monRequest, NULL, handler_IMMChanged, NULL);
	if (monResponse.statusCode == UA_STATUSCODE_GOOD)
	{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
		fprintf(stderr, "IMMSN:%d is monitoring [0:2258], id %u\n", intIMMSN, monResponse.monitoredItemId);
#endif
		intMonIDNodeIndex[monResponse.monitoredItemId] = IMMSTATE_CURRENT_TIME;
	}
	else
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]IMMSN:%d fails to monitor [0:2258], id %u",
				 intIMMSN, monResponse.monitoredItemId);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
	}
#endif

	return EXIT_SUCCESS;
}

static void stateCallback(UA_Client *client, UA_ClientState clientState)
{
	MYSQL mysqlCon;
	int intRetval;
	char charErrMsg[MEDIUM_STRING_SIZE];
	char charStatement[MAX_STRING_SIZE];

	//Check Client State
	switch (clientState)
	{
	case UA_CLIENTSTATE_DISCONNECTED:
	{
//The client is disconnected
#ifdef DEBUG_MODE_SUBSCRIBEIMM
		fprintf(stderr, "[IMMSN:%d]OPCUA client is disconnected\n", intIMMSN);
#endif
	}
	break;
	case UA_CLIENTSTATE_SESSION:
	{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
		fprintf(stderr, "[IMMSN:%d]A session with the server is open\n", intIMMSN);
#endif

		//Connect to MYSQL Server
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

		//Update OPCUAStatus=RUNNING,SubscribePID and IMMLastUpdateTime to POSDSS_Data.IMMList
		snprintf(charStatement, MAX_STRING_SIZE,
				 "UPDATE %s_%s.IMMList SET SubscribePID=%d,OPCUAStatus=%d,IMMLastUpdateTime=NOW() WHERE IMMSN=%d",
				 SYS_NAME, DATA_DATABASE_NAME, getpid(), OPCUA_STATUS_RUNNING, intIMMSN);
		intRetval = mysql_query(&mysqlCon, charStatement);
		if (intRetval)
		{
			snprintf(charErrMsg, MEDIUM_STRING_SIZE,
					 "Fail to update SubscribePID,OPCUAStatus and IMMLastUpdateTime for IMMSN=%d (%d):%s",
					 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
			return;
		}
		subscribeIMM(client, mysqlCon);
		mysql_close(&mysqlCon);
	}
	break;
	}
	return;
}

int main(int argc, char *argv[])
{
	char charOPCUAIP[15];
	int intOPCUAPort;
	int intIMMModelSN;
	int intRetval;
	char charEndPointURL[MEDIUM_STRING_SIZE];
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[MEDIUM_STRING_SIZE];
	char charTableName[MEDIUM_STRING_SIZE];
	char *charShotSN;
	char *charIMMParaSN;
	bool boolHasInsertErrMsg = false;
	MYSQL mysqlCon;

	signal(SIGINT, handler_StopSubscribeIMM);
	signal(SIGTERM, handler_StopSubscribeIMM);

	//Connect to MYSQL Server
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

	//Get IMMSN from Parameters
	if (argc == 3 && strcmp(argv[1], "-IMMSN") == 0)
	{
		intIMMSN = atoi(argv[2]);
	}
	else
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Fail to subscribe IMM due to parameter format error");
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, 0, intIMMSN, EXIT_FAILURE, charErrMsg);
		return EXIT_FAILURE;
	}

	//Select IMM Model, MOSN andOPCUA Information According to The Given IMMSN
	snprintf(charStatement, MAX_STRING_SIZE, "SELECT IMMModelSN,MOSN,OPCUAVersionSN,OPCUAIP,OPCUAPort,SubscribePID,ShotSN FROM %s_%s.IMMList WHERE IMMSN=%d", SYS_NAME, DATA_DATABASE_NAME, intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Fail to select information where IMMSN=%d from %s_%s.IMMList (%d):%s",
				 intIMMSN, SYS_NAME, DATA_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, 0, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return EXIT_FAILURE;
	}

	MYSQL_RES *mysqlResult = mysql_store_result(&mysqlCon);
	MYSQL_ROW mysqlRow = mysql_fetch_row(mysqlResult);
	if (mysqlRow == NULL)
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Fail to subscribe IMM parameter due to IMMSN=%d is not found in %s_%s.IMMList",
				 intIMMSN, SYS_NAME, DATA_DATABASE_NAME);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, 0, charErrMsg);
		return EXIT_FAILURE;
	}

	//Get IMMModelSN
	if (mysqlRow[0] == NULL)
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Fail to select IMMModelSN of IMMSN:%d", intIMMSN);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, 0, intIMMSN, EXIT_FAILURE, charErrMsg);
		return EXIT_FAILURE;
	}
	else
		intIMMModelSN = atoi(mysqlRow[0]);

	//Get MOSN
	if (mysqlRow[1] == NULL)
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Fail to select MOSN for IMMSN:%d", intIMMSN);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, 0, intIMMSN, EXIT_FAILURE, charErrMsg);
		return EXIT_FAILURE;
	}
	else
		intMOSN = atoi(mysqlRow[1]);

	//Get OPCUAVersionSN
	if (mysqlRow[2] == NULL)
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Fail to select OPCUAVersionSN for IMMSN:%d", intIMMSN);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
		return EXIT_FAILURE;
	}
	else
		intOPCUAVersionSN = atoi(mysqlRow[2]);

	//Get OPC UA IP
	if (mysqlRow[3] == NULL)
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Fail to select OPCUA IP of IMMSN:%d", intIMMSN);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
		return EXIT_FAILURE;
	}
	else
		strcpy(charOPCUAIP, mysqlRow[3]);

	//Get OPC UA Port
	if (mysqlRow[4] == NULL)
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Fail to select OPC UA Port of IMMSN:%d", intIMMSN);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);
		return EXIT_FAILURE;
	}
	else
		intOPCUAPort = atoi(mysqlRow[4]);

	//Get Subscribe Procress ID
	if (mysqlRow[5] != NULL)
	{
		intSubscribePID = atoi(mysqlRow[5]);

		//Notify An Existing Subscribe Porcess Is Executing
		snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]A subscribe process PID:%d of IMMSN:%d is executing", intSubscribePID, intIMMSN);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, EXIT_FAILURE, charErrMsg);

		//Kill the Subscribe Procress
		intRetval = kill(intSubscribePID, SIGTERM);
		if (intRetval != EXIT_SUCCESS)
		{
			snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Fail to kill subscribe process PID:%d of IMMSN:%d.", intSubscribePID, intIMMSN);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);

			snprintf(charStatement, MAX_STRING_SIZE, "UPDATE %s_%s.IMMList SET SubscribePID=NULL WHERE IMMSN=%d",
					 SYS_NAME, DATA_DATABASE_NAME, intIMMSN);
			intRetval = mysql_query(&mysqlCon, charStatement);
			if (intRetval)
			{
				snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Fail to Update SubscribePID=NULL for IMMSN=%d while terminating (%d):%s",
						 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
				return EXIT_FAILURE;
			}
		}
		else
		{
			snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]The subscribe process PID:%d of IMMSN:%d is terminated", intSubscribePID, intIMMSN);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		}

		bool boolHasInsertError = false;
		int intTerminateInteval = 5;
		//Wait For The SubscribePID = NULL
		do
		{
			snprintf(charStatement, MAX_STRING_SIZE, "SELECT SubscribePID FROM %s_%s.IMMList WHERE IMMSN=%d",
					 SYS_NAME, DATA_DATABASE_NAME, intIMMSN);
			intRetval = mysql_query(&mysqlCon, charStatement);
			if (intRetval && boolHasInsertError == false)
			{
				snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Fail to select SubscribePID for IMMSN=%d while exiting (%d):%s",
						 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
				boolHasInsertError = true;
			}
			mysqlResult = mysql_store_result(&mysqlCon);
			mysqlRow = mysql_fetch_row(mysqlResult);
			sleep(1);
			intTerminateInteval--;
			if (intTerminateInteval < 0)
			{
				snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Subscribtion terminated. Fail to kill subscribe process PID:%d of IMMSN:%d",
						 intSubscribePID, intIMMSN);
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
				return EXIT_FAILURE;
			}
		} while (mysqlRow[0] != NULL && intTerminateInteval >= 0);
	}
	mysql_free_result(mysqlResult);

	//Select MO Meta Information from POSDSS_Data_MO.MOList
	snprintf(charStatement, MAX_STRING_SIZE, "SELECT ExpectedProdVolume FROM %s_%s_%s.MOList",
			 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Fail to select MO meta information from %s_%s_%s.MOList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return EXIT_FAILURE;
	}
	mysqlResult = mysql_store_result(&mysqlCon);
	mysqlRow = mysql_fetch_row(mysqlResult);
	intExpectedProdVolume = atoi(mysqlRow[0]);
	mysql_free_result(mysqlResult);

	//Select MAX AcceptCriteriaSN from POSDSS_Data_MO_[MOSN]_Info_Meta.MOAcceptCriteriaSNList
	snprintf(charStatement, MAX_STRING_SIZE,
			 "SELECT MOAcceptCriteriaSN,AcceptCriteriaClass,AcceptCriteriaMinTH,AcceptCriteriaMaxTH FROM %s_%s_%s_%d_Info_Meta.MOAcceptCriteriaSNList", SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Fail to select information from %s_%s_MO_%d_Info_Meta.MOAcceptCriteriaSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return EXIT_FAILURE;
	}
	mysqlResult = mysql_store_result(&mysqlCon);
	intCountAcceptCriteriaSN = mysql_num_rows(mysqlResult);
	intMOAcceptCriteriaSNTHFlag = (int *)malloc(intCountAcceptCriteriaSN * 4 * sizeof(int));
	doubleMOAcceptCriteriaSNTH = (double *)malloc(intCountAcceptCriteriaSN * 2 * sizeof(double));
	for (int i = 0; mysqlRow = mysql_fetch_row(mysqlResult); i++)
	{
		//mysqlRow=mysql_fetch_row(mysqlResult);
		*(intMOAcceptCriteriaSNTHFlag + i * 3) = atoi(mysqlRow[0]);
		*(intMOAcceptCriteriaSNTHFlag + i * 3 + 1) = atoi(mysqlRow[1]);
		*(intMOAcceptCriteriaSNTHFlag + i * 3 + 2) = 0;

		//If AcceptCriteriaMinTH!=NULL
		if (mysqlRow[2] != NULL)
		{
			*(intMOAcceptCriteriaSNTHFlag + i * 2 + 2) = 1;
			*(doubleMOAcceptCriteriaSNTH + i * 2) = (double)atof(mysqlRow[2]);
		}

		//If AcceptCriteriaMaxTH!=NULL
		if (mysqlRow[3] != NULL)
		{
			*(intMOAcceptCriteriaSNTHFlag + i * 2 + 2) += 2;
			*(doubleMOAcceptCriteriaSNTH + i * 2 + 1) = (double)atof(mysqlRow[3]);
		}
	}
	mysqlRow = mysql_fetch_row(mysqlResult);
	mysql_free_result(mysqlResult);

	//Update OPCUAStatus,SubscribePID and IMMLastUpdateTime to POSDSS_Data.IMMList
	snprintf(charStatement, MAX_STRING_SIZE, "UPDATE %s_%s.IMMList SET OPCUAStatus=%d,SubscribePID=%d,IMMLastUpdateTime=NOW() WHERE IMMSN=%d",
			 SYS_NAME, DATA_DATABASE_NAME, OPCUA_STATUS_UNKNOW, getpid(), intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE,
				 "Fail to update OPCUAStatus=%d, SubscribePID=getpid() and IMMLastUpdateTime=NOW() where IMMSN=%d (%d):%s",
				 OPCUA_STATUS_UNKNOW, intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return EXIT_FAILURE;
	}

	//Select ShotSN From POSDSS_DATA_MO_[MOSN]_RawData.ShotSNList
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s_%s_%d_RawData.ShotSNList",
			 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	intRetval = DB_SelectColumnMax(mysqlCon, charTableName, "ShotSN", &charShotSN);
	if (intRetval == EXIT_FAILURE)
		return EXIT_FAILURE;
	intShotSN = atoi(charShotSN) + 1;
	if (intShotSN != 1)
	{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
		fprintf(stderr, "IMMSN:%d resumes the MOSN:%d from ShotSN:%d\n", intIMMSN, intMOSN, intShotSN);
#endif
	}

	//Update ShotSN to POSDSS_Data.IMMList
	snprintf(charStatement, MAX_STRING_SIZE, "UPDATE %s_%s.IMMList SET ShotSN=%d WHERE IMMSN=%d",
			 SYS_NAME, DATA_DATABASE_NAME, intShotSN, intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE,
				 "Fail to update ShotSN:%d to %s_%s.IMMList where IMMSN:%d (%d): %s",
				 intShotSN, SYS_NAME, DATA_DATABASE_NAME, intIMMSN,
				 mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		return EXIT_FAILURE;
	}

	//Select MAX IMMParaSN in POSDSS_DATA_MO_[MOSN]_RawData_IMMPara.IMMParaSNList
	snprintf(charStatement, MAX_STRING_SIZE, "%s_%s_%s_%d_RawData_IMMPara.IMMParaSNList",
			 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	intRetval = DB_SelectColumnMax(mysqlCon, charStatement, "IMMParaSN", &charIMMParaSN);

	//Close MySQL Connection
	mysql_close(&mysqlCon);

	if (intRetval == EXIT_FAILURE)
		return EXIT_FAILURE;
	intIMMParaSN = atoi(charIMMParaSN) + 1;

	UA_ClientConfig config = UA_ClientConfig_default;
	//config.inactivityCallback=inactivityCallback;
	config.stateCallback = stateCallback;
	config.subscriptionInactivityCallback = subscriptionInactivityCallback;
	//config.secureChannelLifeTime=200;
	//config.connectivityCheckInterval=2000;
	UA_Client *client = UA_Client_new(config);

	snprintf(charEndPointURL, MEDIUM_STRING_SIZE, "opc.tcp://%s:%d", charOPCUAIP, intOPCUAPort);
	while (boolIsStopSignal == false)
	{
		//fprintf(stderr,"Connect to %s...\n",charEndPointURL);
		//fprintf(stderr,"Connectstate=%d\n",UA_Client_getState(client));
		intRetval = UA_Client_connect(client, charEndPointURL);
		if (intRetval != UA_STATUSCODE_GOOD)
		{
#ifdef DEBUG_MODE_SUBSCRIBEIMM
			fprintf(stderr, "Not connected. Retrying to connect in %d ms\n", UA_CLIENT_INTERVAL);
#endif
			if (boolHasInsertErrMsg == false)
			{
				snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Fail to subscribe IMM:%d due to OPC UA connection fail", intIMMSN);
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_OPCUA, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
				boolHasInsertErrMsg = true;
			}
		}
		else
			boolHasInsertErrMsg = false;
		UA_Client_runAsync(client, UA_CLIENT_INTERVAL);
	};
	UA_Client_delete(client);

	//Connect to MYSQL Server
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

	snprintf(charStatement, MAX_STRING_SIZE, "SELECT SubscribePID FROM %s_%s.IMMList WHERE IMMSN=%d",
			 SYS_NAME, DATA_DATABASE_NAME, intIMMSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Fail to select SubscribePID for IMMSN=%d while exiting (%d):%s",
				 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
	}

	mysqlResult = mysql_store_result(&mysqlCon);
	mysqlRow = mysql_fetch_row(mysqlResult);
	if (mysqlRow[0] != NULL && atoi(mysqlRow[0]) == getpid())
	{
		snprintf(charStatement, MAX_STRING_SIZE, "UPDATE %s_%s.IMMList SET SubscribePID=NULL, OPCUAStatus=%d, IMMLastUpdateTime=NOW() WHERE IMMSN=%d",
				 SYS_NAME, DATA_DATABASE_NAME, OPCUA_STATUS_UNKNOW, intIMMSN);
		intRetval = mysql_query(&mysqlCon, charStatement);
		if (intRetval)
		{
			snprintf(charErrMsg, MEDIUM_STRING_SIZE, "[SubscribeIMM]Fail to update SubscribePID and IMMLastUpdateTime for IMMSN=%d (%d):%s",
					 intIMMSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_IMM, intMOSN, intIMMSN, mysql_errno(&mysqlCon), charErrMsg);
		}
	}
	//Free mysqlResult
	mysql_free_result(mysqlResult);

	//Close MySQL Connection
	mysql_close(&mysqlCon);

	return EXIT_SUCCESS;
}
