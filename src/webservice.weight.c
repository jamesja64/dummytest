#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <json-c/json.h>
#include "fcgi_config.h"
#include "itri_injpro.h"
#include "fcgiapp.h"
#include <regex.h>

#define HTTP_REQUEST_TYPE_UNKNOW 0
#define HTTP_REQUEST_TYPE_GET 1
#define HTTP_REQUEST_TYPE_POST 2

//  Function List
int getHTTPRequestMethodType(char *charRequestMethod);
int regexGetRequestQueryString_MOSN(char *charQueryString);
int regexGetRequestQueryString_OPCUAVersionSN(char *charQueryString);
int regexGetRequestQueryString_IMMSN(char *charQueryString);
int regexGetRequestQueryString_ProdSN(char *charQueryString);
int regexGetRequestQueryString_ShotSN(char *charQueryString);
int regexGetRequestQueryString_SQLLimit(char *charQueryString);
int regexGetRequestQueryString_SQLOffset(char *charQueryString);
int SYS_SelectSysErrMsg(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP);
int MO_SelectMOList(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP, char *charQueryString);
int MO_SelectMOSensorSN_TableSN(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString);
int MO_SelectSPCFeatureValueList(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString);
int MO_SelectSPCAlarmLog(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString);
int MO_SelectShotSNList(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString);
int MO_IMMParaSNList(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString);
int MO_RecomIMMParaSNList(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString);
int MO_JSON_InsertIMMParaSNList(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int MO_SelectMOAcceptCriteriaSN(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString);
int MO_SelectIMMSensorSNList(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString);
int MO_SelectMOAcceptCriteriaSNList(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString);
int MO_SelectMOAcceptCriteriaSN1(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString);
int MO_SelectMOSensorSNList(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString);
int MO_JSON_SetMOList(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int IMM_JSON_SetAutoOPCUAControl(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int IMM_SelectIMMList(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP, char *charQueryString);
int IMM_JSON_UpdateIMMRoundSN(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int IMM_WebService_Subscribe(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString);
int IMM_WebService_Unsubscribe(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString);
int INDEX_SelectAcceptCriteriaIndex(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP);
int INDEX_JSON_InsertAcceptCriteria(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int INDEX_JSON_MaskAcceptCriteria(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int INDEX_JSON_ReplaceAcceptCriteria(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int INDEX_SelectIMMModelIndex(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP);
int INDEX_JSON_InsertIMMModel(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int INDEX_JSON_MaskIMMModelIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int INDEX_JSON_ReplaceIMMModel(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int INDEX_SelectMaterialIndex(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP);
int INDEX_JSON_InsertMaterialIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int INDEX_JSON_MaskMaterialIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int INDEX_JSON_ReplaceMaterialIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int INDEX_SelectMoldIndex(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP);
int INDEX_JSON_InsertMoldIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int INDEX_JSON_MaskMoldIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int INDEX_JSON_ReplaceMoldIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int INDEX_SelectProdIndex(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP, char *charQueryString);
int INDEX_JSON_InsertProdIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int INDEX_JSON_MaskProdIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int INDEX_JSON_ReplaceProdIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int INDEX_SelectSensorModelIndex(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP);
int INDEX_JSON_InsertSensorModelIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int INDEX_JSON_MaskSensorModelIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int INDEX_JSON_ReplaceSensorModelIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
int INDEX_SelectSPCIndex(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP, char *charQueryString);
int INDEX_SelectOPCUAVersionIndex(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP, char *charQueryString);
int INDEX_SelectSysLog(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP);
int INDEX_JSON_SetSPCCL(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP);
//  Function List  End

//  Main Function
int main(void)
{
	int intRetval;
	MYSQL mysqlCon;
	char charErrMsg[LONG_STRING_SIZE];
	FCGX_Request fcgxRequest;

	char *charContentLength;
	char *charRequestMethod;
	char *charRequestURI;
	char *charQueryString;
	int intContentLength;
	char *charContentData;
	struct json_object *jsonResponseHTTP;
	unsigned int intLimit;
	unsigned int intOffset;

	intRetval = FCGX_Init();
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[WebService]Fail to initialize FastCGI");
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, EXIT_FAILURE, charErrMsg);
		return EXIT_FAILURE;
	}
	else
	{
		fprintf(stderr, "[WebService]Initialize Fast-CGI successfully\n");
	}
	FCGX_InitRequest(&fcgxRequest, 0, 0);	//Init Listen request
	while (FCGX_Accept_r(&fcgxRequest) == 0) //Start Listen request
	{

		charRequestMethod = FCGX_GetParam("REQUEST_METHOD", fcgxRequest.envp); //GET or POST
		charRequestURI = FCGX_GetParam("DOCUMENT_URI", fcgxRequest.envp);	  //uri
		charQueryString = FCGX_GetParam("QUERY_STRING", fcgxRequest.envp);	 //When REQUEST_METHOD is GET, use this to get query value
		charContentLength = FCGX_GetParam("CONTENT_LENGTH", fcgxRequest.envp); //When REQUEST_METHOD is POST, use this to get query length,then get query value

		FCGX_FPrintF(fcgxRequest.out,
					 "Content-Type: application/json; charset=utf-8\r\n\r\n");

		switch (getHTTPRequestMethodType(charRequestMethod))
		{
		case HTTP_REQUEST_TYPE_GET:
		{
			//Connect to MySQL Server
			intRetval = DB_DatabaseConnect(&mysqlCon);
			if (intRetval == EXIT_FAILURE)
			{
				fprintf(stderr, "[WebService]Fail to connect to MySQL server\n");
				return EXIT_FAILURE;
			}
			else
			{
				fprintf(stderr, "[WebService]Connect to MySQL successfully\n");
				intLimit = regexGetRequestQueryString_SQLLimit(charQueryString);
				intOffset = regexGetRequestQueryString_SQLOffset(charQueryString);
			}
			//Implement Get
			//Identify uri
			if (strcmp(charRequestURI, "/injpro/IMMList/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				IMM_SelectIMMList(mysqlCon, intLimit, intOffset, false, jsonResponseHTTP, charQueryString);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/data/MOList/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				MO_SelectMOList(mysqlCon, intLimit, intOffset, false, jsonResponseHTTP, charQueryString);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/AcceptCriteriaIndex/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				INDEX_SelectAcceptCriteriaIndex(mysqlCon, intLimit, intOffset, false, jsonResponseHTTP);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/IMMModelIndex/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				INDEX_SelectIMMModelIndex(mysqlCon, intLimit, intOffset, false, jsonResponseHTTP);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/MaterialIndex/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				INDEX_SelectMaterialIndex(mysqlCon, intLimit, intOffset, false, jsonResponseHTTP);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/MoldIndex/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				INDEX_SelectMoldIndex(mysqlCon, intLimit, intOffset, false, jsonResponseHTTP);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/ProdIndex/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				INDEX_SelectProdIndex(mysqlCon, intLimit, intOffset, false, jsonResponseHTTP, charQueryString);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/SensorModelIndex/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				INDEX_SelectSensorModelIndex(mysqlCon, intLimit, intOffset, false, jsonResponseHTTP);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/SPCIndex/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				INDEX_SelectSPCIndex(mysqlCon, intLimit, intOffset, false, jsonResponseHTTP, charQueryString);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/OPCUAVersionIndex/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				INDEX_SelectOPCUAVersionIndex(mysqlCon, intLimit, intOffset, false, jsonResponseHTTP, charQueryString);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/SysLog/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				INDEX_SelectSysLog(mysqlCon, intLimit, intOffset, false, jsonResponseHTTP);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/data/MOSensorSN_TableSN/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				MO_SelectMOSensorSN_TableSN(mysqlCon, jsonResponseHTTP, charQueryString);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/data/SPCFeatureValueList/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				MO_SelectSPCFeatureValueList(mysqlCon, jsonResponseHTTP, charQueryString);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/data/SPCAlarmLog/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				MO_SelectSPCAlarmLog(mysqlCon, jsonResponseHTTP, charQueryString);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/data/ShotSNList/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				MO_SelectShotSNList(mysqlCon, jsonResponseHTTP, charQueryString);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/IMM_SubscribeIMM/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				IMM_WebService_Subscribe(mysqlCon, jsonResponseHTTP, charQueryString);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/IMM_UnsubscribeIMM/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				IMM_WebService_Unsubscribe(mysqlCon, jsonResponseHTTP, charQueryString);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/data/IMMParaSNList/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				MO_IMMParaSNList(mysqlCon, jsonResponseHTTP, charQueryString);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/data/RecomIMMParaSNList/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				MO_RecomIMMParaSNList(mysqlCon, jsonResponseHTTP, charQueryString);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/data/MOAcceptCriteriaSN/get/") == 0)
			{
				jsonResponseHTTP = json_object_new_object();
				MO_SelectMOAcceptCriteriaSN(mysqlCon, jsonResponseHTTP, charQueryString);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/data/IMMSensorSNList/get/") == 0)
			{
				//int MO_SelectIMMSensorSNList(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString)
				jsonResponseHTTP = json_object_new_object();
				MO_SelectIMMSensorSNList(mysqlCon, jsonResponseHTTP, charQueryString);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/data/MOAcceptCriteriaSNList/get/") == 0)
			{
				//int MO_SelectMOAcceptCriteriaSNList(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString)
				jsonResponseHTTP = json_object_new_object();
				MO_SelectMOAcceptCriteriaSNList(mysqlCon, jsonResponseHTTP, charQueryString);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				// fprintf(stderr, "Chinese:"\n");
				fprintf(stderr, "GET:%s\n", json_object_to_json_string(jsonResponseHTTP));
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/data/MOAcceptCriteriaSN1/get/") == 0)
			{
				// int MO_SelectMOAcceptCriteriaSN1(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString);
				jsonResponseHTTP = json_object_new_object();
				MO_SelectMOAcceptCriteriaSN1(mysqlCon, jsonResponseHTTP, charQueryString);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/data/MOSensorSNList/get/") == 0)
			{
				// int MO_SelectMOSensorSNList(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString)
				jsonResponseHTTP = json_object_new_object();
				MO_SelectMOSensorSNList(mysqlCon, jsonResponseHTTP, charQueryString);
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				json_object_put(jsonResponseHTTP);
			}
			else if (strcmp(charRequestURI, "/injpro/testget/") == 0)
			{

				// fprintf(stderr, "%-50s%-2d...", "Inserting Accept Criteria 2:Weight for MOSN:", intMOSN);
				// strcpy(varMOAcceptCriteriaSN.charAcceptCriteriaMeta, "Product Weight");
				// strcpy(varMOAcceptCriteriaSN.charAcceptCriteriaMinTH, "");
				// strcpy(varMOAcceptCriteriaSN.charAcceptCriteriaMaxTH, "");
				// intRetval = MO_InsertMOAcceptCriteriaSNList(mysqlCon, intMOSN, 2, ACCEPTCRITERIA_CLASS_VALUE, "g", varMOAcceptCriteriaSN);
				// if (intRetval == EXIT_SUCCESS)
				// 	fprintf(stderr, "[\033[32mOK\033[m]\n");
				// else
				// {
				// 	fprintf(stderr, "[\033[31mFail\033[m]\n");
				// 	return EXIT_FAILURE;
				// }

				// fprintf(stderr, "GET:%s\n", charQueryString);
				// fprintf(stderr, "GET:%s\n", "[WebService]Request GET");
				// intLimit = 10;
				// intOffset = 0;
				jsonResponseHTTP = json_object_new_object();
				SYS_SelectSysErrMsg(mysqlCon, intLimit, intOffset, true, jsonResponseHTTP);
				// fprintf(stderr, "%s\n", "[WebService]Request GET Response Start");
				// fprintf(stderr, "jsonResponseHTTP:%s\n", json_object_to_json_string(jsonResponseHTTP));
				FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
				// fprintf(stderr, "%s\n", "[WebService]Request GET Response End");
				json_object_put(jsonResponseHTTP);
			}
		}
		break;
		case HTTP_REQUEST_TYPE_POST:
		{
			//Implement POST
			intContentLength = 0;
			if (charContentLength != NULL)
				intContentLength = strtol(charContentLength, NULL, 10);

			if (intContentLength <= 0)
			{
				fprintf(stderr, "No data from standard input.\n");
			}
			else
			{
				charContentData = (char *)malloc(sizeof(char) * intContentLength);
				FCGX_GetStr(charContentData, intContentLength, fcgxRequest.in);
				//Connect to MySQL Server
				intRetval = DB_DatabaseConnect(&mysqlCon);
				if (intRetval == EXIT_FAILURE)
				{
					fprintf(stderr, "[WebService]Fail to connect to MySQL server\n");
					return EXIT_FAILURE;
				}
				else
				{
					fprintf(stderr, "[WebService]Connect to MySQL successfully\n");
				}

				//Implement POST
				//Identify uri
				if (strcmp(charRequestURI, "/injpro/AcceptCriteriaIndex/set/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_InsertAcceptCriteria(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/AcceptCriteriaIndex/mask/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_MaskAcceptCriteria(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/AcceptCriteriaIndex/replace/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_ReplaceAcceptCriteria(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/IMMModelIndex/set/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_InsertIMMModel(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/IMMModelIndex/mask/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_MaskIMMModelIndex(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/IMMModelIndex/replace/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_ReplaceIMMModel(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/MaterialIndex/set/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_InsertMaterialIndex(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/MaterialIndex/mask/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_MaskMaterialIndex(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/MaterialIndex/replace/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_ReplaceMaterialIndex(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/MoldIndex/set/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_InsertMoldIndex(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/MoldIndex/mask/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_MaskMoldIndex(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/MoldIndex/replace/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_ReplaceMoldIndex(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/ProdIndex/set/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_InsertProdIndex(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/ProdIndex/mask/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_MaskProdIndex(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/ProdIndex/replace/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_ReplaceProdIndex(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/SensorModelIndex/set/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_InsertSensorModelIndex(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/SensorModelIndex/mask/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_MaskSensorModelIndex(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/SensorModelIndex/replace/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_ReplaceSensorModelIndex(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/data/IMMParaSNList/set/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					MO_JSON_InsertIMMParaSNList(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/data/IMMRoundSN/replace/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					IMM_JSON_UpdateIMMRoundSN(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/data/SetAutoOPCUAControl/set/") == 0)
				{
					jsonResponseHTTP = json_object_new_object();
					IMM_JSON_SetAutoOPCUAControl(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/SPCIndex/set/") == 0)
				{
					// int INDEX_JSON_SetSPCCL(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
					jsonResponseHTTP = json_object_new_object();
					INDEX_JSON_SetSPCCL(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/MOList/set/") == 0)
				{
					// int MO_JSON_SetMOList(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
					jsonResponseHTTP = json_object_new_object();
					MO_JSON_SetMOList(mysqlCon, charContentData, jsonResponseHTTP);
					FCGX_PutS(json_object_to_json_string(jsonResponseHTTP), fcgxRequest.out);
					json_object_put(jsonResponseHTTP);
					free(charContentData);
				}
				else if (strcmp(charRequestURI, "/injpro/testpost/") == 0)
				{

					// fprintf(stderr, "POST:%s\n", "[WebService]Request POST");
				}
			}
		}
		break;
		case HTTP_REQUEST_TYPE_UNKNOW:
		{
			fprintf(stderr, "[WebService]Unknow HTTP request method type:%d\n", getHTTPRequestMethodType(charRequestMethod));
		}
		break;
		default:
		{
			fprintf(stderr, "[WebService]Fail to identify HTTP request method type\n");
		}
		}
	} //End Main While

	//Close MySQL Connection
	mysql_close(&mysqlCon);

	return EXIT_SUCCESS;
}
// Main Function End

int getHTTPRequestMethodType(char *charRequestMethod)
{

	if (strcmp(charRequestMethod, "GET") == 0)
		return HTTP_REQUEST_TYPE_GET;
	else if (strcmp(charRequestMethod, "POST") == 0)
		return HTTP_REQUEST_TYPE_POST;
	else
		return HTTP_REQUEST_TYPE_UNKNOW;
}
int regexGetRequestQueryString_MOSN(char *charQueryString)
{
	unsigned int mosn;
	regex_t reg;
	const size_t nmatch = 1;
	regmatch_t pmatch[1];
	const char *pattern = "mosn=[^&]+";
	char *regresult;
	int status;

	regcomp(&reg, pattern, REG_EXTENDED);
	status = regexec(&reg, charQueryString, nmatch, pmatch, 0);
	if (status == REG_NOMATCH)
	{
		// printf("No Match\n");
		mosn = 0;
	}
	else if (status == 0)
	{
		// printf("Match\n");
		// fprintf(stderr,"charQueryString:%s\n",charQueryString);
		// fprintf(stderr,"pmatch[0].rm_so:%d\n",pmatch[0].rm_so);
		// fprintf(stderr,"pmatch[0].rm_eo:%d\n",pmatch[0].rm_eo);
		// fprintf(stderr,"pmatch[0].rm_eo - pmatch[0].rm_so - 5:%d\n",pmatch[0].rm_eo - pmatch[0].rm_so - 5);
		regresult = (char *)malloc(sizeof(char) * (pmatch[0].rm_eo - pmatch[0].rm_so - 5));
		strncpy(regresult, charQueryString + (pmatch[0].rm_so + 5), pmatch[0].rm_eo - pmatch[0].rm_so - 5);
		regresult[pmatch[0].rm_eo - pmatch[0].rm_so - 5] = '\0';
		// fprintf(stderr,"regresult:%s\n",regresult);
		mosn = atoi(regresult);
		free(regresult);
	}
	regfree(&reg);

	return mosn;
}

int regexGetRequestQueryString_OPCUAVersionSN(char *charQueryString)
{
	unsigned int intOPCUAVersionSN;
	regex_t reg;
	const size_t nmatch = 1;
	regmatch_t pmatch[1];
	const char *pattern = "intOPCUAVersionSN=[^&]+";
	char *regresult;
	int status;

	regcomp(&reg, pattern, REG_EXTENDED);
	status = regexec(&reg, charQueryString, nmatch, pmatch, 0);
	if (status == REG_NOMATCH)
	{
		// printf("No Match\n");
	}
	else if (status == 0)
	{
		// printf("Match\n");
		// printf("%d\n", pmatch[0].rm_so);
		// printf("%d\n", pmatch[0].rm_eo);
		regresult = (char *)malloc(sizeof(char) * (pmatch[0].rm_eo - pmatch[0].rm_so - 18));
		strncpy(regresult, charQueryString + (pmatch[0].rm_so + 18), pmatch[0].rm_eo - pmatch[0].rm_so - 18);
		regresult[pmatch[0].rm_eo - pmatch[0].rm_so - 18] = '\0';
		// fprintf(stderr,"regresult:%s\n",regresult);
		intOPCUAVersionSN = atoi(regresult);
		free(regresult);
	}
	regfree(&reg);

	return intOPCUAVersionSN;
}

int regexGetRequestQueryString_IMMSN(char *charQueryString)
{
	unsigned int intIMMSN;
	regex_t reg;
	const size_t nmatch = 1;
	regmatch_t pmatch[1];
	const char *pattern = "immsn=[^&]+";
	char *regresult;
	int status;

	regcomp(&reg, pattern, REG_EXTENDED);
	status = regexec(&reg, charQueryString, nmatch, pmatch, 0);
	if (status == REG_NOMATCH)
	{
		// printf("No Match\n");
		intIMMSN = 0;
	}
	else if (status == 0)
	{
		// printf("Match\n");
		// printf("%d\n", pmatch[0].rm_so);
		// printf("%d\n", pmatch[0].rm_eo);
		regresult = (char *)malloc(sizeof(char) * (pmatch[0].rm_eo - pmatch[0].rm_so - 6));
		strncpy(regresult, charQueryString + (pmatch[0].rm_so + 6), pmatch[0].rm_eo - pmatch[0].rm_so - 6);
		regresult[pmatch[0].rm_eo - pmatch[0].rm_so - 6] = '\0';
		// fprintf(stderr,"regresult:%s\n",regresult);
		intIMMSN = atoi(regresult);
		free(regresult);
	}
	regfree(&reg);

	return intIMMSN;
}
int regexGetRequestQueryString_ProdSN(char *charQueryString)
{
	unsigned int intProdSN;
	regex_t reg;
	const size_t nmatch = 1;
	regmatch_t pmatch[1];
	const char *pattern = "prodsn=[^&]+";
	char *regresult;
	int status;

	regcomp(&reg, pattern, REG_EXTENDED);
	status = regexec(&reg, charQueryString, nmatch, pmatch, 0);
	if (status == REG_NOMATCH)
	{
		// printf("No Match\n");
		intProdSN = 0;
	}
	else if (status == 0)
	{
		// printf("Match\n");
		// printf("%d\n", pmatch[0].rm_so);
		// printf("%d\n", pmatch[0].rm_eo);
		regresult = (char *)malloc(sizeof(char) * (pmatch[0].rm_eo - pmatch[0].rm_so - 7));
		strncpy(regresult, charQueryString + (pmatch[0].rm_so + 7), pmatch[0].rm_eo - pmatch[0].rm_so - 7);
		regresult[pmatch[0].rm_eo - pmatch[0].rm_so - 7] = '\0';
		// fprintf(stderr,"regresult:%s\n",regresult);
		intProdSN = atoi(regresult);
		free(regresult);
	}
	regfree(&reg);

	return intProdSN;
}
int regexGetRequestQueryString_ShotSN(char *charQueryString)
{
	unsigned int intShotSN;
	regex_t reg;
	const size_t nmatch = 1;
	regmatch_t pmatch[1];
	const char *pattern = "shotsn=[^&]+";
	char *regresult;
	int status;

	regcomp(&reg, pattern, REG_EXTENDED);
	status = regexec(&reg, charQueryString, nmatch, pmatch, 0);
	if (status == REG_NOMATCH)
	{
		// printf("No Match\n");
		intShotSN = 0;
	}
	else if (status == 0)
	{
		// printf("Match\n");
		// printf("%d\n", pmatch[0].rm_so);
		// printf("%d\n", pmatch[0].rm_eo);
		regresult = (char *)malloc(sizeof(char) * (pmatch[0].rm_eo - pmatch[0].rm_so - 7));
		strncpy(regresult, charQueryString + (pmatch[0].rm_so + 7), pmatch[0].rm_eo - pmatch[0].rm_so - 7);
		regresult[pmatch[0].rm_eo - pmatch[0].rm_so - 7] = '\0';
		// fprintf(stderr,"regresult:%s\n",regresult);
		intShotSN = atoi(regresult);
		free(regresult);
	}
	regfree(&reg);

	return intShotSN;
}
int regexGetRequestQueryString_IMMParaSN(char *charQueryString)
{
	unsigned int intIMMParaSN;
	regex_t reg;
	const size_t nmatch = 1;
	regmatch_t pmatch[1];
	const char *pattern = "immparasn=[^&]+";
	char *regresult;
	int status;

	regcomp(&reg, pattern, REG_EXTENDED);
	status = regexec(&reg, charQueryString, nmatch, pmatch, 0);
	if (status == REG_NOMATCH)
	{
		// printf("No Match\n");
		intIMMParaSN = 0;
	}
	else if (status == 0)
	{
		// printf("Match\n");
		// printf("%d\n", pmatch[0].rm_so);
		// printf("%d\n", pmatch[0].rm_eo);
		regresult = (char *)malloc(sizeof(char) * (pmatch[0].rm_eo - pmatch[0].rm_so - 10));
		strncpy(regresult, charQueryString + (pmatch[0].rm_so + 10), pmatch[0].rm_eo - pmatch[0].rm_so - 10);
		regresult[pmatch[0].rm_eo - pmatch[0].rm_so - 10] = '\0';
		// fprintf(stderr,"regresult:%s\n",regresult);
		intIMMParaSN = atoi(regresult);
		free(regresult);
	}
	regfree(&reg);

	return intIMMParaSN;
}

int regexGetRequestQueryString_RecomIMMParaSN(char *charQueryString)
{
	unsigned int intRecomIMMParaSN;
	regex_t reg;
	const size_t nmatch = 1;
	regmatch_t pmatch[1];
	const char *pattern = "recomimmparasn=[^&]+";
	char *regresult;
	int status;

	regcomp(&reg, pattern, REG_EXTENDED);
	status = regexec(&reg, charQueryString, nmatch, pmatch, 0);
	if (status == REG_NOMATCH)
	{
		// printf("No Match\n");
		intRecomIMMParaSN = 0;
	}
	else if (status == 0)
	{
		// printf("Match\n");
		// printf("%d\n", pmatch[0].rm_so);
		// printf("%d\n", pmatch[0].rm_eo);
		regresult = (char *)malloc(sizeof(char) * (pmatch[0].rm_eo - pmatch[0].rm_so - 15));
		strncpy(regresult, charQueryString + (pmatch[0].rm_so + 15), pmatch[0].rm_eo - pmatch[0].rm_so - 15);
		regresult[pmatch[0].rm_eo - pmatch[0].rm_so - 15] = '\0';
		// fprintf(stderr,"regresult:%s\n",regresult);
		intRecomIMMParaSN = atoi(regresult);
		free(regresult);
	}
	regfree(&reg);

	return intRecomIMMParaSN;
}

int regexGetRequestQueryString_SQLLimit(char *charQueryString)
{
	unsigned int intSQLLimit;
	regex_t reg;
	const size_t nmatch = 1;
	regmatch_t pmatch[1];
	const char *pattern = "limit=[^&]+";
	char *regresult;
	int status;

	regcomp(&reg, pattern, REG_EXTENDED);
	status = regexec(&reg, charQueryString, nmatch, pmatch, 0);
	if (status == REG_NOMATCH)
	{
		// printf("No Match\n");
		intSQLLimit = 0;
	}
	else if (status == 0)
	{
		// printf("Match\n");
		// printf("%d\n", pmatch[0].rm_so);
		// printf("%d\n", pmatch[0].rm_eo);
		regresult = (char *)malloc(sizeof(char) * (pmatch[0].rm_eo - pmatch[0].rm_so - 6));
		strncpy(regresult, charQueryString + (pmatch[0].rm_so + 6), pmatch[0].rm_eo - pmatch[0].rm_so - 6);
		regresult[pmatch[0].rm_eo - pmatch[0].rm_so - 6] = '\0';
		// fprintf(stderr,"regresult:%s\n",regresult);
		intSQLLimit = atoi(regresult);
		free(regresult);
	}
	regfree(&reg);

	return intSQLLimit;
}

int regexGetRequestQueryString_SQLOffset(char *charQueryString)
{
	unsigned int intSQLOffset;
	regex_t reg;
	const size_t nmatch = 1;
	regmatch_t pmatch[1];
	const char *pattern = "offset=[^&]+";
	char *regresult;
	int status;

	regcomp(&reg, pattern, REG_EXTENDED);
	status = regexec(&reg, charQueryString, nmatch, pmatch, 0);
	if (status == REG_NOMATCH)
	{
		// printf("No Match\n");
		intSQLOffset = 0;
	}
	else if (status == 0)
	{
		// printf("Match\n");
		// printf("%d\n", pmatch[0].rm_so);
		// printf("%d\n", pmatch[0].rm_eo);
		regresult = (char *)malloc(sizeof(char) * (pmatch[0].rm_eo - pmatch[0].rm_so - 7));
		strncpy(regresult, charQueryString + (pmatch[0].rm_so + 7), pmatch[0].rm_eo - pmatch[0].rm_so - 7);
		regresult[pmatch[0].rm_eo - pmatch[0].rm_so - 7] = '\0';
		// fprintf(stderr,"regresult:%s\n",regresult);
		intSQLOffset = atoi(regresult);
		free(regresult);
	}
	regfree(&reg);

	return intSQLOffset;
}

int INDEX_JSON_SetSPCCL(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];

	int intRetval;

	struct json_object *jsoncharContentDataList;
	struct json_object *jsonMOSN;
	struct json_object *jsonShotSNStart;
	struct json_object *jsonShotSNEnd;

	snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_SetSPCCL]Fail to parse string:%s to json object", charContentData);

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_SetSPCCL]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[INDEX_JSON_SetSPCCL] Failed JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		json_object_object_get_ex(jsoncharContentDataList, "MOSN", &jsonMOSN);
		//json_object_object_get_ex(jsoncharContentDataList, "ShotSNStart", &jsonShotSNStart);
		json_object_object_get_ex(jsoncharContentDataList, "ShotSNEnd", &jsonShotSNEnd);

		// int INDEX_InsertSPCCL(MYSQL mysqlCon,unsigned int intMOSN, unsigned int intShotSN_Start,unsigned int intShotSN_End);
		//intRetval = INDEX_InsertSPCCL(mysqlCon, json_object_get_int(jsonMOSN), json_object_get_int(jsonShotSNStart), json_object_get_int(jsonShotSNEnd));
		intRetval = SPC_InsertAllSPCRuleCL(mysqlCon, json_object_get_int(jsonMOSN), json_object_get_int(jsonShotSNEnd));

		if (intRetval == EXIT_SUCCESS)
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[INDEX_JSON_SetSPCCL] SUCCESS"));
			json_object_put(jsoncharContentDataList);
			return EXIT_SUCCESS;
		}
		else
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_SetSPCCL]Fail");
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[INDEX_JSON_SetSPCCL] Failed"));
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int IMM_WebService_Subscribe(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString)
{
	char charErrMsg[LONG_STRING_SIZE];

	int intRetval;
	int intIMMSN;

	intIMMSN = regexGetRequestQueryString_IMMSN(charQueryString);

	// int IMM_Subscribe(MYSQL mysqlCon, unsigned int intIMMSN);
	intRetval = IMM_Subscribe(mysqlCon, intIMMSN);

	if (intRetval == EXIT_SUCCESS)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[IMM_WebService_Subscribe] SUCCESS"));
		return EXIT_SUCCESS;
	}
	else
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[IMM_WebService_Subscribe]Fail");
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[IMM_WebService_Subscribe] Failed"));
		return EXIT_FAILURE;
	}
}
int IMM_WebService_Unsubscribe(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString)
{
	char charErrMsg[LONG_STRING_SIZE];

	int intRetval;
	int intIMMSN;

	intIMMSN = regexGetRequestQueryString_IMMSN(charQueryString);

	// int IMM_Unsubscribe(MYSQL mysqlCon, unsigned int intIMMSN);
	intRetval = IMM_Unsubscribe(mysqlCon, intIMMSN);

	if (intRetval == EXIT_SUCCESS)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[IMM_WebService_Unsubscribe] SUCCESS"));
		return EXIT_SUCCESS;
	}
	else
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[IMM_WebService_Unsubscribe]Fail");
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[IMM_WebService_Unsubscribe] Failed"));
		return EXIT_FAILURE;
	}
}

int IMM_JSON_UpdateIMMRoundSN(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];

	int intRetval;

	struct json_object *jsoncharContentDataList;
	struct json_object *jsonIMMSN;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[IMM_JSON_UpdateIMMRoundSN]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[IMM_JSON_UpdateIMMRoundSN] Failed JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		json_object_object_get_ex(jsoncharContentDataList, "IMMSN", &jsonIMMSN);

		// int IMM_AddRoundSN(MYSQL mysqlCon,unsigned int intIMMSN);
		intRetval = IMM_AddRoundSN(mysqlCon, json_object_get_int(jsonIMMSN));

		if (intRetval == EXIT_SUCCESS)
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[IMM_JSON_UpdateIMMRoundSN] SUCCESS"));
			json_object_put(jsoncharContentDataList);
			return EXIT_SUCCESS;
		}
		else
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[IMM_JSON_UpdateIMMRoundSN]Fail");
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[IMM_JSON_UpdateIMMRoundSN] Failed"));
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int IMM_JSON_SetAutoOPCUAControl(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];

	int intRetval;

	struct json_object *jsoncharContentDataList;
	struct json_object *jsonIMMSN;
	struct json_object *jsonValue;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[IMM_JSON_SetAutoOPCUAControl]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[IMM_JSON_SetAutoOPCUAControl] Failed JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		json_object_object_get_ex(jsoncharContentDataList, "IMMSN", &jsonIMMSN);
		json_object_object_get_ex(jsoncharContentDataList, "Value", &jsonValue);

		// int IMM_SetAutoOPCUAControl(MYSQL mysqlCon,unsigned int intIMMSN, bool boolValue);
		intRetval = IMM_SetAutoOPCUAControl(mysqlCon, json_object_get_int(jsonIMMSN), json_object_get_boolean(jsonValue));

		if (intRetval == EXIT_SUCCESS)
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[IMM_JSON_SetAutoOPCUAControl] SUCCESS"));
			json_object_put(jsoncharContentDataList);
			return EXIT_SUCCESS;
		}
		else
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[IMM_JSON_SetAutoOPCUAControl]Fail");
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[IMM_JSON_SetAutoOPCUAControl] Failed"));
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int MO_JSON_InsertIMMParaSNList(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	char charIMMParaValue[OPCUA_IMMPARA_NODE_NUM + 1][TINY_STRING_SIZE];

	int intRetval;

	struct json_object *jsoncharContentDataList;
	struct json_object *jsondata;
	struct json_object *jsonIMMSN;
	struct json_object *jsonTechnicianUserSN;
	struct json_object *jsonInjectStage;
	struct json_object *jsonHoldStage;
	struct json_object *jsonPlasStage;
	struct json_object *jsonMeltTemperature;
	struct json_object *jsonPlastificationVolume1;
	struct json_object *jsonPlastificationVolume2;
	struct json_object *jsonPlastificationVolume3;
	struct json_object *jsonPlastificationVolume4;
	struct json_object *jsonPlastificationVolume5;
	struct json_object *jsonInjectionPressure1;
	struct json_object *jsonInjectionPressure2;
	struct json_object *jsonInjectionPressure3;
	struct json_object *jsonInjectionPressure4;
	struct json_object *jsonInjectionPressure5;
	struct json_object *jsonInjectionSpeed1;
	struct json_object *jsonInjectionSpeed2;
	struct json_object *jsonInjectionSpeed3;
	struct json_object *jsonInjectionSpeed4;
	struct json_object *jsonInjectionSpeed5;
	struct json_object *jsonVPChangeOverPosition;
	struct json_object *jsonVPChangeOverTime;
	struct json_object *jsonPackingPressure;
	struct json_object *jsonHoldingPressure1;
	struct json_object *jsonHoldingPressure2;
	struct json_object *jsonHoldingPressure3;
	struct json_object *jsonHoldingPressure4;
	struct json_object *jsonHoldingPressure5;
	struct json_object *jsonPackingTime;
	struct json_object *jsonHoldingTime1;
	struct json_object *jsonHoldingTime2;
	struct json_object *jsonHoldingTime3;
	struct json_object *jsonHoldingTime4;
	struct json_object *jsonHoldingTime5;
	struct json_object *jsonCoolingTime;
	struct json_object *jsonScrewRPM1;
	struct json_object *jsonScrewRPM2;
	struct json_object *jsonScrewRPM3;
	struct json_object *jsonScrewRPM4;
	struct json_object *jsonScrewRPM5;
	struct json_object *jsonBackPressure1;
	struct json_object *jsonBackPressure2;
	struct json_object *jsonBackPressure3;
	struct json_object *jsonBackPressure4;
	struct json_object *jsonBackPressure5;
	struct json_object *jsonMoldTemperature;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_JSON_InsertIMMParaSNList]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[MO_JSON_InsertIMMParaSNList] Failed JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		json_object_object_get_ex(jsoncharContentDataList, "data", &jsondata);
		json_object_object_get_ex(jsoncharContentDataList, "IMMSN", &jsonIMMSN);
		json_object_object_get_ex(jsoncharContentDataList, "TechnicianUserSN", &jsonTechnicianUserSN);
		json_object_object_get_ex(jsondata, "InjectStage", &jsonInjectStage);
		json_object_object_get_ex(jsondata, "HoldStage", &jsonHoldStage);
		json_object_object_get_ex(jsondata, "PlasStage", &jsonPlasStage);
		json_object_object_get_ex(jsondata, "MeltTemperature", &jsonMeltTemperature);
		json_object_object_get_ex(jsondata, "PlastificationVolume1", &jsonPlastificationVolume1);
		json_object_object_get_ex(jsondata, "PlastificationVolume2", &jsonPlastificationVolume2);
		json_object_object_get_ex(jsondata, "PlastificationVolume3", &jsonPlastificationVolume3);
		json_object_object_get_ex(jsondata, "PlastificationVolume4", &jsonPlastificationVolume4);
		json_object_object_get_ex(jsondata, "PlastificationVolume5", &jsonPlastificationVolume5);
		json_object_object_get_ex(jsondata, "InjectionPressure1", &jsonInjectionPressure1);
		json_object_object_get_ex(jsondata, "InjectionPressure2", &jsonInjectionPressure2);
		json_object_object_get_ex(jsondata, "InjectionPressure3", &jsonInjectionPressure3);
		json_object_object_get_ex(jsondata, "InjectionPressure4", &jsonInjectionPressure4);
		json_object_object_get_ex(jsondata, "InjectionPressure5", &jsonInjectionPressure5);
		json_object_object_get_ex(jsondata, "InjectionSpeed1", &jsonInjectionSpeed1);
		json_object_object_get_ex(jsondata, "InjectionSpeed2", &jsonInjectionSpeed2);
		json_object_object_get_ex(jsondata, "InjectionSpeed3", &jsonInjectionSpeed3);
		json_object_object_get_ex(jsondata, "InjectionSpeed4", &jsonInjectionSpeed4);
		json_object_object_get_ex(jsondata, "InjectionSpeed5", &jsonInjectionSpeed5);
		json_object_object_get_ex(jsondata, "VPChangeOverPosition", &jsonVPChangeOverPosition);
		json_object_object_get_ex(jsondata, "VPChangeOverTime", &jsonVPChangeOverTime);
		json_object_object_get_ex(jsondata, "PackingPressure", &jsonPackingPressure);
		json_object_object_get_ex(jsondata, "HoldingPressure1", &jsonHoldingPressure1);
		json_object_object_get_ex(jsondata, "HoldingPressure2", &jsonHoldingPressure2);
		json_object_object_get_ex(jsondata, "HoldingPressure3", &jsonHoldingPressure3);
		json_object_object_get_ex(jsondata, "HoldingPressure4", &jsonHoldingPressure4);
		json_object_object_get_ex(jsondata, "HoldingPressure5", &jsonHoldingPressure5);
		json_object_object_get_ex(jsondata, "PackingTime", &jsonPackingTime);
		json_object_object_get_ex(jsondata, "HoldingTime1", &jsonHoldingTime1);
		json_object_object_get_ex(jsondata, "HoldingTime2", &jsonHoldingTime2);
		json_object_object_get_ex(jsondata, "HoldingTime3", &jsonHoldingTime3);
		json_object_object_get_ex(jsondata, "HoldingTime4", &jsonHoldingTime4);
		json_object_object_get_ex(jsondata, "HoldingTime5", &jsonHoldingTime5);
		json_object_object_get_ex(jsondata, "CoolingTime", &jsonCoolingTime);
		json_object_object_get_ex(jsondata, "ScrewRPM1", &jsonScrewRPM1);
		json_object_object_get_ex(jsondata, "ScrewRPM2", &jsonScrewRPM2);
		json_object_object_get_ex(jsondata, "ScrewRPM3", &jsonScrewRPM3);
		json_object_object_get_ex(jsondata, "ScrewRPM4", &jsonScrewRPM4);
		json_object_object_get_ex(jsondata, "ScrewRPM5", &jsonScrewRPM5);
		json_object_object_get_ex(jsondata, "BackPressure1", &jsonBackPressure1);
		json_object_object_get_ex(jsondata, "BackPressure2", &jsonBackPressure2);
		json_object_object_get_ex(jsondata, "BackPressure3", &jsonBackPressure3);
		json_object_object_get_ex(jsondata, "BackPressure4", &jsonBackPressure4);
		json_object_object_get_ex(jsondata, "BackPressure5", &jsonBackPressure5);
		json_object_object_get_ex(jsondata, "MoldTemperature", &jsonMoldTemperature);

		// strcpy(charIMMParaValue[IMMPARA_INJECT_STAGE],"2.3456");
		strcpy(charIMMParaValue[IMMPARA_INJECT_STAGE], json_object_get_string(jsonInjectStage));
		strcpy(charIMMParaValue[IMMPARA_HOLD_STAGE], json_object_get_string(jsonHoldStage));
		strcpy(charIMMParaValue[IMMPARA_PLAS_STAGE], json_object_get_string(jsonPlasStage));
		strcpy(charIMMParaValue[IMMPARA_MELT_TEMPERATURE], json_object_get_string(jsonMeltTemperature));
		strcpy(charIMMParaValue[IMMPARA_PLASTIFICATION_VOLUME_1], json_object_get_string(jsonPlastificationVolume1));
		strcpy(charIMMParaValue[IMMPARA_PLASTIFICATION_VOLUME_2], json_object_get_string(jsonPlastificationVolume2));
		strcpy(charIMMParaValue[IMMPARA_PLASTIFICATION_VOLUME_3], json_object_get_string(jsonPlastificationVolume3));
		strcpy(charIMMParaValue[IMMPARA_PLASTIFICATION_VOLUME_4], json_object_get_string(jsonPlastificationVolume4));
		strcpy(charIMMParaValue[IMMPARA_PLASTIFICATION_VOLUME_5], json_object_get_string(jsonPlastificationVolume5));
		strcpy(charIMMParaValue[IMMPARA_INJECTION_PRESSURE_1], json_object_get_string(jsonInjectionPressure1));
		strcpy(charIMMParaValue[IMMPARA_INJECTION_PRESSURE_2], json_object_get_string(jsonInjectionPressure2));
		strcpy(charIMMParaValue[IMMPARA_INJECTION_PRESSURE_3], json_object_get_string(jsonInjectionPressure3));
		strcpy(charIMMParaValue[IMMPARA_INJECTION_PRESSURE_4], json_object_get_string(jsonInjectionPressure4));
		strcpy(charIMMParaValue[IMMPARA_INJECTION_PRESSURE_5], json_object_get_string(jsonInjectionPressure5));
		strcpy(charIMMParaValue[IMMPARA_INJECTION_SPEED_1], json_object_get_string(jsonInjectionSpeed1));
		strcpy(charIMMParaValue[IMMPARA_INJECTION_SPEED_2], json_object_get_string(jsonInjectionSpeed2));
		strcpy(charIMMParaValue[IMMPARA_INJECTION_SPEED_3], json_object_get_string(jsonInjectionSpeed3));
		strcpy(charIMMParaValue[IMMPARA_INJECTION_SPEED_4], json_object_get_string(jsonInjectionSpeed4));
		strcpy(charIMMParaValue[IMMPARA_INJECTION_SPEED_5], json_object_get_string(jsonInjectionSpeed5));
		strcpy(charIMMParaValue[IMMPARA_VPCHANGEOVER_POSITION], json_object_get_string(jsonVPChangeOverPosition));
		strcpy(charIMMParaValue[IMMPARA_VPCHANGEOVER_TIME], json_object_get_string(jsonVPChangeOverTime));
		strcpy(charIMMParaValue[IMMPARA_PACKING_PRESSURE], json_object_get_string(jsonPackingPressure));
		strcpy(charIMMParaValue[IMMPARA_HOLDING_PRESSURE_1], json_object_get_string(jsonHoldingPressure1));
		strcpy(charIMMParaValue[IMMPARA_HOLDING_PRESSURE_2], json_object_get_string(jsonHoldingPressure2));
		strcpy(charIMMParaValue[IMMPARA_HOLDING_PRESSURE_3], json_object_get_string(jsonHoldingPressure3));
		strcpy(charIMMParaValue[IMMPARA_HOLDING_PRESSURE_4], json_object_get_string(jsonHoldingPressure4));
		strcpy(charIMMParaValue[IMMPARA_HOLDING_PRESSURE_5], json_object_get_string(jsonHoldingPressure5));
		strcpy(charIMMParaValue[IMMPARA_PACKING_TIME], json_object_get_string(jsonPackingTime));
		strcpy(charIMMParaValue[IMMPARA_HOLDING_TIME_1], json_object_get_string(jsonHoldingTime1));
		strcpy(charIMMParaValue[IMMPARA_HOLDING_TIME_2], json_object_get_string(jsonHoldingTime2));
		strcpy(charIMMParaValue[IMMPARA_HOLDING_TIME_3], json_object_get_string(jsonHoldingTime3));
		strcpy(charIMMParaValue[IMMPARA_HOLDING_TIME_4], json_object_get_string(jsonHoldingTime4));
		strcpy(charIMMParaValue[IMMPARA_HOLDING_TIME_5], json_object_get_string(jsonHoldingTime5));
		strcpy(charIMMParaValue[IMMPARA_COOLING_TIME], json_object_get_string(jsonCoolingTime));
		strcpy(charIMMParaValue[IMMPARA_SCREW_RPM_1], json_object_get_string(jsonScrewRPM1));
		strcpy(charIMMParaValue[IMMPARA_SCREW_RPM_2], json_object_get_string(jsonScrewRPM2));
		strcpy(charIMMParaValue[IMMPARA_SCREW_RPM_3], json_object_get_string(jsonScrewRPM3));
		strcpy(charIMMParaValue[IMMPARA_SCREW_RPM_4], json_object_get_string(jsonScrewRPM4));
		strcpy(charIMMParaValue[IMMPARA_SCREW_RPM_5], json_object_get_string(jsonScrewRPM5));
		strcpy(charIMMParaValue[IMMPARA_BACK_PRESSURE_1], json_object_get_string(jsonBackPressure1));
		strcpy(charIMMParaValue[IMMPARA_BACK_PRESSURE_2], json_object_get_string(jsonBackPressure2));
		strcpy(charIMMParaValue[IMMPARA_BACK_PRESSURE_3], json_object_get_string(jsonBackPressure3));
		strcpy(charIMMParaValue[IMMPARA_BACK_PRESSURE_4], json_object_get_string(jsonBackPressure4));
		strcpy(charIMMParaValue[IMMPARA_BACK_PRESSURE_5], json_object_get_string(jsonBackPressure5));
		strcpy(charIMMParaValue[IMMPARA_MOLD_TEMPERATURE], json_object_get_string(jsonMoldTemperature));

		// int IMM_TuneIMMPara(MYSQL mysqlCon,unsigned int intIMMSN,unsigned int intTechnicianUserSN,char charIMMParaValue[OPCUA_IMMPARA_NODE_NUM][TINY_STRING_SIZE]);
		intRetval = IMM_TuneIMMPara(mysqlCon, json_object_get_int(jsonIMMSN), json_object_get_int(jsonTechnicianUserSN), charIMMParaValue);

		if (intRetval == EXIT_SUCCESS)
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[MO_JSON_InsertIMMParaSNList] SUCCESS"));
			json_object_put(jsoncharContentDataList);
			return EXIT_SUCCESS;
		}
		else
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_JSON_InsertIMMParaSNList]Fail");
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[MO_JSON_InsertIMMParaSNList] Failed"));
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int INDEX_SelectSysLog(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP)
{

	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	int intLogSN;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	//Select Number of Row
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s.SysLog", SYS_NAME, LOG_DATABASE_NAME);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectSysLog]Fail to select COUNT(*) of %s_%s.SysLog (%d):%s",
				 SYS_NAME, LOG_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_SUCCESS;
	}
	//Set MySQL Statement
	if (intLimit == 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT('LogSN',LogSN,'Time',Time,'ErrClass',ErrClass,'ErrMOSN',ErrMOSN,'ErrIMMSN',ErrIMMSN,'ErrCode',ErrCode,'ErrMsg',ErrMsg) FROM %s_%s.SysLog ORDER BY LogSN %s",
				 SYS_NAME, LOG_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC");
	}
	else
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT('LogSN',LogSN,'Time',Time,'ErrClass',ErrClass,'ErrMOSN',ErrMOSN,'ErrIMMSN',ErrIMMSN,'ErrCode',ErrCode,'ErrMsg',ErrMsg) FROM %s_%s.SysLog ORDER BY LogSN %s LIMIT %d OFFSET %d",
				 SYS_NAME, LOG_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC", intLimit, intOffset);
	}
	// fprintf(stderr, "charStatement:%s\n",charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectSysLog]Fail to select %s_%s.SysLog (%d):%s",
				 SYS_NAME, LOG_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectSysLog]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}

