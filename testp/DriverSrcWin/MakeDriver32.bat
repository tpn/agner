rem                     MakeDriver32.bat              2009-08-13 AgF

rem Makes 32 bit driver for access to MSR registers

rem System requirements:
rem Windows 2000 or NT or later needed to run this driver.

rem Tools needed for building this driver:
rem Windows Driver Kit (WDK) 7.0.0

rem Note that you need the 64-bit version of this driver if the test
rem program is running under a 64-bit Windows system, even if running
rem in 32-bit mode.

rem The driver has no signature. If running under Windows Vista 
rem or later then you have to accept the unsigned driver. 
rem Run compiler as administrator.



rem Define filename
set drv=MSRDriver

rem set path to DDK
set DDK=C:\WinDDK\7600.16385.0

rem Set path to compiler
set path=%DDK%\bin\x86\x86

rem set include path
set include=%DDK%\inc\crt;%DDK%\inc\ddk;%DDK%\inc\api

rem set library path to DDK
set lib=%DDK%\lib\win7\i386;%DDK%\lib\crt\i386

rem Delete old driver
del %drv%32.sys

rem Compile driver cpp file
cl /c /Gz /O2 /Fo%drv%32.obj %drv%.cpp
if errorlevel 1 pause

rem Link into .sys file
link /driver /base:0x10000 /align:32 /out:%drv%32.sys /subsystem:native /entry:DriverEntry ntoskrnl.lib %drv%32.obj
if errorlevel 1 pause

pause