

cd /d %~dp0

mbed compile -m GR_LYCHEE -t GCC_ARM --profile debug

@echo off
pause >nul