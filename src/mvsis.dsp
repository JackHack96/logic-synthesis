# Microsoft Developer Studio Project File - Name="mvsis" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=mvsis - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mvsis.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mvsis.mak" CFG="mvsis - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mvsis - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "mvsis - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mvsis - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "map\mapper" /I "map\mio" /I "map\super" /I "bdd\parse" /I "opt\fmb" /I "opt\fmm" /I "graph\ftb" /I "base\cmd" /I "base\io" /I "base\main" /I "base\ntk" /I "base\ntki" /I "bdd\cudd" /I "bdd\epd" /I "bdd\extra" /I "bdd\reo" /I "bdd\mtr" /I "bdd\dsd" /I "sis\espresso" /I "sis\mincov" /I "sis\minimize" /I "glu\array" /I "glu\avl" /I "glu\graph" /I "glu\list" /I "glu\sparse" /I "glu\st" /I "glu\util" /I "glu\var_set" /I "mv\cvr" /I "mv\fnc" /I "mv\mvr" /I "mv\vm" /I "mv\vmx" /I "mv\mvc" /I "mv\mva" /I "opt\fxu" /I "opt\simp" /I "opt\pd" /I "opt\fm" /I "opt\fmn" /I "opt\enc" /I "graph\cb" /I "graph\ft" /I "graph\sh" /I "graph\wn" /I "seq\aut" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "__STDC__" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /profile /machine:I386 /out:"_TEST/mvsis.exe"

!ELSEIF  "$(CFG)" == "mvsis - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "map\mapper" /I "map\mio" /I "map\super" /I "bdd\parse" /I "opt\fmb" /I "opt\fmm" /I "graph\ftb" /I "base\cmd" /I "base\io" /I "base\main" /I "base\ntk" /I "base\ntki" /I "bdd\cudd" /I "bdd\epd" /I "bdd\extra" /I "bdd\reo" /I "bdd\mtr" /I "bdd\dsd" /I "sis\espresso" /I "sis\mincov" /I "sis\minimize" /I "glu\array" /I "glu\avl" /I "glu\graph" /I "glu\list" /I "glu\sparse" /I "glu\st" /I "glu\util" /I "glu\var_set" /I "mv\cvr" /I "mv\fnc" /I "mv\mvr" /I "mv\vm" /I "mv\vmx" /I "mv\mvc" /I "mv\mva" /I "opt\fxu" /I "opt\simp" /I "opt\pd" /I "opt\fm" /I "opt\fmn" /I "opt\enc" /I "graph\cb" /I "graph\ft" /I "graph\sh" /I "graph\wn" /I "seq\aut" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "__STDC__" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"_TEST/mvsis.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "mvsis - Win32 Release"
# Name "mvsis - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "base"

# PROP Default_Filter ""
# Begin Group "main"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\base\main\mv.c
# End Source File
# Begin Source File

SOURCE=.\base\main\mv.h
# End Source File
# Begin Source File

SOURCE=.\base\main\mvFrame.c
# End Source File
# Begin Source File

SOURCE=.\base\main\mvInit.c
# End Source File
# Begin Source File

SOURCE=.\base\main\mvInt.h
# End Source File
# Begin Source File

SOURCE=.\base\main\mvtypes.h
# End Source File
# Begin Source File

SOURCE=.\base\main\mvUtils.c
# End Source File
# End Group
# Begin Group "cmd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\base\cmd\cmd.c
# End Source File
# Begin Source File

SOURCE=.\base\cmd\cmd.h
# End Source File
# Begin Source File

SOURCE=.\base\cmd\cmdAlias.c
# End Source File
# Begin Source File

SOURCE=.\base\cmd\cmdApi.c
# End Source File
# Begin Source File

SOURCE=.\base\cmd\cmdFlag.c
# End Source File
# Begin Source File

SOURCE=.\base\cmd\cmdHist.c
# End Source File
# Begin Source File

SOURCE=.\base\cmd\cmdInt.h
# End Source File
# Begin Source File

SOURCE=.\base\cmd\cmdUtils.c
# End Source File
# End Group
# Begin Group "io"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\base\io\io.c
# End Source File
# Begin Source File

SOURCE=.\base\io\io.h
# End Source File
# Begin Source File

SOURCE=.\base\io\ioInt.h
# End Source File
# Begin Source File

SOURCE=.\base\io\ioRead.c
# End Source File
# Begin Source File

SOURCE=.\base\io\ioReadFile.c
# End Source File
# Begin Source File

SOURCE=.\base\io\ioReadKiss.c
# End Source File
# Begin Source File

SOURCE=.\base\io\ioReadLines.c
# End Source File
# Begin Source File

SOURCE=.\base\io\ioReadNet.c
# End Source File
# Begin Source File

SOURCE=.\base\io\ioReadNode.c
# End Source File
# Begin Source File

