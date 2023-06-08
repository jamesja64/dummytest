#include <stdio.h>
#include <string.h>
//#include <signal.h>
//#include <time.h>
#include <mysql/mysql.h>
//#include <unistd.h>
#include "open62541.h"
#include "itri_posdss.h"
#include "config.h"

bool	boolIsStopSignal=false;
char	charOPCUAIP[15];
int	intOPCUAPort;

static void stateCallback (UA_Client *client, UA_ClientState clientState) {
	MYSQL	mysqlCon;
	int	intRetval;
	char	charErrMsg[MEDIUM_STRING_SIZE];
	char	charStatement[MAX_STRING_SIZE];

	mysql_init(&mysqlCon);
	if(!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD,NULL,0,NULL,CLIENT_FOUND_ROWS)){
		fprintf(stderr, "MYSQL connection failed.\n");
		if (mysql_errno(&mysqlCon)){
			fprintf(stderr, "MYSQL connection error %d: %s\n",mysql_errno(&mysqlCon),mysql_error(&mysqlCon));
		}
		return;
	}

	snprintf(charStatement,MAX_STRING_SIZE,"UPDATE %s_%s.IMMList SET OPCUAStatus=%d,IMMLastUpdateTime=NOW() WHERE OPCUAIP=%s AND OPCUAPort=%d",
			 SYS_NAME,DATA_DATABASE_NAME,clientState,);
			intRetval=mysql_query(&mysqlCon,charStatement);
			if(intRetval){
				snprintf(charErrMsg,MEDIUM_STRING_SIZE,
				 "Fail to update SubscribePID,OPCUAStatus and IMMLastUpdateTime for IMMSN=%d (%d):%s",
				 intIMMSN,mysql_errno(&mysqlCon),mysql_error(&mysqlCon));
				SYS_InsertSysErrMsg(mysqlCon,ERRCLASS_IMM,intMOSN,intIMMSN,mysql_errno(&mysqlCon),charErrMsg);
				return;
			}

	//Check Client State
	switch(clientState){
		case UA_CLIENTSTATE_DISCONNECTED:{
			fprintf(stderr, "OPCUA UA_CLIENTSTATE_DISCONNECTED\n");
			boolIsStopSignal=true;
		}break;
		case UA_CLIENTSTATE_CONNECTED:{
			fprintf(stderr, "OPCUA UA_CLIENTSTATE_CONNECTED\n");
		}break;
		case UA_CLIENTSTATE_SECURECHANNEL:{
			fprintf(stderr, "OPCUA UA_CLIENTSTATE_SECURECHANNEL\n");
		}break;
		case UA_CLIENTSTATE_SESSION:{
			fprintf(stderr, "OPCUA UA_CLIENTSTATE_SESSION\n");
		}break;
		case UA_CLIENTSTATE_SESSION_RENEWED:{
			fprintf(stderr, "OPCUA UA_CLIENTSTATE_SESSION_RENEWED\n");
		}break;


		/*
			#ifdef DEBUG_MODE_SUBSCRIBEIMM
			fprintf(stderr, "[IMMSN:%d]A session with the server is open\n",intIMMSN);
			#endif

			//Connect to MYSQL Server
			mysql_init(&mysqlCon);
			if(!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD,NULL,0,NULL,CLIENT_FOUND_ROWS)){
				fprintf(stderr, "MYSQL connection failed.\n");
				if (mysql_errno(&mysqlCon)){
					fprintf(stderr, "MYSQL connection error %d: %s\n",mysql_errno(&mysqlCon),mysql_error(&mysqlCon));
				}
				return;
			}

			//Update OPCUAStatus=RUNNING,SubscribePID and IMMLastUpdateTime to POSDSS_Data.IMMList
			
			subscribeIMM(client,mysqlCon);
			mysql_close(&mysqlCon);

		*/
	}
	return;
}

int main(int argc, char *argv[])
{

	/*
	int	intIMMModelSN;
	int	intRetval;
	char	charEndPointURL[MEDIUM_STRING_SIZE];
	char	charStatement[MAX_STRING_SIZE];
	char	charErrMsg[MEDIUM_STRING_SIZE];
	char	charTableName[MEDIUM_STRING_SIZE];
	char	*charShotSN;
	char	*charIMMParaSN;
	bool	boolHasInsertErrMsg=false;
	MYSQL	mysqlCon;
	*/

	//Get IP and Port of OPC UA Server
	if(argc==5 && strcmp(argv[1],"-ip")==0 && strcmp(argv[3],"-port")==0){
		strcpy(charOPCUAIP,argv[2]);
		intOPCUAPort=atoi(argv[4]);		
	}else{
		snprintf(charErrMsg,MEDIUM_STRING_SIZE,"Fail to listen IMM %s:%d due to parameter format error");
		//SYS_InsertSysErrMsg(mysqlCon,ERRCLASS_IMM,0,intIMMSN,EXIT_FAILURE,charErrMsg);
		return EXIT_FAILURE;
	}

	//Connect to MYSQL Server
	/*
	mysql_init(&mysqlCon);
	if(!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD,NULL,0,NULL,CLIENT_FOUND_ROWS)){
		fprintf(stderr, "MYSQL connection failed.\n");
		if (mysql_errno(&mysqlCon)){
			fprintf(stderr, "MYSQL connection error %d: %s\n",mysql_errno(&mysqlCon),mysql_error(&mysqlCon));
		}
		return EXIT_FAILURE;
	}*/

	UA_ClientConfig config=UA_ClientConfig_default;
	config.stateCallback=stateCallback;
	config.subscriptionInactivityCallback=subscriptionInactivityCallback;
	//config.secureChannelLifeTime=10;
	//config.inactivityCallback=inactivityCallback;
	//config.connectivityCheckInterval=2000;
	//config.subscriptionInactivityCallback=subscriptionInactivityCallback;
	UA_Client *client=UA_Client_new(config);


	snprintf(charEndPointURL,MEDIUM_STRING_SIZE,"opc.tcp://%s:%d",charOPCUAIP,intOPCUAPort);
	while(boolIsStopSignal==false){
		fprintf(stderr,"Connect to %s...\n",charEndPointURL);
		fprintf(stderr,"Connectstate=%d\n",UA_Client_getState(client));
		intRetval=UA_Client_connect(client, charEndPointURL);
		if(intRetval != UA_STATUSCODE_GOOD) {
			fprintf(stderr,"Not connected. Retrying to connect in %d ms\n",UA_CLIENT_INTERVAL);


		}else
			boolHasInsertErrMsg=false;
		UA_Client_runAsync(client,UA_CLIENT_INTERVAL);
	};
	UA_Client_delete(client);


	if(boolHasInsertErrMsg==false){
		snprintf(charErrMsg,MEDIUM_STRING_SIZE,"Fail to subscribe IMM:%d due to OPC UA connection fail",intIMMSN);
		SYS_InsertSysErrMsg(mysqlCon,ERRCLASS_OPCUA,intMOSN,intIMMSN,mysql_errno(&mysqlCon),charErrMsg);
		boolHasInsertErrMsg=true;
	}




	return EXIT_SUCCESS;

}
