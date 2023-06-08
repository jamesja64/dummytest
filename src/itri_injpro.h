#ifndef __ITRI_POSDSS_H__
#define __ITRI_POSDSS_H__

#include <mysql/mysql.h>
#include <stdbool.h>
#include "config.h"
#include <time.h>
#include <math.h>
#include "open62541.h"

#define TIME_OFFSET_HOUR 8

#define SYS_NAME "INJPRO_Local"
#define LOG_DATABASE_NAME "Sys_Log"
//#define ALARM_DATABASE_NAME "Sys_Alarm"
#define INFO_DATABASE_NAME "Sys_Info"
#define CONFIG_DATABASE_NAME "Sys_Config"
#define INDEX_DATABASE_NAME "Sys_Index"
#define OPCUA_DATABASE_NAME "OPCUA"
#define DATA_DATABASE_NAME "Data"
#define MO_DATABASE_NAME "MO"
#define NN_DATABASE_NAME "NN"

#define ERRCLASS_SYS 1
#define ERRCLASS_DB 2
#define ERRCLASS_IMM 3
#define ERRCLASS_OPCUA_CLIENT 4
#define ERRCLASS_OPCUA_SERVER 6
#define ERRCLASS_MYSQL 7
#define ERRCLASS_MODBUS 8
#define ERRCLASS_WEBSERVICE 9

#define ERROR_SHOT_OK 0
#define ERROR_SHOT_IMCOMPLETE 1
#define ERROR_SHOT_EXCEED_MAX_CYCLE_TIME 2

#define EMPTY_SHOT_TRUE 1
#define EMPTY_SHOT_FALSE 0

#define IMMPARA_CHANGE_SHOT_TRUE 1
#define IMMPARA_CHANGE_SHOT_FALSE 0

#define ACCEPTCRITERIA_CATEGORY_BOOLEAN 1
#define ACCEPTCRITERIA_CATEGORY_VALUE 2
#define ACCEPTCRITERIA_LEVEL 3

#define ACCEPTCRATERIA_META_NUM 4
#define ACCEPTCRATERIA_META_SN 0
#define ACCEPTCRATERIA_META_CLASS 1
#define ACCEPTCRATERIA_META_HASMIN 2
#define ACCEPTCRATERIA_META_HASMAX 3

#define MODBUS_MOLD_SIGNAL_SOURCE_MODBUS 1
#define MODBUS_MOLD_SIGNAL_SOURCE_OPCUA 2

#define MODBUS_MOLD_SIGNAL_TYPE_SIGNAL_LOOP 1
#define MODBUS_MOLD_SIGNAL_TYPE_MULTIPLE_LOOP 2

#define IMM_SYNC_FAIL_FALSE 0
#define IMM_SYNC_FAIL_TRUE 1

#define IMM_LIGHT_STATUS_GREEN 0
#define IMM_LIGHT_STATUS_RED 1
#define IMM_LIGHT_STATUS_YELLOW 2
#define IMM_LIGHT_STATUS_UNKNOW 3

#define MODBUS_CLIENT_STATUS_RUNNING 0
#define MODBUS_CLIENT_STATUS_WAITING 1
#define MODBUS_CLIENT_STATUS_SHUTDOWN 2 //No Use
#define MODBUS_CLIENT_STATUS_UNKNOW 3

#define OPCUA_CLIENT_STATUS_RUNNING 0
#define OPCUA_CLIENT_STATUS_WAITING 1
#define OPCUA_CLIENT_STATUS_SHUTDOWN 2 //No Use
#define OPCUA_CLIENT_STATUS_UNKNOW 3

#define AI_STATUS_RUNNING 0
#define AI_STATUS_FAILED 1
#define AI_STATUS_SHUTDOWN 2
#define AI_STATUS_UNKNOW 3

#define SPC_RULE_STATUS_GOOD 0
#define SPC_RULE_STATUS_FAILED 1
#define SPC_RULE_STATUS_ALARMING 2
#define SPC_RULE_STATUS_UNKNOW 3

#define MO_STATUS_FINISHED 0
#define MO_STATUS_STOP 1
#define MO_STATUS_START 2
#define MO_STATUS_REGISTERED 3

#define MO_YIELDRATE_STATUS_GOOD 0
#define MO_YIELDRATE_STATUS_ACCEPTABLE 1
#define MO_YIELDRATE_STATUS_WORSE 2
#define MO_YIELDRATE_STATUS_UNKNOW 3

#define MO_PROGRESS_STATUS_GOOD 0
#define MO_PROGRESS_STATUS_DELAY 1
#define MO_PROGRESS_STATUS_ALARMING 2
#define MO_PROGRESS_STATUS_UNKNOW 3

#define OPCUA_META_NUM 5
#define OPCUA_META_NAME 0
#define OPCUA_META_IDENTIFIER_TYPE 1
#define OPCUA_META_NS 2
#define OPCUA_META_ID 3
#define OPCUA_META_RATIO 4

#define OPCUA_IDENTIFIER_TYPE_NUMERIC "'i'"
#define OPCUA_IDENTIFIER_TYPE_STRING "'s'"
#define OPCUA_IDENTIFIER_TYPE_GUID "'g'"
#define OPCUA_IDENTIFIER_TYPE_OPAQUE "'b'"

#define OPCUA_NAMESPACE "'1'"
#define OPCUA_VERSION_META_NUM 3 //"OPCUAVersionSN" and "OPCUAVersionID and OPCUA VersionMask"
#define OPCUA_IMMPARA_NODE_NUM 53
#define OPCUA_IMMSENSOR_NODE_NUM 42
#define OPCUA_FIRST_IMMPARA 9
#define OPCUA_FIRST_IMMSENSOR OPCUA_IMMPARA_NODE_NUM + 1

#define DO_MOLD_CLAMPED 1
#define DO_MOLD_RELEASED 2
#define DO_ALARM_LAMP 3
#define DO_STOPPED_LAMP 4
#define DO_RUNNING_LAMP 5
#define DO_MOTOR_START 6
#define DO_OPERATION_MODE 7
#define DO_SHOT_COMPLETE 8

