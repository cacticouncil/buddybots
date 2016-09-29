@ECHO OFF
cmake -DBOOST_ROOT:PATH=d:/boost/boost_1_61_0 -DBOOST_LIBRARYDIR:PATH=d:/boost/boost_1_61_0/lib32-msvc-14.0 -DDHEWM3LIBS=../dhewm3-libs/i686-w64-mingw32 ./neo
pause