int INDEX_SelectOPCUAVersionIndex(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP, char *charQueryString)
{
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	unsigned int intLogSN;
	unsigned int intOPCUAVersionSN;

	struct json_object *jsonRowObjectList;
	int intTableCount = 1;
	char charTableName[MEDIUM_STRING_SIZE];

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	intOPCUAVersionSN = regexGetRequestQueryString_OPCUAVersionSN(charQueryString);

	//Select Number of Row
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s.OPCUAVersionIndex", SYS_NAME, INDEX_DATABASE_NAME);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectOPCUAVersionIndex]Fail to select COUNT(*) of %s_%s.OPCUAVersionIndex (%d):%s",
				 SYS_NAME, INDEX_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_SUCCESS;
	}

	//Set MySQL Statement
	if (intLimit == 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT "
				 "JSON_OBJECT("
				 "'OPCUAVersionSN',OPCUAVersionSN,"
				 "'OPCUAVersionMask',OPCUAVersionMask,"
				 "'OPCUAVersionStr',OPCUAVersionStr,"
				 "'DIMoldClampedName',DIMoldClampedName,"
				 "'DIMoldClampedType',DIMoldClampedType,"
				 "'DIMoldClampedNS',DIMoldClampedNS,"
				 "'DIMoldClampedID',DIMoldClampedID,"
				 "'DIMoldReleasedName',DIMoldReleasedName,"
				 "'DIMoldReleasedType',DIMoldReleasedType,"
				 "'DIMoldReleasedNS',DIMoldReleasedNS,"
				 "'DIMoldReleasedID',DIMoldReleasedID,"
				 "'DIAlarmLampName',DIAlarmLampName,"
				 "'DIAlarmLampType',DIAlarmLampType,"
				 "'DIAlarmLampNS',DIAlarmLampNS,"
				 "'DIAlarmLampID',DIAlarmLampID,"
				 "'DIStoppedLampName',DIStoppedLampName,"
				 "'DIStoppedLampType',DIStoppedLampType,"
				 "'DIStoppedLampNS',DIStoppedLampNS,"
				 "'DIStoppedLampID',DIStoppedLampID,"
				 "'DIRunningLampName',DIRunningLampName,"
				 "'DIRunningLampType',DIRunningLampType,"
				 "'DIRunningLampNS',DIRunningLampNS,"
				 "'DIRunningLampID',DIRunningLampID,"
				 "'DIMotorStartName',DIMotorStartName,"
				 "'DIMotorStartType',DIMotorStartType,"
				 "'DIMotorStartNS',DIMotorStartNS,"
				 "'DIMotorStartID',DIMotorStartID,"
				 "'DIOperationModeName',DIOperationModeName,"
				 "'DIOperationModeType',DIOperationModeType,"
				 "'DIOperationModeNS',DIOperationModeNS,"
				 "'DIOperationModeID',DIOperationModeID,"
				 "'DIHeatOnName',DIHeatOnName,"
				 "'DIHeatOnType',DIHeatOnType,"
				 "'DIHeatOnNS',DIHeatOnNS,"
				 "'DIHeatOnID',DIHeatOnID,"
				 "'IMMParaInjectStageName',IMMParaInjectStageName,"
				 "'IMMParaInjectStageType',IMMParaInjectStageType,"
				 "'IMMParaInjectStageNS',IMMParaInjectStageNS,"
				 "'IMMParaInjectStageID',IMMParaInjectStageID,"
				 "'IMMParaHoldStageName',IMMParaHoldStageName,"
				 "'IMMParaHoldStageType',IMMParaHoldStageType,"
				 "'IMMParaHoldStageNS',IMMParaHoldStageNS,"
				 "'IMMParaHoldStageID',IMMParaHoldStageID,"
				 "'IMMParaPlasStageName',IMMParaPlasStageName,"
				 "'IMMParaPlasStageType',IMMParaPlasStageType,"
				 "'IMMParaPlasStageNS',IMMParaPlasStageNS,"
				 "'IMMParaPlasStageID',IMMParaPlasStageID,"
				 "'IMMParaMeltTemperatureName',IMMParaMeltTemperatureName,"
				 "'IMMParaMeltTemperatureType',IMMParaMeltTemperatureType,"
				 "'IMMParaMeltTemperatureNS',IMMParaMeltTemperatureNS,"
				 "'IMMParaMeltTemperatureID',IMMParaMeltTemperatureID,"
				 "'IMMParaPlastificationVolume1Name',IMMParaPlastificationVolume1Name,"
				 "'IMMParaPlastificationVolume1Type',IMMParaPlastificationVolume1Type,"
				 "'IMMParaPlastificationVolume1NS',IMMParaPlastificationVolume1NS,"
				 "'IMMParaPlastificationVolume1ID',IMMParaPlastificationVolume1ID,"
				 "'IMMParaPlastificationVolume2Name',IMMParaPlastificationVolume2Name,"
				 "'IMMParaPlastificationVolume2Type',IMMParaPlastificationVolume2Type,"
				 "'IMMParaPlastificationVolume2NS',IMMParaPlastificationVolume2NS,"
				 "'IMMParaPlastificationVolume2ID',IMMParaPlastificationVolume2ID,"
				 "'IMMParaPlastificationVolume3Name',IMMParaPlastificationVolume3Name,"
				 "'IMMParaPlastificationVolume3Type',IMMParaPlastificationVolume3Type,"
				 "'IMMParaPlastificationVolume3NS',IMMParaPlastificationVolume3NS,"
				 "'IMMParaPlastificationVolume3ID',IMMParaPlastificationVolume3ID,"
				 "'IMMParaPlastificationVolume4Name',IMMParaPlastificationVolume4Name,"
				 "'IMMParaPlastificationVolume4Type',IMMParaPlastificationVolume4Type,"
				 "'IMMParaPlastificationVolume4NS',IMMParaPlastificationVolume4NS,"
				 "'IMMParaPlastificationVolume4ID',IMMParaPlastificationVolume4ID,"
				 "'IMMParaPlastificationVolume5Name',IMMParaPlastificationVolume5Name,"
				 "'IMMParaPlastificationVolume5Type',IMMParaPlastificationVolume5Type,"
				 "'IMMParaPlastificationVolume5NS',IMMParaPlastificationVolume5NS,"
				 "'IMMParaPlastificationVolume5ID',IMMParaPlastificationVolume5ID,"
				 "'IMMParaInjectionPressure1Name',IMMParaInjectionPressure1Name,"
				 "'IMMParaInjectionPressure1Type',IMMParaInjectionPressure1Type,"
				 "'IMMParaInjectionPressure1NS',IMMParaInjectionPressure1NS,"
				 "'IMMParaInjectionPressure1ID',IMMParaInjectionPressure1ID,"
				 "'IMMParaInjectionPressure2Name',IMMParaInjectionPressure2Name,"
				 "'IMMParaInjectionPressure2Type',IMMParaInjectionPressure2Type,"
				 "'IMMParaInjectionPressure2NS',IMMParaInjectionPressure2NS,"
				 "'IMMParaInjectionPressure2ID',IMMParaInjectionPressure2ID,"
				 "'IMMParaInjectionPressure3Name',IMMParaInjectionPressure3Name,"
				 "'IMMParaInjectionPressure3Type',IMMParaInjectionPressure3Type,"
				 "'IMMParaInjectionPressure3NS',IMMParaInjectionPressure3NS,"
				 "'IMMParaInjectionPressure3ID',IMMParaInjectionPressure3ID,"
				 "'IMMParaInjectionPressure4Name',IMMParaInjectionPressure4Name,"
				 "'IMMParaInjectionPressure4Type',IMMParaInjectionPressure4Type,"
				 "'IMMParaInjectionPressure4NS',IMMParaInjectionPressure4NS,"
				 "'IMMParaInjectionPressure4ID',IMMParaInjectionPressure4ID,"
				 "'IMMParaInjectionPressure5Name',IMMParaInjectionPressure5Name,"
				 "'IMMParaInjectionPressure5Type',IMMParaInjectionPressure5Type,"
				 "'IMMParaInjectionPressure5NS',IMMParaInjectionPressure5NS,"
				 "'IMMParaInjectionPressure5ID',IMMParaInjectionPressure5ID,"
				 "'IMMParaInjectionSpeed1Name',IMMParaInjectionSpeed1Name,"
				 "'IMMParaInjectionSpeed1Type',IMMParaInjectionSpeed1Type,"
				 "'IMMParaInjectionSpeed1NS',IMMParaInjectionSpeed1NS,"
				 "'IMMParaInjectionSpeed1ID',IMMParaInjectionSpeed1ID,"
				 "'IMMParaInjectionSpeed2Name',IMMParaInjectionSpeed2Name,"
				 "'IMMParaInjectionSpeed2Type',IMMParaInjectionSpeed2Type,"
				 "'IMMParaInjectionSpeed2NS',IMMParaInjectionSpeed2NS,"
				 "'IMMParaInjectionSpeed2ID',IMMParaInjectionSpeed2ID,"
				 "'IMMParaInjectionSpeed3Name',IMMParaInjectionSpeed3Name,"
				 "'IMMParaInjectionSpeed3Type',IMMParaInjectionSpeed3Type,"
				 "'IMMParaInjectionSpeed3NS',IMMParaInjectionSpeed3NS,"
				 "'IMMParaInjectionSpeed3ID',IMMParaInjectionSpeed3ID,"
				 "'IMMParaInjectionSpeed4Name',IMMParaInjectionSpeed4Name,"
				 "'IMMParaInjectionSpeed4Type',IMMParaInjectionSpeed4Type,"
				 "'IMMParaInjectionSpeed4NS',IMMParaInjectionSpeed4NS,"
				 "'IMMParaInjectionSpeed4ID',IMMParaInjectionSpeed4ID,"
				 "'IMMParaInjectionSpeed5Name',IMMParaInjectionSpeed5Name,"
				 "'IMMParaInjectionSpeed5Type',IMMParaInjectionSpeed5Type,"
				 "'IMMParaInjectionSpeed5NS',IMMParaInjectionSpeed5NS,"
				 "'IMMParaInjectionSpeed5ID',IMMParaInjectionSpeed5ID,"
				 "'IMMParaVPChangeOverPositionName',IMMParaVPChangeOverPositionName,"
				 "'IMMParaVPChangeOverPositionType',IMMParaVPChangeOverPositionType,"
				 "'IMMParaVPChangeOverPositionNS',IMMParaVPChangeOverPositionNS,"
				 "'IMMParaVPChangeOverPositionID',IMMParaVPChangeOverPositionID,"
				 "'IMMParaVPChangeOverTimeName',IMMParaVPChangeOverTimeName,"
				 "'IMMParaVPChangeOverTimeType',IMMParaVPChangeOverTimeType,"
				 "'IMMParaVPChangeOverTimeNS',IMMParaVPChangeOverTimeNS,"
				 "'IMMParaVPChangeOverTimeID',IMMParaVPChangeOverTimeID,"
				 "'IMMParaPackingPressureName',IMMParaPackingPressureName,"
				 "'IMMParaPackingPressureType',IMMParaPackingPressureType,"
				 "'IMMParaPackingPressureNS',IMMParaPackingPressureNS,"
				 "'IMMParaPackingPressureID',IMMParaPackingPressureID,"
				 "'IMMParaHoldingPressure1Name',IMMParaHoldingPressure1Name,"
				 "'IMMParaHoldingPressure1Type',IMMParaHoldingPressure1Type,"
				 "'IMMParaHoldingPressure1NS',IMMParaHoldingPressure1NS,"
				 "'IMMParaHoldingPressure1ID',IMMParaHoldingPressure1ID,"
				 "'IMMParaHoldingPressure2Name',IMMParaHoldingPressure2Name,"
				 "'IMMParaHoldingPressure2Type',IMMParaHoldingPressure2Type,"
				 "'IMMParaHoldingPressure2NS',IMMParaHoldingPressure2NS,"
				 "'IMMParaHoldingPressure2ID',IMMParaHoldingPressure2ID,"
				 "'IMMParaHoldingPressure3Name',IMMParaHoldingPressure3Name,"
				 "'IMMParaHoldingPressure3Type',IMMParaHoldingPressure3Type,"
				 "'IMMParaHoldingPressure3NS',IMMParaHoldingPressure3NS,"
				 "'IMMParaHoldingPressure3ID',IMMParaHoldingPressure3ID,"
				 "'IMMParaHoldingPressure4Name',IMMParaHoldingPressure4Name,"
				 "'IMMParaHoldingPressure4Type',IMMParaHoldingPressure4Type,"
				 "'IMMParaHoldingPressure4NS',IMMParaHoldingPressure4NS,"
				 "'IMMParaHoldingPressure4ID',IMMParaHoldingPressure4ID,"
				 "'IMMParaHoldingPressure5Name',IMMParaHoldingPressure5Name,"
				 "'IMMParaHoldingPressure5Type',IMMParaHoldingPressure5Type,"
				 "'IMMParaHoldingPressure5NS',IMMParaHoldingPressure5NS,"
				 "'IMMParaHoldingPressure5ID',IMMParaHoldingPressure5ID,"
				 "'IMMParaPackingTimeName',IMMParaPackingTimeName,"
				 "'IMMParaPackingTimeType',IMMParaPackingTimeType,"
				 "'IMMParaPackingTimeNS',IMMParaPackingTimeNS,"
				 "'IMMParaPackingTimeID',IMMParaPackingTimeID,"
				 "'IMMParaHoldingTime1Name',IMMParaHoldingTime1Name,"
				 "'IMMParaHoldingTime1Type',IMMParaHoldingTime1Type,"
				 "'IMMParaHoldingTime1NS',IMMParaHoldingTime1NS,"
				 "'IMMParaHoldingTime1ID',IMMParaHoldingTime1ID,"
				 "'IMMParaHoldingTime2Name',IMMParaHoldingTime2Name,"
				 "'IMMParaHoldingTime2Type',IMMParaHoldingTime2Type,"
				 "'IMMParaHoldingTime2NS',IMMParaHoldingTime2NS,"
				 "'IMMParaHoldingTime2ID',IMMParaHoldingTime2ID,"
				 "'IMMParaHoldingTime3Name',IMMParaHoldingTime3Name,"
				 "'IMMParaHoldingTime3Type',IMMParaHoldingTime3Type,"
				 "'IMMParaHoldingTime3NS',IMMParaHoldingTime3NS,"
				 "'IMMParaHoldingTime3ID',IMMParaHoldingTime3ID,"
				 "'IMMParaHoldingTime4Name',IMMParaHoldingTime4Name,"
				 "'IMMParaHoldingTime4Type',IMMParaHoldingTime4Type,"
				 "'IMMParaHoldingTime4NS',IMMParaHoldingTime4NS,"
				 "'IMMParaHoldingTime4ID',IMMParaHoldingTime4ID,"
				 "'IMMParaHoldingTime5Name',IMMParaHoldingTime5Name,"
				 "'IMMParaHoldingTime5Type',IMMParaHoldingTime5Type,"
				 "'IMMParaHoldingTime5NS',IMMParaHoldingTime5NS,"
				 "'IMMParaHoldingTime5ID',IMMParaHoldingTime5ID,"
				 "'IMMParaCoolingTimeName',IMMParaCoolingTimeName,"
				 "'IMMParaCoolingTimeType',IMMParaCoolingTimeType,"
				 "'IMMParaCoolingTimeNS',IMMParaCoolingTimeNS,"
				 "'IMMParaCoolingTimeID',IMMParaCoolingTimeID,"
				 "'IMMParaScrewRPM1Name',IMMParaScrewRPM1Name,"
				 "'IMMParaScrewRPM1Type',IMMParaScrewRPM1Type,"
				 "'IMMParaScrewRPM1NS',IMMParaScrewRPM1NS,"
				 "'IMMParaScrewRPM1ID',IMMParaScrewRPM1ID,"
				 "'IMMParaScrewRPM2Name',IMMParaScrewRPM2Name,"
				 "'IMMParaScrewRPM2Type',IMMParaScrewRPM2Type,"
				 "'IMMParaScrewRPM2NS',IMMParaScrewRPM2NS,"
				 "'IMMParaScrewRPM2ID',IMMParaScrewRPM2ID,"
				 "'IMMParaScrewRPM3Name',IMMParaScrewRPM3Name,"
				 "'IMMParaScrewRPM3Type',IMMParaScrewRPM3Type,"
				 "'IMMParaScrewRPM3NS',IMMParaScrewRPM3NS,"
				 "'IMMParaScrewRPM3ID',IMMParaScrewRPM3ID,"
				 "'IMMParaScrewRPM4Name',IMMParaScrewRPM4Name,"
				 "'IMMParaScrewRPM4Type',IMMParaScrewRPM4Type,"
				 "'IMMParaScrewRPM4NS',IMMParaScrewRPM4NS,"
				 "'IMMParaScrewRPM4ID',IMMParaScrewRPM4ID,"
				 "'IMMParaScrewRPM5Name',IMMParaScrewRPM5Name,"
				 "'IMMParaScrewRPM5Type',IMMParaScrewRPM5Type,"
				 "'IMMParaScrewRPM5NS',IMMParaScrewRPM5NS,"
				 "'IMMParaScrewRPM5ID',IMMParaScrewRPM5ID,"
				 "'IMMParaBackPressure1Name',IMMParaBackPressure1Name,"
				 "'IMMParaBackPressure1Type',IMMParaBackPressure1Type,"
				 "'IMMParaBackPressure1NS',IMMParaBackPressure1NS,"
				 "'IMMParaBackPressure1ID',IMMParaBackPressure1ID,"
				 "'IMMParaBackPressure2Name',IMMParaBackPressure2Name,"
				 "'IMMParaBackPressure2Type',IMMParaBackPressure2Type,"
				 "'IMMParaBackPressure2NS',IMMParaBackPressure2NS,"
				 "'IMMParaBackPressure2ID',IMMParaBackPressure2ID,"
				 "'IMMParaBackPressure3Name',IMMParaBackPressure3Name,"
				 "'IMMParaBackPressure3Type',IMMParaBackPressure3Type,"
				 "'IMMParaBackPressure3NS',IMMParaBackPressure3NS,"
				 "'IMMParaBackPressure3ID',IMMParaBackPressure3ID,"
				 "'IMMParaBackPressure4Name',IMMParaBackPressure4Name,"
				 "'IMMParaBackPressure4Type',IMMParaBackPressure4Type,"
				 "'IMMParaBackPressure4NS',IMMParaBackPressure4NS,"
				 "'IMMParaBackPressure4ID',IMMParaBackPressure4ID,"
				 "'IMMParaBackPressure5Name',IMMParaBackPressure5Name,"
				 "'IMMParaBackPressure5Type',IMMParaBackPressure5Type,"
				 "'IMMParaBackPressure5NS',IMMParaBackPressure5NS,"
				 "'IMMParaBackPressure5ID',IMMParaBackPressure5ID,"
				 "'IMMParaMoldTemperatureName',IMMParaMoldTemperatureName,"
				 "'IMMParaMoldTemperatureType',IMMParaMoldTemperatureType,"
				 "'IMMParaMoldTemperatureNS',IMMParaMoldTemperatureNS,"
				 "'IMMParaMoldTemperatureID',IMMParaMoldTemperatureID,"
				 "'IMMSensorCycleTimeName',IMMSensorCycleTimeName,"
				 "'IMMSensorCycleTimeType',IMMSensorCycleTimeType,"
				 "'IMMSensorCycleTimeNS',IMMSensorCycleTimeNS,"
				 "'IMMSensorCycleTimeID',IMMSensorCycleTimeID,"
				 "'IMMSensorMoldClampingTimeName',IMMSensorMoldClampingTimeName,"
				 "'IMMSensorMoldClampingTimeType',IMMSensorMoldClampingTimeType,"
				 "'IMMSensorMoldClampingTimeNS',IMMSensorMoldClampingTimeNS,"
				 "'IMMSensorMoldClampingTimeID',IMMSensorMoldClampingTimeID,"
				 "'IMMSensorMoldReleasingTimeName',IMMSensorMoldReleasingTimeName,"
				 "'IMMSensorMoldReleasingTimeType',IMMSensorMoldReleasingTimeType,"
				 "'IMMSensorMoldReleasingTimeNS',IMMSensorMoldReleasingTimeNS,"
				 "'IMMSensorMoldReleasingTimeID',IMMSensorMoldReleasingTimeID,"
				 "'IMMSensorInjectionTimeName',IMMSensorInjectionTimeName,"
				 "'IMMSensorInjectionTimeType',IMMSensorInjectionTimeType,"
				 "'IMMSensorInjectionTimeNS',IMMSensorInjectionTimeNS,"
				 "'IMMSensorInjectionTimeID',IMMSensorInjectionTimeID,"
				 "'IMMSensorVPChangeOverDelayTimeName',IMMSensorVPChangeOverDelayTimeName,"
				 "'IMMSensorVPChangeOverDelayTimeType',IMMSensorVPChangeOverDelayTimeType,"
				 "'IMMSensorVPChangeOverDelayTimeNS',IMMSensorVPChangeOverDelayTimeNS,"
				 "'IMMSensorVPChangeOverDelayTimeID',IMMSensorVPChangeOverDelayTimeID,"
				 "'IMMSensorHoldingTimeName',IMMSensorHoldingTimeName,"
				 "'IMMSensorHoldingTimeType',IMMSensorHoldingTimeType,"
				 "'IMMSensorHoldingTimeNS',IMMSensorHoldingTimeNS,"
				 "'IMMSensorHoldingTimeID',IMMSensorHoldingTimeID,"
				 "'IMMSensorPlastificationTimeName',IMMSensorPlastificationTimeName,"
				 "'IMMSensorPlastificationTimeType',IMMSensorPlastificationTimeType,"
				 "'IMMSensorPlastificationTimeNS',IMMSensorPlastificationTimeNS,"
				 "'IMMSensorPlastificationTimeID',IMMSensorPlastificationTimeID,"
				 "'IMMSensorVPChangeOverPositionName',IMMSensorVPChangeOverPositionName,"
				 "'IMMSensorVPChangeOverPositionType',IMMSensorVPChangeOverPositionType,"
				 "'IMMSensorVPChangeOverPositionNS',IMMSensorVPChangeOverPositionNS,"
				 "'IMMSensorVPChangeOverPositionID',IMMSensorVPChangeOverPositionID,"
				 "'IMMSensorCusionVolumeName',IMMSensorCusionVolumeName,"
				 "'IMMSensorCusionVolumeType',IMMSensorCusionVolumeType,"
				 "'IMMSensorCusionVolumeNS',IMMSensorCusionVolumeNS,"
				 "'IMMSensorCusionVolumeID',IMMSensorCusionVolumeID,"
				 "'IMMSensorEndHoldingPositionName',IMMSensorEndHoldingPositionName,"
				 "'IMMSensorEndHoldingPositionType',IMMSensorEndHoldingPositionType,"
				 "'IMMSensorEndHoldingPositionNS',IMMSensorEndHoldingPositionNS,"
				 "'IMMSensorEndHoldingPositionID',IMMSensorEndHoldingPositionID,"
				 "'IMMSensorEndPlastificationPositionName',IMMSensorEndPlastificationPositionName,"
				 "'IMMSensorEndPlastificationPositionType',IMMSensorEndPlastificationPositionType,"
				 "'IMMSensorEndPlastificationPositionNS',IMMSensorEndPlastificationPositionNS,"
				 "'IMMSensorEndPlastificationPositionID',IMMSensorEndPlastificationPositionID,"
				 "'IMMSensorNozzle1TemperatureName',IMMSensorNozzle1TemperatureName,"
				 "'IMMSensorNozzle1TemperatureType',IMMSensorNozzle1TemperatureType,"
				 "'IMMSensorNozzle1TemperatureNS',IMMSensorNozzle1TemperatureNS,"
				 "'IMMSensorNozzle1TemperatureID',IMMSensorNozzle1TemperatureID,"
				 "'IMMSensorNozzle2TemperatureName',IMMSensorNozzle2TemperatureName,"
				 "'IMMSensorNozzle2TemperatureType',IMMSensorNozzle2TemperatureType,"
				 "'IMMSensorNozzle2TemperatureNS',IMMSensorNozzle2TemperatureNS,"
				 "'IMMSensorNozzle2TemperatureID',IMMSensorNozzle2TemperatureID,"
				 "'IMMSensorNozzle3TemperatureName',IMMSensorNozzle3TemperatureName,"
				 "'IMMSensorNozzle3TemperatureType',IMMSensorNozzle3TemperatureType,"
				 "'IMMSensorNozzle3TemperatureNS',IMMSensorNozzle3TemperatureNS,"
				 "'IMMSensorNozzle3TemperatureID',IMMSensorNozzle3TemperatureID,"
				 "'IMMSensorNozzle4TemperatureName',IMMSensorNozzle4TemperatureName,"
				 "'IMMSensorNozzle4TemperatureType',IMMSensorNozzle4TemperatureType,"
				 "'IMMSensorNozzle4TemperatureNS',IMMSensorNozzle4TemperatureNS,"
				 "'IMMSensorNozzle4TemperatureID',IMMSensorNozzle4TemperatureID,"
				 "'IMMSensorNozzle5TemperatureName',IMMSensorNozzle5TemperatureName,"
				 "'IMMSensorNozzle5TemperatureType',IMMSensorNozzle5TemperatureType,"
				 "'IMMSensorNozzle5TemperatureNS',IMMSensorNozzle5TemperatureNS,"
				 "'IMMSensorNozzle5TemperatureID',IMMSensorNozzle5TemperatureID,"
				 "'IMMSensorIMMOilTemperatureName',IMMSensorIMMOilTemperatureName,"
				 "'IMMSensorIMMOilTemperatureType',IMMSensorIMMOilTemperatureType,"
				 "'IMMSensorIMMOilTemperatureNS',IMMSensorIMMOilTemperatureNS,"
				 "'IMMSensorIMMOilTemperatureID',IMMSensorIMMOilTemperatureID,"
				 "'IMMSensorFallMaterialTemperatureName',IMMSensorFallMaterialTemperatureName,"
				 "'IMMSensorFallMaterialTemperatureType',IMMSensorFallMaterialTemperatureType,"
				 "'IMMSensorFallMaterialTemperatureNS',IMMSensorFallMaterialTemperatureNS,"
				 "'IMMSensorFallMaterialTemperatureID',IMMSensorFallMaterialTemperatureID,"
				 "'IMMSensorMaxInjectionPressureName',IMMSensorMaxInjectionPressureName,"
				 "'IMMSensorMaxInjectionPressureType',IMMSensorMaxInjectionPressureType,"
				 "'IMMSensorMaxInjectionPressureNS',IMMSensorMaxInjectionPressureNS,"
				 "'IMMSensorMaxInjectionPressureID',IMMSensorMaxInjectionPressureID,"
				 "'IMMSensorVPChangeOverPressureName',IMMSensorVPChangeOverPressureName,"
				 "'IMMSensorVPChangeOverPressureType',IMMSensorVPChangeOverPressureType,"
				 "'IMMSensorVPChangeOverPressureNS',IMMSensorVPChangeOverPressureNS,"
				 "'IMMSensorVPChangeOverPressureID',IMMSensorVPChangeOverPressureID,"
				 "'IMMSensorSumInjectionPressureName',IMMSensorSumInjectionPressureName,"
				 "'IMMSensorSumInjectionPressureType',IMMSensorSumInjectionPressureType,"
				 "'IMMSensorSumInjectionPressureNS',IMMSensorSumInjectionPressureNS,"
				 "'IMMSensorSumInjectionPressureID',IMMSensorSumInjectionPressureID,"
				 "'IMMSensorSumHoldingPressureName',IMMSensorSumHoldingPressureName,"
				 "'IMMSensorSumHoldingPressureType',IMMSensorSumHoldingPressureType,"
				 "'IMMSensorSumHoldingPressureNS',IMMSensorSumHoldingPressureNS,"
				 "'IMMSensorSumHoldingPressureID',IMMSensorSumHoldingPressureID,"
				 "'IMMSensorMaxCavityNearGatePressureName',IMMSensorMaxCavityNearGatePressureName,"
				 "'IMMSensorMaxCavityNearGatePressureType',IMMSensorMaxCavityNearGatePressureType,"
				 "'IMMSensorMaxCavityNearGatePressureNS',IMMSensorMaxCavityNearGatePressureNS,"
				 "'IMMSensorMaxCavityNearGatePressureID',IMMSensorMaxCavityNearGatePressureID,"
				 "'IMMSensorMaxCavityFarGatePressureName',IMMSensorMaxCavityFarGatePressureName,"
				 "'IMMSensorMaxCavityFarGatePressureType',IMMSensorMaxCavityFarGatePressureType,"
				 "'IMMSensorMaxCavityFarGatePressureNS',IMMSensorMaxCavityFarGatePressureNS,"
				 "'IMMSensorMaxCavityFarGatePressureID',IMMSensorMaxCavityFarGatePressureID,"
				 "'IMMSensorSumCavityPressureName',IMMSensorSumCavityPressureName,"
				 "'IMMSensorSumCavityPressureType',IMMSensorSumCavityPressureType,"
				 "'IMMSensorSumCavityPressureNS',IMMSensorSumCavityPressureNS,"
				 "'IMMSensorSumCavityPressureID',IMMSensorSumCavityPressureID,"
				 "'IMMSensorSumMoldClampingForceName',IMMSensorSumMoldClampingForceName,"
				 "'IMMSensorSumMoldClampingForceType',IMMSensorSumMoldClampingForceType,"
				 "'IMMSensorSumMoldClampingForceNS',IMMSensorSumMoldClampingForceNS,"
				 "'IMMSensorSumMoldClampingForceID',IMMSensorSumMoldClampingForceID,"
				 "'IMMSensorMaxMoldClampingForceName',IMMSensorMaxMoldClampingForceName,"
				 "'IMMSensorMaxMoldClampingForceType',IMMSensorMaxMoldClampingForceType,"
				 "'IMMSensorMaxMoldClampingForceNS',IMMSensorMaxMoldClampingForceNS,"
				 "'IMMSensorMaxMoldClampingForceID',IMMSensorMaxMoldClampingForceID,"
				 "'IMMSensorBackPressureName',IMMSensorBackPressureName,"
				 "'IMMSensorBackPressureType',IMMSensorBackPressureType,"
				 "'IMMSensorBackPressureNS',IMMSensorBackPressureNS,"
				 "'IMMSensorBackPressureID',IMMSensorBackPressureID,"
				 "'IMMSensorDelayPlastificationTimeName',IMMSensorDelayPlastificationTimeName,"
				 "'IMMSensorDelayPlastificationTimeType',IMMSensorDelayPlastificationTimeType,"
				 "'IMMSensorDelayPlastificationTimeNS',IMMSensorDelayPlastificationTimeNS,"
				 "'IMMSensorDelayPlastificationTimeID',IMMSensorDelayPlastificationTimeID,"
				 "'IMMSensorInMoldCoolingTemperatureName',IMMSensorInMoldCoolingTemperatureName,"
				 "'IMMSensorInMoldCoolingTemperatureType',IMMSensorInMoldCoolingTemperatureType,"
				 "'IMMSensorInMoldCoolingTemperatureNS',IMMSensorInMoldCoolingTemperatureNS,"
				 "'IMMSensorInMoldCoolingTemperatureID',IMMSensorInMoldCoolingTemperatureID,"
				 "'IMMSensorOutMoldCoolingTemperatureName',IMMSensorOutMoldCoolingTemperatureName,"
				 "'IMMSensorOutMoldCoolingTemperatureType',IMMSensorOutMoldCoolingTemperatureType,"
				 "'IMMSensorOutMoldCoolingTemperatureNS',IMMSensorOutMoldCoolingTemperatureNS,"
				 "'IMMSensorOutMoldCoolingTemperatureID',IMMSensorOutMoldCoolingTemperatureID,"
				 "'IMMSensorMoldCoolingVolumeName',IMMSensorMoldCoolingVolumeName,"
				 "'IMMSensorMoldCoolingVolumeType',IMMSensorMoldCoolingVolumeType,"
				 "'IMMSensorMoldCoolingVolumeNS',IMMSensorMoldCoolingVolumeNS,"
				 "'IMMSensorMoldCoolingVolumeID',IMMSensorMoldCoolingVolumeID,"
				 "'IMMSensorMoldReleasingPosition2Name',IMMSensorMoldReleasingPosition2Name,"
				 "'IMMSensorMoldReleasingPosition2Type',IMMSensorMoldReleasingPosition2Type,"
				 "'IMMSensorMoldReleasingPosition2NS',IMMSensorMoldReleasingPosition2NS,"
				 "'IMMSensorMoldReleasingPosition2ID',IMMSensorMoldReleasingPosition2ID,"
				 "'IMMSensorMoldReleasingPosition3Name',IMMSensorMoldReleasingPosition3Name,"
				 "'IMMSensorMoldReleasingPosition3Type',IMMSensorMoldReleasingPosition3Type,"
				 "'IMMSensorMoldReleasingPosition3NS',IMMSensorMoldReleasingPosition3NS,"
				 "'IMMSensorMoldReleasingPosition3ID',IMMSensorMoldReleasingPosition3ID,"
				 "'IMMSensorMoldReleasingPosition4Name',IMMSensorMoldReleasingPosition4Name,"
				 "'IMMSensorMoldReleasingPosition4Type',IMMSensorMoldReleasingPosition4Type,"
				 "'IMMSensorMoldReleasingPosition4NS',IMMSensorMoldReleasingPosition4NS,"
				 "'IMMSensorMoldReleasingPosition4ID',IMMSensorMoldReleasingPosition4ID,"
				 "'IMMSensorMoldReleasingPosition5Name',IMMSensorMoldReleasingPosition5Name,"
				 "'IMMSensorMoldReleasingPosition5Type',IMMSensorMoldReleasingPosition5Type,"
				 "'IMMSensorMoldReleasingPosition5NS',IMMSensorMoldReleasingPosition5NS,"
				 "'IMMSensorMoldReleasingPosition5ID',IMMSensorMoldReleasingPosition5ID,"
				 "'IMMSensorScrewSuckBackPositionName',IMMSensorScrewSuckBackPositionName,"
				 "'IMMSensorScrewSuckBackPositionType',IMMSensorScrewSuckBackPositionType,"
				 "'IMMSensorScrewSuckBackPositionNS',IMMSensorScrewSuckBackPositionNS,"
				 "'IMMSensorScrewSuckBackPositionID',IMMSensorScrewSuckBackPositionID,"
				 "'IMMSensorScrewRPMName',IMMSensorScrewRPMName,"
				 "'IMMSensorScrewRPMType',IMMSensorScrewRPMType,"
				 "'IMMSensorScrewRPMNS',IMMSensorScrewRPMNS,"
				 "'IMMSensorScrewRPMID',IMMSensorScrewRPMID,"
				 "'IMMSensorPartWeightName',IMMSensorPartWeightName,"
				 "'IMMSensorPartWeightType',IMMSensorPartWeightType,"
				 "'IMMSensorPartWeightNS',IMMSensorPartWeightNS,"
				 "'IMMSensorPartWeightID',IMMSensorPartWeightID,"
				 "'IMMSensorGateClosingTimeName',IMMSensorGateClosingTimeName,"
				 "'IMMSensorGateClosingTimeType',IMMSensorGateClosingTimeType,"
				 "'IMMSensorGateClosingTimeNS',IMMSensorGateClosingTimeNS,"
				 "'IMMSensorGateClosingTimeID',IMMSensorGateClosingTimeID,"
				 "'IMMSensorMeltTemperatureName',IMMSensorMeltTemperatureName,"
				 "'IMMSensorMeltTemperatureType',IMMSensorMeltTemperatureType,"
				 "'IMMSensorMeltTemperatureNS',IMMSensorMeltTemperatureNS,"
				 "'IMMSensorMeltTemperatureID',IMMSensorMeltTemperatureID,"
				 "'IMMSensorMoldTemperatureName',IMMSensorMoldTemperatureName,"
				 "'IMMSensorMoldTemperatureType',IMMSensorMoldTemperatureType,"
				 "'IMMSensorMoldTemperatureNS',IMMSensorMoldTemperatureNS,"
				 "'IMMSensorMoldTemperatureID',IMMSensorMoldTemperatureID"
				 ") "
				 "FROM "
				 "%s_%s.OPCUAVersionIndex WHERE OPCUAVersionMask=0 ORDER BY OPCUAVersionSN ASC",
				 SYS_NAME, INDEX_DATABASE_NAME);
	}

	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectOPCUAVersionIndex]Fail to select %s_%s.OPCUAVersionIndex (%d):%s",
				 SYS_NAME, INDEX_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectOPCUAVersionIndex]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}