#define IMMPARA_INJECTION_STAGE 9
#define IMMPARA_HOLDING_STAGE 10
#define IMMPARA_PLASTIFICATION_STAGE 11
#define IMMPARA_MELT_TEMPERATURE 12
#define IMMPARA_PLASTIFICATION_VOLUME_1 13
#define IMMPARA_PLASTIFICATION_VOLUME_2 14
#define IMMPARA_PLASTIFICATION_VOLUME_3 15
#define IMMPARA_PLASTIFICATION_VOLUME_4 16
#define IMMPARA_PLASTIFICATION_VOLUME_5 17
#define IMMPARA_INJECTION_PRESSURE_1 18
#define IMMPARA_INJECTION_PRESSURE_2 19
#define IMMPARA_INJECTION_PRESSURE_3 20
#define IMMPARA_INJECTION_PRESSURE_4 21
#define IMMPARA_INJECTION_PRESSURE_5 22
#define IMMPARA_INJECTION_SPEED_1 23
#define IMMPARA_INJECTION_SPEED_2 24
#define IMMPARA_INJECTION_SPEED_3 25
#define IMMPARA_INJECTION_SPEED_4 26
#define IMMPARA_INJECTION_SPEED_5 27
#define IMMPARA_VPCHANGEOVER_POSITION 28
#define IMMPARA_VPCHANGEOVER_TIME 29
#define IMMPARA_PACKING_PRESSURE 30
#define IMMPARA_HOLDING_PRESSURE_1 31
#define IMMPARA_HOLDING_PRESSURE_2 32
#define IMMPARA_HOLDING_PRESSURE_3 33
#define IMMPARA_HOLDING_PRESSURE_4 34
#define IMMPARA_HOLDING_PRESSURE_5 35
#define IMMPARA_PACKING_TIME 36
#define IMMPARA_HOLDING_TIME_1 37
#define IMMPARA_HOLDING_TIME_2 38
#define IMMPARA_HOLDING_TIME_3 39
#define IMMPARA_HOLDING_TIME_4 40
#define IMMPARA_HOLDING_TIME_5 41
#define IMMPARA_COOLING_TIME 42
#define IMMPARA_SCREW_RPM_1 43
#define IMMPARA_SCREW_RPM_2 44
#define IMMPARA_SCREW_RPM_3 45
#define IMMPARA_SCREW_RPM_4 46
#define IMMPARA_SCREW_RPM_5 47
#define IMMPARA_BACK_PRESSURE_1 48
#define IMMPARA_BACK_PRESSURE_2 49
#define IMMPARA_BACK_PRESSURE_3 50
#define IMMPARA_BACK_PRESSURE_4 51
#define IMMPARA_BACK_PRESSURE_5 52
#define IMMPARA_MOLD_TEMPERATURE 53

#define IMMSENSOR_CYCLE_TIME 54
#define IMMSENSOR_MOLD_CLAMPING_TIME 55
#define IMMSENSOR_MOLD_RELEASING_TIME 56
#define IMMSENSOR_INJECTION_TIME 57
#define IMMSENSOR_VPCHANGEOVER_DELAY_TIME 58
#define IMMSENSOR_HOLDING_TIME 59
#define IMMSENSOR_PLASTIFICATION_TIME 60
#define IMMSENSOR_VPCHANGEOVER_POSITION 61
#define IMMSENSOR_CUSION_VOLUME 62
#define IMMSENSOR_END_HOLDING_POSITION 63
#define IMMSENSOR_END_PLASTIFICATION_POSITION 64
#define IMMSENSOR_NOZZLE1_TEMPERATURE 65
#define IMMSENSOR_NOZZLE2_TEMPERATURE 66
#define IMMSENSOR_NOZZLE3_TEMPERATURE 67
#define IMMSENSOR_NOZZLE4_TEMPERATURE 68
#define IMMSENSOR_NOZZLE5_TEMPERATURE 69
#define IMMSENSOR_IMM_OIL_TEMPERATURE 70
#define IMMSENSOR_FALL_MATERIAL_TEMPERATURE 71
#define IMMSENSOR_MAX_INJECTION_PRESSURE 72
#define IMMSENSOR_MAX_VPCHANGEOVER_PRESSURE 73
#define IMMSENSOR_SUM_INJECTION_PRESSURE 74
#define IMMSENSOR_SUM_HOLDING_PRESSURE 75
#define IMMSENSOR_MAX_CAVITYNEARGATE_PRESSURE 76
#define IMMSENSOR_MAX_CAVITYFARGATE_PRESSURE 77
#define IMMSENSOR_SUM_CAVITY_PRESSURE 78
#define IMMSENSOR_SUM_MOLD_CLAMPING_FORCE 79
#define IMMSENSOR_MAX_MOLD_CLAMPING_FORCE 80
#define IMMSENSOR_MAX_BACK_PRESSURE 81
#define IMMSENSOR_DELAY_PLASTIFICATION_TIME 82
#define IMMSENSOR_IN_MOLD_COOLING_TEMPERATURE 83
#define IMMSENSOR_OUT_MOLD_COOLING_TEMPERATURE 84
#define IMMSENSOR_MOLD_COOLING_VOLUME 85
#define IMMSENSOR_MOLD_RELEASING_POSITION_2 86
#define IMMSENSOR_MOLD_RELEASING_POSITION_3 87
#define IMMSENSOR_MOLD_RELEASING_POSITION_4 88
#define IMMSENSOR_MOLD_RELEASING_POSITION_5 89
#define IMMSENSOR_SCREW_SUCK_BACK_POSITION 90
#define IMMSENSOR_SCREW_RPM 91
#define IMMSENSOR_PART_WEIGHT 92
#define IMMSENSOR_GATE_CLOSING_TIME 93
#define IMMSENSOR_MELT_TEMPERATURE 94
#define IMMSENSOR_MOLD_TEMPERATURE 95

