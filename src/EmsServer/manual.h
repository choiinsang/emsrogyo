#ifndef __REENQUEUE_PROCESS_MANUAL_HEADER__
#define __REENQUEUE_PROCESS_MANUAL_HEADER__

#define SZ_STRING 127

const char manual_help[][SZ_STRING+1] = {
	"manual_help"
};

enum enumMENU{
	iEXIT = 0,
	iENQUEUE_FROM_FILES,
	iGET_FILES_FROM_QUEUE
};

const char manual_menu[][SZ_STRING+1] = {
	"exit",
	"Enqueue Message From Files",
	"Get Messages from Queue And Make Files"
};

const char manual_cmd[][SZ_STRING+1] = {
	"",
	"manual_cmd"
};

const char manual_base[][SZ_STRING+1] = {
	"manual_base"
};

#endif //__REENQUEUE_PROCESS_MANUAL_HEADER__
