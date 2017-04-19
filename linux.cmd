@echo off

REM ############################
REM # Build and run test image #
REM ############################

docker build -t proclist .
docker run -it --rm -w /src -v %cd%:/src proclist

pause
exit
