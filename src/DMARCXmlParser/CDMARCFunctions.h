#ifndef __DMARC_FUNCTIONS_HEADER__
#define __DMARC_FUNCTIONS_HEADER__

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <vector>
#include <dirent.h>


#define UNKNOWN_ERROR    -5
#define FILE_OPEN_ERROR  -4
#define MEM_ERROR        -3
#define EXT_NOT_EXIST    -2
#define EXT_OUT_OF_LIST  -1
#define EXT_XML           0
#define EXT_ZIP           1
#define EXT_GZ            2

using namespace std;
 
int   file_is_modified   (const char *path, time_t &oldMTime, string &errStr);
bool  checkDir           (const char *pDirPath, bool bCreate);
bool  getFilesFromDir    (const char *pDirPath, vector<std::string> &vecFiles, std::string &errStr);
int   checkFileExtention (const char *pFileName, std::string &errStr);
void  moveFile           (const char *pOldFilePath, const char *pNewFilePath);
void  to_lowercase       (char *str);

#endif //__DMARC_FUNCTIONS_HEADER__