int INDEX_SelectSPCIndex(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP, char *charQueryString)
{
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	unsigned int intLogSN;
	unsigned int intMOSN;
	unsigned int intMoldSN;
	unsigned int intProdSN;

	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	intMOSN = regexGetRequestQueryString_MOSN(charQueryString);
	intTableCount = 0;

	//Set MySQL Statement
	if (intLimit == 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'SPCSN',SPCCLList.SPCSN,"
				 "'XUCL',SPCCLList.XUCL,"
				 "'XCL',SPCCLList.XCL,"
				 "'XLCL',SPCCLList.XLCL,"
				 "'RUCL',SPCCLList.RUCL,"
				 "'RCL',SPCCLList.RCL,"
				 "'RLCL',SPCCLList.RLCL"
				 ") FROM"
				 " INJPRO_Data_MO_%d_Info_Config.SPCSNList AS SPCSNList,"
				 " INJPRO_Data_MO_%d_Info_Meta.SPCCLList AS SPCCLList"
				 " WHERE"
				 " SPCSNList.SPCSN=SPCCLList.SPCSN AND"
				 " SPCSNList.SPCRuleEnable = 1"
				 " ORDER BY SPCCLList.SPCSN LIMIT 9",
				 intMOSN, intMOSN);
	}
	// fprintf(stderr,"charStatement:%s",charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectSPCIndex]Fail to select %s_%s.SPCIndex (%d):%s",
				 SYS_NAME, INDEX_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectSPCIndex]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}