#define SHOT_NUM_PER_TABLE 100000

#define CUSTOM_SENSOR_CLASS_IMM 1
#define CUSTOM_SENSOR_CLASS_CAVITY 2

#define CUSTOM_SENSOR_CATEGORY_ELSE 0
#define CUSTOM_SENSOR_CATEGORY_PRESSURE 1
#define CUSTOM_SENSOR_CATEGORY_TEMPERATURE 2


#define CAVITY_PRESSURE_SOFT_SENSOR_FEATURE_NUM 3
#define MOLD_RELATED_SOFT_SENSOR_FEATURE_NUM 4


//MonitorItemClass = Soft && MonitorItemSensorSN = NULL
//Features of Mold Clamp and Mold Release
#define SOFT_MOLD_CLAMPED_CYCLETIME 1
#define SOFT_MOLD_BASED_CYCLETIME 2
#define SOFT_MOLD_SHOT_INTERVAL 3
#define SOFT_MOLD_COMPLETE_CYCLETIME 4
//Features of Double Cavity Pressure Sensor
#define SOFT_PRESSURE_SENSOR_FEATURE_DIFF_MAX_PRESSURE 5
#define SOFT_PRESSURE_SENSOR_FEATURE_DIFF_INCREASE_PRESSURE_TIME 6
#define SOFT_PRESSURE_SENSOR_FEATURE_DIFF_MOLD_RELEASEPRESSURE 7

//Features of Single Mold Pressure Sensor
#define CAVITY_PRESSURE_SENSOR_SINGLE_FEATURE_NUM 6
#define CAVITY_PRESSURE_SENSOR_FEATURE_MAX_PRESSURE 1
#define CAVITY_PRESSURE_SENSOR_FEATURE_STAGE1_PRESSURE_SUM 2
#define CAVITY_PRESSURE_SENSOR_FEATURE_MAX_PRESSURE_TIME 3
#define CAVITY_PRESSURE_SENSOR_FEATURE_AVG_STAGE1_PRESSURESLOPE 4
#define CAVITY_PRESSURE_SENSOR_FEATURE_AVG_STAGE2_PRESSURESLOPE 5
#define CAVITY_PRESSURE_SENSOR_FEATURE_RELEASED_PRESSURE 6



//Features of Single Mold Temperature Sensor
#define CAVITY_TEMPERATURE_SENSOR_SINGLE_FEATURE_NUM 5
#define CAVITY_TEMPERATURE_SENSOR_FEATURE_START_TEMPERATURE 1
#define CAVITY_TEMPERATURE_SENSOR_FEATURE_MAX_TEMPERATURE 2
#define CAVITY_TEMPERATURE_SENSOR_FEATURE_MAX_TEMPERATURE_TIME 3
#define CAVITY_TEMPERATURE_SENSOR_FEATURE_AVG_STAGE2_PRESSURESLOPE 4
#define CAVITY_TEMPERATURE_SENSOR_FEATURE_RELEASED_TEMPERATURE 5

//Features of IMM Oil Pressure Sensor
#define IMM_SYSPRESSURE_SENSOR_SINGLE_FEATURE_NUM 19

#define IMM_SYSPRESSURE_SENSOR_FEATURE_STAGE1_START_TIME 1
#define IMM_SYSPRESSURE_SENSOR_FEATURE_STAGE1_END_TIME 2
#define IMM_SYSPRESSURE_SENSOR_FEATURE_STAGE1_PERIOD 3
#define IMM_SYSPRESSURE_SENSOR_FEATURE_AVG_STAGE1_PRESSURESLOPE 4
#define IMM_SYSPRESSURE_SENSOR_FEATURE_SUM_STAGE1_PRESSURE 5
#define IMM_SYSPRESSURE_SENSOR_FEATURE_STAGE1_END_PRESSURE 6

#define IMM_SYSPRESSURE_SENSOR_FEATURE_STAGE2_START_TIME 7
#define IMM_SYSPRESSURE_SENSOR_FEATURE_STAGE2_END_TIME 8
#define IMM_SYSPRESSURE_SENSOR_FEATURE_STAGE2_PERIOD 9
#define IMM_SYSPRESSURE_SENSOR_FEATURE_AVG_STAGE2_PRESSURESLOPE 10
#define IMM_SYSPRESSURE_SENSOR_FEATURE_SUM_STAGE2_PRESSURE 11

#define IMM_SYSPRESSURE_SENSOR_FEATURE_STAGE3_START_TIME 12
#define IMM_SYSPRESSURE_SENSOR_FEATURE_STAGE3_END_TIME 13
#define IMM_SYSPRESSURE_SENSOR_FEATURE_STAGE3_PERIOD 14
#define IMM_SYSPRESSURE_SENSOR_FEATURE_MAX_STAGE3_PRESSURE 15
#define IMM_SYSPRESSURE_SENSOR_FEATURE_MAX_STAGE3_PRESSURE_TIME 16
#define IMM_SYSPRESSURE_SENSOR_FEATURE_SUM_STAGE3_PRESSURE 17

#define IMM_SYSPRESSURE_SENSOR_FEATURE_CYCLE_TIME 18
#define IMM_SYSPRESSURE_SENSOR_FEATURE_SUM_PRESSURE 19

#define EC_IMM_SYSPRESSURE_RATIO_STAGE1_END_TIME 0.5
#define EC_IMM_SYSPRESSURE_RATIO_AVG_STAGE1_PRESSURESLOPE 1.2
#define EC_IMM_SYSPRESSURE_RATIO_STAGE1_END_PRESSURE 1.0
#define EC_IMM_SYSPRESSURE_RATIO_SUM_STAGE2_PRESSURE 1.2
#define EC_IMM_SYSPRESSURE_RATIO_STAGE3_PERIOD 0.5
#define EC_IMM_SYSPRESSURE_RATIO_SUM_STAGE3_PRESSURE 0.5
#define EC_IMM_SYSPRESSURE_RATIO_SUM_PRESSURE 0.8