SOURCE=.\base\io\ioReadPla.c
# End Source File
# Begin Source File

SOURCE=.\base\io\ioReadPlaMv.c
# End Source File
# Begin Source File

SOURCE=.\base\io\ioWrite.c
# End Source File
# Begin Source File

SOURCE=.\base\io\ioWritePla.c
# End Source File
# Begin Source File

SOURCE=.\base\io\ioWritePlaMv.c
# End Source File
# Begin Source File

SOURCE=.\base\io\ioWriteSplit.c
# End Source File
# End Group
# Begin Group "ntk"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\base\ntk\ntk.c
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntk.h
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntkApi.c
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntkCheck.c
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntkDfs.c
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntkFanio.c
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntkGlobal.c
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntkInt.h
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntkLatch.c
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntkList.c
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntkMdd.c
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntkMinBase.c
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntkNames.c
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntkNode.c
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntkNodeCol.c
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntkNodeHeap.c
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntkSort.c
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntkSubnet.c
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntkTravId.c
# End Source File
# Begin Source File

SOURCE=.\base\ntk\ntkUtils.c
# End Source File
# End Group
# Begin Group "ntki"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\base\ntki\ntki.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntki.h
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiBalance.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiCollapse.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiCollapseNew.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiDecomp.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiDefault.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiDsd.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiEliminate.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiEncode.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiFxu.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiGlobal.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiInt.h
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiMerge.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiOrder.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiPower.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiPrint.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiResub.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiSat.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiStrash.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiSweep.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiUnreach.c
# End Source File
# Begin Source File

SOURCE=.\base\ntki\ntkiVerify.c
# End Source File
# End Group
# End Group
# Begin Group "mv"

# PROP Default_Filter ""
# Begin Group "fnc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\mv\fnc\fnc.c
# End Source File
# Begin Source File

SOURCE=.\mv\fnc\fnc.h
# End Source File
# Begin Source File

SOURCE=.\mv\fnc\fncApi.c
# End Source File
# Begin Source File

SOURCE=.\mv\fnc\fncCvr.c
# End Source File
# Begin Source File

SOURCE=.\mv\fnc\fncGlobal.c
# End Source File
# Begin Source File

SOURCE=.\mv\fnc\fncInt.h
# End Source File
# Begin Source File

SOURCE=.\mv\fnc\fncMan.c
# End Source File
# Begin Source File

SOURCE=.\mv\fnc\fncMvr.c
# End Source File
# Begin Source File

SOURCE=.\mv\fnc\fncNode.c
# End Source File
# Begin Source File

SOURCE=.\mv\fnc\fncSopMin.c
# End Source File
# End Group
# Begin Group "mva"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\mv\mva\mva.h
# End Source File
# Begin Source File

SOURCE=.\mv\mva\mvaFunc.c
# End Source File
# Begin Source File

SOURCE=.\mv\mva\mvaFuncSet.c
# End Source File
# End Group
# Begin Group "mvc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\mv\mvc\mvc.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvc.h
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcApi.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcCompare.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcCompl.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcContain.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcCover.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcCube.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcData.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcDd.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcDivide.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcDivisor.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcList.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcLits.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcMan.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcMerge.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcOpAlg.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcOpBool.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcPrint.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcReshape.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcSharp.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcSort.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcTau.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvc\mvcUtils.c
# End Source File
# End Group
# Begin Group "cvr"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\mv\cvr\cvr.h
# End Source File
# Begin Source File

SOURCE=.\mv\cvr\cvrApi.c
# End Source File
# Begin Source File

SOURCE=.\mv\cvr\cvrEspresso.c
# End Source File
# Begin Source File

SOURCE=.\mv\cvr\cvrFactor.c
# End Source File
# Begin Source File

SOURCE=.\mv\cvr\cvrFunc.c
# End Source File
# Begin Source File

SOURCE=.\mv\cvr\cvrInt.h
# End Source File
# Begin Source File

SOURCE=.\mv\cvr\cvrRelation.c
# End Source File
# Begin Source File

SOURCE=.\mv\cvr\cvrUtils.c
# End Source File
# End Group
# Begin Group "mvr"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\mv\mvr\mvr.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvr\mvr.h
# End Source File
# Begin Source File

SOURCE=.\mv\mvr\mvrApi.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvr\mvrCofs.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvr\mvrInt.h
# End Source File
# Begin Source File

SOURCE=.\mv\mvr\mvrMan.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvr\mvrPrint.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvr\mvrRel.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvr\mvrUcp.c
# End Source File
# Begin Source File

SOURCE=.\mv\mvr\mvrUtils.c
# End Source File
# End Group
# Begin Group "vm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\mv\vm\vm.h
# End Source File
# Begin Source File

SOURCE=.\mv\vm\vmApi.c
# End Source File
# Begin Source File

