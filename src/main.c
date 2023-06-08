/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include "itri_injpro.h"
#include "open62541.h"
#include "config.h"

bool boolIsStopSignal = false;

static void handler_StopMain(int intSign)
{
	boolIsStopSignal = true;
}

int main(int argc, char *argv[])
{

	unsigned int intIMMSN;
	unsigned int intMOSN;

	stMOAcceptCriteriaSN varMOAcceptCriteriaSN;
	stMOCustomSensorSN varMOCustomSensorSN;
	stCustomSensorModelSN varCustomSensorModelSN;
	stMoldSN varMoldSN;
	stIMMModelSN varIMMModelSN;
	stIMMSN varIMMSN;
	stMaterialSN varMaterialSN;
	stProdSN varProdSN;
	//stUAIMMParaValue	varUAIMMParaValue;
	char charIMMParaValue[OPCUA_IMMPARA_NODE_NUM + 1][TINY_STRING_SIZE];
	char charOPCUAIMMNodeList[OPCUA_IMMPARA_NODE_NUM + OPCUA_IMMSENSOR_NODE_NUM + 1][OPCUA_META_NUM][MEDIUM_STRING_SIZE];
	struct tm tmExpectedStartTime;
	struct tm tmExpectedEndTime;
	long long int intShotSN[8] = {0};
	int intAutoIMM[8];
	unsigned int intMoldSignalSource;
	unsigned int intMoldSignalType;

	pid_t pidIMMSensor;
	char charRandValue[TINY_STRING_SIZE];
	char *charCountMOCustomSensorSN;
	int intCountMOCustomSensorSN;
	char charStatement[MAX_STRING_SIZE];

	unsigned int intCountMOCustomSensorModelSN;
	
	unsigned int intCountMOCustomSensorChannelSN;

	MYSQL mysqlCon;
	MYSQL_RES *mysqlResult;
	int intRetval;

	unsigned int intCavityPressureCustomSensorModelSN;
	unsigned int intCavityTemperatureCustomSensorModelSN;
	unsigned int intSystemPressureCustomSensorModelSN;
	unsigned int intCylinderPressureCustomSensorModelSN;
	unsigned int intPowerMeterCustomSensorModelSN;
	unsigned int intIMMStationID, intIMMModelSN, intMoldSN, intMOCustomSensorChannelSN, intCustomSensorModelSN, doubleMOCustomSensorConvertRatio, intMaterialSN, intProdSN, intAcceptCriteriaSN, intOPCUAVersionSN;

	signal(SIGINT, handler_StopMain);
	signal(SIGTERM, handler_StopMain);

	fprintf(stderr, "%-50s\n", "Starting INJPRO System...");

	/* Connect to MySQL Database */

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

	//snprintf(charStatement, MAX_STRING_SIZE, "SET time_zone = \"+8:00\"");
	snprintf(charStatement, MAX_STRING_SIZE, "SET GLOBAL time_zone = \"+8:00\"");
	//snprintf(charStatement, MAX_STRING_SIZE, "SET GLOBAL time_zone = \"SYSTEM\"");
	mysql_query(&mysqlCon, charStatement);

	//system("mysql -u itri -pitrimstcm0202a -h127.0.0.1 INJPRO_Local_Data < ./set_time_zone.sql")

	//DEMO_SetCYCUOPCUAVersion(charOPCUAIMMNodeList);
	//intRetval = INDEX_InsertOPCUAVersion(mysqlCon, "CYCU-OPCUA", charOPCUAIMMNodeList);
	//return EXIT_SUCCESS;

/* Stop MO */
#ifdef ENABLE_STOP_MOSN_1
	fprintf(stderr, "Stop MOSN:1 on IMMSN:%d%-33s", STOP_MOSN_1_ON_IMMSN, "...");
	intRetval = MO_StopMOonIMM(mysqlCon, 1, STOP_MOSN_1_ON_IMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[33mWarning\033[m]\n");
	}
#endif

/* Delete All MO */
#ifdef ENABLE_DELETE_ALLMO
	fprintf(stderr, "%-55s", "Deleteing Existing MO in Database...");
	intRetval = SYS_DeleteAllMO(mysqlCon);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		//fprintf(stderr,"[\033[33mWarning\033[m]\n");
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}
#endif

/* Delete System Database */
#ifdef ENABLE_DELETE_INJPRO
	fprintf(stderr, "%-55s", "Deleteing INJPRO Database...");
	intRetval = SYS_DropSysDB(mysqlCon);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		//fprintf(stderr,"[\033[33mWarning\033[m]\n");
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}
#endif

#ifdef ENABLE_DELETE_INJPRO_EXIT
	return EXIT_SUCCESS;
#endif

#ifdef ENABLE_CREATE_INJPRO
	fprintf(stderr, "%-55s", "Creating INJPRO System Database...");
	intRetval = SYS_CreateSysDB(mysqlCon);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}
#endif