#define MOLD_STATUS_UNKNOW 0
#define MOLD_STATUS_RELEASED 1
#define MOLD_STATUS_CLAMPING 2
#define MOLD_STATUS_CLAMPED 3
#define MOLD_STATUS_RELEASING 4

#define SPC_MAX_MONITOR_ITEM 9

#define MONITOR_ITEM_CLASS_QUALITY 1
#define MONITOR_ITEM_CLASS_OPCUA 2
#define MONITOR_ITEM_CLASS_MODBUS 3
#define MONITOR_ITEM_CLASS_SOFT 4


//OPCUA, ACCEPT_CRITERIA
#define MONITOR_ITEM_INDEX_SINGLE_VALUE 1

#define SPC_PROGRESS_FLAG_MODBUS_READY 4
#define SPC_PROGRESS_FLAG_OPCUA_READY 2
#define SPC_PROGRESS_FLAG_FINISHED 1

#define QC_PROGRESS_FLAG_MODBUS_READY 4
#define QC_PROGRESS_FLAG_OPCUA_READY 2
#define QC_PROGRESS_FLAG_FINISHED 1

#define EC_PROGRESS_FLAG_MODBUS_READY 4
#define EC_PROGRESS_FLAG_OPCUA_READY 2
#define EC_PROGRESS_FLAG_FINISHED 1

#define SPC_RULE_NUM 8
#define SPC_XUCL3 0
#define SPC_XUCL2 1
#define SPC_XUCL1 2
#define SPC_XCL 3
#define SPC_XLCL1 4
#define SPC_XLCL2 5
#define SPC_XLCL3 6

#define SPC_RULE1_POINT_SIZE 1
#define SPC_RULE2_POINT_SIZE 9
#define SPC_RULE3_POINT_SIZE 6
#define SPC_RULE4_POINT_SIZE 14
#define SPC_RULE5_POINT_SIZE 3
#define SPC_RULE6_POINT_SIZE 5
#define SPC_RULE7_POINT_SIZE 15
#define SPC_RULE8_POINT_SIZE 8

#define UNSUBSCRIBE_OPCUA_TIME_OUT 3  //3seconds
#define UNSUBSCRIBE_MODBUS_TIME_OUT 3 //3seconds

//#define ENABLE_SUBSCRIBE_CURRENT_TIME

typedef struct stMOCustomSensorSN
{
	char charMOCustomSensorMeta[MEDIUM_STRING_SIZE];
} stMOCustomSensorSN;

typedef struct stMOAcceptCriteriaSN
{
	char charAcceptCriteriaMeta[MEDIUM_STRING_SIZE];
	char charAcceptCriteriaMinTH[TINY_STRING_SIZE];
	char charAcceptCriteriaMaxTH[TINY_STRING_SIZE];
} stMOAcceptCriteriaSN;

typedef struct stCustomSensorModelSN
{
	char charSensorModelMeta[MEDIUM_STRING_SIZE];
} stCustomSensorModelSN;

typedef struct stMoldSN
{
	char charMoldMeta[MEDIUM_STRING_SIZE];
} stMoldSN;

typedef struct stIMMModelSN
{
	char charIMMModelMeta[MEDIUM_STRING_SIZE];
} stIMMModelSN;

typedef struct stMaterialSN
{
	char charMaterialMeta[MEDIUM_STRING_SIZE];
} stMaterialSN;

typedef struct stProdSN
{
	char charProdMeta[MEDIUM_STRING_SIZE];
} stProdSN;

typedef struct stIMMSN
{
	char charOPCUAVersionSN[TINY_STRING_SIZE];
	char charOPCUAUserName[TINY_STRING_SIZE];
	char charOPCUAPassword[TINY_STRING_SIZE];
	char charOPCUAIP[TINY_STRING_SIZE];
	char charOPCUAPort[TINY_STRING_SIZE];
	char charModbusIP[TINY_STRING_SIZE];
	char charModbusPort[TINY_STRING_SIZE];
	char charModbusMoldSignalSource[TINY_STRING_SIZE];
	char charModbusMoldSignalType[TINY_STRING_SIZE];
} stIMMSN;

//[SYSTEM]

unsigned int SYS_InsertSysErrMsg(MYSQL mysqlCon, unsigned int intErrClass, unsigned int intMOSN, unsigned int intIMMSN, int intErrCode, const char *charErrMsg);
int SYS_CreateSysDB(MYSQL mysqlCon);
int SYS_DropSysDB(MYSQL mysqlCon);
int SYS_DeleteAllMO(MYSQL mysqlCon);

//[DATA BASE]

