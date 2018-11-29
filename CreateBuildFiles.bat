@ECHO OFF
cmake -DBOOST_ROOT:PATH=c:/local/boost_1_64_0 -DBOOST_LIBRARYDIR:PATH=c:/local/boost_1_64_0/lib32-msvc-14.1 -DDHEWM3LIBS=../dhewm3-libs/i686-w64-mingw32 ./neo
pause
