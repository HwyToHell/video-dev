::scan current directory for firstFrame and lastFrame of debug image
::create directory 'startFrame - stopFrame' underneath workPath
@echo off
setlocal EnableDelayedExpansion
rem set work_path=%HomeDrive%%HomePath%\counter\2017-09-18\opposite
set work_path=%HomeDrive%%HomePath%\counter\traffic640x480_low_pass
goto check_args


:usage
echo usage: move_debug -s
echo scan current directory for firstFrame and lastFrame of debug image
echo create directory 'startFrame - stopFrame' underneath workPath
goto :eof


:check_args
::check command args
if not [%1]==[-s] (
	goto :usage
)


::find first and last frame number
set first=1000000
set last=0
for /f "tokens=1,2,3 delims=_" %%G in ('dir debug*.png') do (
	if not [%%H]==[] (
		set num_string=%%H
		for /f "tokens=1,2 delims=." %%G in ("!num_string!") do (
			set num=%%G
		)
		if !num! LSS !first! (
			set first=!num!
		)		
		if !num! GTR !last! (
			set last=!num!
		)		
	)
)


::create directory
set dir_name=!first! - !last!
set pathToCreate=!work_path!\!dir_name!

if exist !pathToCreate! (
	echo directory already exists: !pathToCreate!
) else (
	echo mkdir !pathToCreate!
	md "!pathToCreate!"
)

set pathTrack=!pathToCreate!\track debug
if exist !pathTrack! (
	echo image directory already exists: !pathTrack!
) else (
	echo mkdir !pathTrack!
	md "!pathTrack!"
)


::copy files
echo copy files
copy debug*.png "!pathTrack!"

goto :eof




