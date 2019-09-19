g++ -shared -o libftpapi.so ftpapi.cpp interface.cpp -L. -Wl,-rpath=./ -std=c++11 -lpthread -fpermissive -DJSON_IS_AMALGAMATION
g++ -o test test.cpp libftpapi.so -L. -ljsoncpp -lcurl -Wl,-rpath=./ -std=c++11 -lpthread -fpermissive -DJSON_IS_AMALGAMATION
