#include <stdio.h>
#include <mysql/mysql.h>
#include "config.h"
#include "itri_injpro.h"

int main(int argc, char *argv[])
{
	unsigned int	intIMMSN;
	unsigned int	intMOSN;
	unsigned int	intRecomIMMParaSN;
	unsigned int	intMOAcceptCriteriaSN;
	char		charPredictValue[TINY_STRING_SIZE];	
	MYSQL		mysqlCon;
	unsigned int	intRetval;

        //exeTuneRecomIMMPara -IMMSN [$IMMSN] -MOSN [$MOSN] -RecomIMMParaSN [$RecomIMMParaSN]
        if(argc==7){
                if(strcmp(argv[1],"-IMMSN")==0){
                        intIMMSN=atoi(argv[2]);
                }else{
			fprintf(stderr, "1st Arguement Format Error:exeTuneRecomIMMPara -IMMSN [$IMMSN] -MOSN [$MOSN] -RecomIMMParaSN [$RecomIMMParaSN]\n");
                        return EXIT_FAILURE;
		}

                if(strcmp(argv[3],"-MOSN")==0){
                        intMOSN=atoi(argv[4]);
                }else{
			fprintf(stderr, "2nd Arguement Format Error:exeTuneRecomIMMPara -IMMSN [$IMMSN] -MOSN [$MOSN] -RecomIMMParaSN [$RecomIMMParaSN]\n");
                        return EXIT_FAILURE;
		}

		if(strcmp(argv[5],"-RecomIMMParaSN")==0){
			intRecomIMMParaSN=atoi(argv[6]);
                }else{
			fprintf(stderr, "3rd Arguement Format Error:exeTuneRecomIMMPara -IMMSN [$IMMSN] -MOSN [$MOSN] -RecomIMMParaSN [$RecomIMMParaSN]\n");
                        return EXIT_FAILURE;
		}
        }else{
		fprintf(stderr, "Format Error:insertMOAcceptCriteriaPredictValue -MOSN [MOSN] -ShotSN [ShotSN] -MOAcceptCriteriaSN [AcceptCriteriaSN] [PredictValue]\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr,"IMMSN:%d MOSN:%d RecomIMMParaSN:%d\n",intIMMSN,intMOSN,intRecomIMMParaSN);

	mysql_init(&mysqlCon);
	if(!mysql_real_connect(&mysqlCon, MYSQL_SERVER_IP, MYSQL_USER_NAME, MYSQL_PASSWORD,NULL,0,NULL,CLIENT_FOUND_ROWS))
	{
		if (mysql_errno(&mysqlCon)){
			fprintf(stderr, "Fail to connect to MySql server %d: %s\n",mysql_errno(&mysqlCon),mysql_error(&mysqlCon));
		}
		return EXIT_FAILURE;
	}
	intRetval=IMM_TuneIMMParaByRecommend(mysqlCon,intIMMSN,intMOSN,intRecomIMMParaSN);
	if(intRetval!=EXIT_SUCCESS){
			fprintf(stderr,"Fail to tune IMMSN:%d given MOSN:%d RecomIMMParaSN:%d\n",intIMMSN,intMOSN,intRecomIMMParaSN);
		
	}
	mysql_close(&mysqlCon);
	return intRetval;

}