SOURCE=.\mv\vm\vmInt.h
# End Source File
# Begin Source File

SOURCE=.\mv\vm\vmMan.c
# End Source File
# Begin Source File

SOURCE=.\mv\vm\vmMap.c
# End Source File
# Begin Source File

SOURCE=.\mv\vm\vmUtils.c
# End Source File
# End Group
# Begin Group "vmx"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\mv\vmx\vmx.h
# End Source File
# Begin Source File

SOURCE=.\mv\vmx\vmxApi.c
# End Source File
# Begin Source File

SOURCE=.\mv\vmx\vmxCode.c
# End Source File
# Begin Source File

SOURCE=.\mv\vmx\vmxCreate.c
# End Source File
# Begin Source File

SOURCE=.\mv\vmx\vmxCube.c
# End Source File
# Begin Source File

SOURCE=.\mv\vmx\vmxInt.h
# End Source File
# Begin Source File

SOURCE=.\mv\vmx\vmxMan.c
# End Source File
# Begin Source File

SOURCE=.\mv\vmx\vmxMap.c
# End Source File
# Begin Source File

SOURCE=.\mv\vmx\vmxRemap.c
# End Source File
# Begin Source File

SOURCE=.\mv\vmx\vmxUtils.c
# End Source File
# End Group
# End Group
# Begin Group "graph"

# PROP Default_Filter ""
# Begin Group "cb"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\graph\cb\cb.h
# End Source File
# Begin Source File

SOURCE=.\graph\cb\cbApi.c
# End Source File
# Begin Source File

SOURCE=.\graph\cb\cbClub.c
# End Source File
# Begin Source File

SOURCE=.\graph\cb\cbCmd.c
# End Source File
# Begin Source File

SOURCE=.\graph\cb\cbDfs.c
# End Source File
# Begin Source File

SOURCE=.\graph\cb\cbDom.c
# End Source File
# Begin Source File

SOURCE=.\graph\cb\cbGreedy.c
# End Source File
# Begin Source File

SOURCE=.\graph\cb\cbNtk.c
# End Source File
# Begin Source File

SOURCE=.\graph\cb\cbPrint.c
# End Source File
# Begin Source File

SOURCE=.\graph\cb\cbUtils.c
# End Source File
# End Group
# Begin Group "ft"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\graph\ft\ft.h
# End Source File
# Begin Source File

SOURCE=.\graph\ft\ftFactor.c
# End Source File
# Begin Source File

SOURCE=.\graph\ft\ftList.c
# End Source File
# Begin Source File

SOURCE=.\graph\ft\ftPrint.c
# End Source File
# Begin Source File

SOURCE=.\graph\ft\ftSop.c
# End Source File
# Begin Source File

SOURCE=.\graph\ft\ftTrans.c
# End Source File
# Begin Source File

SOURCE=.\graph\ft\ftTree.c
# End Source File
# Begin Source File

SOURCE=.\graph\ft\ftTriv.c
# End Source File
# End Group
# Begin Group "sh"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\graph\sh\sh.h
# End Source File
# Begin Source File

SOURCE=.\graph\sh\shApi.c
# End Source File
# Begin Source File

SOURCE=.\graph\sh\shCanon.c
# End Source File
# Begin Source File

SOURCE=.\graph\sh\shCreate.c
# End Source File
# Begin Source File

SOURCE=.\graph\sh\shHash.c
# End Source File
# Begin Source File

SOURCE=.\graph\sh\shInt.h
# End Source File
# Begin Source File

SOURCE=.\graph\sh\shNetwork.c
# End Source File
# Begin Source File

SOURCE=.\graph\sh\shSign.c
# End Source File
# Begin Source File

SOURCE=.\graph\sh\shTravId.c
# End Source File
# End Group
# Begin Group "wn"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\graph\wn\wn.h
# End Source File
# Begin Source File

SOURCE=.\graph\wn\wnApi.c
# End Source File
# Begin Source File

SOURCE=.\graph\wn\wnCreate.c
# End Source File
# Begin Source File

SOURCE=.\graph\wn\wnDerive.c
# End Source File
# Begin Source File

SOURCE=.\graph\wn\wnDfs.c
# End Source File
# Begin Source File

SOURCE=.\graph\wn\wnInt.h
# End Source File
# Begin Source File

SOURCE=.\graph\wn\wnStrash.c
# End Source File
# Begin Source File

SOURCE=.\graph\wn\wnUtils.c
# End Source File
# End Group
# Begin Group "ftb"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\graph\ftb\ftb.h
# End Source File
# Begin Source File

SOURCE=.\graph\ftb\ftbCore.c
# End Source File
# Begin Source File

SOURCE=.\graph\ftb\ftbDiv.c
# End Source File
# Begin Source File

SOURCE=.\graph\ftb\ftbMan.c
# End Source File
# Begin Source File