int DB_DatabaseConnect(MYSQL *mysqlCon);
int DB_SelectColumnMax(MYSQL mysqlCon, char charTableName[MEDIUM_STRING_SIZE], char charColumnName[SMALL_STRING_SIZE], char **charMaxValue);
int DB_SelectTableRowCount(MYSQL mysqlCon, char charTableName[MEDIUM_STRING_SIZE], int *intTableCount);
int DB_SelectIMMSNofPrimaryIMM(MYSQL mysqlCon, unsigned int *intIMMSN);
int DB_SelectMOSNbyIMMSN(MYSQL mysqlCon, unsigned int intIMMSN, unsigned int *intMOSN);
int DB_SelectShotSNbyIMMSN(MYSQL mysqlCon, unsigned int intIMMSN, unsigned int *intShotSN);
int DB_SelectOPCUAVersionSNbyIMMSN(MYSQL mysqlCon, unsigned int intIMMSN, unsigned int *intOPCUAVersionSN);
int DB_SelectPIDbyIMMSN(MYSQL mysqlCon, unsigned int intIMMSN, unsigned int *intOPCUAClientPID, unsigned int *intModbusClientPID);
int DB_CheckIsomorphismbyMOSN(MYSQL mysqlCon, unsigned int intMOSN1, unsigned int MOSN2, bool *boolIsomorphism);
int DB_SelectMOStatusbyMOSN(MYSQL mysqlCon, unsigned int intMOSN, unsigned int *intMOStatus);
int DB_SelectOPCUAVersionSNbyMOSN(MYSQL mysqlCon, unsigned int intMOSN, unsigned int *intOPCUAVersionSN);
int DB_SelectReferenceMOSNbyMOSN(MYSQL mysqlCon, unsigned int intMOSN, unsigned int *intReferenceMOSN);
int DB_SelectIMMSNbyMOSN(MYSQL mysqlCon, unsigned int intMOSN, unsigned int *intIMMSN);
int DB_SelectMOIDbyMOSN(MYSQL mysqlCon, unsigned int intMOSN, char *charMOID);
int DB_SelectMoldSNbyMOSN(MYSQL mysqlCon, unsigned int intMOSN, unsigned int *intMoldSN);
int DB_SelectProdSNbyMOSN(MYSQL mysqlCon, unsigned int intMOSN, unsigned int *intProdSN);
int DB_SelectShotSNbyMOSN(MYSQL mysqlCon, unsigned int intMOSN, unsigned int *intShotSN);
int DB_SelectIMMShotSNbyMOSN(MYSQL mysqlCon, unsigned int intMOSN, unsigned int *intIMMShotSN);
int DB_SelectCurrentRoundSN(MYSQL mysqlCon, unsigned int intIMMSN, unsigned int *intRoundSN);
int DB_SelectFirstShotSNbyRoundSN(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intRoundSN, unsigned int *intShotSN);
int DB_SelectRoundSNbyShotSN(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN, unsigned int *intRoundSN);
int DB_SelectCustomSensorModelSNbyMOCustomSensorSN(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intMOCustomSensorSN, unsigned int *intCustomSensorModelSN);
int DB_SelectMOCustomSensorClassAndCategorybyMOCustomSensorSN(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intMOCustomSensorSN, unsigned int *intMOCustomSensorClass, unsigned int *intCustomSensorCategory);
int DB_SelectMOCustomSensorClassAndCategorybyCustomSensorModelSN(MYSQL mysqlCon, unsigned int intCustomSensorModelSN, unsigned int *intCustomSensorClass, unsigned int *intCustomSensorCategory);
int DB_SelectMOCustomSensorRawData(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN, unsigned int intMOCustomSensorSN, double **doubleSensorData, unsigned int *intSensorDataSize);
int DB_SelectMonitorItemDBName(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intMonitorItemSN, char *charMonitorItemDBName);
int DB_SelectMonitorItemDisplayName(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intMonitorItemSN, char *charMonitorItemDisplayName);
int DB_InsertMOCustomSensorData(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN, unsigned int intMOCustomSensorSN, double doubleElapsedTime, double doubleValue, double doubleMOCustomSensorConvertRatio);
int DB_InsertMOCustomSensorDataBatch(MYSQL mysqlCon, unsigned int intMOSN, unsigned int *intShotSN, unsigned int intMOCustomSensorSN, double *doubleElapsedTime, double *doubleValue, double doubleMOCustomSensorConvertRatio, unsigned int intTail, unsigned int intHead);
int DB_InsertMoldActionFeatureValue(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN);
int DB_GetIMMSysPressureSensorFeatureValue(MYSQL mysqlCon, unsigned int intMOSN, double *doubleSensorData, unsigned int intSensorDataSize, char charIMMSysPressureSensorFeatureValue[IMM_SYSPRESSURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE]);
int DB_InsertIMMSysPressureSensorFeatureValue(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN, unsigned int intMOCustomSensorSN, char charIMMSysPressureSensorFeatureValue[IMM_SYSPRESSURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE]);
int DB_GetCavityPressureSensorFeatureValue(MYSQL mysqlCon, unsigned int intMOSN, double *doubleSensorData, unsigned int intSensorDataSize, char charCavityPressureSensorFeatureValue[CAVITY_PRESSURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE]);
int DB_InsertCavityPressureSensorFeatureValue(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN, unsigned int intMOCustomSensorSN, char charMOCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE]);
int DB_GetCavityTemperatureSensorFeatureValue(MYSQL mysqlCon, unsigned int intMOSN, double *doubleSensorData, unsigned int intSensorDataSize, char charCavityTemperatureSensorFeatureValue[CAVITY_TEMPERATURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE]);
int DB_InsertCavityTemperatureSensorFeatureValue(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN, unsigned int intMOCustomSensorSN, char charCavityTemperatureSensorFeatureValue[CAVITY_TEMPERATURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE]);
int DB_InsertCavityPressureSensorSoftSensorFeatureValue(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN, char charNGMOCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE], char charFGMOCustomSensorFeatureValue[CAVITY_PRESSURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE]);
int DB_InsertAllCaCpCpkPpk(MYSQL mysqlCon, int intMOSN, int intShotSNStart, int intShotSNEnd);
int DB_UpdateCarbonEmission(MYSQL mysqlCon, int intMOSN);
int DB_UpdateCustomSN(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN, char charCustomSN[TINY_STRING_SIZE]);

//[INDEX]

