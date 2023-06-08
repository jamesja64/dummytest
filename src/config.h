#define DEBUG_MODE
#define DEBUG_MODE_MAIN
#define DEBUG_MODE_SUBSCRIBE_OPCUA

#define DEBUG_MODE_SUBSCRIBE_MODBUS
#define DEBUG_MODE_SUBSCRIBE_OPCUA_MONITOR_ITEM

//#define CARBON_EMISSION_MODE_ON
#define CARBON_EMISSION_MODE_OFF

#define PRIMARY_IMMSN 18

#define IMMSN_LOCALHOST 1
#define IMMSN_DEMO_IMM 2
#define IMMSN_GINZMO_K03 3
#define IMMSN_GINZMO_K04 4
#define IMMSN_GINZMO_K06 5
#define IMMSN_GINZMO_K11 6
#define IMMSN_GINZMO_K12 7
#define IMMSN_GINZMO_K13 8
#define IMMSN_GINZMO_K14 9
#define IMMSN_GINZMO_K18 10
#define IMMSN_GINZMO_K19 11
#define IMMSN_GINZMO_K20 12
#define IMMSN_GINZMO_K25 13
#define IMMSN_GINZMO_K26 14
#define IMMSN_CHIMEI_ABS 15
#define IMMSN_CHIMEI_MABSAS 16
#define IMMSN_LONSO 17
#define IMMSN_ITRI_VICTOR_DEMO_1 18
#define IMMSN_ITRI_VICTOR_DEMO_2 19


/*
 IMMSNList
  1: Localhost
  2: DEMO-IMM
  3: GinzMO-K03
  4: GinzMO-K04
  5: GinzMO-K06
  6: GinzMO-K11
  7: GinzMO-K12
  8: GinzMO-K13
  9: GinzMO-K14
 10: GinzMO-K18
 11: GinzMO-K19
 12: GinzMO-K20
 13: GinzMO-K25
 14: GinzMO-K26
 15: ChiMei-ABS
 16: ChiMei-MABS-AS
 17: ETI-IMM1
 18: YongChangs-IMM1
 19: LonSo-IMM1
 20: Minjoint-JonWai
 21: Minjoint-HuaChin
*/

#define IGNORE_SYS_PRESSURE_STAGE3 0

#define MODBUS_CLIENT_TIME_OUT_SEC 1
#define MODBUS_MAX_CYCLE_TIME 600
#define UA_CLIENT_TIME_OUT_SEC 1
#define UA_CLIENT_INTERVAL 1

#define WATCHDOG_CHECK_INTEVAL 5.0
#define MODBUS_CONNECT_INTEVAL 10.0
#define OPCUA_CONNECT_INTEVAL 2.0

//#define ENABLE_SET_ALL_MONITOR_ON

//#define ENABLE_SPCCL_ROUNDSN_CHECK
//#define ENABLE_QCCL_ROUNDSN_CHECK
//#define ENABLE_ECCL_ROUNDSN_CHECK

//#define ENABLE_STOP_MOSN_1
/*
#define ENABLE_STOP_MOSN_2
#define ENABLE_STOP_MOSN_3
#define ENABLE_STOP_MOSN_4
*/
//#define ENABLE_STOP_MOSN_5

//#define ENABLE_STOP_MOSN_6
/*#define ENABLE_STOP_MOSN_7
#define ENABLE_STOP_MOSN_8
#define ENABLE_STOP_MOSN_9
*/

#define STOP_MOSN_1_ON_IMMSN 1

/* NEVER USE */
//#define ENABLE_DELETE_ALLMO
//#define ENABLE_DELETE_INJPRO
//#define ENABLE_DELETE_INJPRO_EXIT
#define ENABLE_CREATE_INJPRO
#define ENABLE_INSERT_INDEX
#define ENABLE_INSERT_IMM
#define ENABLE_CREATE_MOSN_1
#define START_MOSN_1_ON_IMMSN 1

#define YIELD_RATE_USE_ACTUAL 1
#define YIELD_RATE_USE_PREDICT 0

//#define ENABLE_AUTO_SUBSCRIBE_IMMSN_1

#define AUTO_MODE_IMM_RAND 0.8
#define AUTO_MODE_TUNEIMM_RAND 0.5
#define AUTO_MODE_RAND 0.8

#define SLEEP_AFTER_MOLD_RELEASED 4
#define SLEEP_AFTER_TUNE_IMM 4
#define SLEEP_AFTER_MOLD_CLAMPLING 1
#define SLEEP_AFTER_MOLD_CLAMPED 2
#define SLEEP_AFTER_MOLD_RELEASING 4
//#define ENABLE_IMMSN_1
//#define ENABLE_TUNE_IMMSN_1
//#define ENABLE_AUTO_MOLD_1
#define ENABLE_IMM_SENSOR
//#define ENABLE_CAVITY_SENSOR
//#define ENABLE_QUALITY_INFERENCE
#define ENABLE_CUSTOM_SENSOR_FEATURE_VALUE
#define MODBUS_INSERT_ENABLE

#define MAX_STRING_SIZE 65535
#define LONG_STRING_SIZE 1000
#define MEDIUM_STRING_SIZE 250
#define SMALL_STRING_SIZE 50
#define TINY_STRING_SIZE 20

//#define MYSQL_SERVER_IP         "127.0.0.1"
#define MYSQL_SERVER_IP       "my-sql"
//#define MYSQL_SERVER_IP		"140.96.196.12"
//#define MYSQL_SERVER_IP		"202.39.234.26"
//#define MYSQL_SERVER_IP		"192.168.200.101"
//#define MYSQL_SERVER_IP		192.168.77.133"
#define MYSQL_USER_NAME "itri"
#define MYSQL_PASSWORD "itrimstcm0202a"
//#define MYSQL_USER_NAME "bermuda"
//#define MYSQL_PASSWORD "itrismtcm0202a"
//#define MYSQL_USER_NAME "chang"
//#define MYSQL_PASSWORD "0"

#define VIRTUAL_OPCUA_SERVER_IMMSN 0
#define VIRTUAL_OPCUA_SERVER_OPCUA_VERSIONSN 1
#define VIRTUAL_OPCUA_SERVER_IP "127.0.0.1"
#define VIRTUAL_OPCUA_SERVER_PORT 9527
#define VIRTUAL_OPCUA_SERVER_NAMESPACE 1