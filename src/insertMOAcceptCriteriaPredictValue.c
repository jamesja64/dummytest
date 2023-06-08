#include <stdio.h>
#include <mysql/mysql.h>
#include "config.h"
#include "itri_injpro.h"

int main(int argc, char *argv[])
{
	unsigned int	intMOSN;
	unsigned int	intShotSN;
	unsigned int	intMOAcceptCriteriaSN;
	char		charPredictValue[TINY_STRING_SIZE];	
	MYSQL		mysqlCon;
	unsigned int	intRetval;

        //insertMOAcceptCriteriaPredictValue -MOSN [MOSN] -ShotSN [ShotSN] -MOAcceptCriteriaSN [MOAcceptCriteriaSN] [PredictValue]
        if(argc==8){
                if(strcmp(argv[1],"-MOSN")==0){
                        intMOSN=atoi(argv[2]);
                }else{
			fprintf(stderr, "Format Error:insertMOAcceptCriteriaPredictValue -MOSN [MOSN] -ShotSN [ShotSN] -MOAcceptCriteriaSN [AcceptCriteriaSN] [PredictValue]\n");
                        return EXIT_FAILURE;
		}

                if(strcmp(argv[3],"-ShotSN")==0){
                        intShotSN=atoi(argv[4]);
                }else{
			fprintf(stderr, "Format Error:insertMOAcceptCriteriaPredictValue -MOSN [MOSN] -ShotSN [ShotSN] -MOAcceptCriteriaSN [AcceptCriteriaSN] [PredictValue]\n");
                        return EXIT_FAILURE;
		}

		if(strcmp(argv[5],"-MOAcceptCriteriaSN")==0){
                        intMOAcceptCriteriaSN=atoi(argv[6]);
                }else{
			fprintf(stderr, "Format Error:insertMOAcceptCriteriaPredictValue -MOSN [MOSN] -ShotSN [ShotSN] -MOAcceptCriteriaSN [AcceptCriteriaSN] [PredictValue]\n");
                        return EXIT_FAILURE;
		}
		strcpy(charPredictValue,argv[7]);
        }else{
		fprintf(stderr, "Format Error:insertMOAcceptCriteriaPredictValue -MOSN [MOSN] -ShotSN [ShotSN] -MOAcceptCriteriaSN [AcceptCriteriaSN] [PredictValue]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr,"MOSN:%d ShotSN:%d AcceptCriteriaSN:%d Value:%s\n",intMOSN,intShotSN,intMOAcceptCriteriaSN,charPredictValue);

	mysql_init(&mysqlCon);
	if(!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD,NULL,0,NULL,CLIENT_FOUND_ROWS))
	{
		if (mysql_errno(&mysqlCon)){
			fprintf(stderr, "Fail to connect to MySql server %d: %s\n",mysql_errno(&mysqlCon),mysql_error(&mysqlCon));
		}
		return EXIT_FAILURE;
	}
	intRetval=MO_InsertMOAcceptCriteriaPredictValue(mysqlCon,intMOSN,intShotSN,intMOAcceptCriteriaSN,charPredictValue);
	if(intRetval!=EXIT_SUCCESS){
			fprintf(stderr,"Fail to insert MOAcceptCriteriaPredictValue\n");
		
	}
	mysql_close(&mysqlCon);
	return intRetval;
}