int INDEX_InsertOPCUAVersion(MYSQL mysqlCon, unsigned int *intOPCUAVersionSN, const char *charOPCUAVersionID, char charOPCUAIMMNodeList[OPCUA_IMMPARA_NODE_NUM + 1][OPCUA_META_NUM][MEDIUM_STRING_SIZE]);
int INDEX_UpdateOPCUAVersion(MYSQL mysqlCon, unsigned int intOPCUAVersionSN, const char *charOPCUAVersionID, char charOPCUAIMMNodeList[OPCUA_IMMPARA_NODE_NUM + 1][OPCUA_META_NUM][MEDIUM_STRING_SIZE]);
int INDEX_MaskOPCUAVersion(MYSQL mysqlCon, unsigned int intOPCUAVersionSN);
int INDEX_DeleteOPCUAVersion(MYSQL mysqlCon, unsigned int intOPCUAVersionSN);
int INDEX_InsertIMMModel(MYSQL mysqlCon, unsigned int *intIMMModelSN, const char *charIMMVendor, const char *charIMMModel, stIMMModelSN varIMMModelSN);
int INDEX_UpdateIMMModel(MYSQL mysqlCon, unsigned int intIMMModelSN, const char *charIMMVendor, const char *charIMMModel, stIMMModelSN varIMMModelSN);
int INDEX_MaskIMMModel(MYSQL mysqlCon, unsigned int intIMMModelSN);
int INDEX_DeleteIMMModel(MYSQL mysqlCon, unsigned int intIMMModelSN);
int INDEX_InsertMold(MYSQL mysqlCon, unsigned int *intMoldSN, const char *charMoldVendor, const char *charMoldID, stMoldSN varMoldSN);
int INDEX_UpdateMold(MYSQL mysqlCon, unsigned int intMoldSN, const char *charMoldVendor, const char *charMoldID, stMoldSN varMoldSN);
int INDEX_MaskMold(MYSQL mysqlCon, unsigned int intMoldSN);
int INDEX_DeleteMold(MYSQL mysqlCon, unsigned int intMoldSN);
int INDEX_InsertMOCustomSensor(MYSQL mysqlCon, unsigned int *intCustomSensorModelSN, unsigned int intMOCustomSensorClass, unsigned int intCustomSensorCategory, const char *charCustomSensorVendor, const char *charCustomSensorModel, stCustomSensorModelSN varCustomSensorModelSN);
int INDEX_UpdateMOCustomSensor(MYSQL mysqlCon, unsigned int intCustomSensorModelSN, unsigned int intMOCustomSensorClass, const char *charCustomSensorVendor, const char *charCustomSensorModel, stCustomSensorModelSN varCustomSensorModelSN);
int INDEX_MaskMOCustomSensor(MYSQL mysqlCon, unsigned int intCustomSensorModelSN);
int INDEX_DeleteCustomSensorModel(MYSQL mysqlCon, unsigned int intCustomSensorModelSN);
int INDEX_InsertMaterial(MYSQL mysqlCon, unsigned int *intMaterialSN, const char *charMaterialID, stMaterialSN varMaterialSN);
int INDEX_UpdateMaterial(MYSQL mysqlCon, unsigned int intMaterialSN, const char *charMaterialID, stMaterialSN varMaterialSN);
int INDEX_MaskMaterial(MYSQL mysqlCon, unsigned int intMaterialSN);
int INDEX_DeleteMaterial(MYSQL mysqlCon, unsigned int intMaterialSN);
int INDEX_InsertProd(MYSQL mysqlCon, unsigned int *intProdSN, const char *charProdID, stProdSN varProdSN);
int INDEX_UpdateProd(MYSQL mysqlCon, unsigned int intMoldSN, const char *charProdID, stProdSN varProdSN);
int INDEX_MaskProd(MYSQL mysqlCon, unsigned int intProdSN);
int INDEX_DeleteProd(MYSQL mysqlCon, unsigned int intProdSN);
int INDEX_InsertAcceptCriteria(MYSQL mysqlCon, unsigned int *intAcceptCriteriaSN, const char *charAcceptCriteriaStr);
int INDEX_UpdateAcceptCriteria(MYSQL mysqlCon, unsigned int intAcceptCriteriaSN, const char *charAcceptCriteriaStr);
int INDEX_MaskAcceptCriteria(MYSQL mysqlCon, unsigned int intAcceptCriteriaSN);
int INDEX_DeleteAcceptCriteria(MYSQL mysqlCon, unsigned int intAcceptCriteriaSN);

//[OPCUA]

int OPCUA_SetLocalhostOPCUAVersion(char charOPCUAIMMNodeList[OPCUA_IMMPARA_NODE_NUM + OPCUA_IMMSENSOR_NODE_NUM + 1][OPCUA_META_NUM][MEDIUM_STRING_SIZE]);
int OPCUA_InsertModBusOPCUAVersion(char charOPCUAIMMNodeList[OPCUA_IMMPARA_NODE_NUM + OPCUA_IMMSENSOR_NODE_NUM + 1][OPCUA_META_NUM][MEDIUM_STRING_SIZE]);
int OPCUA_CheckOPCUAStatus(MYSQL mysqlCon, char charOPCUAIP[16], unsigned int intOPCUAPort, int *intOPCUAState);
int OPCUA_InitConvertRatio(MYSQL mysqlCon, unsigned int intOPCUAVersionSN, double *doubleOPCUAConvertRatio);
int OPCUA_ConvertOPCUAtoPanel(double doubleRatio[OPCUA_IMMPARA_NODE_NUM + OPCUA_IMMSENSOR_NODE_NUM + 1], int intNodeSN, UA_Variant *originalVariant, UA_Variant *convertedVariant);
int OPCUA_ConvertPaneltoOPCUA(double doubleRatio[OPCUA_IMMPARA_NODE_NUM + OPCUA_IMMSENSOR_NODE_NUM + 1], int intNodeSN, UA_Variant *value);
int OPCUA_WriteOPCUANodeValue(MYSQL mysqlCon, unsigned int intIMMSN, unsigned int intOPCUANodeSN, char charValue[TINY_STRING_SIZE]);
int OPCUA_ReadOPCUANodeValue(MYSQL mysqlCon, unsigned int intIMMSN, unsigned int intOPCUANodeSN, UA_Variant *readVariant);
int OPCUA_ResetOPCUAServer(MYSQL mysqlCon, unsigned int intIMMSN);

//[MODBUS]

int MODBUS_SelectModbusClientStatusbyIMMSN(MYSQL mysqlCon, unsigned int intIMMSN, unsigned int *intModbusClientStatus);

//[IMM]