int INDEX_JSON_MaskSensorModelIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	struct json_object *jsoncharContentDataList, *jsonSensorModelSN;
	int intRetval;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_MaskSensorModelIndex]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_MaskSensorModelIndex JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		if (json_object_object_get_ex(jsoncharContentDataList, "SensorModelSN", &jsonSensorModelSN))
		{
			intRetval = INDEX_MaskSensor(mysqlCon, json_object_get_int(jsonSensorModelSN));
			if (intRetval == EXIT_SUCCESS)
			{
				json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
				json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS INDEX_JSON_MaskSensorModelIndex"));
				json_object_put(jsonSensorModelSN);
				json_object_put(jsoncharContentDataList);
				return EXIT_SUCCESS;
			}
			else
			{
				json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
				json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_MaskSensorModelIndex"));
				json_object_put(jsonSensorModelSN);
				json_object_put(jsoncharContentDataList);
				return EXIT_FAILURE;
			}
		}
		else
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_MaskSensorModelIndex json_object_object_get_ex"));
			json_object_put(jsonSensorModelSN);
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int INDEX_JSON_ReplaceSensorModelIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	struct json_object *jsoncharContentDataList, *jsonSensorModelSN, *jsonSensorVendor, *jsonSensorCategory, *jsonSensorModel, *jsonSensorModelMeta;
	int intRetval;
	stSensorModelSN varSensorModelSN;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_ReplaceSensorModelIndex]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_ReplaceSensorModelIndex JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		json_object_object_get_ex(jsoncharContentDataList, "SensorModelSN", &jsonSensorModelSN);
		json_object_object_get_ex(jsoncharContentDataList, "SensorVendor", &jsonSensorVendor);
		json_object_object_get_ex(jsoncharContentDataList, "SensorCategory", &jsonSensorCategory);
		json_object_object_get_ex(jsoncharContentDataList, "SensorModel", &jsonSensorModel);
		json_object_object_get_ex(jsoncharContentDataList, "SensorModelMeta", &jsonSensorModelMeta);
		strcpy(varSensorModelSN.charSensorModelMeta, json_object_get_string(jsonSensorModelMeta));

		// int INDEX_UpdateSensor(MYSQL mysqlCon, unsigned int intSensorModelSN, unsigned int intSensorCategory, const char *charSensorVendor,
		//    const char *charSensorModel, stSensorModelSN varSensorModelSN);
		intRetval = INDEX_UpdateSensor(mysqlCon, json_object_get_int(jsonSensorModelSN), json_object_get_int(jsonSensorCategory), json_object_get_string(jsonSensorVendor), json_object_get_string(jsonSensorModel), varSensorModelSN);
		if (intRetval == EXIT_SUCCESS)
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS INDEX_JSON_ReplaceSensorModelIndex"));
			json_object_put(jsonSensorModelSN);
			json_object_put(jsonSensorVendor);
			json_object_put(jsonSensorCategory);
			json_object_put(jsonSensorModel);
			json_object_put(jsonSensorModelMeta);
			json_object_put(jsoncharContentDataList);
			return EXIT_SUCCESS;
		}
		else
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_ReplaceSensorModelIndex"));
			json_object_put(jsonSensorModelSN);
			json_object_put(jsonSensorVendor);
			json_object_put(jsonSensorCategory);
			json_object_put(jsonSensorModel);
			json_object_put(jsonSensorModelMeta);
			json_object_put(jsoncharContentDataList);
			;
			return EXIT_FAILURE;
		}
	}
}

int INDEX_JSON_InsertSensorModelIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	struct json_object *jsoncharContentDataList, *jsonSensorVendor, *jsonSensorCategory, *jsonSensorModel, *jsonSensorModelMeta;
	int intRetval;
	stSensorModelSN varSensorModelSN;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_InsertSensorModelIndex]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[INDEX_JSON_InsertSensorModelIndex] Failed JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		json_object_object_get_ex(jsoncharContentDataList, "SensorVendor", &jsonSensorVendor);
		json_object_object_get_ex(jsoncharContentDataList, "SensorCategory", &jsonSensorCategory);
		json_object_object_get_ex(jsoncharContentDataList, "SensorModel", &jsonSensorModel);
		json_object_object_get_ex(jsoncharContentDataList, "SensorModelMeta", &jsonSensorModelMeta);
		strcpy(varSensorModelSN.charSensorModelMeta, json_object_get_string(jsonSensorModelMeta));

		// int INDEX_InsertSensor(MYSQL mysqlCon, const char *charSensorCategory,const char *charSensorVendor,const char *charSensorModel,stSensorModelSN varSensorModelSN);
		intRetval = INDEX_InsertSensor(mysqlCon, json_object_get_int(jsonSensorCategory), json_object_get_string(jsonSensorVendor), json_object_get_string(jsonSensorModel), varSensorModelSN);
		if (intRetval == EXIT_SUCCESS)
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[INDEX_JSON_InsertSensorModelIndex] SUCCESS"));
			json_object_put(jsonSensorVendor);
			json_object_put(jsonSensorCategory);
			json_object_put(jsonSensorModel);
			json_object_put(jsonSensorModelMeta);
			json_object_put(jsoncharContentDataList);
			return EXIT_SUCCESS;
		}
		else
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_InsertSensorModelIndex]Fail");
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[INDEX_JSON_InsertSensorModelIndex] Failed"));
			json_object_put(jsonSensorVendor);
			json_object_put(jsonSensorCategory);
			json_object_put(jsonSensorModel);
			json_object_put(jsonSensorModelMeta);
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int INDEX_SelectSensorModelIndex(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP)
{

	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	int intLogSN;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	//Select Number of Row
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s.SensorModelIndex", SYS_NAME, INDEX_DATABASE_NAME);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectSensorModelIndex]Fail to select COUNT(*) of %s_%s.SensorModelIndex (%d):%s",
				 SYS_NAME, INDEX_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_SUCCESS;
	}
	//Set MySQL Statement
	if (intLimit == 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT('SensorModelSN',SensorModelSN,'SensorModelMask',SensorModelMask,'SensorVendor',SensorVendor,'SensorCategory',SensorCategory,'SensorModel',SensorModel,'SensorModelMeta',SensorModelMeta) FROM %s_%s.SensorModelIndex WHERE SensorModelMask=0 ORDER BY SensorModelSN %s",
				 SYS_NAME, INDEX_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC");
	}
	else
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT('SensorModelSN',SensorModelSN,'SensorModelMask',SensorModelMask,'SensorVendor',SensorVendor,'SensorCategory',SensorCategory,'SensorModel',SensorModel,'SensorModelMeta',SensorModelMeta) FROM %s_%s.SensorModelIndex SensorModelMask=0 ORDER BY SensorModelSN %s LIMIT %d OFFSET %d",
				 SYS_NAME, INDEX_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC", intLimit, intOffset);
	}
	// fprintf(stderr, "charStatement:%s\n",charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectSensorModelIndex]Fail to select %s_%s.SensorModelIndex (%d):%s",
				 SYS_NAME, INDEX_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectSensorModelIndex]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}

int INDEX_JSON_MaskProdIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	struct json_object *jsoncharContentDataList, *jsonProdSN;
	int intRetval;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_MaskProdIndex]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_MaskProdIndex JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		if (json_object_object_get_ex(jsoncharContentDataList, "ProdSN", &jsonProdSN))
		{
			intRetval = INDEX_MaskProd(mysqlCon, json_object_get_int(jsonProdSN));
			if (intRetval == EXIT_SUCCESS)
			{
				json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
				json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS INDEX_JSON_MaskProdIndex"));
				json_object_put(jsonProdSN);
				json_object_put(jsoncharContentDataList);
				return EXIT_SUCCESS;
			}
			else
			{
				json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
				json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_MaskProdIndex"));
				json_object_put(jsonProdSN);
				json_object_put(jsoncharContentDataList);
				return EXIT_FAILURE;
			}
		}
		else
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_MaskProdIndex json_object_object_get_ex"));
			json_object_put(jsonProdSN);
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int INDEX_JSON_ReplaceProdIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	struct json_object *jsoncharContentDataList, *jsonProdSN, *jsonMoldSN, *jsonProdStr;
	int intRetval;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_ReplaceProdIndex]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_ReplaceProdIndex JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		json_object_object_get_ex(jsoncharContentDataList, "ProdSN", &jsonProdSN);
		json_object_object_get_ex(jsoncharContentDataList, "MoldSN", &jsonMoldSN);
		json_object_object_get_ex(jsoncharContentDataList, "ProdStr", &jsonProdStr);

		// int INDEX_UpdateProd(MYSQL mysqlCon, unsigned int intProdSN, unsigned int intMoldSN, const char *charProdStr);
		intRetval = INDEX_UpdateProd(mysqlCon, json_object_get_int(jsonProdSN), json_object_get_int(jsonMoldSN), json_object_get_string(jsonProdStr));
		if (intRetval == EXIT_SUCCESS)
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS INDEX_JSON_ReplaceProdIndex"));
			json_object_put(jsonProdSN);
			json_object_put(jsonMoldSN);
			json_object_put(jsonProdStr);
			json_object_put(jsoncharContentDataList);
			return EXIT_SUCCESS;
		}
		else
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_ReplaceProdIndex"));
			json_object_put(jsonProdSN);
			json_object_put(jsonMoldSN);
			json_object_put(jsonProdStr);
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int INDEX_JSON_InsertProdIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	struct json_object *jsoncharContentDataList, *jsonMoldSN, *jsonProdStr;
	int intRetval;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_InsertProdIndex]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[INDEX_JSON_InsertProdIndex] Failed JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		json_object_object_get_ex(jsoncharContentDataList, "MoldSN", &jsonMoldSN);
		json_object_object_get_ex(jsoncharContentDataList, "ProdStr", &jsonProdStr);

		// int INDEX_InsertProd(MYSQL mysqlCon, unsigned int intMoldSN,const char *charProdStr);
		intRetval = INDEX_InsertProd(mysqlCon, json_object_get_int(jsonMoldSN), json_object_get_string(jsonProdStr));
		if (intRetval == EXIT_SUCCESS)
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[INDEX_JSON_InsertProdIndex] SUCCESS"));
			json_object_put(jsonMoldSN);
			json_object_put(jsonProdStr);
			json_object_put(jsoncharContentDataList);
			return EXIT_SUCCESS;
		}
		else
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_InsertProdIndex]Fail");
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[INDEX_JSON_InsertProdIndex] Failed"));
			json_object_put(jsonMoldSN);
			json_object_put(jsonProdStr);
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int INDEX_SelectProdIndex(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP, char *charQueryString)
{

	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	int intLogSN;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];
	int intProdSN;

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	//Select Number of Row
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s.ProdIndex", SYS_NAME, INDEX_DATABASE_NAME);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectProdIndex]Fail to select COUNT(*) of %s_%s.ProdIndex (%d):%s",
				 SYS_NAME, INDEX_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_SUCCESS;
	}
	intProdSN = regexGetRequestQueryString_ProdSN(charQueryString);
	if (intProdSN != 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT('ProdSN',ProdSN,'ProdMask',ProdMask,'MoldSN',MoldSN,'ProdStr',ProdStr) FROM %s_%s.ProdIndex WHERE ProdSN=%d ORDER BY ProdSN %s",
				 SYS_NAME, INDEX_DATABASE_NAME, intProdSN, boolOrderByDESC == true ? "DESC" : "ASC");
	}
	else
	{
		if (intLimit == 0)
		{
			snprintf(charStatement, MAX_STRING_SIZE,
					 "SELECT JSON_OBJECT('ProdSN',ProdSN,'ProdMask',ProdMask,'MoldSN',MoldSN,'ProdStr',ProdStr) FROM %s_%s.ProdIndex WHERE ProdMask=0 ORDER BY ProdSN %s",
					 SYS_NAME, INDEX_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC");
		}
		else
		{
			snprintf(charStatement, MAX_STRING_SIZE,
					 "SELECT JSON_OBJECT('ProdSN',ProdSN,'ProdMask',ProdMask,'MoldSN',MoldSN,'ProdStr',ProdStr) FROM %s_%s.ProdIndex WHERE ProdMask=0 ORDER BY ProdSN %s LIMIT %d OFFSET %d",
					 SYS_NAME, INDEX_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC", intLimit, intOffset);
		}
	}
	//Set MySQL Statement

	// fprintf(stderr, "charStatement:%s\n",charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectProdIndex]Fail to select %s_%s.ProdIndex (%d):%s",
				 SYS_NAME, INDEX_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectProdIndex]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}

int INDEX_JSON_MaskMoldIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	struct json_object *jsoncharContentDataList, *jsonMoldSN;
	int intRetval;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_MaskMoldIndex]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_MaskMoldIndex JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		if (json_object_object_get_ex(jsoncharContentDataList, "MoldSN", &jsonMoldSN))
		{
			intRetval = INDEX_MaskMold(mysqlCon, json_object_get_int(jsonMoldSN));
			if (intRetval == EXIT_SUCCESS)
			{
				json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
				json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS INDEX_JSON_MaskMoldIndex"));
				json_object_put(jsonMoldSN);
				json_object_put(jsoncharContentDataList);
				return EXIT_SUCCESS;
			}
			else
			{
				json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
				json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_MaskMoldIndex"));
				json_object_put(jsonMoldSN);
				json_object_put(jsoncharContentDataList);
				return EXIT_FAILURE;
			}
		}
		else
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_MaskMoldIndex json_object_object_get_ex"));
			json_object_put(jsonMoldSN);
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int INDEX_JSON_ReplaceMoldIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	struct json_object *jsoncharContentDataList, *jsonMoldSN, *jsonMoldID, *jsonMoldVendor, *jsonMoldMeta;
	int intRetval;
	stMoldSN varMoldSN;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_ReplaceMoldIndex]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_ReplaceMoldIndex JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		json_object_object_get_ex(jsoncharContentDataList, "MoldSN", &jsonMoldSN);
		json_object_object_get_ex(jsoncharContentDataList, "MoldVendor", &jsonMoldVendor);
		json_object_object_get_ex(jsoncharContentDataList, "MoldID", &jsonMoldID);
		json_object_object_get_ex(jsoncharContentDataList, "MoldMeta", &jsonMoldMeta);
		strcpy(varMoldSN.charMoldMeta, json_object_get_string(jsonMoldMeta));

		// int INDEX_UpdateMold(MYSQL mysqlCon, unsigned int intMoldSN, const char *charMoldVendor, const char *charMoldID, stMoldSN varMoldSN);
		intRetval = INDEX_UpdateMold(mysqlCon, json_object_get_int(jsonMoldSN), json_object_get_string(jsonMoldVendor), json_object_get_string(jsonMoldID), varMoldSN);
		if (intRetval == EXIT_SUCCESS)
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS INDEX_JSON_ReplaceMoldIndex"));
			json_object_put(jsonMoldSN);
			json_object_put(jsonMoldID);
			json_object_put(jsonMoldVendor);
			json_object_put(jsonMoldMeta);
			json_object_put(jsoncharContentDataList);
			return EXIT_SUCCESS;
		}
		else
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_ReplaceMoldIndex"));
			json_object_put(jsonMoldSN);
			json_object_put(jsonMoldID);
			json_object_put(jsonMoldVendor);
			json_object_put(jsonMoldMeta);
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int INDEX_JSON_InsertMoldIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	struct json_object *jsoncharContentDataList, *jsonMoldID, *jsonMoldVendor, *jsonMoldMeta;
	int intRetval;
	stMoldSN varMoldSN;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_InsertMoldIndex]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[INDEX_JSON_InsertMoldIndex] Failed JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		json_object_object_get_ex(jsoncharContentDataList, "MoldVendor", &jsonMoldVendor);
		json_object_object_get_ex(jsoncharContentDataList, "MoldID", &jsonMoldID);
		json_object_object_get_ex(jsoncharContentDataList, "MoldMeta", &jsonMoldMeta);
		strcpy(varMoldSN.charMoldMeta, json_object_get_string(jsonMoldMeta));

		// int INDEX_InsertMold(MYSQL mysqlCon, const char *charMoldVendor,const char *charMoldID, stMoldSN varMoldSN);
		intRetval = INDEX_InsertMold(mysqlCon, json_object_get_string(jsonMoldVendor), json_object_get_string(jsonMoldID), varMoldSN);
		if (intRetval == EXIT_SUCCESS)
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[INDEX_JSON_InsertMoldIndex] SUCCESS"));
			json_object_put(jsonMoldID);
			json_object_put(jsonMoldVendor);
			json_object_put(jsonMoldMeta);
			json_object_put(jsoncharContentDataList);
			return EXIT_SUCCESS;
		}
		else
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_InsertMoldIndex]Fail");
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[INDEX_JSON_InsertMoldIndex] Failed"));
			json_object_put(jsonMoldID);
			json_object_put(jsonMoldVendor);
			json_object_put(jsonMoldMeta);
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int INDEX_SelectMoldIndex(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP)
{

	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	int intLogSN;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	//Select Number of Row
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s.MoldIndex", SYS_NAME, INDEX_DATABASE_NAME);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectMoldIndex]Fail to select COUNT(*) of %s_%s.MoldIndex (%d):%s",
				 SYS_NAME, INDEX_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_SUCCESS;
	}
	//Set MySQL Statement
	if (intLimit == 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT('MoldSN',MoldSN,'MoldMask',MoldMask,'MoldVendor',MoldVendor,'MoldID',MoldID,'MoldMeta',MoldMeta) FROM %s_%s.MoldIndex WHERE MoldMask=0 ORDER BY MoldSN %s",
				 SYS_NAME, INDEX_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC");
	}
	else
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT('MoldSN',MoldSN,'MoldMask',MoldMask,'MoldVendor',MoldVendor,'MoldID',MoldID,'MoldMeta',MoldMeta) FROM %s_%s.MoldIndex WHERE MoldMask=0 ORDER BY MoldSN %s LIMIT %d OFFSET %d",
				 SYS_NAME, INDEX_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC", intLimit, intOffset);
	}
	// fprintf(stderr, "charStatement:%s\n",charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectMoldIndex]Fail to select %s_%s.MoldIndex (%d):%s",
				 SYS_NAME, INDEX_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectMoldIndex]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}

