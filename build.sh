if [ ! -d "build" ]; then
  mkdir build
fi
g++ -std=c++17 -c *.cc
g++ client.o clientMain.o -o ./build/clientMain.exe
g++ server.o serverMain.o -o ./build/serverMain.exe -lpthread
rm *.o