int IMM_InsertIMM(MYSQL mysqlCon, unsigned int *intIMMSN, char charIMMID[SMALL_STRING_SIZE], unsigned int intIMMStationID, unsigned int intIMMModelSN, stIMMSN varIMMSN);
int IMM_UpdateIMM(MYSQL mysqlCon, unsigned int intIMMSN, char charIMMID[SMALL_STRING_SIZE], unsigned int intIMMStationID, unsigned int intIMMModelSN, stIMMSN varIMMSN);
int IMM_MaskIMM(MYSQL mysqlCon, unsigned int intIMMSN);
int IMM_DeleteIMM(MYSQL mysqlCon, unsigned int intIMMSN);
int IMM_SubscribeOPCUA(MYSQL mysqlCon, unsigned int intIMMSN);
int IMM_UnsubscribeOPCUA(MYSQL mysqlCon, unsigned int intIMMSN);
int IMM_SubscribeModbus(MYSQL mysqlCon, unsigned int intIMMSN);
int IMM_UnsubscribeModbus(MYSQL mysqlCon, unsigned int intIMMSN);
int IMM_Subscribe(MYSQL mysqlCon, unsigned int intIMMSN);
int IMM_Unsubscribe(MYSQL mysqlCon, unsigned int intIMMSN);
int IMM_TuneIMMPara(MYSQL mysqlCon, unsigned int intIMMSN, unsigned int intTechnicianUserSN, char charIMMParaValue[OPCUA_IMMPARA_NODE_NUM][TINY_STRING_SIZE]);
int IMM_TuneIMMParaByRecommend(MYSQL mysqlCon, unsigned int intIMMSN, unsigned int intMOSN, unsigned int intRecomIMMParaSN);
int IMM_SetPrimaryIMM(MYSQL mysqlCon, unsigned int intIMMSN);
int IMM_SetIMMMoldStatus(MYSQL mysqlCon, unsigned int intIMMSN, unsigned int intMoldStatus);
int IMM_AddRoundSN(MYSQL mysqlCon, unsigned int intIMMSN);
int IMM_EnableAutoOPCUAIMMParaControl(MYSQL mysqlCon, unsigned int intIMMSN, bool boolValue);

//[MANUFACTURING ORDER]

int MO_CreateMO(MYSQL mysqlCon, unsigned int *intMOSN, const char *charMOID, unsigned int intIMMSN, unsigned int intProdSN, unsigned int intMoldSN, unsigned int intMaterialSN, struct tm tmExpectedStartTime, struct tm tmExpectedEndTime, unsigned int intPeicePerShot, unsigned int intExpectedProductVolume);
int MO_UpdateMO(MYSQL mysqlCon, unsigned int intMOSN, const char *charMOID, unsigned int intIMMSN, unsigned int intProdSN, unsigned int intMoldSN, unsigned int intMaterialSN, struct tm tmExpectedStartTime, struct tm tmExpectedEndTime, unsigned int intPiecePerShot, unsigned int intExpectedProductVolume);
int MO_MaskMO(MYSQL mysqlCon, unsigned int intMOSN);
int MO_DeleteMO(MYSQL mysqlCon, unsigned int intMOSN);
int MO_DeleteShot(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN_Start, unsigned int intShotSN_End);
int MO_TruncateMO(MYSQL mysqlCon, unsigned int intMOSN);
int MO_ExportMO(MYSQL mysqlCon, unsigned int intMOSN);
int MO_InsertMOCustomSensorSNList(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intCustomSensorModelSN, unsigned int intMOCustomSensorChannelSN, double doubleMOCustomSensorConvertRatio, stMOCustomSensorSN varMOCustomSensorSN);
int MO_InsertMOAcceptCriteriaSNList(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intMOAcceptCriteriaSN, unsigned int intAcceptCriteriaClass, const char *charAcceptCriteriaUnit, stMOAcceptCriteriaSN varAcceptCriteriaSN);
int MO_StartMOonIMM(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intIMMSN);
int MO_StopMOonIMM(MYSQL mysqlCon, unsigned int intIMMSN, bool boolFinished);
int MO_InsertMOAcceptCriteriaPredictValue(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN, unsigned int intMOAcceptCriteriaSN, char charPredictValue[TINY_STRING_SIZE]);
int MO_InsertMOAcceptCriteriaActualValue(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN, unsigned int intMOAcceptCriteriaSN, char charActualValue[TINY_STRING_SIZE]);
int MO_UpdateMOSNListAfterShot(MYSQL mysqlCon, unsigned int intMOSN, bool boolUseActual);
int MO_UpdateVirtualOPCUAServer(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intRealTimeShotSN, unsigned int intIMMSensorDataReadyShotSN);
int MO_AutoCopyAllControlLine(MYSQL mysqlCon, unsigned int intMOSN);

//[EMPTY SHOT CONTROL]

int EC_SelectECProgressFlag(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN, unsigned int *intECProgressFlag);
int EC_TriggerECProgressFlag(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN, unsigned int intECProgressFlagBit);
int EC_InsertAllECCL(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSNStart, unsigned int intShotSNEnd);
int EC_InsertECCL(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intMonitorItemSN, unsigned int intShotSNStart, unsigned int intShotSNEnd);
int EC_CopyECControlLine(MYSQL mysqlCon, unsigned int intSourceMOSN, unsigned int intDestinationMOSN);
int EC_CheckEmptyShot(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN);
//int EC_CheckEmptyShotbyIMMSysPressure(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN,char charIMMSysPressureSensorFeatureValue[IMM_SYSPRESSURE_SENSOR_SINGLE_FEATURE_NUM + 1][TINY_STRING_SIZE]);

//[QUALITY CONTROL]

