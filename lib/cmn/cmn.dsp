# Microsoft Developer Studio Project File - Name="cmn" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=cmn - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cmn.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cmn.mak" CFG="cmn - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cmn - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "cmn - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cmn - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\..\bin\release"
# PROP BASE Intermediate_Dir "..\..\obj\release\cmn"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\bin\release"
# PROP Intermediate_Dir "..\..\obj\release\cmn"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I "..\..\include" /I "..\cmn" /I "..\..\include\win32" /D "NDEBUG" /D "WIN32" /D HAVE_AUTOCONF_H=1 /D REGEX_MALLOC=1 /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "cmn - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\..\bin\debug"
# PROP BASE Intermediate_Dir "..\..\obj\debug\cmn"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bin\debug"
# PROP Intermediate_Dir "..\..\obj\debug\cmn"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "..\..\include" /I "..\cmn" /I "..\..\include\win32" /D "_DEBUG" /D "GETPID_IS_MEANINGLESS" /D "WIN32" /D HAVE_AUTOCONF_H=1 /D REGEX_MALLOC=1 /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "cmn - Win32 Release"
# Name "cmn - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\argcv.c
# End Source File
# Begin Source File

SOURCE=.\base64.c
# End Source File
# Begin Source File

SOURCE=.\crc.c
# End Source File
# Begin Source File

SOURCE=.\des.c
# End Source File
# Begin Source File

SOURCE=..\..\include\des.h
# End Source File
# Begin Source File

SOURCE=.\dirent.c
# End Source File
# Begin Source File

SOURCE=.\dns.c
# End Source File
# Begin Source File

SOURCE=.\getline.c
# End Source File
# Begin Source File

SOURCE=.\getopt.c
# End Source File
# Begin Source File

SOURCE=.\getopt1.c
# End Source File
# Begin Source File

SOURCE=.\hmac.c
# End Source File
# Begin Source File

SOURCE=.\isaac.c
# End Source File
# Begin Source File

SOURCE=.\log.c
# End Source File
# Begin Source File

SOURCE=.\md4.c
# End Source File
# Begin Source File

SOURCE=.\md5.c
# End Source File
# Begin Source File

SOURCE=.\ntserv_func.c
# End Source File
# Begin Source File

SOURCE=.\pwdgrp.c
# End Source File
# Begin Source File

SOURCE=.\rbtree.c
# End Source File
# Begin Source File

SOURCE=.\regex.c
# End Source File
# Begin Source File

SOURCE=.\sha1.c
# End Source File
# Begin Source File

SOURCE=.\time.c
# End Source File
# Begin Source File

SOURCE=.\url.c
# End Source File
# Begin Source File

SOURCE=.\uuid.c
# End Source File
# Begin Source File

SOURCE=.\vector.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\include\argcv.h
# End Source File
# Begin Source File

SOURCE=..\..\include\base64.h
# End Source File
# Begin Source File

SOURCE=.\crc.h
# End Source File
# Begin Source File

SOURCE=..\..\include\dirent.h
# End Source File
# Begin Source File

SOURCE=.\dns.h
# End Source File
# Begin Source File

SOURCE=..\..\include\getline.h
# End Source File
# Begin Source File

SOURCE=.\getopt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\win32\grp.h
# End Source File
# Begin Source File

SOURCE=..\..\include\hmac.h
# End Source File
# Begin Source File

SOURCE=.\isaac.h
# End Source File
# Begin Source File

SOURCE=..\..\include\list.h
# End Source File
# Begin Source File

SOURCE=..\..\include\log.h
# End Source File
# Begin Source File

SOURCE=..\..\include\md4.h
# End Source File
# Begin Source File

SOURCE=..\..\include\md5.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ntserv.h
# End Source File
# Begin Source File

SOURCE=..\..\include\proto.h
# End Source File
# Begin Source File

SOURCE=..\..\include\win32\pwd.h
# End Source File
# Begin Source File

SOURCE=..\..\include\rbtree.h
# End Source File
# Begin Source File

SOURCE=.\regex.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sha1.h
# End Source File
# Begin Source File

SOURCE=..\..\include\win32\shadow.h
# End Source File
# Begin Source File

SOURCE=.\url.h
# End Source File
# Begin Source File

SOURCE=..\..\include\uuid.h
# End Source File
# Begin Source File

SOURCE=..\..\include\vector.h
# End Source File
# End Group
# End Target
# End Project