SOURCE=.\graph\ftb\ftbMin.c
# End Source File
# Begin Source File

SOURCE=.\graph\ftb\ftbTree.c
# End Source File
# End Group
# End Group
# Begin Group "opt"

# PROP Default_Filter ""
# Begin Group "pd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\opt\pd\pd.c
# End Source File
# Begin Source File

SOURCE=.\opt\pd\pd.h
# End Source File
# Begin Source File

SOURCE=.\opt\pd\pdHash.c
# End Source File
# Begin Source File

SOURCE=.\opt\pd\pdHash.h
# End Source File
# Begin Source File

SOURCE=.\opt\pd\pdInt.h
# End Source File
# Begin Source File

SOURCE=.\opt\pd\pdPairDecode.c
# End Source File
# End Group
# Begin Group "enc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\opt\enc\enc.h
# End Source File
# Begin Source File

SOURCE=.\opt\enc\encCmd.c
# End Source File
# Begin Source File

SOURCE=.\opt\enc\encCode.c
# End Source File
# Begin Source File

SOURCE=.\opt\enc\encInt.h
# End Source File
# Begin Source File

SOURCE=.\opt\enc\encNtk.c
# End Source File
# Begin Source File

SOURCE=.\opt\enc\encUtil.c
# End Source File
# End Group
# Begin Group "fxu"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\opt\fxu\fxu.c
# End Source File
# Begin Source File

SOURCE=.\opt\fxu\fxu.h
# End Source File
# Begin Source File

SOURCE=.\opt\fxu\fxuCreate.c
# End Source File
# Begin Source File

SOURCE=.\opt\fxu\fxuHeapD.c
# End Source File
# Begin Source File

SOURCE=.\opt\fxu\fxuHeapS.c
# End Source File
# Begin Source File

SOURCE=.\opt\fxu\fxuInt.h
# End Source File
# Begin Source File

SOURCE=.\opt\fxu\fxuList.c
# End Source File
# Begin Source File

SOURCE=.\opt\fxu\fxuMatrix.c
# End Source File
# Begin Source File

SOURCE=.\opt\fxu\fxuPair.c
# End Source File
# Begin Source File

SOURCE=.\opt\fxu\fxuPrint.c
# End Source File
# Begin Source File

SOURCE=.\opt\fxu\fxuReduce.c
# End Source File
# Begin Source File

SOURCE=.\opt\fxu\fxuSelect.c
# End Source File
# Begin Source File

SOURCE=.\opt\fxu\fxuSingle.c
# End Source File
# Begin Source File

SOURCE=.\opt\fxu\fxuUpdate.c
# End Source File
# End Group
# Begin Group "simp"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\opt\simp\simp.h
# End Source File
# Begin Source File

SOURCE=.\opt\simp\simpArray.c
# End Source File
# Begin Source File

SOURCE=.\opt\simp\simpArray.h
# End Source File
# Begin Source File

SOURCE=.\opt\simp\simpCmd.c
# End Source File
# Begin Source File

SOURCE=.\opt\simp\simpFlex.c
# End Source File
# Begin Source File

SOURCE=.\opt\simp\simpFull.c
# End Source File
# Begin Source File

SOURCE=.\opt\simp\simpImage.c
# End Source File
# Begin Source File

SOURCE=.\opt\simp\simpInt.h
# End Source File
# Begin Source File

SOURCE=.\opt\simp\simpMvf.c
# End Source File
# Begin Source File

SOURCE=.\opt\simp\simpOdc.c
# End Source File
# Begin Source File

SOURCE=.\opt\simp\simpSdc.c
# End Source File
# Begin Source File

SOURCE=.\opt\simp\simpSimplify.c
# End Source File
# Begin Source File

SOURCE=.\opt\simp\simpUtil.c
# End Source File
# End Group
# Begin Group "fm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\opt\fm\fm.h
# End Source File
# Begin Source File

SOURCE=.\opt\fm\fmData.c
# End Source File
# Begin Source File

SOURCE=.\opt\fm\fmGlobal.c
# End Source File
# Begin Source File

SOURCE=.\opt\fm\fmImage.c
# End Source File
# Begin Source File

SOURCE=.\opt\fm\fmImageMv.c
# End Source File
# Begin Source File

SOURCE=.\opt\fm\fmInt.h
# End Source File
# Begin Source File

SOURCE=.\opt\fm\fmMisc.c
# End Source File
# Begin Source File

SOURCE=.\opt\fm\fmsApi.c
# End Source File
# Begin Source File

SOURCE=.\opt\fm\fmsFlex.c
# End Source File
# Begin Source File

SOURCE=.\opt\fm\fmwApi.c
# End Source File
# Begin Source File

SOURCE=.\opt\fm\fmwFlex.c
# End Source File
# End Group
# Begin Group "mfs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\opt\mfs\mfs.c
# End Source File
# Begin Source File