int QC_ConfigMonitorItemSN(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intMonitorItemSN, bool boolQualityControl, double doubleQualityControlCLRatio);
int QC_ConfigAllMonitorItemSN(MYSQL mysqlCon, unsigned int intMOSN, double doubleQCCLRatio);
int QC_SelectQCProgressFlag(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN, unsigned int *intQCProgressFlag);
int QC_TriggerQCProgressFlag(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN, unsigned int intQCProgressFlagBit);
int QC_InsertAllQCCL(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSNStart, unsigned int intShotSNEnd);
int QC_InsertQCCL(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intMonitorItemSN, unsigned int intShotSNStart, unsigned int intShotSNEnd);
int QC_CopyAllMonitorItemSN(MYSQL mysqlCon, unsigned int intSourceMOSN, unsigned int intDestinationMOSN);
int QC_CopyQCControlLine(MYSQL mysqlCon, unsigned int intSourceMOSN, unsigned int intDestinationMOSN);
int QC_CopyQCEnable(MYSQL mysqlCon, unsigned int intSourceMOSN, unsigned int intDestinationMOSN);
int QC_SelectQCXBarValue(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intMonitorItemSN, unsigned int intShotSN, unsigned int intFetchSize, double **doubleXBarValue);
int QC_InsertQCAlarm(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN);
int QC_UpdateQCAlarmRead(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intQualityControlAlarmSN, bool boolQualityControlAlarmRead);

//[SPC]
int SPC_ConfigMonitorItemSN(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intMonitorItemSN, unsigned int intSampleSize, unsigned int intGroupSize, double doubleSPCCLRatio, bool boolSPCRule[8]);
int SPC_ConfigAllMonitorItemSN(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intSPCSampleSize, unsigned int intSPCGroupSize, double doubleSPCCLRatio);
int SPC_SelectSPCProgressFlag(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN, unsigned int *intSPCProgressFlag);
int SPC_TriggerSPCProgressFlag(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN, unsigned int intSPCProgressFlagBit);
int SPC_InsertAllSPCCL(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSNEnd);
int SPC_InsertSPCCL(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intMonitorItemSN, unsigned int intShotSNEnd);
int SPC_CopyAllMonitorItemSN(MYSQL mysqlCon, unsigned int intSourceMOSN, unsigned int intDestinationMOSN);
int SPC_CopySPCControlLine(MYSQL mysqlCon, unsigned int intSourceMOSN, unsigned int intDestinationMOSN);
int SPC_CopySPCEnable(MYSQL mysqlCon, unsigned int intSourceMOSN, unsigned int intDestinationMOSN);
int SPC_SelectSPCXBarValue(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intMonitorItemSN, unsigned int intShotSN, unsigned int intFetchSize, double **doubleXBarValue);
int SPC_SelectRValue(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intMonitorItemSN, unsigned int intShotSN, unsigned int intFetchSize, double **doubleRValue);
int SPC_InsertSPCAlarm(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intShotSN);
int SPC_UpdateSPCAlarmRead(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intSPCAlarmSN, bool boolSPCAlarmRead);
int SPC_Rule1(double *doubleValue, double doubleUCL, double doubleLCL, int *intleRule1Alarm);
int SPC_Rule2(double *doubleValue, unsigned int intPointSize, double doubleCL, int *intleRule2Alarm);
int SPC_Rule3(double *doubleValue, unsigned int intPointSize, int *intleRule3Alarm);
int SPC_Rule4(double *doubleValue, unsigned int intPointSize, int *intleRule4Alarm);
int SPC_Rule5(double *doubleValue, unsigned int intPointSize, double doubleUCL, double doubleCL, double doubleLCL, int *intRule5Alarm);
int SPC_Rule6(double *doubleValue, unsigned int intPointSize, double doubleUCL, double doubleCL, double doubleLCL, int *intRule6Alarm);
int SPC_Rule7(double *doubleValue, unsigned int intPointSize, double doubleUCL, double doubleCL, double doubleLCL, int *intRule7Alarm);
int SPC_Rule8(double *doubleValue, unsigned int intPointSize, double doubleUCL, double doubleCL, double doubleLCL, int *intRule8Alarm);
int SPC_InsertCaCpCpkPpk(MYSQL mysqlCon, int intMOSN, int intMonitorItemSN, int intShotSNStart, int intShotSNEnd);

//Neural Network
int NN_AcceptCriteriaInference(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intIMMSN, unsigned int intExpectedProductVolume, unsigned int intCountAcceptCriteriaSN,
							   unsigned int intShotSN, int *intMOAcceptCriteriaSNTHFlag, double *doubleMOAcceptCriteriaSNTH);

//Demo
int DEMO_SetLocalhostOPCUAVersion(char charOPCUAIMMNodeList[OPCUA_IMMPARA_NODE_NUM + OPCUA_IMMSENSOR_NODE_NUM + 1][OPCUA_META_NUM][MEDIUM_STRING_SIZE]);
int DEMO_SetFCSOPCUAVersion(char charOPCUAIMMNodeList[OPCUA_IMMPARA_NODE_NUM + OPCUA_IMMSENSOR_NODE_NUM + 1][OPCUA_META_NUM][MEDIUM_STRING_SIZE]);
int DEMO_SetCYCUOPCUAVersion(char charOPCUAIMMNodeList[OPCUA_IMMPARA_NODE_NUM + OPCUA_IMMSENSOR_NODE_NUM + 1][OPCUA_META_NUM][MEDIUM_STRING_SIZE]);
int DEMO_SetFCSCT80EOPCUAVersion(char charOPCUAIMMNodeList[OPCUA_IMMPARA_NODE_NUM + OPCUA_IMMSENSOR_NODE_NUM + 1][OPCUA_META_NUM][MEDIUM_STRING_SIZE]);
int DEMO_SetIMMParaValue(char charIMMParaValue[OPCUA_IMMPARA_NODE_NUM + 1][TINY_STRING_SIZE]);

//OPCUA
int OPCUA_SetMX1OPCUAVersion(char charOPCUAIMMNodeList[OPCUA_IMMPARA_NODE_NUM + OPCUA_IMMSENSOR_NODE_NUM + 1][OPCUA_META_NUM][MEDIUM_STRING_SIZE]);
int OPCUA_SetMJ9000S_1_OPCUAVersion(char charOPCUAIMMNodeList[OPCUA_IMMPARA_NODE_NUM + OPCUA_IMMSENSOR_NODE_NUM + 1][OPCUA_META_NUM][MEDIUM_STRING_SIZE]);

#endif