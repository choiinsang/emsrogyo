/* *************************************************
작성자 : 송태형
작성일 : ?
모듈 설명 : MISC 
************************************************* */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include "sth_misc.h"

/* *************************************************
Function: MoveFile
Parameters: szSource
			szDest
			isAppend
Return: void
Comment: 파일을 지정 위치로 이동한다.
************************************************* */
int
CopyFile( const char *szSource, const char *szDest )
{
	char block[1024];
	char *szFilename;
	char szDestFile[1024];
	int in=0, out=0;
	int nread;
	struct stat testst;

	// Destnation이 디렉토리일 경우
	if(szDest[strlen(szDest) - 1] == '/')
	{
		if( stat(szDest, &testst) != -1 )
		{
			if(S_ISDIR(testst.st_mode))
			{
				szFilename = strrchr(szSource, '/');
				if(szFilename == NULL) szFilename = (char *)szSource;
				
				sprintf(szDestFile, "%s%s", szDest, szFilename + 1);
			}
			else
				return FALSE;
		}
		else
			return FALSE;
	}
	else
	{
		sprintf(szDestFile, "%s", szDest);
	}

	in = open( szSource, O_RDONLY );
	out = open( szDestFile, O_WRONLY | O_CREAT , 0666 );

	if( in == -1 || out == -1 )
	{
		if( in != -1 )
			close(in);

		if( out != -1 )
		{
			close(out);
			remove(szDestFile);
		}

		return FALSE;
	}

	while( (nread = read(in, block, sizeof(block))) > 0 )
		write(out, block, nread);

	close(in);
	close(out);

	return TRUE;
}

/* *************************************************
Function: MoveFile
Parameters: szSource
			szDest
			isAppend
Return: void
Comment: 파일을 지정 위치로 이동한다.
************************************************* */
int
MoveFile( const char *szSource, const char *szDest, int isAppend )
{
	char block[1024];
	char *szFilename;
	char szDestFile[1024];
	int in=0, out=0;
	int nread;
	struct stat testst;

	if(isAppend != O_APPEND) isAppend = 0;
	
	// Destnation이 디렉토리일 경우
	if(szDest[strlen(szDest) - 1] == '/')
	{
		if( stat(szDest, &testst) != -1 )
		{
			if(S_ISDIR(testst.st_mode))
			{
				szFilename = strrchr(szSource, '/');
				if(szFilename == NULL) szFilename = (char *)szSource;
				
				sprintf(szDestFile, "%s%s", szDest, szFilename + 1);
			}
			else
				return FALSE;
		}
		else
			return FALSE;
	}
	else
	{
		sprintf(szDestFile, "%s", szDest);
	}

	in = open( szSource, O_RDONLY );
	out = open( szDestFile, O_WRONLY | O_CREAT | isAppend, 0666 );

	if( in == -1 || out == -1 )
	{
		if( in != -1 )
			close(in);

		if( out != -1 )
		{
			close(out);
			remove(szDestFile);
		}

		return FALSE;
	}

	while( (nread = read(in, block, sizeof(block))) > 0 )
		write(out, block, nread);

	close(in);
	close(out);

	remove(szSource);

	return TRUE;
}

/* *************************************************
Function: MoveDirFile
Parameters: szSPath	- 원본
			szDPath	- 사본
			isRemove - 복사한 후 원본 삭제
Return: int	- TRUE, FALSE
Comment: 디렉토리를 이동한다.
************************************************* */
int
MoveDirFile(const char *szSPath, const char *szDPath, int isRemove)
{
	DIR *dp;
	struct dirent *entry;
	char szSFileName[1024];
	char szDFileName[1024];
	char block[1024];
	int in=0, out=0;
	int nread;

	if( ( dp = opendir(szSPath) ) == NULL )
		return 0;

	while( ( entry = readdir(dp)) != NULL )
	{
		if(!strncmp(entry->d_name, ".", 1)) continue;

		sprintf(szSFileName, "%s/%s", szSPath, entry->d_name);
		sprintf(szDFileName, "%s/%s", szDPath, entry->d_name);

		in = open( szSFileName, O_RDONLY );
		out = open( szDFileName, O_WRONLY|O_CREAT, 0666 );

		if( in == -1 || out == -1 )
		{
			if( in != -1 )
				close(in);

			if( out != -1 )
			{
				close(out);
				remove(szDFileName);
			}

			return 0;
		}

		while( (nread = read(in, block,sizeof(block))) > 0 )
			write(out,block,nread);

		close(in);
		close(out);

		remove(szSFileName);
	}

	if(isRemove) rmdir(szSPath);

	return 1;
}

/* *************************************************
Function: RemoveDirFile
Parameters: szSPath	- 삭제대상 디렉토리
Return: int	- TRUE, FALSE
Comment: 디렉토리를 삭제한다.
************************************************* */
int
RemoveDirFile(const char *szPath)
{
	DIR *dp;
	struct dirent *entry;
	char szFileName[256];
	char block[1024];
	int in=0, out=0;
	int nread;

	if( ( dp = opendir(szPath) ) == NULL )
		return 0;

	while( ( entry = readdir(dp)) != NULL )
	{
		if(!strncmp(entry->d_name, ".", 1)) continue;

		sprintf(szFileName, "%s/%s", szPath, entry->d_name);

		remove(szFileName);
	}

	rmdir(szPath);

	return 1;
}

/* *************************************************
Function: CreateDir
Parameters: path	- 디렉토리 명
Return: int	- TRUE, FALSE
Comment: 디렉토리를 생성한다. ( 부모 디렉토리가 없으면 같이 생성한다.
************************************************* */
int
CreateDir( const char *path )
{
	int ret;
	char *ptr = path;
	struct stat testst;

	if(mkdir(path, 0755) == 0) return 0;

	while(1)
	{
		ptr = strchr((const char*)ptr+1, '/');
		if(ptr != NULL) *ptr = 0;

		if(stat(path, &testst) == -1)
			if(mkdir(path, 0755) == -1) 
				return -1;

		if(ptr != NULL) *ptr = '/';
		else break;
	}

	return 0;
}

