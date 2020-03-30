#!/bin/sh

EXFILE=testdkim;


case "$1" in
	"clear")
		rm -if $EXFILE
		;;
	"ea")
		make -f Makefile.ea
		;;
	"es")
		make -f Makefile.es
		;;
	"eaclean")
		make -f Makefile.ea clean
		;;
	"esclean")
		make -f Makefile.es clean
		;;
	"testconf")
		clear
		echo "*** g++ EmsConfig! ***"
		g++ -o testconf  testconf.cpp  EmsDefineString.h ../Common/INIFile.h ../Common/INIFile.cpp EmsConfig.h EmsConfig.cpp ../NetTemplate/LockObject.h  ../NetTemplate/LockObject.cpp  EmsDKIM.cpp EmsDKIM.h EmsDefine.h EmsCommon.h -g  -I/usr/include/opendkim -I../NetTemplate/ -lopendkim  -D_Bool=bool -D__LINUX__
		;;
	*)
		clear
		echo "*** g++ EmsDKIM! ***"
		g++ -o testdkim  testmain.cpp  EmsDefineString.h ../NetTemplate/LockObject.h  ../NetTemplate/LockObject.cpp  EmsDKIM.cpp EmsDKIM.h EmsDefine.h EmsCommon.h -g  -I/usr/include/opendkim -I../NetTemplate/ -lopendkim  -D_Bool=bool -D__LINUX__
		;;
esac