#ifdef ENABLE_INSERT_INDEX
	fprintf(stderr, "%-55s", "Inserting Index IMM Model ITRI VirtualIMM...");
	strcpy(varIMMModelSN.charIMMModelMeta, "");
	intRetval = INDEX_InsertIMMModel(mysqlCon, &intIMMModelSN, "ITRI", "VirtualIMM", varIMMModelSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-55s", "Inserting Index IMM Model ITRI-IMMModel...");
	strcpy(varIMMModelSN.charIMMModelMeta, "");
	intRetval = INDEX_InsertIMMModel(mysqlCon, &intIMMModelSN, "ITRI", "ITRI-IMMModel", varIMMModelSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	/* Insert Mold Index*/
	fprintf(stderr, "%-55s", "Inserting Index Mold:ITRI-Mold...");
	strcpy(varMoldSN.charMoldMeta, "");
	intRetval = INDEX_InsertMold(mysqlCon, &intMoldSN, "ITRI", "ITRI-Mold", varMoldSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	/* Insert Prod Index */
	fprintf(stderr, "%-55s", "Inserting Index Product:ITRI-Product...");
	strcpy(varProdSN.charProdMeta, "");
	intRetval = INDEX_InsertProd(mysqlCon, &intProdSN, "ITRI-Product", varProdSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	/* Insert Sensor Index */
	fprintf(stderr, "%-55s", "Inserting Index Sensor:CavityPressureSensor...");
	strcpy(varCustomSensorModelSN.charSensorModelMeta, "");
	intRetval = INDEX_InsertMOCustomSensor(mysqlCon, &intCustomSensorModelSN, CUSTOM_SENSOR_CLASS_CAVITY, CUSTOM_SENSOR_CATEGORY_PRESSURE, "ITRI", "CavityPressureSensor", varCustomSensorModelSN);
	if (intRetval == EXIT_SUCCESS)
	{
		fprintf(stderr, "[\033[32mOK\033[m]\n");
		intCavityPressureCustomSensorModelSN = intCustomSensorModelSN;
	}
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-55s", "Inserting Index Sensor:CavityTemperatureSensor...");
	strcpy(varCustomSensorModelSN.charSensorModelMeta, "");
	intRetval = INDEX_InsertMOCustomSensor(mysqlCon, &intCustomSensorModelSN, CUSTOM_SENSOR_CLASS_CAVITY, CUSTOM_SENSOR_CATEGORY_TEMPERATURE, "ITRI", "CavityTemperatureSensor", varCustomSensorModelSN);
	if (intRetval == EXIT_SUCCESS)
	{
		fprintf(stderr, "[\033[32mOK\033[m]\n");
		intCavityTemperatureCustomSensorModelSN = intCustomSensorModelSN;
	}
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-55s", "Inserting Index Sensor:SystemPressure...");
	strcpy(varCustomSensorModelSN.charSensorModelMeta, "");
	intRetval = INDEX_InsertMOCustomSensor(mysqlCon, &intCustomSensorModelSN, CUSTOM_SENSOR_CLASS_IMM, CUSTOM_SENSOR_CATEGORY_PRESSURE, "ITRI", "SystemPressure", varCustomSensorModelSN);
	if (intRetval == EXIT_SUCCESS)
	{
		fprintf(stderr, "[\033[32mOK\033[m]\n");
		intSystemPressureCustomSensorModelSN = intCustomSensorModelSN;
	}
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-55s", "Inserting Index Sensor:CylinderPressure...");
	strcpy(varCustomSensorModelSN.charSensorModelMeta, "");
	intRetval = INDEX_InsertMOCustomSensor(mysqlCon, &intCustomSensorModelSN, CUSTOM_SENSOR_CLASS_IMM, CUSTOM_SENSOR_CATEGORY_ELSE, "ITRI", "CylinderPressure", varCustomSensorModelSN);
	if (intRetval == EXIT_SUCCESS)
	{
		fprintf(stderr, "[\033[32mOK\033[m]\n");
		intCylinderPressureCustomSensorModelSN = intCustomSensorModelSN;
	}
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-55s", "Inserting Index Sensor:PowerMeter...");
	strcpy(varCustomSensorModelSN.charSensorModelMeta, "");
	intRetval = INDEX_InsertMOCustomSensor(mysqlCon, &intCustomSensorModelSN, CUSTOM_SENSOR_CLASS_IMM, CUSTOM_SENSOR_CATEGORY_ELSE, "ITRI", "PowerMeter", varCustomSensorModelSN);
	if (intRetval == EXIT_SUCCESS)
	{
		fprintf(stderr, "[\033[32mOK\033[m]\n");
		intPowerMeterCustomSensorModelSN = intCustomSensorModelSN;
	}
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	/* Insert Material Index */
	fprintf(stderr, "%-55s", "Inserting Index Material:ITRI-Material...");
	strcpy(varMaterialSN.charMaterialMeta, "");
	intRetval = INDEX_InsertMaterial(mysqlCon, &intMaterialSN, "ITRI-Material", varMaterialSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	/* Insert OPC UA Version Index */
	/*
	fprintf(stderr, "%-55s", "Inserting Index OPCUA Version:ModBus...");
	OPCUA_InsertModBusOPCUAVersion(charOPCUAIMMNodeList);
	intRetval = INDEX_InsertOPCUAVersion(mysqlCon, &intOPCUAVersionSN, "ModBus", charOPCUAIMMNodeList);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-55s", "Inserting Index OPCUA Version:Localhost-OPCUA...");
	DEMO_SetLocalhostOPCUAVersion(charOPCUAIMMNodeList);
	intRetval = INDEX_InsertOPCUAVersion(mysqlCon, &intOPCUAVersionSN, "Localhost-OPCUA", charOPCUAIMMNodeList);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}
	*/
	/*
	fprintf(stderr, "%-55s", "Inserting Index OPCUA Version:FCS-OPCUA...");
	DEMO_SetFCSOPCUAVersion(charOPCUAIMMNodeList);
	intRetval = INDEX_InsertOPCUAVersion(mysqlCon, "FCS-OPCUA", charOPCUAIMMNodeList);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-55s", "Inserting Index OPCUA Version:CYCU-OPCUA...");
	DEMO_SetCYCUOPCUAVersion(charOPCUAIMMNodeList);
	intRetval = INDEX_InsertOPCUAVersion(mysqlCon, "CYCU-OPCUA", charOPCUAIMMNodeList);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

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

	fprintf(stderr, "%-55s", "Inserting Index OPCUA Version:CYCU-NEW-OPCUA...");
	DEMO_SetCYCUOPCUAVersion(charOPCUAIMMNodeList);
	intRetval = INDEX_InsertOPCUAVersion(mysqlCon, "CYCU-NEW-OPCUA", charOPCUAIMMNodeList);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}
	*/

	/*
	fprintf(stderr, "%-55s", "Inserting Index OPCUA Version:HUA-CHIN-MX1...");
	OPCUA_SetMX1OPCUAVersion(charOPCUAIMMNodeList);
	intRetval = INDEX_InsertOPCUAVersion(mysqlCon, &intOPCUAVersionSN, "HUA-CHIN-MX1", charOPCUAIMMNodeList);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-55s", "Inserting Index OPCUA Version:HUA-CHIN-MJ9000S-1...");
	OPCUA_SetMJ9000S_1_OPCUAVersion(charOPCUAIMMNodeList);
	intRetval = INDEX_InsertOPCUAVersion(mysqlCon, &intOPCUAVersionSN, "HUA-CHIN-MJ9000S-1", charOPCUAIMMNodeList);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}
	*/

	snprintf(charStatement, MAX_STRING_SIZE, "mysql -u itri -pitrimstcm0202a -h%s INJPRO_Local_Sys_Index < ./../OPCUAVersion/OPCUAVersionIndex-2022-4-7.sql", MYSQL_SERVER_IP);

	system(charStatement);
#endif

/* Insert IMM */
#ifdef ENABLE_INSERT_IMM
	/*
	Localhost:	127.0.0.1		:	9527
	Virtual:	127.0.0.1		:	7382
	MX1: 		192.168.29.151	:	4840
	MJ9000S : 	192.168.29.31	:	48020
	*/

	//Localhost
	fprintf(stderr, "%-55s", "Inserting IMM:Localhost to Database...");

	strcpy(varIMMSN.charOPCUAVersionSN, "2");
	strcpy(varIMMSN.charOPCUAIP, "127.0.0.1");
	strcpy(varIMMSN.charOPCUAPort, "7382");
	//strcpy(varIMMSN.charOPCUAPort, "9527");
	//strcpy(varIMMSN.charOPCUAIP, "");
	//strcpy(varIMMSN.charOPCUAPort, "");
	strcpy(varIMMSN.charModbusIP, "192.168.200.102");
	strcpy(varIMMSN.charModbusPort, "502");
	//strcpy(varIMMSN.charModbusIP, "");
	//strcpy(varIMMSN.charModbusPort, "");
	//strcpy(varIMMSN.charModbusMoldSignalSource, "");
	//strcpy(varIMMSN.charModbusMoldSignalType, "");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	//sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_OPCUA);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_SIGNAL_LOOP);

	intIMMStationID = 1;
	intIMMModelSN = 1;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "Localhost", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

	//DEMO-IMM
	fprintf(stderr, "%-55s", "Inserting IMM:DEMO-IMM to Database...");
	sprintf(varIMMSN.charOPCUAVersionSN, "%d", 3);
	strcpy(varIMMSN.charOPCUAIP, "192.168.200.152");
	strcpy(varIMMSN.charOPCUAPort, "48020");
	strcpy(varIMMSN.charModbusIP, "192.168.200.68");
	strcpy(varIMMSN.charModbusPort, "502");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_SIGNAL_LOOP);
	intIMMStationID += 1; //2
	intIMMModelSN = 2;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "DEMO-IMM", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

	//GizMo-K03(TBD)
	fprintf(stderr, "%-55s", "Inserting IMM:GizMo-K03 to Database...");
	sprintf(varIMMSN.charOPCUAVersionSN, "%d", 1); //TBD
	strcpy(varIMMSN.charOPCUAIP, "127.0.0.1");
	strcpy(varIMMSN.charOPCUAPort, "9527");
	strcpy(varIMMSN.charModbusIP, "192.168.200.53");
	strcpy(varIMMSN.charModbusPort, "502");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	//sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_SIGNAL_LOOP);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_MULTIPLE_LOOP);
	intIMMStationID += 1; //3
	intIMMModelSN = 2;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "1103", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

	//GizMo-K04(TBD)
	fprintf(stderr, "%-55s", "Inserting IMM:GizMo-K04 to Database...");
	sprintf(varIMMSN.charOPCUAVersionSN, "%d", 1); //TBD
	strcpy(varIMMSN.charOPCUAIP, "127.0.0.1");
	strcpy(varIMMSN.charOPCUAPort, "9527");
	strcpy(varIMMSN.charModbusIP, "192.168.200.54");
	strcpy(varIMMSN.charModbusPort, "502");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	//sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_SIGNAL_LOOP);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_MULTIPLE_LOOP);
	intIMMStationID += 1; //4
	intIMMModelSN = 2;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "1104", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

	//GizMo-K6
	fprintf(stderr, "%-55s", "Inserting IMM:GizMo-K06 to Database...");
	sprintf(varIMMSN.charOPCUAVersionSN, "%d", 5);
	strcpy(varIMMSN.charOPCUAIP, "127.0.0.1");
	strcpy(varIMMSN.charOPCUAPort, "9527");
	strcpy(varIMMSN.charModbusIP, "192.168.200.56");
	strcpy(varIMMSN.charModbusPort, "502");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	//sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_SIGNAL_LOOP);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_MULTIPLE_LOOP);
	intIMMStationID += 1; //5
	intIMMModelSN = 2;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "1106", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

	//GizMo-K11
	fprintf(stderr, "%-55s", "Inserting IMM:GizMo-K11 to Database...");
	sprintf(varIMMSN.charOPCUAVersionSN, "%d", 6);
	strcpy(varIMMSN.charOPCUAIP, "192.168.200.154");
	strcpy(varIMMSN.charOPCUAPort, "48020");
	strcpy(varIMMSN.charModbusIP, "192.168.200.61");
	strcpy(varIMMSN.charModbusPort, "502");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	//sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_SIGNAL_LOOP);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_MULTIPLE_LOOP);
	intIMMStationID += 1; //6
	intIMMModelSN = 2;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "1111", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

	//GizMo-K12
	fprintf(stderr, "%-55s", "Inserting IMM:GizMo-K12 to Database...");
	sprintf(varIMMSN.charOPCUAVersionSN, "%d", 5);
	strcpy(varIMMSN.charOPCUAIP, "192.168.200.152");
	strcpy(varIMMSN.charOPCUAPort, "48020");
	strcpy(varIMMSN.charModbusIP, "192.168.200.62");
	strcpy(varIMMSN.charModbusPort, "502");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	//sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_SIGNAL_LOOP);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_MULTIPLE_LOOP);
	intIMMStationID += 1; //7
	intIMMModelSN = 2;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "1112", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

	//GizMo-K13(TBD)
	fprintf(stderr, "%-55s", "Inserting IMM:GizMo-K13 to Database...");
	sprintf(varIMMSN.charOPCUAVersionSN, "%d", 1); //TBD
	strcpy(varIMMSN.charOPCUAIP, "127.0.0.1");
	strcpy(varIMMSN.charOPCUAPort, "9527");
	strcpy(varIMMSN.charModbusIP, "192.168.200.63");
	strcpy(varIMMSN.charModbusPort, "502");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	//sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_SIGNAL_LOOP);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_MULTIPLE_LOOP);
	intIMMStationID += 1; //8
	intIMMModelSN = 2;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "1113", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

	//GizMo-K14
	fprintf(stderr, "%-55s", "Inserting IMM:GizMo-K14 to Database...");
	sprintf(varIMMSN.charOPCUAVersionSN, "%d", 4);
	strcpy(varIMMSN.charOPCUAIP, "192.168.200.156");
	strcpy(varIMMSN.charOPCUAPort, "48020");
	strcpy(varIMMSN.charModbusIP, "192.168.200.64");
	strcpy(varIMMSN.charModbusPort, "502");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	//sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_SIGNAL_LOOP);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_MULTIPLE_LOOP);
	intIMMStationID += 1; //9
	intIMMModelSN = 2;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "1114", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

	//GizMo-K18
	fprintf(stderr, "%-55s", "Inserting IMM:GizMo-K18 to Database...");
	sprintf(varIMMSN.charOPCUAVersionSN, "%d", 6);
	strcpy(varIMMSN.charOPCUAIP, "192.168.200.152");
	strcpy(varIMMSN.charOPCUAPort, "48020");
	strcpy(varIMMSN.charModbusIP, "192.168.200.68");
	strcpy(varIMMSN.charModbusPort, "502");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	//sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_SIGNAL_LOOP);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_MULTIPLE_LOOP);
	intIMMStationID += 1; //10
	intIMMModelSN = 2;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "1118", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

	//GizMo-K19
	fprintf(stderr, "%-55s", "Inserting IMM:GizMo-K19 to Database...");
	sprintf(varIMMSN.charOPCUAVersionSN, "%d", 6);
	strcpy(varIMMSN.charOPCUAIP, "192.168.200.156");
	strcpy(varIMMSN.charOPCUAPort, "48020");
	strcpy(varIMMSN.charModbusIP, "192.168.200.69");
	strcpy(varIMMSN.charModbusPort, "502");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	//sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_SIGNAL_LOOP);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_MULTIPLE_LOOP);
	intIMMStationID += 1; //11
	intIMMModelSN = 2;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "1119", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

	//GizMo-K20
	fprintf(stderr, "%-55s", "Inserting IMM:GizMo-K20 to Database...");
	sprintf(varIMMSN.charOPCUAVersionSN, "%d", 7);
	strcpy(varIMMSN.charOPCUAIP, "192.168.200.156");
	strcpy(varIMMSN.charOPCUAPort, "48020");
	strcpy(varIMMSN.charModbusIP, "192.168.200.70");
	strcpy(varIMMSN.charModbusPort, "502");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	//sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_SIGNAL_LOOP);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_MULTIPLE_LOOP);
	intIMMStationID += 1; //12
	intIMMModelSN = 2;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "1120", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

	//GizMo-K25(TBD)
	fprintf(stderr, "%-55s", "Inserting IMM:GizMo-K25 to Database...");
	sprintf(varIMMSN.charOPCUAVersionSN, "%d", 1); //TBD
	strcpy(varIMMSN.charOPCUAIP, "127.0.0.1");
	strcpy(varIMMSN.charOPCUAPort, "9527");
	strcpy(varIMMSN.charModbusIP, "192.168.200.75");
	strcpy(varIMMSN.charModbusPort, "502");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_MULTIPLE_LOOP);
	intIMMStationID += 1; //13
	intIMMModelSN = 2;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "1125", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

	//GizMo-K26
	fprintf(stderr, "%-55s", "Inserting IMM:GizMo-K26 to Database...");
	sprintf(varIMMSN.charOPCUAVersionSN, "%d", 8);
	strcpy(varIMMSN.charOPCUAIP, "192.168.200.153");
	strcpy(varIMMSN.charOPCUAPort, "4840");
	strcpy(varIMMSN.charModbusIP, "192.168.200.76");
	strcpy(varIMMSN.charModbusPort, "502");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	//sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_SIGNAL_LOOP);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_MULTIPLE_LOOP);
	intIMMStationID += 1; //14
	intIMMModelSN = 2;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "1126", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

	//ChiMei-ABS
	fprintf(stderr, "%-55s", "Inserting IMM:CHIMEI-ABS to Database...");
	strcpy(varIMMSN.charOPCUAVersionSN, "9");
	strcpy(varIMMSN.charOPCUAIP, "192.168.200.103");
	strcpy(varIMMSN.charOPCUAPort, "48020");
	strcpy(varIMMSN.charModbusIP, "192.168.200.102");
	strcpy(varIMMSN.charModbusPort, "502");
	//sprintf(varIMMSN.charModbusMoldSignalSource, "");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	//sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_OPCUA);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_MULTIPLE_LOOP);
	intIMMStationID += 1; //15
	intIMMModelSN = 2;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "CHIMEI-ABS", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

	//ChiMei-MABS-AS
	fprintf(stderr, "%-55s", "Inserting IMM:ChiMei-MABS-AS to Database...");
	strcpy(varIMMSN.charOPCUAVersionSN, "10");
	strcpy(varIMMSN.charOPCUAIP, "192.168.200.104");
	strcpy(varIMMSN.charOPCUAPort, "48020");
	strcpy(varIMMSN.charModbusIP, "192.168.200.102");
	strcpy(varIMMSN.charModbusPort, "502");
	//sprintf(varIMMSN.charModbusMoldSignalSource, "");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	//sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_OPCUA);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_MULTIPLE_LOOP);
	intIMMStationID += 1; //16
	intIMMModelSN = 2;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "CHIMEI-MABS-AS", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

	//LonSo-IMM1
	fprintf(stderr, "%-55s", "Inserting IMM:LonSo-IMM1 to Database...");
	strcpy(varIMMSN.charOPCUAVersionSN, "11"); //13
	strcpy(varIMMSN.charOPCUAIP, "192.168.200.2");
	strcpy(varIMMSN.charOPCUAPort, "8884");
	strcpy(varIMMSN.charModbusIP, "192.168.200.102");
	strcpy(varIMMSN.charModbusPort, "502");
	//sprintf(varIMMSN.charModbusMoldSignalSource, "");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	//sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_OPCUA);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_MULTIPLE_LOOP);
	intIMMStationID += 1; //19
	intIMMModelSN = 2;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "LonSo-IMM1", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

	//ITRI-VICTOR-IMM
	fprintf(stderr, "%-55s", "Inserting IMM:ITRI-VICTOR-IMM-1 to Database...");
	strcpy(varIMMSN.charOPCUAVersionSN, "12");
	strcpy(varIMMSN.charOPCUAIP, "192.168.200.100");
	strcpy(varIMMSN.charOPCUAPort, "4840");
	strcpy(varIMMSN.charModbusIP, "192.168.200.102");
	strcpy(varIMMSN.charModbusPort, "502");
	//sprintf(varIMMSN.charModbusMoldSignalSource, "");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	//sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_OPCUA);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_MULTIPLE_LOOP);
	intIMMStationID += 1; //19
	intIMMModelSN = 2;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "ITRI-VICTOR-IMM-1", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

	//ITRI-VICTOR-IMM
	fprintf(stderr, "%-55s", "Inserting IMM:ITRI-VICTOR-IMM-2 to Database...");
	strcpy(varIMMSN.charOPCUAVersionSN, "13");
	strcpy(varIMMSN.charOPCUAIP, "192.168.200.100");
	strcpy(varIMMSN.charOPCUAPort, "4840");
	strcpy(varIMMSN.charModbusIP, "192.168.200.102");
	strcpy(varIMMSN.charModbusPort, "502");
	//sprintf(varIMMSN.charModbusMoldSignalSource, "");
	sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_MODBUS);
	//sprintf(varIMMSN.charModbusMoldSignalSource, "%d", MODBUS_MOLD_SIGNAL_SOURCE_OPCUA);
	sprintf(varIMMSN.charModbusMoldSignalType, "%d", MODBUS_MOLD_SIGNAL_TYPE_MULTIPLE_LOOP);
	intIMMStationID += 1; //19
	intIMMModelSN = 2;
	intRetval = IMM_InsertIMM(mysqlCon, &intIMMSN, "ITRI-VICTOR-IMM-2", intIMMStationID, intIMMModelSN, varIMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
		fprintf(stderr, "[\033[33mWarning\033[m]\n");

#endif

/* Create MO */
	//ITRI-TEST-2
	//========================================================================================================================================
	fprintf(stderr, "%-55s", "Creating MO:ITRI-TEST-1 to Database...");
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
	//stMOReferenceMOSN varMOReferenceMOSN;
	//strcpy(varMOReferenceMOSN.charMOReferenceMOSN, " ");

	//int MO_CreateMO(MYSQL mysqlCon, unsigned int *intMOSN, const char *charMOID,
	//			unsigned int intIMMSN, unsigned int intProdSN, unsigned int intMoldSN, unsigned int intMaterialSN,
	//			struct tm tmExpectedStartTime, struct tm tmExpectedEndTime,
	//			unsigned int intPeicePerShot, unsigned int intExpectedProductVolume, stMOReferenceMOSN varMOReferenceMOSN);
	intIMMSN = IMMSN_LOCALHOST;
	intProdSN = 1;
	intMoldSN = 1;
	intMaterialSN = 1;
	intRetval = MO_CreateMO(mysqlCon, &intMOSN, "ITRI-TEST-1", intIMMSN, intProdSN, intMoldSN, intMaterialSN, tmExpectedStartTime, tmExpectedEndTime, 1, 10);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	//MO_UpdateMO(mysqlCon, intMOSN, "Bermuda-Update-MO-Name", intIMMSN, 1, 1, 1, tmExpectedStartTime, tmExpectedEndTime, 1, 10000);

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 1:Pressure1 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-A1");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	////strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorChannelSN,"2");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorConvertRatio,"1");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 2;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 2:Pressure2 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-A2");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorChannelSN,"3");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorConvertRatio,"1");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 3;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 3:Pressure3 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-A4");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	////strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorChannelSN,"4");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorConvertRatio,"1");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 4;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 4:Temperature1 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Temperature-1");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorChannelSN,"5");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorConvertRatio,"1");
	intCustomSensorModelSN = intCavityTemperatureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 5;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 5:Temperature2 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Temperature-2");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorChannelSN,"6");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorConvertRatio,"1");
	intCustomSensorModelSN = intCavityTemperatureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 6;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 6:Temperature3 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Temperature-3");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorChannelSN,"7");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorConvertRatio,"1");
	intCustomSensorModelSN = intCavityTemperatureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 7;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Accept Criteria 2:Weight for MOSN:", intMOSN);
	strcpy(varMOAcceptCriteriaSN.charAcceptCriteriaMeta, "Product Weight");
	strcpy(varMOAcceptCriteriaSN.charAcceptCriteriaMinTH, "");
	strcpy(varMOAcceptCriteriaSN.charAcceptCriteriaMaxTH, "");
	intRetval = MO_InsertMOAcceptCriteriaSNList(mysqlCon, intMOSN, 2, ACCEPTCRITERIA_CATEGORY_VALUE, "g", varMOAcceptCriteriaSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	//ITRI-TEST-2
	//========================================================================================================================================
	fprintf(stderr, "%-55s", "Creating MO:ITRI-TEST-2 to Database...");
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
	//stMOReferenceMOSN varMOReferenceMOSN;
	//strcpy(varMOReferenceMOSN.charMOReferenceMOSN, " ");

	//int MO_CreateMO(MYSQL mysqlCon, unsigned int *intMOSN, const char *charMOID,
	//			unsigned int intIMMSN, unsigned int intProdSN, unsigned int intMoldSN, unsigned int intMaterialSN,
	//			struct tm tmExpectedStartTime, struct tm tmExpectedEndTime,
	//			unsigned int intPeicePerShot, unsigned int intExpectedProductVolume, stMOReferenceMOSN varMOReferenceMOSN);
	intIMMSN = IMMSN_LOCALHOST;
	intProdSN = 1;
	intMoldSN = 1;
	intMaterialSN = 1;
	intRetval = MO_CreateMO(mysqlCon, &intMOSN, "ITRI-TEST-2", intIMMSN, intProdSN, intMoldSN, intMaterialSN, tmExpectedStartTime, tmExpectedEndTime, 1, 10);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 1:Pressure1 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-A1");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	////strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorChannelSN,"2");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorConvertRatio,"1");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 2;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 2:Pressure2 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-A2");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorChannelSN,"3");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorConvertRatio,"1");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 3;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 3:Pressure3 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-A4");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	////strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorChannelSN,"4");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorConvertRatio,"1");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 4;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 4:Temperature1 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Temperature-1");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorChannelSN,"5");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorConvertRatio,"1");
	intCustomSensorModelSN = intCavityTemperatureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 5;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 5:Temperature2 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Temperature-2");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorChannelSN,"6");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorConvertRatio,"1");
	intCustomSensorModelSN = intCavityTemperatureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 6;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 6:Temperature3 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Temperature-3");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorChannelSN,"7");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorConvertRatio,"1");
	intCustomSensorModelSN = intCavityTemperatureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 7;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Accept Criteria 2:Weight for MOSN:", intMOSN);
	strcpy(varMOAcceptCriteriaSN.charAcceptCriteriaMeta, "Product Weight");
	strcpy(varMOAcceptCriteriaSN.charAcceptCriteriaMinTH, "");
	strcpy(varMOAcceptCriteriaSN.charAcceptCriteriaMaxTH, "");
	intRetval = MO_InsertMOAcceptCriteriaSNList(mysqlCon, intMOSN, 2, ACCEPTCRITERIA_CATEGORY_VALUE, "g", varMOAcceptCriteriaSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	//ITRI-DemoMO-1
	//========================================================================================================================================
	fprintf(stderr, "%-55s", "Creating MO:ITRI-DemoMO-1 to Database...");
	tmExpectedStartTime.tm_year = 2021;
	tmExpectedStartTime.tm_mon = 3;
	tmExpectedStartTime.tm_mday = 16;
	tmExpectedStartTime.tm_hour = 7;
	tmExpectedStartTime.tm_min = 30;
	tmExpectedStartTime.tm_sec = 0;

	tmExpectedEndTime.tm_year = 2021;
	tmExpectedEndTime.tm_mon = 3;
	tmExpectedEndTime.tm_mday = 16;
	tmExpectedEndTime.tm_hour = 16;
	tmExpectedEndTime.tm_min = 30;
	tmExpectedEndTime.tm_sec = 0;
	//stMOReferenceMOSN varMOReferenceMOSN;
	//strcpy(varMOReferenceMOSN.charMOReferenceMOSN, " ");

	//int MO_CreateMO(MYSQL mysqlCon, unsigned int *intMOSN, const char *charMOID,
	//			unsigned int intIMMSN, unsigned int intProdSN, unsigned int intMoldSN, unsigned int intMaterialSN,
	//			struct tm tmExpectedStartTime, struct tm tmExpectedEndTime,
	//			unsigned int intPeicePerShot, unsigned int intExpectedProductVolume, stMOReferenceMOSN varMOReferenceMOSN);
	intIMMSN = IMMSN_DEMO_IMM;
	intProdSN = 1;
	intMoldSN = 1;
	intMaterialSN = 1;
	intRetval = MO_CreateMO(mysqlCon, &intMOSN, "ITRI-DemoMO-1", intIMMSN, intProdSN, intMoldSN, intMaterialSN, tmExpectedStartTime, tmExpectedEndTime, 1, 1000);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 1: OilPressure for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "OilPressure");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorChannelSN,"2");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorConvertRatio,"1");
	intCustomSensorModelSN = intSystemPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 2;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	//ITRI-DemoMO-2
	//========================================================================================================================================
	fprintf(stderr, "%-55s", "Creating MO:ITRI-DemoMO-2 to Database...");
	tmExpectedStartTime.tm_year = 2021;
	tmExpectedStartTime.tm_mon = 3;
	tmExpectedStartTime.tm_mday = 16;
	tmExpectedStartTime.tm_hour = 7;
	tmExpectedStartTime.tm_min = 30;
	tmExpectedStartTime.tm_sec = 0;

	tmExpectedEndTime.tm_year = 2021;
	tmExpectedEndTime.tm_mon = 3;
	tmExpectedEndTime.tm_mday = 16;
	tmExpectedEndTime.tm_hour = 16;
	tmExpectedEndTime.tm_min = 30;
	tmExpectedEndTime.tm_sec = 0;
	//stMOReferenceMOSN varMOReferenceMOSN;
	//strcpy(varMOReferenceMOSN.charMOReferenceMOSN, " ");

	//int MO_CreateMO(MYSQL mysqlCon, unsigned int *intMOSN, const char *charMOID,
	//			unsigned int intIMMSN, unsigned int intProdSN, unsigned int intMoldSN, unsigned int intMaterialSN,
	//			struct tm tmExpectedStartTime, struct tm tmExpectedEndTime,
	//			unsigned int intPeicePerShot, unsigned int intExpectedProductVolume, stMOReferenceMOSN varMOReferenceMOSN);
	intIMMSN = IMMSN_DEMO_IMM;
	intProdSN = 1;
	intMoldSN = 1;
	intMaterialSN = 1;
	intRetval = MO_CreateMO(mysqlCon, &intMOSN, "ITRI-DemoMO-2", intIMMSN, intProdSN, intMoldSN, intMaterialSN, tmExpectedStartTime, tmExpectedEndTime, 1, 50);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 1: Pressure1 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-1");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 2;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 2: Pressure2 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-2");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 3;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 3: OilPressure for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "OilPressure");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorChannelSN,"2");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorConvertRatio,"1");
	intCustomSensorModelSN = intSystemPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 4;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	//GinzMO-TestMO-C
	//========================================================================================================================================
	fprintf(stderr, "%-55s", "Creating MO:GinzMO-TestMO-C to Database...");
	tmExpectedStartTime.tm_year = 2021;
	tmExpectedStartTime.tm_mon = 2;
	tmExpectedStartTime.tm_mday = 4;
	tmExpectedStartTime.tm_hour = 10;
	tmExpectedStartTime.tm_min = 0;
	tmExpectedStartTime.tm_sec = 0;

	tmExpectedEndTime.tm_year = 2020;
	tmExpectedEndTime.tm_mon = 2;
	tmExpectedEndTime.tm_mday = 4;
	tmExpectedEndTime.tm_hour = 16;
	tmExpectedEndTime.tm_min = 30;
	tmExpectedEndTime.tm_sec = 0;
	//stMOReferenceMOSN varMOReferenceMOSN;
	//strcpy(varMOReferenceMOSN.charMOReferenceMOSN, " ");

	//int MO_CreateMO(MYSQL mysqlCon, unsigned int *intMOSN, const char *charMOID,
	//			unsigned int intIMMSN, unsigned int intProdSN, unsigned int intMoldSN, unsigned int intMaterialSN,
	//			struct tm tmExpectedStartTime, struct tm tmExpectedEndTime,
	//			unsigned int intPeicePerShot, unsigned int intExpectedProductVolume, stMOReferenceMOSN varMOReferenceMOSN);
	intIMMSN = IMMSN_GINZMO_K18;
	intProdSN = 1;
	intMoldSN = 1;
	intMaterialSN = 1;
	intRetval = MO_CreateMO(mysqlCon, &intMOSN, "GinzMO-TestMO-C", intIMMSN, intProdSN, intMoldSN, intMaterialSN, tmExpectedStartTime, tmExpectedEndTime, 1, 10000);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 1: OilPressure for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "OilPressure");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	intCustomSensorModelSN = intSystemPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 4;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 1: Pressure1 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-1");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 2;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 2: Pressure2 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-2");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 3;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	//GinzMO-TestMO
	//========================================================================================================================================
	fprintf(stderr, "%-55s", "Creating MO:GinzMO-TestMO to Database...");
	tmExpectedStartTime.tm_year = 2021;
	tmExpectedStartTime.tm_mon = 2;
	tmExpectedStartTime.tm_mday = 4;
	tmExpectedStartTime.tm_hour = 10;
	tmExpectedStartTime.tm_min = 0;
	tmExpectedStartTime.tm_sec = 0;

	tmExpectedEndTime.tm_year = 2020;
	tmExpectedEndTime.tm_mon = 2;
	tmExpectedEndTime.tm_mday = 4;
	tmExpectedEndTime.tm_hour = 16;
	tmExpectedEndTime.tm_min = 30;
	tmExpectedEndTime.tm_sec = 0;
	//stMOReferenceMOSN varMOReferenceMOSN;
	//strcpy(varMOReferenceMOSN.charMOReferenceMOSN, " ");

	//int MO_CreateMO(MYSQL mysqlCon, unsigned int *intMOSN, const char *charMOID,
	//			unsigned int intIMMSN, unsigned int intProdSN, unsigned int intMoldSN, unsigned int intMaterialSN,
	//			struct tm tmExpectedStartTime, struct tm tmExpectedEndTime,
	//			unsigned int intPeicePerShot, unsigned int intExpectedProductVolume, stMOReferenceMOSN varMOReferenceMOSN);
	intIMMSN = IMMSN_GINZMO_K18;
	intProdSN = 1;
	intMoldSN = 1;
	intMaterialSN = 1;
	intRetval = MO_CreateMO(mysqlCon, &intMOSN, "GinzMO-TestMO", intIMMSN, intProdSN, intMoldSN, intMaterialSN, tmExpectedStartTime, tmExpectedEndTime, 1, 10000);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 1: OilPressure for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "OilPressure");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorChannelSN,"2");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorConvertRatio,"1");
	intCustomSensorModelSN = intSystemPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 2;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Accept Criteria 2:Weight for MOSN:", intMOSN);
	strcpy(varMOAcceptCriteriaSN.charAcceptCriteriaMeta, "Product Weight");
	strcpy(varMOAcceptCriteriaSN.charAcceptCriteriaMinTH, "");
	strcpy(varMOAcceptCriteriaSN.charAcceptCriteriaMaxTH, "");
	intRetval = MO_InsertMOAcceptCriteriaSNList(mysqlCon, intMOSN, 2, ACCEPTCRITERIA_CATEGORY_VALUE, "g", varMOAcceptCriteriaSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	//ChiMei-ABS
	//========================================================================================================================================
	fprintf(stderr, "%-55s", "Creating MO:ChiMei-ABS-TestMO to Database...");
	tmExpectedStartTime.tm_year = 2020;
	tmExpectedStartTime.tm_mon = 7;
	tmExpectedStartTime.tm_mday = 21;
	tmExpectedStartTime.tm_hour = 9;
	tmExpectedStartTime.tm_min = 0;
	tmExpectedStartTime.tm_sec = 0;

	tmExpectedEndTime.tm_year = 2020;
	tmExpectedEndTime.tm_mon = 7;
	tmExpectedEndTime.tm_mday = 21;
	tmExpectedEndTime.tm_hour = 16;
	tmExpectedEndTime.tm_min = 30;
	tmExpectedEndTime.tm_sec = 0;
	//stMOReferenceMOSN varMOReferenceMOSN;
	//strcpy(varMOReferenceMOSN.charMOReferenceMOSN, " ");

	//int MO_CreateMO(MYSQL mysqlCon, unsigned int *intMOSN, const char *charMOID,
	//			unsigned int intIMMSN, unsigned int intProdSN, unsigned int intMoldSN, unsigned int intMaterialSN,
	//			struct tm tmExpectedStartTime, struct tm tmExpectedEndTime,
	//			unsigned int intPeicePerShot, unsigned int intExpectedProductVolume, stMOReferenceMOSN varMOReferenceMOSN);
	intIMMSN = IMMSN_CHIMEI_ABS;
	intProdSN = 1;
	intMoldSN = 1;
	intMaterialSN = 1;
	intRetval = MO_CreateMO(mysqlCon, &intMOSN, "ChiMei-ABS-TestMO", intIMMSN, intProdSN, intMoldSN, intMaterialSN, tmExpectedStartTime, tmExpectedEndTime, 1, 10000);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 1: Pressure1 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-1");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 2;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 2: Pressure1 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-2");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 3;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 3: Pressure1 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-3");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 4;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 4: Pressure1 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-4");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 5;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	//ChiMei-MABSAS
	//========================================================================================================================================
	fprintf(stderr, "%-55s", "Creating MO:ChiMei-MABSAS-TestMO to Database...");
	tmExpectedStartTime.tm_year = 2020;
	tmExpectedStartTime.tm_mon = 7;
	tmExpectedStartTime.tm_mday = 21;
	tmExpectedStartTime.tm_hour = 9;
	tmExpectedStartTime.tm_min = 0;
	tmExpectedStartTime.tm_sec = 0;

	tmExpectedEndTime.tm_year = 2020;
	tmExpectedEndTime.tm_mon = 7;
	tmExpectedEndTime.tm_mday = 21;
	tmExpectedEndTime.tm_hour = 16;
	tmExpectedEndTime.tm_min = 30;
	tmExpectedEndTime.tm_sec = 0;
	//stMOReferenceMOSN varMOReferenceMOSN;
	//strcpy(varMOReferenceMOSN.charMOReferenceMOSN, " ");

	//int MO_CreateMO(MYSQL mysqlCon, unsigned int *intMOSN, const char *charMOID,
	//			unsigned int intIMMSN, unsigned int intProdSN, unsigned int intMoldSN, unsigned int intMaterialSN,
	//			struct tm tmExpectedStartTime, struct tm tmExpectedEndTime,
	//			unsigned int intPeicePerShot, unsigned int intExpectedProductVolume, stMOReferenceMOSN varMOReferenceMOSN);
	intIMMSN = IMMSN_CHIMEI_MABSAS;
	intProdSN = 1;
	intMoldSN = 1;
	intMaterialSN = 1;
	intRetval = MO_CreateMO(mysqlCon, &intMOSN, "ChiMei-MABSAS-TestMO", intIMMSN, intProdSN, intMoldSN, intMaterialSN, tmExpectedStartTime, tmExpectedEndTime, 1, 10000);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 1: Pressure1 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-1");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 2;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 2: Pressure1 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-2");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 3;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 3: Pressure1 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-3");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 4;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 4: Pressure1 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-4");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 5;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	//LonSo-TestMO
	//========================================================================================================================================
	fprintf(stderr, "%-55s", "Creating MO:LonSo-TestMO to Database...");
	tmExpectedStartTime.tm_year = 2021;
	tmExpectedStartTime.tm_mon = 8;
	tmExpectedStartTime.tm_mday = 25;
	tmExpectedStartTime.tm_hour = 14;
	tmExpectedStartTime.tm_min = 0;
	tmExpectedStartTime.tm_sec = 0;

	tmExpectedEndTime.tm_year = 2020;
	tmExpectedEndTime.tm_mon = 8;
	tmExpectedEndTime.tm_mday = 31;
	tmExpectedEndTime.tm_hour = 23;
	tmExpectedEndTime.tm_min = 00;
	tmExpectedEndTime.tm_sec = 0;
	//stMOReferenceMOSN varMOReferenceMOSN;
	//strcpy(varMOReferenceMOSN.charMOReferenceMOSN, " ");

	//int MO_CreateMO(MYSQL mysqlCon, unsigned int *intMOSN, const char *charMOID,
	//			unsigned int intIMMSN, unsigned int intProdSN, unsigned int intMoldSN, unsigned int intMaterialSN,
	//			struct tm tmExpectedStartTime, struct tm tmExpectedEndTime,
	//			unsigned int intPeicePerShot, unsigned int intExpectedProductVolume, stMOReferenceMOSN varMOReferenceMOSN);
	intIMMSN = IMMSN_LONSO;
	intProdSN = 1;
	intMoldSN = 1;
	intMaterialSN = 1;
	intRetval = MO_CreateMO(mysqlCon, &intMOSN, "LonSo-TestMO", intIMMSN, intProdSN, intMoldSN, intMaterialSN, tmExpectedStartTime, tmExpectedEndTime, 1, 10000);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 1: OilPressure for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "OilPressure");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorChannelSN,"2");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorConvertRatio,"1");
	intCustomSensorModelSN = intSystemPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 2;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	//ITRI-VICTOR-DEMO-1-MO
	//========================================================================================================================================
	fprintf(stderr, "%-55s", "Creating MO:ITRI-VICTOR-DEMO-1-MO to Database...");
	tmExpectedStartTime.tm_year = 2022;
	tmExpectedStartTime.tm_mon = 3;
	tmExpectedStartTime.tm_mday = 15;
	tmExpectedStartTime.tm_hour = 10;
	tmExpectedStartTime.tm_min = 0;
	tmExpectedStartTime.tm_sec = 0;

	tmExpectedEndTime.tm_year = 2022;
	tmExpectedEndTime.tm_mon = 12;
	tmExpectedEndTime.tm_mday = 30;
	tmExpectedEndTime.tm_hour = 0;
	tmExpectedEndTime.tm_min = 0;
	tmExpectedEndTime.tm_sec = 0;
	//stMOReferenceMOSN varMOReferenceMOSN;
	//strcpy(varMOReferenceMOSN.charMOReferenceMOSN, " ");

	//int MO_CreateMO(MYSQL mysqlCon, unsigned int *intMOSN, const char *charMOID,
	//			unsigned int intIMMSN, unsigned int intProdSN, unsigned int intMoldSN, unsigned int intMaterialSN,
	//			struct tm tmExpectedStartTime, struct tm tmExpectedEndTime,
	//			unsigned int intPeicePerShot, unsigned int intExpectedProductVolume, stMOReferenceMOSN varMOReferenceMOSN);
	intIMMSN = IMMSN_ITRI_VICTOR_DEMO_1;
	intProdSN = 1;
	intMoldSN = 1;
	intMaterialSN = 1;
	intRetval = MO_CreateMO(mysqlCon, &intMOSN, "ITRI-VICTOR-DEMO-1-MO", intIMMSN, intProdSN, intMoldSN, intMaterialSN, tmExpectedStartTime, tmExpectedEndTime, 1, 10000);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 1: Pressure1 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-1");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 4;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 2: Pressure2 for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "Pressure-2");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 5;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 3: InjPressure for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "InjectionPressure");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 2;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}


	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 4: CylinPressure for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "CylinderPressure");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	//intCustomSensorModelSN = intCylinderPressureCustomSensorModelSN;
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 6;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 5: PowerMeter for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "PowerMeter");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	intCustomSensorModelSN = intPowerMeterCustomSensorModelSN;
	intMOCustomSensorChannelSN = 3;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	//ITRI-VICTOR-DEMO-2-MO
	//========================================================================================================================================
	fprintf(stderr, "%-55s", "Creating MO:ITRI-VICTOR-DEMO-2-MO to Database...");
	tmExpectedStartTime.tm_year = 2022;
	tmExpectedStartTime.tm_mon = 4;
	tmExpectedStartTime.tm_mday = 7;
	tmExpectedStartTime.tm_hour = 10;
	tmExpectedStartTime.tm_min = 0;
	tmExpectedStartTime.tm_sec = 0;

	tmExpectedEndTime.tm_year = 2022;
	tmExpectedEndTime.tm_mon = 4;
	tmExpectedEndTime.tm_mday = 7;
	tmExpectedEndTime.tm_hour = 16;
	tmExpectedEndTime.tm_min = 30;
	tmExpectedEndTime.tm_sec = 0;
	//stMOReferenceMOSN varMOReferenceMOSN;
	//strcpy(varMOReferenceMOSN.charMOReferenceMOSN, " ");

	//int MO_CreateMO(MYSQL mysqlCon, unsigned int *intMOSN, const char *charMOID,
	//			unsigned int intIMMSN, unsigned int intProdSN, unsigned int intMoldSN, unsigned int intMaterialSN,
	//			struct tm tmExpectedStartTime, struct tm tmExpectedEndTime,
	//			unsigned int intPeicePerShot, unsigned int intExpectedProductVolume, stMOReferenceMOSN varMOReferenceMOSN);
	intIMMSN = IMMSN_ITRI_VICTOR_DEMO_2;
	intProdSN = 1;
	intMoldSN = 1;
	intMaterialSN = 1;
	intRetval = MO_CreateMO(mysqlCon, &intMOSN, "ITRI-VICTOR-DEMO-2-MO", intIMMSN, intProdSN, intMoldSN, intMaterialSN, tmExpectedStartTime, tmExpectedEndTime, 1, 10000);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "%-50s%-2d...", "Inserting Custom Sensor 1: InjPressure for MOSN:", intMOSN);
	strcpy(varMOCustomSensorSN.charMOCustomSensorMeta, "InjectionPressure");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMinTH, "");
	//strcpy(varMOCustomSensorSN.charMOCustomSensorMaxTH, "");
	intCustomSensorModelSN = intCavityPressureCustomSensorModelSN;
	intMOCustomSensorChannelSN = 2;
	doubleMOCustomSensorConvertRatio = 1.0;
	intRetval = MO_InsertMOCustomSensorSNList(mysqlCon, intMOSN, intCustomSensorModelSN, intMOCustomSensorChannelSN, doubleMOCustomSensorConvertRatio, varMOCustomSensorSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}

	intRetval = IMM_SetPrimaryIMM(mysqlCon, IMMSN_ITRI_VICTOR_DEMO_1);

