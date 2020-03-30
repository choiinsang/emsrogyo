#include "CDMARCFunctions.h"

//***********************************************
// Directory�� ���� ������ �߻��ߴ��� üũ
// pDirPath: ���� Ȯ�� ���丮
// ret     : ���� �� ���� �ð� �� 
// oldMTime: üũ�ϴ� ���丮�� ���� ���� �ð�
// errStr  : error �߻��� ��� error String ����
//***********************************************
int file_is_modified(const char *pDirPath, time_t &oldMTime, string &errStr)
{
	struct stat file_stat;
	int err = stat(pDirPath, &file_stat);
	if (err != 0) {
		//perror(" [Folder is Not Exist]");
		errStr = string("Folder is Not Exist");
		return -1;
	}
	
	//printf("[mode:0x%04X] [a:%d][m:%d][c:%d]\n", file_stat.st_mode, file_stat.st_atime, file_stat.st_mtime, file_stat.st_ctime);  
	int ret = file_stat.st_mtime - oldMTime;
	if (ret > 0){
		oldMTime = file_stat.st_mtime;
	}
	
	return ret;
}

//Check Folder Modification
//int main(int argc, char *argv[])
//{
//	if(argc < 1){
//		printf("please path : %s (folder path)\n", argv[0]);
//		return 0;
//	}
//	
//	int iCount = 0;
//	time_t org_time;
//	time(&org_time);
//	
//	int iRet= 0;
//	while(iCount < 10){
//		if((iRet = file_is_modified(argv[1], org_time)) > 0){
//			printf("time is modified\n");
//		}
//		else if(iRet == 0){
//			//printf("time is not modified\n");
//			sleep(1);
//		}
//		else{
//			sleep(2);
//		}
//
//		sleep (1);
//	}
//	return 0;
//}
//DMARC Usable Functions
//

bool  checkDir (const char *pDirPath, bool bCreate)
{
	DIR *pDir;
	if ((pDir = opendir(pDirPath)) == NULL)
	{
		if(bCreate == true){
			if (mkdir(pDirPath, 00777) < 0){
				return false;
			}
			return true;
		}
		return false;
	}
	closedir(pDir);
	return true;
}

//***********************************************
// vecFiles�� pDirPath�� ���� ����� ����
// pDirPath: ���� ��� Ȯ�� ���丮
// vecFiles: ���丮�� ���� ����� ������ ���� ����
// errStr  : error �߻��� ��� error String ����
//***********************************************
bool getFilesFromDir(const char *pDirPath, vector<std::string> &vecFiles, std::string &errStr)
{
	if(pDirPath == NULL){
		return false;
	}
	else{
		try{
			DIR * pfdir = NULL;
			if((pfdir=opendir(pDirPath))==NULL){
				//-- pDirPath is Not a Directory
				errStr = strerror(errno);
				return false;
			}
			
			struct dirent *pDirEnt = NULL;
			while((pDirEnt = readdir(pfdir)) != NULL){
				//printf("File [%d][%d][%d][%s]\n", (int)pDirEnt->d_ino, (int)pDirEnt->d_off, (int)pDirEnt->d_reclen, pDirEnt->d_name);
				string tmpPath  = pDirPath;
				       tmpPath += "/";
				       tmpPath += pDirEnt->d_name;
				DIR   *pftmpdir = NULL;
				
				if( ( pftmpdir = opendir(tmpPath.c_str()) )==NULL ){ //File
					if(errno == ENOTDIR){
						//printf("[%d][%s][%s]\n", errno, tmpPath.c_str(), strerror(errno));
						//-- File insert into vecFiles
						vecFiles.push_back(pDirEnt->d_name);
						continue;
					}
				}
				else{//Directory
					if((strcmp(".", pDirEnt->d_name) == 0) || (strcmp("..", pDirEnt->d_name) == 0)){
						//-- pDirEnt->d_name is parent or current directory name
						//printf("[%s] PASS\n", tmpPath.c_str());
					}
					else{
						//-- pDirEnt->d_name is directory name
						//printf("[%s] is Directory\n", tmpPath.c_str());
					}
					closedir(pftmpdir);
				}
			}

			closedir(pfdir);
		}
		catch(...){
			errStr = strerror(errno);
			return false;
		}

		return true;
	}
}


//***********************************************
// ������ Ȯ���� ���� üũ�Ͽ� xml/zip ���� ����
// xml:0,zip:1
// pFileName: Ȯ����� Ȯ�� �� ���� �̸�
// errStr   : error �߻��� ��� error String ����
// ��ȯ��   : xml=0, zip=1, ~(xml or zip)=-1, (no extention)=-2
//***********************************************
int  checkFileExtention(const char *pFileName, std::string &errStr)
{
	int   retval = -1;
	char *chPos  = NULL;
	char *pStr   = NULL;
	
	try{
		pStr = strdup(pFileName);
		to_lowercase(pStr);
		
		if(pStr == NULL){
			errStr="memory alloc failed";
			return MEM_ERROR;
		}
	 
	  if((chPos = strrchr(pStr, '.' )) != NULL){
	  	
	  	if( strcmp(chPos, ".xml") == 0 ){
	  		retval = EXT_XML;
	  		errStr = "xml file name";
	  	}
	  	else if( strcmp(chPos, ".zip") == 0 ){
	  		retval = EXT_ZIP;
	  		errStr="zip file name";
	  	}
	  	else if(strcmp(chPos, ".gz") == 0){
	  		retval = EXT_GZ;
	  		errStr="gz file name";
	  	}
	  	else{
	  		retval = EXT_OUT_OF_LIST;
	  		errStr="unknown extention file name";
	  	}
	  }
	  else
	  	retval = EXT_NOT_EXIST;

	  free(pStr);
	  pStr = NULL;
	}
	catch(...){
		try{
			if(pStr != NULL){
				free(pStr);
			}
		}
		catch(...){
		}
		pStr   = NULL;
		retval = UNKNOWN_ERROR;
		errStr = "unknown error occurred";
	}
  
  return retval;
}

//***********************************************
// ���ڿ��� low cast ó��
// ��ȯ�� : low cast ó���� ���ڿ�
//***********************************************
void to_lowercase(char *str)
{
	if(str == NULL){
		return;
	}
	else{
		int i;
		for(i=strlen(str)-1; i>=0; i--){
			str[i]=tolower(str[i]);
		}
	}
}
