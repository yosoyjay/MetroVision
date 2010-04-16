/* 
 * File:   main.cpp
 * Author: Jesse
 *
 * Created on March 18, 2010, 10:59 AM
 */


#ifndef _PLAYER_
#define _PLAYER_
#include <libplayerc++/playerc++.h>
#endif


#include <cstdio>
#include <pthread.h>
#include "camera.h"
using namespace PlayerCc;

PlayerClient    robot("localhost",6665);
CameraProxy 	cp(&robot,0);
BlobfinderProxy bf(&robot,0);
Position2dProxy pp(&robot,0);
PtzProxy	ptz(&robot,0);

void* walk(void *ptr){

	ptz.SetCam(0,1,0);
    // Make this part of struct passed to walk
    bool walk = true;

	while(walk){
		pp.SetSpeed(0.3,0,0);
		sleep(1);
	}

	pthread_exit(0);
}

void* headScan(void *ptr){

    bool scan = true;
    while(scan){
        ptz.SetCam(0,1,0);
    }
}

int main(int argc, char **argv)
{
	// Thread ID for walking
	pthread_t walk_thread;
	
	// Sleep required to wait for Aibo to initialize
	sleep(5);

	pthread_create(&walk_thread, NULL, &walk, NULL);

	Camera* testCamera = new Camera(robot, bf, cp);
	char* windowName = "OpenCV Cam";


	//testCamera->displayBlobs(windowName);
   	const char* cascadeFile = "./haar/goalCascade.xml";
    testCamera->loadCascade(cascadeFile);
    testCamera->displayHaar(windowName);
	//testCamera->displayTetris(windowName);
	//char* test = "./test.jpg";
	//testCamera->captureStream(windowName);

	return 0;
}


