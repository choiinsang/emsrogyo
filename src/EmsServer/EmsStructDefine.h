#ifndef __CEMS_STRUCT_DEFINE_HEADER__
#define __CEMS_STRUCT_DEFINE_HEADER__

#include <iostream>
#include <map>
#include <string.h>
#include <time.h>
#include "EmsDefineString.h"
#include "LockObject.h"


#include <boost/shared_ptr.hpp>
using namespace boost;
using namespace std;

template< typename T >
struct array_deleter
{
  void operator ()( T const * p)
  {
  	delete[] p; 
  }
};


typedef struct _stCampaignInfo{
	_stCampaignInfo(const char* campaign_no, int group_no, const char* dbname, bool delmailbody){
		_strCampaign_no = campaign_no;
		_iGroup_no      = group_no;
		_iCpStep        = CPSTEP0_DbReadRunning;
		_strDBName      = dbname;
		_bDelMailBody  = delmailbody;
	}
	
	const char *getCampaignNo()         { return _strCampaign_no.c_str(); }
	int         getGrouNo    ()         { return _iGroup_no; }
	void        setStep      (int step) { _iCpStep = step; }
	int         getStep      ()         { return _iCpStep; }
	const char *getDBName    ()         { return _strDBName.c_str(); }
	bool        getDelMailBody()        { return _bDelMailBody; }
	
	private:
		string  _strCampaign_no;
		int     _iGroup_no;
		int     _iCpStep;
		string  _strDBName;
		bool    _bDelMailBody;
}stCampaignInfo;


typedef struct _stServerInfo{
	_stServerInfo(int server_no, const char* server_name)
	: _iServer_no    (server_no)
	, _strServer_name(server_name)
	{
		time(&_tHeartBeatTime);
	}
	
	int     getServerNo  () { return _iServer_no; }
	string  getServerName() { return _strServer_name; }
	void    updateHB     () { time(&_tHeartBeatTime); }
	long    diffHBTime   (time_t cTime) { return difftime(cTime, _tHeartBeatTime); }

	public:
		int              _iServer_no;
		string           _strServer_name;
		time_t           _tHeartBeatTime;
}stServerInfo;



typedef struct _stGroupInfo{
	_stGroupInfo(int group_no, const char* group_name, const char *commonQ_name){
		_iGroup_no       = group_no;
		_strGroup_name   = group_name;
		_strCommonQ_name = commonQ_name;
		time(&_tGroupInfoTime);
	}
	
	int                               getGroupNo         () { return _iGroup_no; }
	const char                       *getGroupName       () { return _strGroup_name.c_str();}
	const char                       *getGroupCommonQName() { return _strCommonQ_name.c_str(); }
	unordered_map<int, stServerInfo> *getServerList      () { return &_mapServerNo; }
	time_t                            getTime            () { return _tGroupInfoTime; }
	void                              setTime            (time_t ctime) { _tGroupInfoTime = ctime; }
				
	private:
		int              _iGroup_no;
		string           _strGroup_name;
		string           _strCommonQ_name;
		unordered_map<int, stServerInfo> _mapServerNo;
		time_t           _tGroupInfoTime;
}stGroupInfo;


typedef struct _stSplitInfo
{
	char * _pchSplitPos;
	int    _iSplitLen;
	int    _iSplitInfoType;
}stSplitInfo;


#endif  //__CEMS_STRUCT_DEFINE_HEADER__