SOURCE=.\opt\mfs\mfsInt.h
# End Source File
# Begin Source File

SOURCE=.\opt\mfs\mfsMisc.c
# End Source File
# Begin Source File

SOURCE=.\opt\mfs\mfsSpec.c
# End Source File
# Begin Source File

SOURCE=.\opt\mfs\mfsWnd.c
# End Source File
# End Group
# Begin Group "fmn"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\opt\fmn\fmn.c
# End Source File
# Begin Source File

SOURCE=.\opt\fmn\fmn.h
# End Source File
# Begin Source File

SOURCE=.\opt\fmn\fmnApi.c
# End Source File
# Begin Source File

SOURCE=.\opt\fmn\fmnFlex.c
# End Source File
# Begin Source File

SOURCE=.\opt\fmn\fmnGlobal.c
# End Source File
# Begin Source File

SOURCE=.\opt\fmn\fmnImageMv.c
# End Source File
# Begin Source File

SOURCE=.\opt\fmn\fmnInt.h
# End Source File
# Begin Source File

SOURCE=.\opt\fmn\fmnUtils.c
# End Source File
# End Group
# Begin Group "mfsn"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\opt\mfsn\mfsn.c
# End Source File
# Begin Source File

SOURCE=.\opt\mfsn\mfsnInt.h
# End Source File
# Begin Source File

SOURCE=.\opt\mfsn\mfsnSimp.c
# End Source File
# Begin Source File

SOURCE=.\opt\mfsn\mfsnUtils.c
# End Source File
# End Group
# Begin Group "exp"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\opt\exp\exp.c
# End Source File
# Begin Source File

SOURCE=.\opt\exp\expDecomp.c
# End Source File
# Begin Source File

SOURCE=.\opt\exp\expDouble.c
# End Source File
# Begin Source File

SOURCE=.\opt\exp\expFactor.c
# End Source File
# Begin Source File

SOURCE=.\opt\exp\expInt.h
# End Source File
# Begin Source File

SOURCE=.\opt\exp\expNetFlow.c
# End Source File
# Begin Source File

SOURCE=.\opt\exp\expNetFlowExp.c
# End Source File
# Begin Source File

SOURCE=.\opt\exp\expPerm.c
# End Source File
# Begin Source File

SOURCE=.\opt\exp\expPreNand2.c
# End Source File
# Begin Source File

SOURCE=.\opt\exp\expRedOff.c
# End Source File
# Begin Source File

SOURCE=.\opt\exp\expTest.c
# End Source File
# End Group
# Begin Group "fmm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\opt\fmm\fmm.h
# End Source File
# Begin Source File

SOURCE=.\opt\fmm\fmmApi.c
# End Source File
# Begin Source File

SOURCE=.\opt\fmm\fmmFlex.c
# End Source File
# Begin Source File

SOURCE=.\opt\fmm\fmmImage.c
# End Source File
# Begin Source File

SOURCE=.\opt\fmm\fmmInt.h
# End Source File
# Begin Source File

SOURCE=.\opt\fmm\fmmMan.c
# End Source File
# Begin Source File

SOURCE=.\opt\fmm\fmmSweep.c
# End Source File
# Begin Source File

SOURCE=.\opt\fmm\fmmUtils.c
# End Source File
# End Group
# Begin Group "mfsm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\opt\mfsm\mfsm.c
# End Source File
# Begin Source File

SOURCE=.\opt\mfsm\mfsmInt.h
# End Source File
# Begin Source File

SOURCE=.\opt\mfsm\mfsmSimp.c
# End Source File
# End Group
# Begin Group "fmb"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\opt\fmb\fmb.h
# End Source File
# Begin Source File

SOURCE=.\opt\fmb\fmbApiFunc.c
# End Source File
# Begin Source File

SOURCE=.\opt\fmb\fmbApiRel.c
# End Source File
# Begin Source File

SOURCE=.\opt\fmb\fmbFlexFunc.c
# End Source File
# Begin Source File

SOURCE=.\opt\fmb\fmbFlexRel.c
# End Source File
# Begin Source File

SOURCE=.\opt\fmb\fmbImage.c
# End Source File
# Begin Source File

SOURCE=.\opt\fmb\fmbInt.h
# End Source File
# Begin Source File

SOURCE=.\opt\fmb\fmbMan.c
# End Source File
# Begin Source File

SOURCE=.\opt\fmb\fmbSweep.c
# End Source File
# Begin Source File

SOURCE=.\opt\fmb\fmbUtils.c
# End Source File
# End Group
# End Group
# Begin Group "glu"

# PROP Default_Filter ""
# Begin Group "array"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\glu\array\array.c
# End Source File
# Begin Source File

SOURCE=.\glu\array\array.h
# End Source File
# End Group
# Begin Group "avl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\glu\avl\avl.c
# End Source File
# Begin Source File