#ifdef ENABLE_START_MOSN_1
	fprintf(stderr, "%s%d%-31s", "Update MOSN:1 to IMMSN:", START_MOSN_1_ON_IMMSN, "...");
	intRetval = MO_StartMOonIMM(mysqlCon, 1, START_MOSN_1_ON_IMMSN);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}
#endif

#ifdef ENABLE_AUTO_SUBSCRIBE_IMMSN_1
	OPCUA_WriteOPCUANodeValue(mysqlCon, 1, IMMPARA_DI_MOLD_RELEASED, "1");
	OPCUA_WriteOPCUANodeValue(mysqlCon, 1, IMMPARA_DI_MOLD_CLAMPED, "0");
	fprintf(stderr, "%-55s", "Subscribe IMMSN=1 (JianJie-1)...");
	intRetval = IMM_SubscribeOPCUA(mysqlCon, 1);
	if (intRetval == EXIT_SUCCESS)
		fprintf(stderr, "[\033[32mOK\033[m]\n");
	else
	{
		fprintf(stderr, "[\033[31mFail\033[m]\n");
		return EXIT_FAILURE;
	}
#endif

	//fprintf(stderr, "Database Creation Successfully.\n");

	return EXIT_SUCCESS;
	fprintf(stderr, "Press any key to start automatic shot procress...");
	getchar();
	while (boolIsStopSignal == false)
	{
		//getchar();
		srand((unsigned)time(NULL));

		intAutoIMM[0] = 0;
		intAutoIMM[1] = 0;
		intAutoIMM[2] = 0;
		intAutoIMM[3] = 0;
		intAutoIMM[4] = 0;
		intAutoIMM[5] = 0;
		intAutoIMM[6] = 0;
		intAutoIMM[7] = 0;

#ifdef ENABLE_IMMSN_1
		intAutoIMM[0] = (double)rand() / RAND_MAX <= AUTO_MODE_IMM_RAND ? 1 : 0;
#endif

		intAutoIMM[4] = 1;

		fprintf(stderr, "This round IMMSN:");
		for (int i = 0; i < 8; i++)
		{
			if (intAutoIMM[i] == 1)
			{
				fprintf(stderr, "[%d] ", i + 1);
			}
		}

		fprintf(stderr, "is ON\n");

#ifdef ENABLE_AUTO_MOLD_1
		if (intAutoIMM[0] == 1)
		{
			intShotSN[0]++;
			fprintf(stderr, "IMMSN:1 ShotSN:%lld MoldReleased\n", intShotSN[0]);
			OPCUA_WriteOPCUANodeValue(mysqlCon, 1, IMMPARA_DI_MOLD_RELEASED, "1");
		}
#endif

		//getchar();
		sleep(SLEEP_AFTER_MOLD_RELEASED);

#ifdef ENABLE_TUNE_IMMSN_1
		if (intAutoIMM[0] == 1 && (double)rand() / RAND_MAX <= AUTO_MODE_TUNEIMM_RAND)
		{
			fprintf(stderr, "IMMSN:1 Tunning IMM parameter\n");
			DEMO_SetIMMParaValue(charIMMParaValue);
			IMM_TuneIMMPara(mysqlCon, 1, 70012, charIMMParaValue);
		}
#endif

		sleep(SLEEP_AFTER_TUNE_IMM);

#ifdef ENABLE_IMM_SENSOR
		//Randomize IMM sensor value
		for (int i = 0; i < 8; i++)
		{
			if (intAutoIMM[i] == 1)
			{
				fprintf(stderr, "%-s%d%-14s", "Set synthetic IMM Sensor Data for IMMSN:", i + 1, "...");
				for (int j = OPCUA_FIRST_IMMSENSOR; j <= OPCUA_IMMPARA_NODE_NUM + OPCUA_IMMSENSOR_NODE_NUM; j++)
				{
					snprintf(charRandValue, TINY_STRING_SIZE, "%d", rand() % 1000);
					OPCUA_WriteOPCUANodeValue(mysqlCon, i + 1, j, charRandValue);
				}
				fprintf(stderr, "[\033[32mOK\033[m]\n");
			}
		}
#endif

#ifdef ENABLE_AUTO_MOLD_1
		if (intAutoIMM[0] == 1)
		{
			fprintf(stderr, "IMMSN:1 ShotSN:%lld MoldClosing\n", intShotSN[0]);
			OPCUA_WriteOPCUANodeValue(mysqlCon, 1, IMMPARA_DI_MOLD_RELEASED, "0");
		}
#endif
		sleep(SLEEP_AFTER_MOLD_CLAMPLING);

#ifdef ENABLE_AUTO_MOLD_1
		if (intAutoIMM[0] == 1)
		{
			fprintf(stderr, "IMMSN:1 ShotSN:%lld MoldClosed\n", intShotSN[0]);
			OPCUA_WriteOPCUANodeValue(mysqlCon, 1, IMMPARA_DI_MOLD_CLAMPED, "1");
		}
#endif

#ifdef ENABLE_MOLD_SENSOR
		for (int i = 0; i < 8; i++)
		{
			if (intAutoIMM[i] == 1)
			{

				DB_IMMSNtoMOSN(mysqlCon, i + 1, &intMOSN);

				fprintf(stderr, "%-s%d%s%d%-10s", "Inserting Custom Sensor Data for IMMSN:", i + 1, " MOSN:", intMOSN, "...");
				//Select the total number of MO sensor from INJPRO_Data_MO_[MOSN]_Info_Meta.MOCustomSensorSNList
				snprintf(charStatement, MAX_STRING_SIZE, "%s_%s_%s_%d_Info_Meta.MOCustomSensorSNList",
						 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
				intRetval = DB_SelectColumnMax(mysqlCon, charStatement, "MOCustomSensorSN", &charCountMOCustomSensorSN);
				intCountMOCustomSensorSN = atoi(charCountMOCustomSensorSN);

				//For each MO sensor
				for (int j = 1; j <= intCountMOCustomSensorSN; j++)
				{
					//Example: 10 secs cycle time
					for (int k = 0; k <= 20; k++)
					{
						//int DB_InsertMOCustomSensorData(MYSQL mysqlCon,unsigned int intMOSN,unsigned int intMOCustomSensorSN,unsigned int intTableSN,
						// unsigned int intShotSN,double doubleElapsedTime, double doubleValue);
						intRetval = DB_InsertMOCustomSensorData(mysqlCon, intMOSN, j, 1, intShotSN[i], (double)k / 10.0, (double)rand() / RAND_MAX);
					}
				}
				fprintf(stderr, "[\033[32mOK\033[m]\n");
			}
		}
#endif

		sleep(rand() % 2 + SLEEP_AFTER_MOLD_CLAMPED);

#ifdef ENABLE_AUTO_MOLD_1
		if (intAutoIMM[0] == 1)
		{
			fprintf(stderr, "IMMSN:1 ShotSN:%lld Releasing\n", intShotSN[0]);
			OPCUA_WriteOPCUANodeValue(mysqlCon, 1, IMMPARA_DI_MOLD_CLAMPED, "0");
		}
#endif

		sleep(SLEEP_AFTER_MOLD_RELEASING);
	}

#ifdef ENABLE_AUTO_SUBSCRIBE_IMMSN_1
	IMM_UnSubscribeOPCUA(mysqlCon, 1);
#endif

	//Close connection from MySQL Server
	mysql_close(&mysqlCon);

	return EXIT_SUCCESS;
}