int INDEX_JSON_MaskMaterialIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	struct json_object *jsoncharContentDataList, *jsonMaterialSN;
	int intRetval;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_MaskMaterialIndex]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_MaskMaterialIndex JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		if (json_object_object_get_ex(jsoncharContentDataList, "MaterialSN", &jsonMaterialSN))
		{
			intRetval = INDEX_MaskMaterial(mysqlCon, json_object_get_int(jsonMaterialSN));
			if (intRetval == EXIT_SUCCESS)
			{
				json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
				json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS INDEX_JSON_MaskMaterialIndex"));
				json_object_put(jsonMaterialSN);
				json_object_put(jsoncharContentDataList);
				return EXIT_SUCCESS;
			}
			else
			{
				json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
				json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_MaskMaterialIndex"));
				json_object_put(jsonMaterialSN);
				json_object_put(jsoncharContentDataList);
				return EXIT_FAILURE;
			}
		}
		else
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_MaskMaterialIndex json_object_object_get_ex"));
			json_object_put(jsonMaterialSN);
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int INDEX_JSON_ReplaceMaterialIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	struct json_object *jsoncharContentDataList, *jsonMaterialStr, *jsonMaterialSN;
	int intRetval;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_ReplaceMaterialIndex]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_ReplaceMaterialIndex JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		json_object_object_get_ex(jsoncharContentDataList, "MaterialStr", &jsonMaterialStr);
		json_object_object_get_ex(jsoncharContentDataList, "MaterialSN", &jsonMaterialSN);

		// int INDEX_UpdateMaterial(MYSQL mysqlCon, unsigned int intMaterialSN, const char *charMaterialStr);
		intRetval = INDEX_UpdateMaterial(mysqlCon, json_object_get_int(jsonMaterialSN), json_object_get_string(jsonMaterialStr));
		if (intRetval == EXIT_SUCCESS)
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS INDEX_JSON_ReplaceMaterialIndex"));
			json_object_put(jsonMaterialStr);
			json_object_put(jsonMaterialSN);
			json_object_put(jsoncharContentDataList);
			return EXIT_SUCCESS;
		}
		else
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_ReplaceMaterialIndex"));
			json_object_put(jsonMaterialStr);
			json_object_put(jsonMaterialSN);
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int INDEX_JSON_InsertMaterialIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	struct json_object *jsoncharContentDataList, *jsonMaterialStr;
	int intRetval;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_InsertMaterialIndex]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[INDEX_JSON_InsertMaterialIndex] Failed JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		json_object_object_get_ex(jsoncharContentDataList, "MaterialStr", &jsonMaterialStr);
		// int INDEX_InsertMaterial(MYSQL mysqlCon, const char *charMaterialStr);
		intRetval = INDEX_InsertMaterial(mysqlCon, json_object_get_string(jsonMaterialStr));
		if (intRetval == EXIT_SUCCESS)
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[INDEX_JSON_InsertMaterialIndex] SUCCESS"));
			json_object_put(jsonMaterialStr);
			json_object_put(jsoncharContentDataList);
			return EXIT_SUCCESS;
		}
		else
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_InsertMaterialIndex]Fail to INDEX_InsertMaterial");
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[INDEX_JSON_InsertMaterialIndex] Failed"));
			json_object_put(jsonMaterialStr);
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int INDEX_SelectMaterialIndex(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP)
{

	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	int intLogSN;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	//Select Number of Row
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s.MaterialIndex", SYS_NAME, INDEX_DATABASE_NAME);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectMaterialIndex]Fail to select COUNT(*) of %s_%s.MaterialIndex (%d):%s",
				 SYS_NAME, INDEX_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_SUCCESS;
	}
	//Set MySQL Statement
	if (intLimit == 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT('MaterialSN',MaterialSN,'MaterialMask',MaterialMask,'MaterialStr',MaterialStr) FROM %s_%s.MaterialIndex WHERE MaterialMask=0 ORDER BY MaterialSN %s",
				 SYS_NAME, INDEX_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC");
	}
	else
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT('MaterialSN',MaterialSN,'MaterialMask',MaterialMask,'MaterialStr',MaterialStr) FROM %s_%s.MaterialIndex WHERE MaterialMask=0 ORDER BY MaterialSN %s LIMIT %d OFFSET %d",
				 SYS_NAME, INDEX_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC", intLimit, intOffset);
	}
	// fprintf(stderr, "charStatement:%s\n",charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectMaterialIndex]Fail to select %s_%s.MaterialIndex (%d):%s",
				 SYS_NAME, INDEX_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectMaterialIndex]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}

int INDEX_JSON_ReplaceIMMModel(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	struct json_object *jsoncharContentDataList, *jsonIMMModelSN, *jsonIMMVendor, *jsonIMMModel, *jsonIMMModelMeta;
	int intRetval;
	stIMMModelSN varIMMModelSN;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_ReplaceIMMModel]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_ReplaceIMMModel JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		json_object_object_get_ex(jsoncharContentDataList, "IMMModelSN", &jsonIMMModelSN);
		json_object_object_get_ex(jsoncharContentDataList, "IMMVendor", &jsonIMMVendor);
		json_object_object_get_ex(jsoncharContentDataList, "IMMModel", &jsonIMMModel);
		json_object_object_get_ex(jsoncharContentDataList, "IMMModelMeta", &jsonIMMModelMeta);
		strcpy(varIMMModelSN.charIMMModelMeta, json_object_get_string(jsonIMMModelMeta));
		;

		// int INDEX_UpdateIMMModel(MYSQL mysqlCon, unsigned int intIMMModelSN, const char *charIMMVendor, const char *charIMMModel, stIMMModelSN varIMMModelSN);
		intRetval = INDEX_UpdateIMMModel(mysqlCon, json_object_get_int(jsonIMMModelSN), json_object_get_string(jsonIMMVendor), json_object_get_string(jsonIMMModel), varIMMModelSN);
		if (intRetval == EXIT_SUCCESS)
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS INDEX_JSON_ReplaceIMMModel"));
			json_object_put(jsonIMMModelSN);
			json_object_put(jsonIMMVendor);
			json_object_put(jsonIMMModel);
			json_object_put(jsonIMMModelMeta);
			json_object_put(jsoncharContentDataList);
			return EXIT_SUCCESS;
		}
		else
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_ReplaceIMMModel"));
			json_object_put(jsonIMMModelSN);
			json_object_put(jsonIMMVendor);
			json_object_put(jsonIMMModel);
			json_object_put(jsonIMMModelMeta);
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int INDEX_JSON_InsertIMMModel(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	struct json_object *jsoncharContentDataList, *jsonIMMVendor, *jsonIMMModel, *jsonIMMModelMeta;
	int intRetval;
	stIMMModelSN varIMMModelSN;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_InsertIMMModel]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_InsertIMMModel JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		json_object_object_get_ex(jsoncharContentDataList, "IMMVendor", &jsonIMMVendor);
		json_object_object_get_ex(jsoncharContentDataList, "IMMModel", &jsonIMMModel);
		json_object_object_get_ex(jsoncharContentDataList, "IMMModelMeta", &jsonIMMModelMeta);
		strcpy(varIMMModelSN.charIMMModelMeta, json_object_get_string(jsonIMMModelMeta));

		// INDEX_InsertIMMModel(MYSQL mysqlCon, const char *charIMMVendor, const char *charIMMModel, stIMMModelSN varIMMModelSN);
		intRetval = INDEX_InsertIMMModel(mysqlCon, json_object_get_string(jsonIMMVendor), json_object_get_string(jsonIMMModel), varIMMModelSN);
		if (intRetval == EXIT_SUCCESS)
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS INDEX_JSON_InsertIMMModel"));
			json_object_put(jsonIMMVendor);
			json_object_put(jsonIMMModel);
			json_object_put(jsonIMMModelMeta);
			json_object_put(jsoncharContentDataList);
			return EXIT_SUCCESS;
		}
		else
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_InsertIMMModel"));
			json_object_put(jsonIMMVendor);
			json_object_put(jsonIMMModel);
			json_object_put(jsonIMMModelMeta);
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int INDEX_JSON_MaskIMMModelIndex(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	struct json_object *jsoncharContentDataList, *jsonIMMModelSN;
	int intRetval;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_MaskIMMModelIndex]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_MaskIMMModelIndex JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		if (json_object_object_get_ex(jsoncharContentDataList, "IMMModelSN", &jsonIMMModelSN))
		{
			intRetval = INDEX_MaskIMMModel(mysqlCon, json_object_get_int(jsonIMMModelSN));
			if (intRetval == EXIT_SUCCESS)
			{
				json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
				json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS INDEX_JSON_MaskIMMModelIndex"));
				json_object_put(jsonIMMModelSN);
				json_object_put(jsoncharContentDataList);
				return EXIT_SUCCESS;
			}
			else
			{
				json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
				json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_MaskIMMModelIndex"));
				json_object_put(jsonIMMModelSN);
				json_object_put(jsoncharContentDataList);
				return EXIT_FAILURE;
			}
		}
		else
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_MaskIMMModelIndex json_object_object_get_ex"));
			json_object_put(jsonIMMModelSN);
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int INDEX_SelectIMMModelIndex(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP)
{

	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	int intLogSN;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	//Select Number of Row
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s.IMMModelIndex", SYS_NAME, INDEX_DATABASE_NAME);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectIMMModelIndex]Fail to select COUNT(*) of %s_%s.IMMModelIndex (%d):%s",
				 SYS_NAME, INDEX_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_SUCCESS;
	}
	//Set MySQL Statement
	if (intLimit == 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT('IMMModelSN',IMMModelSN,'IMMModelMask',IMMModelMask,'IMMVendor',IMMVendor,'IMMModel',IMMModel,'IMMModelMeta',IMMModelMeta) FROM %s_%s.IMMModelIndex WHERE IMMModelMask=0 ORDER BY IMMModelSN %s",
				 SYS_NAME, INDEX_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC");
	}
	else
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT('IMMModelSN',IMMModelSN,'IMMModelMask',IMMModelMask,'IMMVendor',IMMVendor,'IMMModel',IMMModel,'IMMModelMeta',IMMModelMeta) FROM %s_%s.IMMModelIndex WHERE IMMModelMask=0 ORDER BY IMMModelSN %s LIMIT %d OFFSET %d",
				 SYS_NAME, INDEX_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC", intLimit, intOffset);
	}
	// fprintf(stderr, "charStatement:%s\n",charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectIMMModelIndex]Fail to select %s_%s.IMMModelIndex (%d):%s",
				 SYS_NAME, INDEX_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectIMMModelIndex]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}

int INDEX_JSON_ReplaceAcceptCriteria(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	struct json_object *jsoncharContentDataList, *jsonAcceptCriteriaStr, *jsonAcceptCriteriaSN;
	int intRetval;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_ReplaceAcceptCriteria]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_ReplaceAcceptCriteria JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		json_object_object_get_ex(jsoncharContentDataList, "AcceptCriteriaName", &jsonAcceptCriteriaStr);
		json_object_object_get_ex(jsoncharContentDataList, "AcceptCriteriaSN", &jsonAcceptCriteriaSN);

		// int INDEX_UpdateAcceptCriteria(MYSQL mysqlCon, unsigned int intAcceptCriteriaSN, const char *charAcceptCriteriaStr);
		intRetval = INDEX_UpdateAcceptCriteria(mysqlCon, json_object_get_int(jsonAcceptCriteriaSN), json_object_get_string(jsonAcceptCriteriaStr));
		if (intRetval == EXIT_SUCCESS)
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS INDEX_JSON_ReplaceAcceptCriteria"));
			json_object_put(jsonAcceptCriteriaStr);
			json_object_put(jsonAcceptCriteriaSN);
			json_object_put(jsoncharContentDataList);
			return EXIT_SUCCESS;
		}
		else
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_ReplaceAcceptCriteria"));
			json_object_put(jsonAcceptCriteriaStr);
			json_object_put(jsonAcceptCriteriaSN);
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int INDEX_JSON_InsertAcceptCriteria(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	struct json_object *jsoncharContentDataList, *jsoncharContentDataObject;
	int intRetval;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_InsertAcceptCriteria]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_InsertAcceptCriteria JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		if (json_object_object_get_ex(jsoncharContentDataList, "AcceptCriteriaName", &jsoncharContentDataObject))
		{
			intRetval = INDEX_InsertAcceptCriteria(mysqlCon, json_object_get_string(jsoncharContentDataObject));
			if (intRetval == EXIT_SUCCESS)
			{
				json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
				json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS INDEX_JSON_InsertAcceptCriteria"));
				json_object_put(jsoncharContentDataObject);
				json_object_put(jsoncharContentDataList);
				return EXIT_SUCCESS;
			}
			else
			{
				json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
				json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_InsertAcceptCriteria"));
				json_object_put(jsoncharContentDataObject);
				json_object_put(jsoncharContentDataList);
				return EXIT_FAILURE;
			}
		}
		else
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_InsertAcceptCriteria json_object_object_get_ex"));
			json_object_put(jsoncharContentDataObject);
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}
int INDEX_JSON_MaskAcceptCriteria(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];
	struct json_object *jsoncharContentDataList, *jsonAcceptCriteriaSN;
	int intRetval;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_MaskAcceptCriteria]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_MaskAcceptCriteria JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		if (json_object_object_get_ex(jsoncharContentDataList, "AcceptCriteriaSN", &jsonAcceptCriteriaSN))
		{
			intRetval = INDEX_MaskAcceptCriteria(mysqlCon, json_object_get_int(jsonAcceptCriteriaSN));
			if (intRetval == EXIT_SUCCESS)
			{
				json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
				json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS INDEX_JSON_MaskAcceptCriteria"));
				json_object_put(jsonAcceptCriteriaSN);
				json_object_put(jsoncharContentDataList);
				return EXIT_SUCCESS;
			}
			else
			{
				json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
				json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_MaskAcceptCriteria"));
				json_object_put(jsonAcceptCriteriaSN);
				json_object_put(jsoncharContentDataList);
				return EXIT_FAILURE;
			}
		}
		else
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed INDEX_JSON_MaskAcceptCriteria json_object_object_get_ex"));
			json_object_put(jsonAcceptCriteriaSN);
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int INDEX_SelectAcceptCriteriaIndex(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP)
{

	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	int intLogSN;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	//Select Number of Row
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s.AcceptCriteriaIndex", SYS_NAME, INDEX_DATABASE_NAME);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectAcceptCriteriaIndex]Fail to select COUNT(*) of %s_%s.AcceptCriteriaIndex (%d):%s",
				 SYS_NAME, INDEX_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_SUCCESS;
	}
	//Set MySQL Statement
	if (intLimit == 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT('AcceptCriteriaSN',AcceptCriteriaSN,'AcceptCriteriaMask',AcceptCriteriaMask,'AcceptCriteriaCategory',AcceptCriteriaCategory,'AcceptCriteriaName',AcceptCriteriaName) FROM %s_%s.AcceptCriteriaIndex WHERE AcceptCriteriaMask=0 ORDER BY AcceptCriteriaSN %s",
				 SYS_NAME, INDEX_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC");
	}
	else
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT('AcceptCriteriaSN',AcceptCriteriaSN,'AcceptCriteriaMask',AcceptCriteriaMask,'AcceptCriteriaCategory',AcceptCriteriaCategory,'AcceptCriteriaName',AcceptCriteriaName) FROM %s_%s.AcceptCriteriaIndex WHERE AcceptCriteriaMask=0 ORDER BY AcceptCriteriaSN %s LIMIT %d OFFSET %d",
				 SYS_NAME, INDEX_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC", intLimit, intOffset);
	}
	// fprintf(stderr, "charStatement:%s\n",charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectAcceptCriteriaIndex]Fail to select %s_%s.AcceptCriteriaIndex (%d):%s",
				 SYS_NAME, INDEX_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_SelectAcceptCriteriaIndex]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}

int SYS_SelectSysErrMsg(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP)
{

	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	int intLogSN;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	//Select Number of Row
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s.SysLog", SYS_NAME, LOG_DATABASE_NAME);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SYS_SelectSysErrMsg]Fail to select COUNT(*) of %s_%s.SysLog (%d):%s",
				 SYS_NAME, LOG_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_SUCCESS;
	}
	//Set MySQL Statement
	if (intLimit == 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT('LogSN',LogSN,'ErrMsg',ErrMsg) FROM %s_%s.SysLog ORDER BY LogSN %s",
				 SYS_NAME, LOG_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC");
	}
	else
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT('LogSN',LogSN,'ErrMsg',ErrMsg) FROM %s_%s.SysLog ORDER BY LogSN %s LIMIT %d OFFSET %d",
				 SYS_NAME, LOG_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC", intLimit, intOffset);
	}

	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[SYS_SelectSysErrMsg]Fail to select %s_%s.SysLog (%d):%s",
				 SYS_NAME, LOG_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[SYS_SelectSysErrMsg]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}

int IMM_SelectIMMList(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP, char *charQueryString)
{

	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	int intLogSN;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];
	int intIMMSN;

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	//Select Number of Row
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s.IMMList", SYS_NAME, DATA_DATABASE_NAME);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[IMM_SelectIMMList]Fail to select COUNT(*) of %s_%s.SysLog (%d):%s",
				 SYS_NAME, LOG_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_SUCCESS;
	}
	// fprintf(stderr, "immsn1\n");
	intIMMSN = regexGetRequestQueryString_IMMSN(charQueryString);
	// fprintf(stderr, "immsn2\n");
	if (intIMMSN != 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'IMMSN',IMMSN,"
				 "'IMMID',IMMID,"
				 "'IMMMask',IMMMask,"
				 "'IMMModelSN',IMMModelSN,"
				 "'IMMStationID',IMMStationID,"
				 "'TechnicianUserSN',TechnicianUserSN,"
				 "'OperatorSN',OperatorSN,"
				 "'MOSN',MOSN,"
				 "'RoundSN',RoundSN,"
				 "'MoldStatus',MoldStatus,"
				 "'ShotSN',ShotSN,"
				 "'OPCUAVersionSN',OPCUAVersionSN,"
				 "'OPCUAIP',OPCUAIP,"
				 "'OPCUAPort',OPCUAPort,"
				 "'OPCUAUserName',OPCUAUserName,"
				 "'OPCAPassword',OPCAPassword,"
				 "'ModBusIP',ModBusIP,"
				 "'ModBusPort',ModBusPort,"
				 "'SubscribeModbusPID',SubscribeModbusPID,"
				 "'MoldSignalType',MoldSignalType,"
				 "'SubscribeIMMPID',SubscribeIMMPID,"
				 "'AutoOPCUAControl',AutoOPCUAControl,"
				 "'IMMInsertTime',IMMInsertTime,"
				 "'IMMLastUpdateTime',IMMLastUpdateTime"
				 ") FROM %s_%s.IMMList WHERE IMMSN=%d",
				 SYS_NAME, DATA_DATABASE_NAME, intIMMSN);
	}
	else
	{
		//Set MySQL Statement
		if (intLimit == 0)
		{
			snprintf(charStatement, MAX_STRING_SIZE,
					 "SELECT JSON_OBJECT("
					 "'IMMSN',IMMSN,"
					 "'IMMID',IMMID,"
					 "'IMMMask',IMMMask,"
					 "'IMMModelSN',IMMModelSN,"
					 "'IMMStationID',IMMStationID,"
					 "'TechnicianUserSN',TechnicianUserSN,"
					 "'OperatorSN',OperatorSN,"
					 "'MOSN',MOSN,"
					 "'RoundSN',RoundSN,"
					 "'MoldStatus',MoldStatus,"
					 "'ShotSN',ShotSN,"
					 "'OPCUAVersionSN',OPCUAVersionSN,"
					 "'OPCUAIP',OPCUAIP,"
					 "'OPCUAPort',OPCUAPort,"
					 "'OPCUAUserName',OPCUAUserName,"
					 "'OPCAPassword',OPCAPassword,"
					 "'ModBusIP',ModBusIP,"
					 "'ModBusPort',ModBusPort,"
					 "'SubscribeModbusPID',SubscribeModbusPID,"
					 "'MoldSignalType',MoldSignalType,"
					 "'SubscribeIMMPID',SubscribeIMMPID,"
					 "'AutoOPCUAControl',AutoOPCUAControl,"
					 "'IMMInsertTime',IMMInsertTime,"
					 "'IMMLastUpdateTime',IMMLastUpdateTime"
					 ") FROM %s_%s.IMMList ORDER BY IMMSN %s",
					 SYS_NAME, DATA_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC");
		}
		else
		{
			snprintf(charStatement, MAX_STRING_SIZE,
					 "SELECT JSON_OBJECT("
					 "'IMMSN',IMMSN,"
					 "'IMMID',IMMID,"
					 "'IMMMask',IMMMask,"
					 "'IMMModelSN',IMMModelSN,"
					 "'IMMStationID',IMMStationID,"
					 "'TechnicianUserSN',TechnicianUserSN,"
					 "'OperatorSN',OperatorSN,"
					 "'MOSN',MOSN,"
					 "'RoundSN',RoundSN,"
					 "'MoldStatus',MoldStatus,"
					 "'ShotSN',ShotSN,"
					 "'OPCUAVersionSN',OPCUAVersionSN,"
					 "'OPCUAIP',OPCUAIP,"
					 "'OPCUAPort',OPCUAPort,"
					 "'OPCUAUserName',OPCUAUserName,"
					 "'OPCAPassword',OPCAPassword,"
					 "'ModBusIP',ModBusIP,"
					 "'ModBusPort',ModBusPort,"
					 "'SubscribeModbusPID',SubscribeModbusPID,"
					 "'MoldSignalType',MoldSignalType,"
					 "'SubscribeIMMPID',SubscribeIMMPID,"
					 "'AutoOPCUAControl',AutoOPCUAControl,"
					 "'IMMInsertTime',IMMInsertTime,"
					 "'IMMLastUpdateTime',IMMLastUpdateTime"
					 ") FROM %s_%s.IMMList ORDER BY IMMSN %s LIMIT %d OFFSET %d",
					 SYS_NAME, DATA_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC", intLimit, intOffset);
		}
	}
	// fprintf(stderr, "charStatement:%s\n", charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[IMM_SelectIMMList]Fail to select %s_%s.IMMList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[IMM_SelectIMMList]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}

int MO_SelectMOList(MYSQL mysqlCon, int intLimit, int intOffset, bool boolOrderByDESC, struct json_object *jsonResponseHTTP, char *charQueryString)
{

	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	int intLogSN;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];
	int intMOSN;

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	//Select Number of Row
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s_%s.MOList", SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOList]Fail to select COUNT(*) of %s_%s.SysLog (%d):%s",
				 SYS_NAME, LOG_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_SUCCESS;
	}
	// fprintf(stderr, "immsn1\n");
	intMOSN = regexGetRequestQueryString_MOSN(charQueryString);
	// fprintf(stderr, "immsn2\n");
	if (intMOSN != 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'MOSN',MOSN,"
				 "'MOID',MOID,"
				 "'MOMask',MOMask,"
				 "'ProdouctShot',ProdouctShot,"
				 "'ProdVolume',ProdVolume,"
				 "'PiecePerShot',PiecePerShot,"
				 "'AcceptPart',AcceptPart,"
				 "'RejectPart',RejectPart,"
				 "'YieldRate',YieldRate,"
				 "'MOProgress',MOProgress,"
				 "'IMMSN',IMMSN,"
				 "'ProdSN',ProdSN,"
				 "'MoldSN',MoldSN,"
				 "'MaterialSN',MaterialSN,"
				 "'ExpectedStartTime',ExpectedStartTime,"
				 "'ExpectedEndTime',ExpectedEndTime,"
				 "'ExpectedProdouctVolume',ExpectedProdouctVolume,"
				 "'EstimatedEndTime',EstimatedEndTime,"
				 "'ActualStartTime',ActualStartTime,"
				 "'ActualEndTime',ActualEndTime,"
				 "'MOLastUpdateTime',MOLastUpdateTime"
				 ") FROM %s_%s_%s.MOList WHERE MOSN=%d ORDER BY MOSN %s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, boolOrderByDESC == true ? "DESC" : "ASC");
	}
	else
	{
		//Set MySQL Statement
		if (intLimit == 0)
		{
			snprintf(charStatement, MAX_STRING_SIZE,
					 "SELECT JSON_OBJECT("
					 "'MOSN',MOSN,"
					 "'MOID',MOID,"
					 "'MOMask',MOMask,"
					 "'ProdouctShot',ProdouctShot,"
					 "'ProdVolume',ProdVolume,"
					 "'PiecePerShot',PiecePerShot,"
					 "'AcceptPart',AcceptPart,"
					 "'RejectPart',RejectPart,"
					 "'YieldRate',YieldRate,"
					 "'MOProgress',MOProgress,"
					 "'IMMSN',IMMSN,"
					 "'ProdSN',ProdSN,"
					 "'MoldSN',MoldSN,"
					 "'MaterialSN',MaterialSN,"
					 "'ExpectedStartTime',ExpectedStartTime,"
					 "'ExpectedEndTime',ExpectedEndTime,"
					 "'ExpectedProdouctVolume',ExpectedProdouctVolume,"
					 "'EstimatedEndTime',EstimatedEndTime,"
					 "'ActualStartTime',ActualStartTime,"
					 "'ActualEndTime',ActualEndTime,"
					 "'MOLastUpdateTime',MOLastUpdateTime"
					 ") FROM %s_%s_%s.MOList ORDER BY MOSN %s",
					 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC");
		}
		else
		{
			snprintf(charStatement, MAX_STRING_SIZE,
					 "SELECT JSON_OBJECT("
					 "'MOSN',MOSN,"
					 "'MOID',MOID,"
					 "'MOMask',MOMask,"
					 "'ProdouctShot',ProdouctShot,"
					 "'ProdVolume',ProdVolume,"
					 "'PiecePerShot',PiecePerShot,"
					 "'AcceptPart',AcceptPart,"
					 "'RejectPart',RejectPart,"
					 "'YieldRate',YieldRate,"
					 "'MOProgress',MOProgress,"
					 "'IMMSN',IMMSN,"
					 "'ProdSN',ProdSN,"
					 "'MoldSN',MoldSN,"
					 "'MaterialSN',MaterialSN,"
					 "'ExpectedStartTime',ExpectedStartTime,"
					 "'ExpectedEndTime',ExpectedEndTime,"
					 "'ExpectedProdouctVolume',ExpectedProdouctVolume,"
					 "'EstimatedEndTime',EstimatedEndTime,"
					 "'ActualStartTime',ActualStartTime,"
					 "'ActualEndTime',ActualEndTime,"
					 "'MOLastUpdateTime',MOLastUpdateTime"
					 ") FROM %s_%s_%s.MOList ORDER BY MOSN %s LIMIT %d OFFSET %d",
					 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, boolOrderByDESC == true ? "DESC" : "ASC", intLimit, intOffset);
		}
	}
	// fprintf(stderr, "charStatement:%s\n", charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOList]Fail to select %s_%s_%s.MOList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOList]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}

int MO_SelectMOSensorSN_TableSN(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString)
{
	// INJPRO_Data_MO_4_Info_Meta
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	char charJsonDataSetName[SMALL_STRING_SIZE];
	int intRetval;
	int intLogSN;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	int intSensorCount;
	char charTableName_MOSensorSN_TableSN[MEDIUM_STRING_SIZE];
	char charTableName_MOSensorSNList[MEDIUM_STRING_SIZE];
	int intMOSN;
	int intShotSN;

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	intMOSN = regexGetRequestQueryString_MOSN(charQueryString);
	intShotSN = regexGetRequestQueryString_ShotSN(charQueryString);
	snprintf(charTableName_MOSensorSNList, MEDIUM_STRING_SIZE, "%s_%s_%s_%d_Info_Meta.MOSensorSNList", SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName_MOSensorSNList, &intSensorCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MOSensorSNList]Fail to select COUNT(*) of %s_%s_%s_%d_Info_Meta.MOSensorSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		return EXIT_FAILURE;
	}
	if (intSensorCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		return EXIT_SUCCESS;
	}

	for (int i = 1; i <= intSensorCount; i++)
	{
		//INJPRO_Data_MO_4_RawData_MoldSensor
		// snprintf(charTableName_MOSensorSN_TableSN, MEDIUM_STRING_SIZE, "%s_%s_%s_%d_RawData_MoldSensor.MOSensorSN%d_TableSN1", SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME,intMOSN,i);
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'x',ElapsedTime,"
				 "'y',SensorValue"
				 ") FROM %s_%s_%s_%d_RawData_MoldSensor.MOSensorSN%d_TableSN1 WHERE ShotSN=%d ORDER BY ElapsedTime ASC",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, i, intShotSN);
		intRetval = mysql_query(&mysqlCon, charStatement);

		//data to json
		if (intRetval)
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOSensorSN_TableSN]Fail to select %s_%s_%s_%d_RawData_MoldSensor.MOSensorSN%d_TableSN1 (%d):%s",
					 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, i, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			return EXIT_FAILURE;
		}
		else
		{
			mysqlResult = mysql_store_result(&mysqlCon);
			jsonRowObjectList = json_object_new_array();
		}
		while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
		{
			if (is_error(json_tokener_parse(mysqlRow[0])))
			{
				snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOSensorSN_TableSN]Fail to parse string:%s to json object", mysqlRow[0]);
				SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
				json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
				json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
				return EXIT_FAILURE;
			}
			else
			{
				json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
			}
		}
		mysql_free_result(mysqlResult);

		snprintf(charJsonDataSetName, SMALL_STRING_SIZE, "Data%d", i);
		json_object_object_add(jsonResponseHTTP, charJsonDataSetName, jsonRowObjectList);
	}

	// fprintf(stderr, "immsn2\n");
	// fprintf(stderr, "charStatement:%s\n", charStatement);
	//Query MySQL Database

	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "SensorCount", json_object_new_int(intSensorCount));
	// json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);
	return EXIT_SUCCESS;
}