SOURCE=.\glu\avl\avl.h
# End Source File
# End Group
# Begin Group "list"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\glu\list\list.c
# End Source File
# Begin Source File

SOURCE=.\glu\list\list.h
# End Source File
# Begin Source File

SOURCE=.\glu\list\lsort.h
# End Source File
# End Group
# Begin Group "sparse"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\glu\sparse\cols.c
# End Source File
# Begin Source File

SOURCE=.\glu\sparse\matrix.c
# End Source File
# Begin Source File

SOURCE=.\glu\sparse\rows.c
# End Source File
# Begin Source File

SOURCE=.\glu\sparse\sparse.h
# End Source File
# Begin Source File

SOURCE=.\glu\sparse\sparse_int.h
# End Source File
# End Group
# Begin Group "st"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\glu\st\st.c
# End Source File
# Begin Source File

SOURCE=.\glu\st\st.h
# End Source File
# End Group
# Begin Group "util"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\glu\util\cpu_stats.c
# End Source File
# Begin Source File

SOURCE=.\glu\util\cpu_time.c
# End Source File
# Begin Source File

SOURCE=.\glu\util\datalimit.c
# End Source File
# Begin Source File

SOURCE=.\glu\util\getopt.c
# End Source File
# Begin Source File

SOURCE=.\glu\util\pathsearch.c
# End Source File
# Begin Source File

SOURCE=.\glu\util\prtime.c
# End Source File
# Begin Source File

SOURCE=.\glu\util\random.c
# End Source File
# Begin Source File

SOURCE=.\glu\util\safe_mem.c
# End Source File
# Begin Source File

SOURCE=.\glu\util\strsav.c
# End Source File
# Begin Source File

SOURCE=.\glu\util\texpand.c
# End Source File
# Begin Source File

SOURCE=.\glu\util\tmpfile.c
# End Source File
# Begin Source File

SOURCE=.\glu\util\util.h
# End Source File
# End Group
# Begin Group "var_set"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\glu\var_set\var_set.c
# End Source File
# Begin Source File

SOURCE=.\glu\var_set\var_set.h
# End Source File
# End Group
# End Group
# Begin Group "sis"

# PROP Default_Filter ""
# Begin Group "espresso"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sis\espresso\cofactor.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\compl.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\contain.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\cubestr.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\cvrin.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\cvrm.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\cvrmisc.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\cvrout.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\equiv.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\espresso.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\espresso.h
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\essen.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\exact.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\expand.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\gasp.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\globals.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\hack.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\irred.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\map.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\opo.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\pair.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\primes.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\reduce.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\set.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\setc.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\sharp.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\sis.h
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\sminterf.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\sparse.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\unate.c
# End Source File
# Begin Source File

SOURCE=.\sis\espresso\verify.c
# End Source File
# End Group
# Begin Group "mincov"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sis\mincov\bin_mincov.c
# End Source File
# Begin Source File

SOURCE=.\sis\mincov\bin_sol.c
# End Source File
# Begin Source File

SOURCE=.\sis\mincov\dominate.c
# End Source File
# Begin Source File

SOURCE=.\sis\mincov\gimpel.c
# End Source File
# Begin Source File

SOURCE=.\sis\mincov\indep.c
# End Source File
# Begin Source File

SOURCE=.\sis\mincov\mincov.c
# End Source File
# Begin Source File

SOURCE=.\sis\mincov\mincov.h
# End Source File
# Begin Source File

SOURCE=.\sis\mincov\mincov_int.h
# End Source File
# Begin Source File

SOURCE=.\sis\mincov\part.c
# End Source File
# Begin Source File

SOURCE=.\sis\mincov\solution.c
# End Source File
# End Group
# Begin Group "minimize"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sis\minimize\dcsimp.c
# End Source File
# Begin Source File

SOURCE=.\sis\minimize\min_int.h
# End Source File
# Begin Source File

SOURCE=.\sis\minimize\minimize.c
# End Source File
# Begin Source File

SOURCE=.\sis\minimize\minimize.h
# End Source File
# Begin Source File

SOURCE=.\sis\minimize\ros.c
# End Source File
# Begin Source File

SOURCE=.\sis\minimize\ros.h
# End Source File
# End Group
# End Group
# Begin Group "bdd"

