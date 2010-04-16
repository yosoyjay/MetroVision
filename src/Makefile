cameraTest: camera.o main.o
	g++ -g -Wall `pkg-config --libs --cflags opencv` `pkg-config --libs --cflags playerc++` \
main.o camera.o -o ../cameraTest

main.o: main.cpp camera.h
	g++ -c main.cpp `pkg-config --libs --cflags opencv` `pkg-config --libs --cflags playerc++`

camera.o: camera.cpp camera.h
	g++ -c camera.cpp `pkg-config --libs --cflags opencv` `pkg-config --libs --cflags playerc++`