int MO_SelectSPCFeatureValueList(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString)
{
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];
	int intMOSN;
	int intShotSN;

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	intMOSN = regexGetRequestQueryString_MOSN(charQueryString);
	intShotSN = regexGetRequestQueryString_ShotSN(charQueryString);
	//Select Number of Row
	// INJPRO_Data_MO_4_RawData_FeatureValue
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s_%s_%d_RawData_FeatureValue.MOSensorSN1", SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectSPCFeatureValueList]Fail to select COUNT(*) of %s_%s.MOSensorSN1 (%d):%s",
				 SYS_NAME, INDEX_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		mysql_close(&mysqlCon);
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		mysql_close(&mysqlCon);
		return EXIT_SUCCESS;
	}
	//Set MySQL Statement
	if (intShotSN != 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'ShotSN',s1.ShotSN,"
				 "'MaxPressure',s1.MaxPressure,"
				 "'SumStage1Pressure',s1.SumStage1Pressure,"
				 "'MaxPressureTime',s1.MaxPressureTime,"
				 "'AvgStage1PressureSlope',s1.AvgStage1PressureSlope,"
				 "'AvgStage2PressureSlope',s1.AvgStage2PressureSlope,"
				 "'MoldReleasePressure',s1.MoldReleasePressure,"
				 "'DiffMaxPressure',s2.DiffMaxPressure,"
				 "'DiffIncreasePressureTime',s2.DiffIncreasePressureTime,"
				 "'DiffMoldReleasePressure',s2.DiffMoldReleasePressure"
				 ") FROM "
				 "%s_%s_%s_%d_RawData_FeatureValue.MOSensorSN1 s1,"
				 "%s_%s_%s_%d_RawData_FeatureValue.SoftSensor s2"
				 " WHERE s1.ShotSN=s2.ShotSN AND ShotSN=%d",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
				 intShotSN);
	}
	else
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'ShotSN',s1.ShotSN,"
				 "'MaxPressure',s1.MaxPressure,"
				 "'SumStage1Pressure',s1.SumStage1Pressure,"
				 "'MaxPressureTime',s1.MaxPressureTime,"
				 "'AvgStage1PressureSlope',s1.AvgStage1PressureSlope,"
				 "'AvgStage2PressureSlope',s1.AvgStage2PressureSlope,"
				 "'MoldReleasePressure',s1.MoldReleasePressure,"
				 "'DiffMaxPressure',s2.DiffMaxPressure,"
				 "'DiffIncreasePressureTime',s2.DiffIncreasePressureTime,"
				 "'DiffMoldReleasePressure',s2.DiffMoldReleasePressure"
				 ") FROM "
				 "%s_%s_%s_%d_RawData_FeatureValue.MOSensorSN1 s1,"
				 "%s_%s_%s_%d_RawData_FeatureValue.SoftSensor s2"
				 " WHERE s1.ShotSN=s2.ShotSN"
				 " ORDER BY s1.ShotSN ASC",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	}
	//fprintf(stderr, "charStatement:%s\n", charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectSPCFeatureValueList]Fail to select %s_%s.SPCFeatureValueList (%d):%s",
				 SYS_NAME, INDEX_DATABASE_NAME, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectSPCFeatureValueList]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	// fprintf(stderr, "charStatement:%s\n",json_object_to_json_string(jsonRowObjectList));
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}
int MO_SelectSPCAlarmLog(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString)
{
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];
	int intMOSN;
	int intShotSN;

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	intMOSN = regexGetRequestQueryString_MOSN(charQueryString);
	intShotSN = regexGetRequestQueryString_ShotSN(charQueryString);
	// intShotSN = regexGetRequestQueryString_ShotSN(charQueryString);
	//Select Number of Row
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s_%s_%d_RawData_Alarm.SPCAlarmLog", SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectSPCAlarmLog]Fail to select COUNT(*) of %s_%s_%s_%d_RawData_Alarm.SPCAlarmLog (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_SUCCESS;
	}
	//Set MySQL Statement
	if (intShotSN != 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'ShotSN',ShotSN,"
				 //  "'SPCBoolReject',SPCBoolReject,"
				 //  "'Rule1Bool',Rule1Bool,"
				 "'Rule1BoolNGMaxPressure',Rule1BoolNGMaxPressure,"
				 "'Rule1BoolNGSumStage1Pressure',Rule1BoolNGSumStage1Pressure,"
				 "'Rule1BoolNGMaxPressureTime',Rule1BoolNGMaxPressureTime,"
				 "'Rule1BoolNGAvgStage1Slope',Rule1BoolNGAvgStage1Slope,"
				 "'Rule1BoolNGAvgStage2Slope',Rule1BoolNGAvgStage2Slope,"
				 "'Rule1BoolNGMoldReleasedPressure',Rule1BoolNGMoldReleasedPressure,"
				 "'Rule1BoolDiffMaxPressure',Rule1BoolDiffMaxPressure,"
				 "'Rule1BoolDiffIncreasePressureTime',Rule1BoolDiffIncreasePressureTime,"
				 "'Rule1BoolDiffMoldReleasedPressure',Rule1BoolDiffMoldReleasedPressure"
				 //  "'Rule2Bool',Rule2Bool,"
				 //  "'Rule2BoolNGMaxPressure',Rule2BoolNGMaxPressure,"
				 //  "'Rule2BoolNGSumStage1Pressure',Rule2BoolNGSumStage1Pressure,"
				 //  "'Rule2BoolNGMaxPressureTime',Rule2BoolNGMaxPressureTime,"
				 //  "'Rule2BoolNGAvgStage1Slope',Rule2BoolNGAvgStage1Slope,"
				 //  "'Rule2BoolNGAvgStage2Slope',Rule2BoolNGAvgStage2Slope,"
				 //  "'Rule2BoolNGMoldReleasedPressure',Rule2BoolNGMoldReleasedPressure,"
				 //  "'Rule2BoolDiffMaxPressure',Rule2BoolDiffMaxPressure,"
				 //  "'Rule2BoolDiffIncreasePressureTime',Rule2BoolDiffIncreasePressureTime,"
				 //  "'Rule2BoolDiffMoldReleasedPressure',Rule2BoolDiffMoldReleasedPressure,"
				 //  "'Rule3Bool',Rule3Bool,"
				 //  "'Rule3BoolNGMaxPressure',Rule3BoolNGMaxPressure,"
				 //  "'Rule3BoolNGSumStage1Pressure',Rule3BoolNGSumStage1Pressure,"
				 //  "'Rule3BoolNGMaxPressureTime',Rule3BoolNGMaxPressureTime,"
				 //  "'Rule3BoolNGAvgStage1Slope',Rule3BoolNGAvgStage1Slope,"
				 //  "'Rule3BoolNGAvgStage2Slope',Rule3BoolNGAvgStage2Slope,"
				 //  "'Rule3BoolNGMoldReleasedPressure',Rule3BoolNGMoldReleasedPressure,"
				 //  "'Rule3BoolDiffMaxPressure',Rule3BoolDiffMaxPressure,"
				 //  "'Rule3BoolDiffIncreasePressureTime',Rule3BoolDiffIncreasePressureTime,"
				 //  "'Rule3BoolDiffMoldReleasedPressure',Rule3BoolDiffMoldReleasedPressure,"
				 //  "'Rule4Bool',Rule4Bool,"
				 //  "'Rule4BoolNGMaxPressure',Rule4BoolNGMaxPressure,"
				 //  "'Rule4BoolNGSumStage1Pressure',Rule4BoolNGSumStage1Pressure,"
				 //  "'Rule4BoolNGMaxPressureTime',Rule4BoolNGMaxPressureTime,"
				 //  "'Rule4BoolNGAvgStage1Slope',Rule4BoolNGAvgStage1Slope,"
				 //  "'Rule4BoolNGAvgStage2Slope',Rule4BoolNGAvgStage2Slope,"
				 //  "'Rule4BoolNGMoldReleasedPressure',Rule4BoolNGMoldReleasedPressure,"
				 //  "'Rule4BoolDiffMaxPressure',Rule4BoolDiffMaxPressure,"
				 //  "'Rule4BoolDiffIncreasePressureTime',Rule4BoolDiffIncreasePressureTime,"
				 //  "'Rule4BoolDiffMoldReleasedPressure',Rule4BoolDiffMoldReleasedPressure,"
				 //  "'Rule5Bool',Rule5Bool,"
				 //  "'Rule5BoolNGMaxPressure',Rule5BoolNGMaxPressure,"
				 //  "'Rule5BoolNGSumStage1Pressure',Rule5BoolNGSumStage1Pressure,"
				 //  "'Rule5BoolNGMaxPressureTime',Rule5BoolNGMaxPressureTime,"
				 //  "'Rule5BoolNGAvgStage1Slope',Rule5BoolNGAvgStage1Slope,"
				 //  "'Rule5BoolNGAvgStage2Slope',Rule5BoolNGAvgStage2Slope,"
				 //  "'Rule5BoolNGMoldReleasedPressure',Rule5BoolNGMoldReleasedPressure,"
				 //  "'Rule5BoolDiffMaxPressure',Rule5BoolDiffMaxPressure,"
				 //  "'Rule5BoolDiffIncreasePressureTime',Rule5BoolDiffIncreasePressureTime,"
				 //  "'Rule5BoolDiffMoldReleasedPressure',Rule5BoolDiffMoldReleasedPressure,"
				 //  "'Rule6Bool',Rule6Bool,"
				 //  "'Rule6BoolNGMaxPressure',Rule6BoolNGMaxPressure,"
				 //  "'Rule6BoolNGSumStage1Pressure',Rule6BoolNGSumStage1Pressure,"
				 //  "'Rule6BoolNGMaxPressureTime',Rule6BoolNGMaxPressureTime,"
				 //  "'Rule6BoolNGAvgStage1Slope',Rule6BoolNGAvgStage1Slope,"
				 //  "'Rule6BoolNGAvgStage2Slope',Rule6BoolNGAvgStage2Slope,"
				 //  "'Rule6BoolNGMoldReleasedPressure',Rule6BoolNGMoldReleasedPressure,"
				 //  "'Rule6BoolDiffMaxPressure',Rule6BoolDiffMaxPressure,"
				 //  "'Rule6BoolDiffIncreasePressureTime',Rule6BoolDiffIncreasePressureTime,"
				 //  "'Rule6BoolDiffMoldReleasedPressure',Rule6BoolDiffMoldReleasedPressure,"
				 //  "'Rule7Bool',Rule7Bool,"
				 //  "'Rule7BoolNGMaxPressure',Rule7BoolNGMaxPressure,"
				 //  "'Rule7BoolNGSumStage1Pressure',Rule7BoolNGSumStage1Pressure,"
				 //  "'Rule7BoolNGMaxPressureTime',Rule7BoolNGMaxPressureTime,"
				 //  "'Rule7BoolNGAvgStage1Slope',Rule7BoolNGAvgStage1Slope,"
				 //  "'Rule7BoolNGAvgStage2Slope',Rule7BoolNGAvgStage2Slope,"
				 //  "'Rule7BoolNGMoldReleasedPressure',Rule7BoolNGMoldReleasedPressure,"
				 //  "'Rule7BoolDiffMaxPressure',Rule7BoolDiffMaxPressure,"
				 //  "'Rule7BoolDiffIncreasePressureTime',Rule7BoolDiffIncreasePressureTime,"
				 //  "'Rule7BoolDiffMoldReleasedPressure',Rule7BoolDiffMoldReleasedPressure,"
				 //  "'Rule8Bool',Rule8Bool,"
				 //  "'Rule8BoolNGMaxPressure',Rule8BoolNGMaxPressure,"
				 //  "'Rule8BoolNGSumStage1Pressure',Rule8BoolNGSumStage1Pressure,"
				 //  "'Rule8BoolNGMaxPressureTime',Rule8BoolNGMaxPressureTime,"
				 //  "'Rule8BoolNGAvgStage1Slope',Rule8BoolNGAvgStage1Slope,"
				 //  "'Rule8BoolNGAvgStage2Slope',Rule8BoolNGAvgStage2Slope,"
				 //  "'Rule8BoolNGMoldReleasedPressure',Rule8BoolNGMoldReleasedPressure,"
				 //  "'Rule8BoolDiffMaxPressure',Rule8BoolDiffMaxPressure,"
				 //  "'Rule8BoolDiffIncreasePressureTime',Rule8BoolDiffIncreasePressureTime,"
				 //  "'Rule8BoolDiffMoldReleasedPressure',Rule8BoolDiffMoldReleasedPressure"
				 ") FROM %s_%s_%s_%d_RawData_Alarm.SPCAlarmLog WHERE ShotSN=%d",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, intShotSN);
	}
	else
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'ShotSN',ShotSN,"
				 //  "'SPCBoolReject',SPCBoolReject,"
				 //  "'Rule1Bool',Rule1Bool,"
				 "'Rule1BoolNGMaxPressure',Rule1BoolNGMaxPressure,"
				 "'Rule1BoolNGSumStage1Pressure',Rule1BoolNGSumStage1Pressure,"
				 "'Rule1BoolNGMaxPressureTime',Rule1BoolNGMaxPressureTime,"
				 "'Rule1BoolNGAvgStage1Slope',Rule1BoolNGAvgStage1Slope,"
				 "'Rule1BoolNGAvgStage2Slope',Rule1BoolNGAvgStage2Slope,"
				 "'Rule1BoolNGMoldReleasedPressure',Rule1BoolNGMoldReleasedPressure,"
				 "'Rule1BoolDiffMaxPressure',Rule1BoolDiffMaxPressure,"
				 "'Rule1BoolDiffIncreasePressureTime',Rule1BoolDiffIncreasePressureTime,"
				 "'Rule1BoolDiffMoldReleasedPressure',Rule1BoolDiffMoldReleasedPressure"
				 //  "'Rule2Bool',Rule2Bool,"
				 //  "'Rule2BoolNGMaxPressure',Rule2BoolNGMaxPressure,"
				 //  "'Rule2BoolNGSumStage1Pressure',Rule2BoolNGSumStage1Pressure,"
				 //  "'Rule2BoolNGMaxPressureTime',Rule2BoolNGMaxPressureTime,"
				 //  "'Rule2BoolNGAvgStage1Slope',Rule2BoolNGAvgStage1Slope,"
				 //  "'Rule2BoolNGAvgStage2Slope',Rule2BoolNGAvgStage2Slope,"
				 //  "'Rule2BoolNGMoldReleasedPressure',Rule2BoolNGMoldReleasedPressure,"
				 //  "'Rule2BoolDiffMaxPressure',Rule2BoolDiffMaxPressure,"
				 //  "'Rule2BoolDiffIncreasePressureTime',Rule2BoolDiffIncreasePressureTime,"
				 //  "'Rule2BoolDiffMoldReleasedPressure',Rule2BoolDiffMoldReleasedPressure,"
				 //  "'Rule3Bool',Rule3Bool,"
				 //  "'Rule3BoolNGMaxPressure',Rule3BoolNGMaxPressure,"
				 //  "'Rule3BoolNGSumStage1Pressure',Rule3BoolNGSumStage1Pressure,"
				 //  "'Rule3BoolNGMaxPressureTime',Rule3BoolNGMaxPressureTime,"
				 //  "'Rule3BoolNGAvgStage1Slope',Rule3BoolNGAvgStage1Slope,"
				 //  "'Rule3BoolNGAvgStage2Slope',Rule3BoolNGAvgStage2Slope,"
				 //  "'Rule3BoolNGMoldReleasedPressure',Rule3BoolNGMoldReleasedPressure,"
				 //  "'Rule3BoolDiffMaxPressure',Rule3BoolDiffMaxPressure,"
				 //  "'Rule3BoolDiffIncreasePressureTime',Rule3BoolDiffIncreasePressureTime,"
				 //  "'Rule3BoolDiffMoldReleasedPressure',Rule3BoolDiffMoldReleasedPressure,"
				 //  "'Rule4Bool',Rule4Bool,"
				 //  "'Rule4BoolNGMaxPressure',Rule4BoolNGMaxPressure,"
				 //  "'Rule4BoolNGSumStage1Pressure',Rule4BoolNGSumStage1Pressure,"
				 //  "'Rule4BoolNGMaxPressureTime',Rule4BoolNGMaxPressureTime,"
				 //  "'Rule4BoolNGAvgStage1Slope',Rule4BoolNGAvgStage1Slope,"
				 //  "'Rule4BoolNGAvgStage2Slope',Rule4BoolNGAvgStage2Slope,"
				 //  "'Rule4BoolNGMoldReleasedPressure',Rule4BoolNGMoldReleasedPressure,"
				 //  "'Rule4BoolDiffMaxPressure',Rule4BoolDiffMaxPressure,"
				 //  "'Rule4BoolDiffIncreasePressureTime',Rule4BoolDiffIncreasePressureTime,"
				 //  "'Rule4BoolDiffMoldReleasedPressure',Rule4BoolDiffMoldReleasedPressure,"
				 //  "'Rule5Bool',Rule5Bool,"
				 //  "'Rule5BoolNGMaxPressure',Rule5BoolNGMaxPressure,"
				 //  "'Rule5BoolNGSumStage1Pressure',Rule5BoolNGSumStage1Pressure,"
				 //  "'Rule5BoolNGMaxPressureTime',Rule5BoolNGMaxPressureTime,"
				 //  "'Rule5BoolNGAvgStage1Slope',Rule5BoolNGAvgStage1Slope,"
				 //  "'Rule5BoolNGAvgStage2Slope',Rule5BoolNGAvgStage2Slope,"
				 //  "'Rule5BoolNGMoldReleasedPressure',Rule5BoolNGMoldReleasedPressure,"
				 //  "'Rule5BoolDiffMaxPressure',Rule5BoolDiffMaxPressure,"
				 //  "'Rule5BoolDiffIncreasePressureTime',Rule5BoolDiffIncreasePressureTime,"
				 //  "'Rule5BoolDiffMoldReleasedPressure',Rule5BoolDiffMoldReleasedPressure,"
				 //  "'Rule6Bool',Rule6Bool,"
				 //  "'Rule6BoolNGMaxPressure',Rule6BoolNGMaxPressure,"
				 //  "'Rule6BoolNGSumStage1Pressure',Rule6BoolNGSumStage1Pressure,"
				 //  "'Rule6BoolNGMaxPressureTime',Rule6BoolNGMaxPressureTime,"
				 //  "'Rule6BoolNGAvgStage1Slope',Rule6BoolNGAvgStage1Slope,"
				 //  "'Rule6BoolNGAvgStage2Slope',Rule6BoolNGAvgStage2Slope,"
				 //  "'Rule6BoolNGMoldReleasedPressure',Rule6BoolNGMoldReleasedPressure,"
				 //  "'Rule6BoolDiffMaxPressure',Rule6BoolDiffMaxPressure,"
				 //  "'Rule6BoolDiffIncreasePressureTime',Rule6BoolDiffIncreasePressureTime,"
				 //  "'Rule6BoolDiffMoldReleasedPressure',Rule6BoolDiffMoldReleasedPressure,"
				 //  "'Rule7Bool',Rule7Bool,"
				 //  "'Rule7BoolNGMaxPressure',Rule7BoolNGMaxPressure,"
				 //  "'Rule7BoolNGSumStage1Pressure',Rule7BoolNGSumStage1Pressure,"
				 //  "'Rule7BoolNGMaxPressureTime',Rule7BoolNGMaxPressureTime,"
				 //  "'Rule7BoolNGAvgStage1Slope',Rule7BoolNGAvgStage1Slope,"
				 //  "'Rule7BoolNGAvgStage2Slope',Rule7BoolNGAvgStage2Slope,"
				 //  "'Rule7BoolNGMoldReleasedPressure',Rule7BoolNGMoldReleasedPressure,"
				 //  "'Rule7BoolDiffMaxPressure',Rule7BoolDiffMaxPressure,"
				 //  "'Rule7BoolDiffIncreasePressureTime',Rule7BoolDiffIncreasePressureTime,"
				 //  "'Rule7BoolDiffMoldReleasedPressure',Rule7BoolDiffMoldReleasedPressure,"
				 //  "'Rule8Bool',Rule8Bool,"
				 //  "'Rule8BoolNGMaxPressure',Rule8BoolNGMaxPressure,"
				 //  "'Rule8BoolNGSumStage1Pressure',Rule8BoolNGSumStage1Pressure,"
				 //  "'Rule8BoolNGMaxPressureTime',Rule8BoolNGMaxPressureTime,"
				 //  "'Rule8BoolNGAvgStage1Slope',Rule8BoolNGAvgStage1Slope,"
				 //  "'Rule8BoolNGAvgStage2Slope',Rule8BoolNGAvgStage2Slope,"
				 //  "'Rule8BoolNGMoldReleasedPressure',Rule8BoolNGMoldReleasedPressure,"
				 //  "'Rule8BoolDiffMaxPressure',Rule8BoolDiffMaxPressure,"
				 //  "'Rule8BoolDiffIncreasePressureTime',Rule8BoolDiffIncreasePressureTime,"
				 //  "'Rule8BoolDiffMoldReleasedPressure',Rule8BoolDiffMoldReleasedPressure"
				 ") FROM %s_%s_%s_%d_RawData_Alarm.SPCAlarmLog WHERE SPCBoolReject=1 ORDER BY ShotSN DESC",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	}

	// fprintf(stderr, "charStatement:%s\n",charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectSPCAlarmLog]Fail to select %s_%s_%s_%d_RawData_Alarm.SPCAlarmLog (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectSPCAlarmLog]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	// fprintf(stderr, "charStatement:%s\n",json_object_to_json_string(jsonRowObjectList));
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}