# PROP Default_Filter ""
# Begin Group "cudd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\bdd\cudd\cudd.h
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddAddAbs.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddAddApply.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddAddFind.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddAddInv.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddAddIte.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddAddNeg.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddAddWalsh.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddAndAbs.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddAnneal.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddApa.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddAPI.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddApprox.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddBddAbs.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddBddCorr.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddBddIte.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddBridge.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddCache.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddCheck.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddClip.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddCof.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddCompose.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddDecomp.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddEssent.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddExact.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddExport.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddGenCof.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddGenetic.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddGroup.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddHarwell.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddInit.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddInt.h
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddInteract.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddLCache.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddLevelQ.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddLinear.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddLiteral.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddMatMult.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddPriority.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddRead.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddRef.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddReorder.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddSat.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddSign.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddSolve.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddSplit.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddSubsetHB.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddSubsetSP.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddSymmetry.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddTable.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddUtil.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddWindow.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddZddCount.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddZddFuncs.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddZddGroup.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddZddIsop.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddZddLin.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddZddMisc.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddZddPort.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddZddReord.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddZddSetop.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddZddSymm.c
# End Source File
# Begin Source File

SOURCE=.\bdd\cudd\cuddZddUtil.c
# End Source File
# End Group
# Begin Group "mtr"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\bdd\mtr\mtr.h
# End Source File
# Begin Source File

SOURCE=.\bdd\mtr\mtrBasic.c
# End Source File
# Begin Source File

SOURCE=.\bdd\mtr\mtrGroup.c
# End Source File
# Begin Source File

SOURCE=.\bdd\mtr\mtrInt.h
# End Source File
# End Group
# Begin Group "epd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\bdd\epd\epd.c
# End Source File
# Begin Source File

SOURCE=.\bdd\epd\epd.h
# End Source File
# End Group
# Begin Group "extra"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\bdd\extra\extra.h
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraAddMisc.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraAddSpectra.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraBddAuto.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraBddBoundSet.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraBddDistance.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraBddImage.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraBddKmap.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraBddMisc.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraBddPermute.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraBddSupp.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraBddSymm.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraBddUnate.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraBddWidth.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraDdMinterm.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraDdMisc.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraDdNodePath.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraDdPrint.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraDdShift.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraDdSigma.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraDdTimed.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraDdTransfer.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraUtilFile.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraUtilGraph.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraUtilMemory.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraUtilMisc.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraUtilProgress.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraZddCover.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraZddExor.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraZddFactor.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraZddGraph.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraZddIsop.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraZddLitCount.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraZddMaxMin.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraZddMisc.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraZddPermute.c
# End Source File
# Begin Source File

SOURCE=.\bdd\extra\extraZddSubSup.c
# End Source File
# End Group
# Begin Group "reo"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\bdd\reo\reo.h
# End Source File
# Begin Source File

SOURCE=.\bdd\reo\reoApi.c
# End Source File
# Begin Source File

SOURCE=.\bdd\reo\reoCore.c
# End Source File
# Begin Source File

SOURCE=.\bdd\reo\reoProfile.c
# End Source File
# Begin Source File

SOURCE=.\bdd\reo\reoSift.c
# End Source File
# Begin Source File

SOURCE=.\bdd\reo\reoSwap.c
# End Source File
# Begin Source File

SOURCE=.\bdd\reo\reoTransfer.c
# End Source File
# Begin Source File

SOURCE=.\bdd\reo\reoUnits.c
# End Source File
# End Group
# Begin Group "dsd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\bdd\dsd\dsd.h
# End Source File
# Begin Source File

SOURCE=.\bdd\dsd\dsdApi.c
# End Source File
# Begin Source File

SOURCE=.\bdd\dsd\dsdCheck.c
# End Source File
# Begin Source File

SOURCE=.\bdd\dsd\dsdInt.h
# End Source File
# Begin Source File

SOURCE=.\bdd\dsd\dsdLocal.c
# End Source File
# Begin Source File

SOURCE=.\bdd\dsd\dsdMan.c
# End Source File
# Begin Source File

SOURCE=.\bdd\dsd\dsdProc.c
# End Source File
# Begin Source File

SOURCE=.\bdd\dsd\dsdTree.c
# End Source File
# End Group
# Begin Group "parse"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\bdd\parse\parse.h
# End Source File
# Begin Source File

SOURCE=.\bdd\parse\parseCore.c
# End Source File
# Begin Source File

SOURCE=.\bdd\parse\parseInt.h
# End Source File
# Begin Source File

SOURCE=.\bdd\parse\parseStack.c
# End Source File
# End Group
# End Group
# Begin Group "seq"

# PROP Default_Filter ""
# Begin Group "aut"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\seq\aut\aut.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\aut.h
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autBench.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autCheck.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autCompl.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autCreate.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autDcMin.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autDetExp.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autDetFull.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autDetPart.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autDot.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autEncode.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autFilter.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autInt.h
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autLang.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autMinim.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autPart.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autProd.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autRead.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autReadFsm.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autReadPara.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autRedSat.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autReduce.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autRel.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autSupport.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autUtils.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autWrite.c
# End Source File
# Begin Source File

