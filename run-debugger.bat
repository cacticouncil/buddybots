@ECHO OFF
call "%VS140COMNTOOLS%"\vsdevcmd.bat
devenv /debugexe RelWithDebInfo\dhewm3.exe +set fs_basepath "./" +set com_allowConsole 1 +set si_pure 0 +set fs_game d3xp +set fs_game_base base