int MO_SelectShotSNList(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString)
{
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];
	int intMOSN;
	// int intShotSN;

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	intMOSN = regexGetRequestQueryString_MOSN(charQueryString);
	// intShotSN = regexGetRequestQueryString_ShotSN(charQueryString);
	//Select Number of Row
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s_%s_%d_RawData.ShotSNList", SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectShotSNList]Fail to select COUNT(*) of %s_%s_%s_%d_RawData.ShotSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_SUCCESS;
	}
	//Set MySQL Statement
	snprintf(charStatement, MAX_STRING_SIZE,
			 "SELECT JSON_OBJECT("
			 "'ShotSN',ShotSN,"
			 "'CycleTime',CycleTime"
			 ") FROM %s_%s_%s_%d_RawData.ShotSNList ORDER BY ShotSN DESC",
			 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	// fprintf(stderr, "charStatement:%s\n",charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectShotSNList]Fail to select %s_%s_%s_%d_RawData.ShotSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectShotSNList]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	// fprintf(stderr, "charStatement:%s\n",json_object_to_json_string(jsonRowObjectList));
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}

int MO_IMMParaSNList(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString)
{
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];
	int intMOSN;
	int intIMMParaSN;

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	intMOSN = regexGetRequestQueryString_MOSN(charQueryString);
	intIMMParaSN = regexGetRequestQueryString_IMMParaSN(charQueryString);
	// intShotSN = regexGetRequestQueryString_ShotSN(charQueryString);
	//Select Number of Row
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s_%s_%d_RawData_IMMPara.IMMParaSNList", SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_IMMParaSNList]Fail to select COUNT(*) of %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_SUCCESS;
	}
	//Set MySQL Statement
	if (intIMMParaSN != 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'IMMParaSN',IMMParaSN,"
				 "'TechnicianUserSN',TechnicianUserSN,"
				 "'IMMParaLastUpdateTime',IMMParaLastUpdateTime,"
				 "'InjectStage',InjectStage,"
				 "'HoldStage',HoldStage,"
				 "'PlasStage',PlasStage,"
				 "'MeltTemperature',MeltTemperature,"
				 "'PlastificationVolume1',PlastificationVolume1,"
				 "'PlastificationVolume2',PlastificationVolume2,"
				 "'PlastificationVolume3',PlastificationVolume3,"
				 "'PlastificationVolume4',PlastificationVolume4,"
				 "'PlastificationVolume5',PlastificationVolume5,"
				 "'InjectionPressure1',InjectionPressure1,"
				 "'InjectionPressure2',InjectionPressure2,"
				 "'InjectionPressure3',InjectionPressure3,"
				 "'InjectionPressure4',InjectionPressure4,"
				 "'InjectionPressure5',InjectionPressure5,"
				 "'InjectionSpeed1',InjectionSpeed1,"
				 "'InjectionSpeed2',InjectionSpeed2,"
				 "'InjectionSpeed3',InjectionSpeed3,"
				 "'InjectionSpeed4',InjectionSpeed4,"
				 "'InjectionSpeed5',InjectionSpeed5,"
				 "'VPChangeOverPosition',VPChangeOverPosition,"
				 "'VPChangeOverTime',VPChangeOverTime,"
				 "'PackingPressure',PackingPressure,"
				 "'HoldingPressure1',HoldingPressure1,"
				 "'HoldingPressure2',HoldingPressure2,"
				 "'HoldingPressure3',HoldingPressure3,"
				 "'HoldingPressure4',HoldingPressure4,"
				 "'HoldingPressure5',HoldingPressure5,"
				 "'PackingTime',PackingTime,"
				 "'HoldingTime1',HoldingTime1,"
				 "'HoldingTime2',HoldingTime2,"
				 "'HoldingTime3',HoldingTime3,"
				 "'HoldingTime4',HoldingTime4,"
				 "'HoldingTime5',HoldingTime5,"
				 "'CoolingTime',CoolingTime,"
				 "'ScrewRPM1',ScrewRPM1,"
				 "'ScrewRPM2',ScrewRPM2,"
				 "'ScrewRPM3',ScrewRPM3,"
				 "'ScrewRPM4',ScrewRPM4,"
				 "'ScrewRPM5',ScrewRPM5,"
				 "'BackPressure1',BackPressure1,"
				 "'BackPressure2',BackPressure2,"
				 "'BackPressure3',BackPressure3,"
				 "'BackPressure4',BackPressure4,"
				 "'BackPressure5',BackPressure5,"
				 "'MoldTemperature',MoldTemperature"
				 ") FROM %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList WHERE IMMParaSN=%d",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, intIMMParaSN);
	}
	else
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'IMMParaSN',IMMParaSN,"
				 "'TechnicianUserSN',TechnicianUserSN,"
				 "'IMMParaLastUpdateTime',IMMParaLastUpdateTime,"
				 "'InjectStage',InjectStage,"
				 "'HoldStage',HoldStage,"
				 "'PlasStage',PlasStage,"
				 "'MeltTemperature',MeltTemperature,"
				 "'PlastificationVolume1',PlastificationVolume1,"
				 "'PlastificationVolume2',PlastificationVolume2,"
				 "'PlastificationVolume3',PlastificationVolume3,"
				 "'PlastificationVolume4',PlastificationVolume4,"
				 "'PlastificationVolume5',PlastificationVolume5,"
				 "'InjectionPressure1',InjectionPressure1,"
				 "'InjectionPressure2',InjectionPressure2,"
				 "'InjectionPressure3',InjectionPressure3,"
				 "'InjectionPressure4',InjectionPressure4,"
				 "'InjectionPressure5',InjectionPressure5,"
				 "'InjectionSpeed1',InjectionSpeed1,"
				 "'InjectionSpeed2',InjectionSpeed2,"
				 "'InjectionSpeed3',InjectionSpeed3,"
				 "'InjectionSpeed4',InjectionSpeed4,"
				 "'InjectionSpeed5',InjectionSpeed5,"
				 "'VPChangeOverPosition',VPChangeOverPosition,"
				 "'VPChangeOverTime',VPChangeOverTime,"
				 "'PackingPressure',PackingPressure,"
				 "'HoldingPressure1',HoldingPressure1,"
				 "'HoldingPressure2',HoldingPressure2,"
				 "'HoldingPressure3',HoldingPressure3,"
				 "'HoldingPressure4',HoldingPressure4,"
				 "'HoldingPressure5',HoldingPressure5,"
				 "'PackingTime',PackingTime,"
				 "'HoldingTime1',HoldingTime1,"
				 "'HoldingTime2',HoldingTime2,"
				 "'HoldingTime3',HoldingTime3,"
				 "'HoldingTime4',HoldingTime4,"
				 "'HoldingTime5',HoldingTime5,"
				 "'CoolingTime',CoolingTime,"
				 "'ScrewRPM1',ScrewRPM1,"
				 "'ScrewRPM2',ScrewRPM2,"
				 "'ScrewRPM3',ScrewRPM3,"
				 "'ScrewRPM4',ScrewRPM4,"
				 "'ScrewRPM5',ScrewRPM5,"
				 "'BackPressure1',BackPressure1,"
				 "'BackPressure2',BackPressure2,"
				 "'BackPressure3',BackPressure3,"
				 "'BackPressure4',BackPressure4,"
				 "'BackPressure5',BackPressure5,"
				 "'MoldTemperature',MoldTemperature"
				 ") FROM %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList ORDER BY IMMParaSN ASC",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	}

	// fprintf(stderr, "charStatement:%s\n",charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_IMMParaSNList]Fail to select %s_%s_%s_%d_RawData_IMMPara.IMMParaSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_IMMParaSNList]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	// fprintf(stderr, "charStatement:%s\n",json_object_to_json_string(jsonRowObjectList));
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}

int MO_RecomIMMParaSNList(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString)
{
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];
	int intMOSN;
	int intRecomIMMParaSN;

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	intMOSN = regexGetRequestQueryString_MOSN(charQueryString);
	intRecomIMMParaSN = regexGetRequestQueryString_RecomIMMParaSN(charQueryString);
	// intShotSN = regexGetRequestQueryString_ShotSN(charQueryString);
	//Select Number of Row
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s_%s_%d_RawData_IMMPara.RecomIMMParaSNList", SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_RecomIMMParaSNList]Fail to select COUNT(*) of %s_%s_%s_%d_RawData_IMMPara.RecomIMMParaSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_SUCCESS;
	}
	//Set MySQL Statement
	if (intRecomIMMParaSN != 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'RecomIMMParaSN',RecomIMMParaSN,"
				 "'InjectStage',InjectStage,"
				 "'HoldStage',HoldStage,"
				 "'PlasStage',PlasStage,"
				 "'MeltTemperature',MeltTemperature,"
				 "'PlastificationVolume1',PlastificationVolume1,"
				 "'PlastificationVolume2',PlastificationVolume2,"
				 "'PlastificationVolume3',PlastificationVolume3,"
				 "'PlastificationVolume4',PlastificationVolume4,"
				 "'PlastificationVolume5',PlastificationVolume5,"
				 "'InjectionPressure1',InjectionPressure1,"
				 "'InjectionPressure2',InjectionPressure2,"
				 "'InjectionPressure3',InjectionPressure3,"
				 "'InjectionPressure4',InjectionPressure4,"
				 "'InjectionPressure5',InjectionPressure5,"
				 "'InjectionSpeed1',InjectionSpeed1,"
				 "'InjectionSpeed2',InjectionSpeed2,"
				 "'InjectionSpeed3',InjectionSpeed3,"
				 "'InjectionSpeed4',InjectionSpeed4,"
				 "'InjectionSpeed5',InjectionSpeed5,"
				 "'VPChangeOverPosition',VPChangeOverPosition,"
				 "'VPChangeOverTime',VPChangeOverTime,"
				 "'PackingPressure',PackingPressure,"
				 "'HoldingPressure1',HoldingPressure1,"
				 "'HoldingPressure2',HoldingPressure2,"
				 "'HoldingPressure3',HoldingPressure3,"
				 "'HoldingPressure4',HoldingPressure4,"
				 "'HoldingPressure5',HoldingPressure5,"
				 "'PackingTime',PackingTime,"
				 "'HoldingTime1',HoldingTime1,"
				 "'HoldingTime2',HoldingTime2,"
				 "'HoldingTime3',HoldingTime3,"
				 "'HoldingTime4',HoldingTime4,"
				 "'HoldingTime5',HoldingTime5,"
				 "'CoolingTime',CoolingTime,"
				 "'ScrewRPM1',ScrewRPM1,"
				 "'ScrewRPM2',ScrewRPM2,"
				 "'ScrewRPM3',ScrewRPM3,"
				 "'ScrewRPM4',ScrewRPM4,"
				 "'ScrewRPM5',ScrewRPM5,"
				 "'BackPressure1',BackPressure1,"
				 "'BackPressure2',BackPressure2,"
				 "'BackPressure3',BackPressure3,"
				 "'BackPressure4',BackPressure4,"
				 "'BackPressure5',BackPressure5,"
				 "'MoldTemperature',MoldTemperature"
				 ") FROM %s_%s_%s_%d_RawData_IMMPara.RecomIMMParaSNList WHERE RecomIMMParaSN=%d",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, intRecomIMMParaSN);
	}
	else
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'RecomIMMParaSN',RecomIMMParaSN,"
				 "'InjectStage',InjectStage,"
				 "'HoldStage',HoldStage,"
				 "'PlasStage',PlasStage,"
				 "'MeltTemperature',MeltTemperature,"
				 "'PlastificationVolume1',PlastificationVolume1,"
				 "'PlastificationVolume2',PlastificationVolume2,"
				 "'PlastificationVolume3',PlastificationVolume3,"
				 "'PlastificationVolume4',PlastificationVolume4,"
				 "'PlastificationVolume5',PlastificationVolume5,"
				 "'InjectionPressure1',InjectionPressure1,"
				 "'InjectionPressure2',InjectionPressure2,"
				 "'InjectionPressure3',InjectionPressure3,"
				 "'InjectionPressure4',InjectionPressure4,"
				 "'InjectionPressure5',InjectionPressure5,"
				 "'InjectionSpeed1',InjectionSpeed1,"
				 "'InjectionSpeed2',InjectionSpeed2,"
				 "'InjectionSpeed3',InjectionSpeed3,"
				 "'InjectionSpeed4',InjectionSpeed4,"
				 "'InjectionSpeed5',InjectionSpeed5,"
				 "'VPChangeOverPosition',VPChangeOverPosition,"
				 "'VPChangeOverTime',VPChangeOverTime,"
				 "'PackingPressure',PackingPressure,"
				 "'HoldingPressure1',HoldingPressure1,"
				 "'HoldingPressure2',HoldingPressure2,"
				 "'HoldingPressure3',HoldingPressure3,"
				 "'HoldingPressure4',HoldingPressure4,"
				 "'HoldingPressure5',HoldingPressure5,"
				 "'PackingTime',PackingTime,"
				 "'HoldingTime1',HoldingTime1,"
				 "'HoldingTime2',HoldingTime2,"
				 "'HoldingTime3',HoldingTime3,"
				 "'HoldingTime4',HoldingTime4,"
				 "'HoldingTime5',HoldingTime5,"
				 "'CoolingTime',CoolingTime,"
				 "'ScrewRPM1',ScrewRPM1,"
				 "'ScrewRPM2',ScrewRPM2,"
				 "'ScrewRPM3',ScrewRPM3,"
				 "'ScrewRPM4',ScrewRPM4,"
				 "'ScrewRPM5',ScrewRPM5,"
				 "'BackPressure1',BackPressure1,"
				 "'BackPressure2',BackPressure2,"
				 "'BackPressure3',BackPressure3,"
				 "'BackPressure4',BackPressure4,"
				 "'BackPressure5',BackPressure5,"
				 "'MoldTemperature',MoldTemperature"
				 ") FROM %s_%s_%s_%d_RawData_IMMPara.RecomIMMParaSNList ORDER BY RecomIMMParaSN ASC",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	}

	// fprintf(stderr, "charStatement:%s\n",charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_RecomIMMParaSNList]Fail to select %s_%s_%s_%d_RawData_IMMPara.RecomIMMParaSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_RecomIMMParaSNList]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	// fprintf(stderr, "charStatement:%s\n",json_object_to_json_string(jsonRowObjectList));
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}

int MO_SelectMOAcceptCriteriaSN(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString)
{
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];
	int intMOSN;
	int intShotSN;

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	intMOSN = regexGetRequestQueryString_MOSN(charQueryString);
	intShotSN = regexGetRequestQueryString_ShotSN(charQueryString);
	// intShotSN = regexGetRequestQueryString_ShotSN(charQueryString);
	//Select Number of Row
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s_%s_%d_RawData.ShotSNList", SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOAcceptCriteriaSN]Fail to select COUNT(*) of %s_%s_%s_%d_RawData.ShotSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_SUCCESS;
	}
	//Set MySQL Statement
	if (intShotSN != 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'ShotSN',s1.ShotSN,"
				 "'RoundSN',s1.RoundSN,"
				 "'CycleTime',s1.CycleTime,"
				 "'IMMParaSN',s1.IMMParaSN,"
				 "'ErrorShot',s1.ErrorShot,"
				 "'MOAcceptCriteriaSN1',a1.PredictBool,"
				 "'MOAcceptCriteriaSN2',a2.PredictValue"
				 ") FROM "
				 "%s_%s_%s_%d_RawData_AcceptCriteria.MOAcceptCriteriaSN1 a1, "
				 "%s_%s_%s_%d_RawData_AcceptCriteria.MOAcceptCriteriaSN2 a2, "
				 "%s_%s_%s_%d_RawData.ShotSNList s1 "
				 "WHERE "
				 "s1.ShotSN = a1.ShotSN and s1.ShotSN = a2.ShotSN and s1.ShotSN=%d and s1.ErrorShot = 0",
				 //"s1.ShotSN = a1.ShotSN and s1.ShotSN = %d",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
				 intShotSN);
	}
	else
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'ShotSN',s1.ShotSN,"
				 "'RoundSN',s1.RoundSN,"
				 "'CycleTime',s1.CycleTime,"
				 "'IMMParaSN',s1.IMMParaSN,"
				 "'ErrorShot',s1.ErrorShot,"
				 "'MOAcceptCriteriaSN1',a1.PredictBool,"
				 "'MOAcceptCriteriaSN2',a2.PredictValue"
				 ") FROM "
				 "%s_%s_%s_%d_RawData_AcceptCriteria.MOAcceptCriteriaSN1 a1, "
				  "%s_%s_%s_%d_RawData_AcceptCriteria.MOAcceptCriteriaSN2 a2, "
				 "%s_%s_%s_%d_RawData.ShotSNList s1 "
				 "WHERE "
				  "s1.ShotSN = a1.ShotSN and s1.ShotSN = a2.ShotSN and s1.ErrorShot = 0 ORDER BY s1.ShotSN DESC",
				 //"s1.ShotSN = a1.ShotSN and s1.ErrorShot <> 1 ORDER BY s1.ShotSN DESC",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN,
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	}

	fprintf(stderr, "charStatement:%s\n",charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOAcceptCriteriaSN]Fail to select %s_%s_%s_%d_RawData.ShotSN (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOAcceptCriteriaSN]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	// fprintf(stderr, "charStatement:%s\n",json_object_to_json_string(jsonRowObjectList));
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}
// select s1.ShotSN, s1.CycleTime,s1.IMMParaSN,s1.ErrorShot, a1.PredictBool
// from INJPRO_Data_MO_5_RawData_AcceptCriteria.MOAcceptCriteriaSN1 a1, INJPRO_Data_MO_5_RawData_AcceptCriteria.MOAcceptCriteriaSN2 a2, INJPRO_Data_MO_5_RawData.ShotSNList s1
// where s1.ShotSN = a1.ShotSN and s1.ShotSN = a2.ShotSN
int MO_SelectIMMSensorSNList(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString)
{
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];
	int intMOSN;
	int intShotSN;

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	intMOSN = regexGetRequestQueryString_MOSN(charQueryString);
	intShotSN = regexGetRequestQueryString_ShotSN(charQueryString);
	// intShotSN = regexGetRequestQueryString_ShotSN(charQueryString);
	//Select Number of Row
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s_%s_%d_RawData_IMMSensor.IMMSensorSNList", SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectIMMSensorSNList]Fail to select COUNT(*) of %s_%s_%s_%d_RawData_IMMSensor.IMMSensorSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		mysql_close(&mysqlCon);
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		mysql_close(&mysqlCon);
		return EXIT_SUCCESS;
	}
	//Set MySQL Statement
	if (intShotSN != 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'ShotSN',ShotSN,"
				 "'CycleTime',CycleTime/1000000,"
				 "'InjectionTime',InjectionTime/1000000,"
				 "'PlastificationTime',PlastificationTime/1000000,"
				 "'VPChangeOverPosition',VPChangeOverPosition/1.5205309,"
				 "'EndPlastificationPosition',EndPlastificationPosition,"
				 "'MaxInjectionPressure',MaxInjectionPressure/13.0514053,"
				 "'VPChangeOverPressure',VPChangeOverPressure/13.0514053,"
				 "'BackPressure',BackPressure/13.0514053,"
				 "'EndHoldingPosition',EndHoldingPosition,"
				 "'ScrewSuckBackPosition',ScrewSuckBackPosition/0.2303835"
				 ") FROM %s_%s_%s_%d_RawData_IMMSensor.IMMSensorSNList WHERE ShotSN=%d",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, intShotSN);
	}
	else
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'ShotSN',ShotSN,"
				 "'CycleTime',CycleTime/1000000,"
				 "'InjectionTime',InjectionTime/1000000,"
				 "'PlastificationTime',PlastificationTime/1000000,"
				 "'VPChangeOverPosition',VPChangeOverPosition/1.5205309,"
				 "'EndPlastificationPosition',EndPlastificationPosition,"
				 "'MaxInjectionPressure',MaxInjectionPressure/13.0514053,"
				 "'VPChangeOverPressure',VPChangeOverPressure/13.0514053,"
				 "'BackPressure',BackPressure/13.0514053,"
				 "'EndHoldingPosition',EndHoldingPosition,"
				 "'ScrewSuckBackPosition',ScrewSuckBackPosition/0.2303835"
				 ") FROM %s_%s_%s_%d_RawData_IMMSensor.IMMSensorSNList ORDER BY ShotSN DESC LIMIT 1",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	}
	 fprintf(stderr, "charStatement:%s\n",charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectIMMSensorSNList]Fail to select %s_%s_%s_%d_RawData_IMMSensor.IMMSensorSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectIMMSensorSNList]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	// fprintf(stderr, "charStatement:%s\n",json_object_to_json_string(jsonRowObjectList));
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}
int MO_SelectMOAcceptCriteriaSNList(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString)
{
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];
	int intMOSN;

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	intMOSN = regexGetRequestQueryString_MOSN(charQueryString);
	//Select Number of RowtINJPRO_Data_MO_2_
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s_%s_%d_Info_Meta.MOAcceptCriteriaSNList", SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOAcceptCriteriaSNList]Fail to select COUNT(*) of %s_%s_%s_%d_Info_Meta.MOAcceptCriteriaSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		mysql_close(&mysqlCon);
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		mysql_close(&mysqlCon);
		return EXIT_SUCCESS;
	}
	//Set MySQL Statement
	if (intMOSN != 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'MOAcceptCriteriaSN',MOAcceptCriteriaSN,"
				 "'AcceptCriteriaSN',AcceptCriteriaSN,"
				 "'MOAcceptCriteriaMeta',MOAcceptCriteriaMeta,"
				 "'MOAcceptCriteriaClass',MOAcceptCriteriaClass,"
				 "'MOAcceptCriteriaUnit',MOAcceptCriteriaUnit,"
				 "'MOAcceptCriteriaMinTH',MOAcceptCriteriaMinTH,"
				 "'MOAcceptCriteriaMaxTH',MOAcceptCriteriaMaxTH"
				 ") FROM %s_%s_%s_%d_Info_Meta.MOAcceptCriteriaSNList",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	}
	else
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOAcceptCriteriaSNList]Fail to select %s_%s_%s_%d_Info_Meta.MOAcceptCriteriaSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		mysql_close(&mysqlCon);
		return EXIT_FAILURE;
	}
	// fprintf(stderr, "charStatement:%s\n",charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOAcceptCriteriaSNList]Fail to select %s_%s_%s_%d_Info_Meta.MOAcceptCriteriaSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		mysql_close(&mysqlCon);
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOAcceptCriteriaSNList]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			mysql_close(&mysqlCon);
			return EXIT_FAILURE;
		}
		else
		{
			fprintf(stderr, "mysqlRow:%s\n",mysqlRow[0]);
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	fprintf(stderr, "charStatement:%s\n",json_object_to_json_string(jsonRowObjectList));
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}
int MO_SelectMOAcceptCriteriaSN1(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString)
{
	char charStatement[MAX_STRING_SIZE];
	char charStatement1[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	int intMOSN;

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	intMOSN = regexGetRequestQueryString_MOSN(charQueryString);

	snprintf(charStatement, MAX_STRING_SIZE, "SELECT COUNT(*) FROM `INJPRO_Data_MO_%d_RawData`.ShotSNList AS a LEFT JOIN `INJPRO_Data_MO_%d_RawData_AcceptCriteria`.MOAcceptCriteriaSN1 AS b ON a.ShotSN = b.ShotSN WHERE b.ActualBool is not null", intMOSN, intMOSN);
	intRetval = mysql_query(&mysqlCon, charStatement);
	// fprintf(stderr, "intRetval:%d\n",intRetval);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOAcceptCriteriaSN1]Fail to select %s_%s_%s_%d_RawData_AcceptCriteria.MOAcceptCriteriaSN1 (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		mysql_close(&mysqlCon);
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_use_result(&mysqlCon);
		mysqlRow = mysql_fetch_row(mysqlResult);
		if (mysqlRow[0] != NULL)
		{
			intTableCount = atoi(mysqlRow[0]);
		}
		else
		{
			intTableCount = 0;
		}
		// mysql_close(&mysqlCon);
		mysql_free_result(mysqlResult);
	}

	//Set MySQL Statement
	if (intMOSN != 0)
	{
		//SELECT * FROM `INJPRO_Data_MO_2_RawData`.ShotSNList AS a LEFT JOIN `INJPRO_Data_MO_2_RawData_AcceptCriteria`.MOAcceptCriteriaSN1 AS b ON a.ShotSN = b.ShotSN
		snprintf(charStatement1, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'ShotSN',a.ShotSN,"
				 "'ActualBool',b.ActualBool,"
				 "'PredictBool',b.PredictBool,"
				 "'ActualValue',b.ActualValue,"
				 "'PredictValue',b.PredictValue"
				 ") FROM `%s_%s_%s_%d_RawData`.ShotSNList a LEFT JOIN `%s_%s_%s_%d_RawData_AcceptCriteria`.MOAcceptCriteriaSN1 b ON a.ShotSN = b.ShotSN ORDER BY a.ShotSN DESC",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	}
	else
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOAcceptCriteriaSN1]Fail to select %s_%s_%s_%d_RawData_AcceptCriteria.MOAcceptCriteriaSN1 (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		mysql_close(&mysqlCon);
		return EXIT_FAILURE;
	}
	// fprintf(stderr, "charStatement1:%s\n",charStatement1);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement1);
	// fprintf(stderr, "intRetval1:%d\n",intRetval);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOAcceptCriteriaSN1]Fail to select %s_%s_%s_%d_RawData_AcceptCriteria.MOAcceptCriteriaSN1 (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		mysql_close(&mysqlCon);
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOAcceptCriteriaSN1]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			mysql_close(&mysqlCon);
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	// fprintf(stderr, "charStatement:%s\n",json_object_to_json_string(jsonRowObjectList));
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}

