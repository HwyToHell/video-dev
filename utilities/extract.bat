::extract part of video from start <min:sec> to stop <min:sec>
@echo off
setlocal EnableDelayedExpansion

::check command args
if [%1]==[] (
	echo missing input-video
	goto :usage
)
if [%2]==[] (
	echo missing start-seconds
	goto :usage
)
if [%3]==[] (
	echo missing length-seconds
	goto :usage
)
if [%4]==[] (set output=out) else (set output=%4)
:: echo input %1, start %2, stop %3, output !output!

::convert start time to seconds
set start=%2
for /f "tokens=1,2 delims=:" %%G in ("!start!") do (
	if [%%H]==[] (
		set min=0	
		set sec=%%G
	) else (
		set min=%%G
		set sec=%%H
	)
)
set /a start_sec=!min!*60+!sec! 
echo start in seconds: !start_sec!

::convert stop time to seconds
set stop=%3
for /f "tokens=1,2 delims=:" %%G in ("!stop!") do (
	if [%%H]==[] (
		set min=0	
		set sec=%%G
	) else (
		set min=%%G
		set sec=%%H
	)
)
set /a stop_sec=!min!*60+!sec! 
echo stop in seconds: !stop_sec!

::length in seconds
set /a length_sec=!stop_sec!-!start_sec!
echo length in seconds: !length_sec!

ffmpeg -ss !start_sec! -i %1 -t !length_sec! -c copy !output! 
goto :eof

:usage
echo usage: extract 'input-video' 'start-seconds' 'stop-seconds' 'output-video'
goto :eof



