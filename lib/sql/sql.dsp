# Microsoft Developer Studio Project File - Name="sql" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=sql - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sql.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sql.mak" CFG="sql - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sql - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "sql - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sql - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\..\bin\release"
# PROP BASE Intermediate_Dir "..\..\obj\release\sql"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\bin\release"
# PROP Intermediate_Dir "..\..\obj\release\sql"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I "..\..\include" /I "..\sql" /I "..\..\include\win32" /I "..\mysql\include" /I "..\pgsql\include" /I "..\tds\win32" /D "NDEBUG" /D "WIN32" /D HAVE_AUTOCONF_H=1 /D REGEX_MALLOC=1 /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "sql - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\..\bin\debug"
# PROP BASE Intermediate_Dir "..\..\obj\debug\sql"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bin\debug"
# PROP Intermediate_Dir "..\..\obj\debug\sql"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\cmn" /I "." /I "..\..\include" /I "..\sql" /I "..\..\include\win32" /I "..\mysql\include" /I "..\pgsql\include" /I "..\tds\win32" /D "_DEBUG" /D "GETPID_IS_MEANINGLESS" /D "WIN32" /D HAVE_AUTOCONF_H=1 /D REGEX_MALLOC=1 /FD /GZ /c
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

# Name "sql - Win32 Release"
# Name "sql - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\alloca.c
# End Source File
# Begin Source File

SOURCE=.\config.c
# End Source File
# Begin Source File

SOURCE=.\driver.c
# End Source File
# Begin Source File

SOURCE=.\log.c
# End Source File
# Begin Source File

SOURCE=.\nodes.c
# End Source File
# Begin Source File

SOURCE=.\parse.y

!IF  "$(CFG)" == "sql - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Compiling $(InputPath)...
InputPath=.\parse.y
InputName=parse

BuildCmds= \
	BISON -d -psql_ -v $(InputName).y -vd -o ..\..\obj\$(InputName).yy.c \
	COPY ..\..\obj\$(InputName).yy.h ..\..\obj\sql_parse.h \
	

"..\..\obj\$(InputName).yy.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\..\obj\sql_parse.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "sql - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Compiling $(InputPath)...
InputPath=.\parse.y
InputName=parse

BuildCmds= \
	BISON -d -psql_ -v $(InputName).y -vd -o ..\..\obj\$(InputName).yy.c \
	COPY ..\..\obj\$(InputName).yy.h ..\..\obj\sql_parse.h \
	

"..\..\obj\$(InputName).yy.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\..\obj\sql_parse.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\obj\parse.yy.c
# End Source File
# Begin Source File

SOURCE=.\pgsql.c
# End Source File
# Begin Source File

SOURCE=.\scan.l

!IF  "$(CFG)" == "sql - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Compiling $(InputPath)...
InputPath=.\scan.l
InputName=scan

"..\..\obj\$(InputName).ll.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	FLEX -Psql_ -o..\..\obj\$(InputName).ll.c $(InputName).l

# End Custom Build

!ELSEIF  "$(CFG)" == "sql - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Compiling $(InputPath)...
InputPath=.\scan.l
InputName=scan

"..\..\obj\$(InputName).ll.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	FLEX -Psql_ -o..\..\obj\$(InputName).ll.c $(InputName).l

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\obj\scan.ll.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\obj\parse.yy.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sql.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sql_conf.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sql_driver.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sql_node.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sql_parser.h
# End Source File
# Begin Source File

SOURCE=.\sql_priv.h
# End Source File
# End Group
# End Target
# End Project