int MO_SelectMOSensorSNList(MYSQL mysqlCon, struct json_object *jsonResponseHTTP, char *charQueryString)
{
	char charStatement[MAX_STRING_SIZE];
	char charErrMsg[LONG_STRING_SIZE];
	int intRetval;
	struct json_object *jsonRowObjectList;
	int intTableCount;
	char charTableName[MEDIUM_STRING_SIZE];
	int intMOSN;

	MYSQL_RES *mysqlResult;
	MYSQL_ROW mysqlRow;

	intMOSN = regexGetRequestQueryString_MOSN(charQueryString);
	//Select Number of RowtINJPRO_Data_MO_2_
	snprintf(charTableName, MEDIUM_STRING_SIZE, "%s_%s_%s_%d_Info_Meta.MOSensorSNList", SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	intRetval = DB_SelectTableCount(mysqlCon, charTableName, &intTableCount);
	if (intRetval != EXIT_SUCCESS)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOSensorSNList]Fail to select COUNT(*) of %s_%s_%s_%d_Info_Meta.MOSensorSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		mysql_close(&mysqlCon);
		return EXIT_FAILURE;
	}
	if (intTableCount == 0)
	{
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		mysql_close(&mysqlCon);
		return EXIT_SUCCESS;
	}
	//Set MySQL Statement
	if (intMOSN != 0)
	{
		snprintf(charStatement, MAX_STRING_SIZE,
				 "SELECT JSON_OBJECT("
				 "'MOSensorSN',MOSensorSN,"
				 "'MOSensorMeta',MOSensorMeta"
				 ") FROM %s_%s_%s_%d_Info_Meta.MOSensorSNList",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN);
	}
	else
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOSensorSNList]Fail to select %s_%s_%s_%d_Info_Meta.MOSensorSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		mysql_close(&mysqlCon);
		return EXIT_FAILURE;
	}
	// fprintf(stderr, "charStatement:%s\n",charStatement);
	//Query MySQL Database
	intRetval = mysql_query(&mysqlCon, charStatement);
	if (intRetval)
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOSensorSNList]Fail to select %s_%s_%s_%d_Info_Meta.MOSensorSNList (%d):%s",
				 SYS_NAME, DATA_DATABASE_NAME, MO_DATABASE_NAME, intMOSN, mysql_errno(&mysqlCon), mysql_error(&mysqlCon));
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
		json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
		mysql_close(&mysqlCon);
		return EXIT_FAILURE;
	}
	else
	{
		mysqlResult = mysql_store_result(&mysqlCon);
		jsonRowObjectList = json_object_new_array();
	}

	//json_object_object_add(jsonResponseHTTP,"Result",
	while ((mysqlRow = mysql_fetch_row(mysqlResult)) != NULL)
	{
		if (is_error(json_tokener_parse(mysqlRow[0])))
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_SelectMOSensorSNList]Fail to parse string:%s to json object", mysqlRow[0]);
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("Failed"));
			json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
			mysql_close(&mysqlCon);
			return EXIT_FAILURE;
		}
		else
		{
			json_object_array_add(jsonRowObjectList, json_tokener_parse(mysqlRow[0]));
		}
	}
	// fprintf(stderr, "charStatement:%s\n",json_object_to_json_string(jsonRowObjectList));
	mysql_free_result(mysqlResult);
	mysql_close(&mysqlCon);

	json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
	json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("SUCCESS"));
	json_object_object_add(jsonResponseHTTP, "TableCount", json_object_new_int(intTableCount));
	json_object_object_add(jsonResponseHTTP, "Data", jsonRowObjectList);

	return EXIT_SUCCESS;
}

int INDEX_JSON_SetMOAcceptCriteriaActualValue(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	char charErrMsg[LONG_STRING_SIZE];

	int intRetval;

	struct json_object *jsoncharContentDataList;
	struct json_object *jsonMOSN;
	struct json_object *jsonShotSN;
	struct json_object *jsonMOAcceptCriteriaSN;
	struct json_object *jsonActualValue;
	char charActualValue[TINY_STRING_SIZE];

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_SetMOAcceptCriteriaActualValue]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[INDEX_JSON_SetMOAcceptCriteriaActualValue] Failed JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		jsoncharContentDataList = json_tokener_parse(charContentData);
		json_object_object_get_ex(jsoncharContentDataList, "MOSN", &jsonMOSN);
		json_object_object_get_ex(jsoncharContentDataList, "ShotSN", &jsonShotSN);
		json_object_object_get_ex(jsoncharContentDataList, "MOAcceptCriteriaSN", &jsonMOAcceptCriteriaSN);
		json_object_object_get_ex(jsoncharContentDataList, "ActualValue", &jsonActualValue);
		strcpy(charActualValue, json_object_get_string(jsonActualValue));

		// int DB_InsertMOAcceptCriteriaActualValue(MYSQL mysqlCon,unsigned int intMOSN,unsigned int intShotSN, unsigned int intMOAcceptCriteriaSN, char charPredictValue[TINY_STRING_SIZE]);
		intRetval = DB_InsertMOAcceptCriteriaActualValue(mysqlCon, json_object_get_int(jsonMOSN), json_object_get_int(jsonShotSN), json_object_get_int(jsonMOAcceptCriteriaSN), charActualValue);
		if (intRetval == EXIT_SUCCESS)
		{
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(1));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[INDEX_JSON_SetMOAcceptCriteriaActualValue] SUCCESS"));
			json_object_put(jsoncharContentDataList);
			return EXIT_SUCCESS;
		}
		else
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[INDEX_JSON_SetMOAcceptCriteriaActualValue]Fail");
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[INDEX_JSON_SetMOAcceptCriteriaActualValue] Failed"));
			json_object_put(jsoncharContentDataList);
			return EXIT_FAILURE;
		}
	}
}

int MO_JSON_SetMOList(MYSQL mysqlCon, char *charContentData, struct json_object *jsonResponseHTTP)
{
	//	   {"charMOID": "Create_MO_By_JSON",
	//     "intIMMSN": 0,
	//     "intProdSN": 0,
	//     "intMoldSN": 0,
	//     "intMaterialSN": 0,
	//     "tmEST_year": 0, //ex:2018
	//     "tmEST_mon": 0,  //ex:4
	//     "tmEST_day": 0,  //ex:4
	//     "tmEST_hour": 0, //ex:2
	//     "tmEST_min": 0,  //ex:4
	//     "tmEST_sec": 0,  //ex:0
	//     "tmENT_year": 0, //ex:2018
	//     "tmENT_mon": 0,  //ex:4
	//     "tmENT_day": 0,  //ex:4
	//     "tmENT_hour": 0, //ex:2
	//     "tmENT_min": 0,  //ex:4
	//     "tmENT_sec": 0,  //ex:0
	//     "intExpectedProdVolume": 0,
	//     "SensorSNList": 2}
	// { "intSensorModelSN": 4, "intMOSensorChannelSN": 3, "doubleMOSensorConvertRatio": 1, "varMOSensorSN": "Pressure A" }
	// {"intMOAcceptCriteriaSN":2,"intAcceptCriteriaClass":2, "charAcceptCriteriaUnit":"g", "varAcceptCriteriaSN":"Product Weight"}

	int i, j;
	int intRetval;
	char charErrMsg[LONG_STRING_SIZE];
	unsigned int intMOSN;
	struct tm tmExpectedStartTime;
	struct tm tmExpectedEndTime;
	stMOAcceptCriteriaSN varMOAcceptCriteriaSN;
	stMOSensorSN varMOSensorSN;
	struct json_object *jsoncharContentDataList;
	struct json_object *jsonSensorSNListData;
	struct json_object *jsonAcceptCriteriaSNListData;
	struct json_object *jsonTempSensorSNListDataObject;
	struct json_object *jsonTempAcceptCriteriaSNListDataObject;

	struct json_object *jsoncharMOID;
	struct json_object *jsonintIMMSN;
	struct json_object *jsonintProdSN;
	struct json_object *jsonintMoldSN;
	struct json_object *jsonintMaterialSN;
	struct json_object *jsontmEST_year;
	struct json_object *jsontmEST_mon;
	struct json_object *jsontmEST_day;
	struct json_object *jsontmEST_hour;
	struct json_object *jsontmEST_min;
	struct json_object *jsontmEST_sec;
	struct json_object *jsontmENT_year;
	struct json_object *jsontmENT_mon;
	struct json_object *jsontmENT_day;
	struct json_object *jsontmENT_hour;
	struct json_object *jsontmENT_min;
	struct json_object *jsontmENT_sec;
	struct json_object *jsonintExpectedProdVolume;
	struct json_object *jsonSensorSNList;
	struct json_object *jsonAcceptCriteriaSNList;
	struct json_object *jsonintPeicePerShot;

	struct json_object *jsonintSensorModelSN;
	struct json_object *jsonintMOSensorChannelSN;
	struct json_object *jsondoubleMOSensorConvertRatio;
	struct json_object *jsonvarMOSensorSN;

	struct json_object *jsonintMOAcceptCriteriaSN;
	struct json_object *jsonintAcceptCriteriaClass;
	struct json_object *jsoncharAcceptCriteriaUnit;
	struct json_object *jsonvarAcceptCriteriaSN;

	if (is_error(json_tokener_parse(charContentData)))
	{
		snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_JSON_SetMOList]Fail to parse string:%s to json object", charContentData);
		SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
		json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
		json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[MO_JSON_SetMOList] Failed JSON parse"));
		return EXIT_FAILURE;
	}
	else
	{
		// fprintf(stderr, "charStatement:%s\n", charContentData);
		jsoncharContentDataList = json_tokener_parse(charContentData);
		json_object_object_get_ex(jsoncharContentDataList, "charMOID", &jsoncharMOID);
		json_object_object_get_ex(jsoncharContentDataList, "intIMMSN", &jsonintIMMSN);
		json_object_object_get_ex(jsoncharContentDataList, "intProdSN", &jsonintProdSN);
		json_object_object_get_ex(jsoncharContentDataList, "intMoldSN", &jsonintMoldSN);
		json_object_object_get_ex(jsoncharContentDataList, "intMaterialSN", &jsonintMaterialSN);
		json_object_object_get_ex(jsoncharContentDataList, "tmEST_year", &jsontmEST_year);
		json_object_object_get_ex(jsoncharContentDataList, "tmEST_mon", &jsontmEST_mon);
		json_object_object_get_ex(jsoncharContentDataList, "tmEST_day", &jsontmEST_day);
		json_object_object_get_ex(jsoncharContentDataList, "tmEST_hour", &jsontmEST_hour);
		json_object_object_get_ex(jsoncharContentDataList, "tmEST_min", &jsontmEST_min);
		json_object_object_get_ex(jsoncharContentDataList, "tmEST_sec", &jsontmEST_sec);
		json_object_object_get_ex(jsoncharContentDataList, "tmENT_year", &jsontmENT_year);
		json_object_object_get_ex(jsoncharContentDataList, "tmENT_mon", &jsontmENT_mon);
		json_object_object_get_ex(jsoncharContentDataList, "tmENT_day", &jsontmENT_day);
		json_object_object_get_ex(jsoncharContentDataList, "tmENT_hour", &jsontmENT_hour);
		json_object_object_get_ex(jsoncharContentDataList, "tmENT_min", &jsontmENT_min);
		json_object_object_get_ex(jsoncharContentDataList, "tmENT_sec", &jsontmENT_sec);
		json_object_object_get_ex(jsoncharContentDataList, "intExpectedProdVolume", &jsonintExpectedProdVolume);
		json_object_object_get_ex(jsoncharContentDataList, "SensorSNListData", &jsonSensorSNListData);
		json_object_object_get_ex(jsoncharContentDataList, "AcceptCriteriaSNListData", &jsonAcceptCriteriaSNListData);
		json_object_object_get_ex(jsoncharContentDataList, "intPeicePerShot", &jsonintPeicePerShot);
		fprintf(stderr,
				"charMOID:%s\n"
				"intIMMSN:%d\n"
				"intProdSN:%d\n"
				"intMoldSN:%d\n"
				"intMaterialSN:%d\n"
				"tmEST_year:%d\n"
				"tmEST_mon:%d\n"
				"tmEST_day:%d\n"
				"tmEST_hour:%d\n"
				"tmEST_min:%d\n"
				"tmEST_sec:%d\n"
				"tmENT_year:%d\n"
				"tmENT_mon:%d\n"
				"tmENT_day:%d\n"
				"tmENT_hour:%d\n"
				"tmENT_min:%d\n"
				"tmENT_sec:%d\n"
				"intExpectedProdVolume:%d\n"
				"intPeicePerShot:%d\n",
				json_object_get_string(jsoncharMOID),
				json_object_get_int(jsonintIMMSN),
				json_object_get_int(jsonintProdSN),
				json_object_get_int(jsonintMoldSN),
				json_object_get_int(jsonintMaterialSN),
				json_object_get_int(jsontmEST_year),
				json_object_get_int(jsontmEST_mon),
				json_object_get_int(jsontmEST_day),
				json_object_get_int(jsontmEST_hour),
				json_object_get_int(jsontmEST_min),
				json_object_get_int(jsontmEST_sec),
				json_object_get_int(jsontmENT_year),
				json_object_get_int(jsontmENT_mon),
				json_object_get_int(jsontmENT_day),
				json_object_get_int(jsontmENT_hour),
				json_object_get_int(jsontmENT_min),
				json_object_get_int(jsontmENT_sec),
				json_object_get_int(jsonintExpectedProdVolume),
				json_object_get_int(jsonintPeicePerShot));
		tmExpectedStartTime.tm_year = json_object_get_int(jsontmEST_year);
		tmExpectedStartTime.tm_mon = json_object_get_int(jsontmEST_mon);
		tmExpectedStartTime.tm_mday = json_object_get_int(jsontmEST_day);
		tmExpectedStartTime.tm_hour = json_object_get_int(jsontmEST_hour);
		tmExpectedStartTime.tm_min = json_object_get_int(jsontmEST_min);
		tmExpectedStartTime.tm_sec = json_object_get_int(jsontmEST_sec);

		tmExpectedEndTime.tm_year = json_object_get_int(jsontmENT_year);
		tmExpectedEndTime.tm_mon = json_object_get_int(jsontmENT_mon);
		tmExpectedEndTime.tm_mday = json_object_get_int(jsontmENT_day);
		tmExpectedEndTime.tm_hour = json_object_get_int(jsontmENT_hour);
		tmExpectedEndTime.tm_min = json_object_get_int(jsontmENT_min);
		tmExpectedEndTime.tm_sec = json_object_get_int(jsontmENT_sec);
		intRetval = MO_CreateMO(
			mysqlCon,
			&intMOSN,
			json_object_get_string(jsoncharMOID),
			json_object_get_int(jsonintIMMSN),
			json_object_get_int(jsonintProdSN),
			json_object_get_int(jsonintMoldSN),
			json_object_get_int(jsonintMaterialSN),
			tmExpectedStartTime,
			tmExpectedEndTime,
			json_object_get_int(jsonintPeicePerShot),
			json_object_get_int(jsonintExpectedProdVolume));
		if (intRetval == EXIT_SUCCESS)
		{
			for (i = 0; i < json_object_array_length(jsonSensorSNListData); i++)
			{
				jsonTempSensorSNListDataObject = json_object_array_get_idx(jsonSensorSNListData, i);

				/* Get results info */
				json_object_object_get_ex(jsonTempSensorSNListDataObject, "intSensorModelSN", &jsonintSensorModelSN);
				json_object_object_get_ex(jsonTempSensorSNListDataObject, "intMOSensorChannelSN", &jsonintMOSensorChannelSN);
				json_object_object_get_ex(jsonTempSensorSNListDataObject, "doubleMOSensorConvertRatio", &jsondoubleMOSensorConvertRatio);
				json_object_object_get_ex(jsonTempSensorSNListDataObject, "varMOSensorSN", &jsonvarMOSensorSN);
				fprintf(stderr,
						"intSensorModelSN:%d\n"
						"intMOSensorChannelSN:%d\n"
						"doubleMOSensorConvertRatio:%d\n"
						"varMOSensorSN:%s\n",
						json_object_get_int(jsonintSensorModelSN),
						json_object_get_int(jsonintMOSensorChannelSN),
						json_object_get_int(jsondoubleMOSensorConvertRatio),
						json_object_get_string(jsonvarMOSensorSN));

				/* Start Insert Sensor */
				strcpy(varMOSensorSN.charMOSensorMeta, json_object_get_string(jsonvarMOSensorSN));
				intRetval = MO_InsertMOSensorSNList(
					mysqlCon,
					intMOSN,
					json_object_get_int(jsonintSensorModelSN),
					json_object_get_int(jsonintMOSensorChannelSN),
					json_object_get_int(jsondoubleMOSensorConvertRatio),
					varMOSensorSN);
				if (intRetval == EXIT_SUCCESS)
					fprintf(stderr, "[\033[32mOK\033[m]\n");
				else
				{
					snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_JSON_SetMOList] Failed MO_InsertMOSensorSNList");
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
					json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
					json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[MO_JSON_SetMOList] Failed MO_InsertMOSensorSNList"));
					return EXIT_FAILURE;
				}
			}
			for (j = 0; j < json_object_array_length(jsonAcceptCriteriaSNListData); j++)
			{
				jsonTempAcceptCriteriaSNListDataObject = json_object_array_get_idx(jsonAcceptCriteriaSNListData, j);

				/* Get results info */
				json_object_object_get_ex(jsonTempAcceptCriteriaSNListDataObject, "intMOAcceptCriteriaSN", &jsonintMOAcceptCriteriaSN);
				json_object_object_get_ex(jsonTempAcceptCriteriaSNListDataObject, "intAcceptCriteriaClass", &jsonintAcceptCriteriaClass);
				json_object_object_get_ex(jsonTempAcceptCriteriaSNListDataObject, "charAcceptCriteriaUnit", &jsoncharAcceptCriteriaUnit);
				json_object_object_get_ex(jsonTempAcceptCriteriaSNListDataObject, "varAcceptCriteriaSN", &jsonvarAcceptCriteriaSN);
				fprintf(stderr,
						"intMOAcceptCriteriaSN:%d\n"
						"intAcceptCriteriaClass:%d\n"
						"charAcceptCriteriaUnit:%s\n"
						"varAcceptCriteriaSN:%s\n",
						json_object_get_int(jsonintMOAcceptCriteriaSN),
						json_object_get_int(jsonintAcceptCriteriaClass),
						json_object_get_string(jsoncharAcceptCriteriaUnit),
						json_object_get_string(jsonvarAcceptCriteriaSN));
				strcpy(varMOAcceptCriteriaSN.charAcceptCriteriaMeta, json_object_get_string(jsonvarAcceptCriteriaSN));
				strcpy(varMOAcceptCriteriaSN.charAcceptCriteriaMinTH, "");
				strcpy(varMOAcceptCriteriaSN.charAcceptCriteriaMaxTH, "");
				intRetval = MO_InsertMOAcceptCriteriaSNList(
					mysqlCon,
					intMOSN,
					json_object_get_int(jsonintMOAcceptCriteriaSN),
					json_object_get_int(jsonintAcceptCriteriaClass),
					json_object_get_string(jsoncharAcceptCriteriaUnit),
					varMOAcceptCriteriaSN);
				if (intRetval == EXIT_SUCCESS)
					fprintf(stderr, "[\033[32mOK\033[m]\n");
				else
				{
					snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_JSON_SetMOList] Failed MO_InsertMOAcceptCriteriaSNList");
					SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
					json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
					json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[MO_JSON_SetMOList] Failed MO_InsertMOAcceptCriteriaSNList"));
					return EXIT_FAILURE;
				}
			}
		}
		else
		{
			snprintf(charErrMsg, LONG_STRING_SIZE, "[MO_JSON_SetMOList] Failed MO_CreateMO");
			SYS_InsertSysErrMsg(mysqlCon, ERRCLASS_WEBSERVICE, 0, 0, mysql_errno(&mysqlCon), charErrMsg);
			json_object_object_add(jsonResponseHTTP, "Result", json_object_new_int(0));
			json_object_object_add(jsonResponseHTTP, "Msg", json_object_new_string("[MO_JSON_SetMOList] Failed MO_CreateMO"));
			return EXIT_FAILURE;
		}
	}

	// int MO_CreateMO(MYSQL mysqlCon, unsigned int *intMOSN, char charMOID[TINY_STRING_SIZE],
	// 				unsigned int intIMMSN, unsigned int intProdSN, unsigned int intMoldSN, unsigned int intMaterialSN,
	// 				struct tm tmExpectedStartTime, struct tm tmExpectedEndTime, unsigned int intExpectedProdVolume);
	// int MO_InsertMOSensorSNList(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intSensorModelSN, unsigned int intMOSensorChannelSN,
	// 							double doubleMOSensorConvertRatio, stMOSensorSN varMOSensorSN);
	// int MO_InsertMOAcceptCriteriaSNList(MYSQL mysqlCon, unsigned int intMOSN, unsigned int intMOAcceptCriteriaSN, unsigned int intAcceptCriteriaClass,
	// 									const char *charAcceptCriteriaUnit, stMOAcceptCriteriaSN varAcceptCriteriaSN);

	// void json_parse_input(json_object * jobj)
	// {
	// 	int exists, i, j, k, l;
	// 	char *results;
	// 	json_object *queriesObj, *resultsObj, *valuesObj, *tmpQueries, *tmpResults, *tmpValues, *tmpSeparateVals;

	// 	/* Get query key */
	// 	exists = json_object_object_get_ex(jobj, "queries", &queriesObj);
	// 	if (FALSE == exists)
	// 	{
	// 		printf("\"queries\" not found in JSON\n");
	// 		return;
	// 	}

	// 	/* Loop through array of queries */
	// 	for (i = 0; i < json_object_array_length(queriesObj); i++)
	// 	{
	// 		tmpQueries = json_object_array_get_idx(queriesObj, i);

	// 		/* Get results info */
	// 		exists = json_object_object_get_ex(tmpQueries, "results", &resultsObj);
	// 		if (FALSE == exists)
	// 		{
	// 			printf("\"results\" not found in JSON\n");
	// 			return;
	// 		}

	// 		/* Loop through array of results */
	// 		for (j = 0; j < json_object_array_length(resultsObj); j++)
	// 		{
	// 			tmpResults = json_object_array_get_idx(resultsObj, j);

	// 			/* Get values */
	// 			exists = json_object_object_get_ex(tmpResults, "values", &valuesObj);
	// 			if (FALSE == exists)
	// 			{
	// 				printf("\"values\" not found in JSON\n");
	// 				return;
	// 			}

	// 			/* Loop through array of array of values */
	// 			for (k = 0; k < json_object_array_length(valuesObj); k++)
	// 			{
	// 				tmpValues = json_object_array_get_idx(valuesObj, k);

	// 				/* Loop through array of values */
	// 				for (l = 0; l < json_object_array_length(tmpValues); l++)
	// 				{
	// 					tmpSeparateVals = json_object_array_get_idx(tmpValues, l);
	// 					printf("Values:[%d] = %s \n", l, json_object_to_json_string(tmpSeparateVals));
	// 				}
	// 			}
	// 		}
	// 	}
	// }

	// int DB_InsertMOAcceptCriteriaActualValue(
	// MYSQL mysqlCon,
	// unsigned int intMOSN,
	// unsigned int intShotSN,
	// unsigned int intMOAcceptCriteriaSN,
	// char charPredictValue[TINY_STRING_SIZE]);
}
