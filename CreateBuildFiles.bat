@ECHO OFF
cmake -DBOOST_ROOT:PATH=c:/local/boost -DBOOST_LIBRARYDIR:PATH=c:/local/boost/lib32-msvc-14.0 -DDHEWM3LIBS=../dhewm3-libs/i686-w64-mingw32 ./neo
pause