SOURCE=.\seq\aut\autWriteFsm.c
# End Source File
# End Group
# Begin Group "mvn"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\seq\mvn\mvn.c
# End Source File
# Begin Source File

SOURCE=.\seq\mvn\mvn.h
# End Source File
# Begin Source File

SOURCE=.\seq\mvn\mvnEncode.c
# End Source File
# Begin Source File

SOURCE=.\seq\mvn\mvnInt.h
# End Source File
# Begin Source File

SOURCE=.\seq\mvn\mvnProd.c
# End Source File
# Begin Source File

SOURCE=.\seq\mvn\mvnSeqDc.c
# End Source File
# Begin Source File

SOURCE=.\seq\mvn\mvnSeqDcOld.c
# End Source File
# Begin Source File

SOURCE=.\seq\mvn\mvnSolve.c
# End Source File
# Begin Source File

SOURCE=.\seq\mvn\mvnStgExt.c
# End Source File
# Begin Source File

SOURCE=.\seq\mvn\mvnUtils.c
# End Source File
# End Group
# Begin Group "dual"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\seq\dual\dual.c
# End Source File
# Begin Source File

SOURCE=.\seq\dual\dual.h
# End Source File
# Begin Source File

SOURCE=.\seq\dual\dualForm.c
# End Source File
# Begin Source File

SOURCE=.\seq\dual\dualInt.h
# End Source File
# Begin Source File

SOURCE=.\seq\dual\dualNet.c
# End Source File
# Begin Source File

SOURCE=.\seq\dual\dualNormal.c
# End Source File
# Begin Source File

SOURCE=.\seq\dual\dualRead.c
# End Source File
# Begin Source File

SOURCE=.\seq\dual\dualSynth.c
# End Source File
# Begin Source File

SOURCE=.\seq\dual\dualUtils.c
# End Source File
# End Group
# Begin Group "lang"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\seq\lang\lang.c
# End Source File
# Begin Source File

SOURCE=.\seq\lang\lang.h
# End Source File
# Begin Source File

SOURCE=.\seq\lang\langCheck.c
# End Source File
# Begin Source File

SOURCE=.\seq\lang\langDet.c
# End Source File
# Begin Source File

SOURCE=.\seq\lang\langFilter.c
# End Source File
# Begin Source File

SOURCE=.\seq\lang\langInt.h
# End Source File
# Begin Source File

SOURCE=.\seq\lang\langMinim.c
# End Source File
# Begin Source File

SOURCE=.\seq\lang\langOper.c
# End Source File
# Begin Source File

SOURCE=.\seq\lang\langProd.c
# End Source File
# Begin Source File

SOURCE=.\seq\lang\langReach.c
# End Source File
# Begin Source File

SOURCE=.\seq\lang\langRead.c
# End Source File
# Begin Source File

SOURCE=.\seq\lang\langReenc.c
# End Source File
# Begin Source File

SOURCE=.\seq\lang\langStack.c
# End Source File
# Begin Source File

SOURCE=.\seq\lang\langStg.c
# End Source File
# Begin Source File

SOURCE=.\seq\lang\langSupp.c
# End Source File
# Begin Source File

SOURCE=.\seq\lang\langUtils.c
# End Source File
# Begin Source File

SOURCE=.\seq\lang\langVarMap.c
# End Source File
# Begin Source File

SOURCE=.\seq\lang\langWrite.c
# End Source File
# End Group
# End Group
# Begin Group "map"

# PROP Default_Filter ""
# Begin Group "mio"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\map\mio\mio.c
# End Source File
# Begin Source File

SOURCE=.\map\mio\mio.h
# End Source File
# Begin Source File

SOURCE=.\map\mio\mioApi.c
# End Source File
# Begin Source File

SOURCE=.\map\mio\mioFunc.c
# End Source File
# Begin Source File

SOURCE=.\map\mio\mioInt.h
# End Source File
# Begin Source File

SOURCE=.\map\mio\mioRead.c
# End Source File
# Begin Source File

SOURCE=.\map\mio\mioUtils.c
# End Source File
# End Group
# Begin Group "super"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\map\super\stmm.c
# End Source File
# Begin Source File

SOURCE=.\map\super\stmm.h
# End Source File
# Begin Source File

SOURCE=.\map\super\super.c
# End Source File
# Begin Source File

SOURCE=.\map\super\super.h
# End Source File
# Begin Source File

SOURCE=.\map\super\superAnd.c
# End Source File
# Begin Source File

SOURCE=.\map\super\superGENERIC.c
# End Source File
# Begin Source File

SOURCE=.\map\super\superInt.h
# End Source File
# Begin Source File

SOURCE=.\map\super\superUtils.c
# End Source File
# Begin Source File

SOURCE=.\map\super\superWrite.c
# End Source File
# End Group
# Begin Group "mapper"

# PROP Default_Filter ""
# End Group
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
