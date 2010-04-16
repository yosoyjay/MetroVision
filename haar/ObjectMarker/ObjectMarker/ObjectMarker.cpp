/**
 * Modifications to be made:
 * 1. Two possible modes: free-size windowing and fixed-scale windowing
 * !!DONE!! 2. Add and remove the newly added window
 * !!DONE!! 3. Expand the width or height of the window by a constant number of pixels
 * !!DONE!! 4. Expand the dimension (both width and height) of window by a constant scale (e.g. 0.1)
 * !!DONE!! 5. Move the window to left, right, up, and down
 */
#include "stdafx.h"

#ifdef WIN32
//#include "stdafx.h"
#include <io.h>
#else
#include <sys/types.h>
#include <dirent.h>
#endif

#include "cv.h"
#include "cvaux.h"
#include "highgui.h"
#include <stdio.h>
#include <time.h>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

struct DPrecRect {
	int x;
	int y;
	double width;
	double height;
};

IplImage* image  = 0;
IplImage* image2 = 0;
double roi_x0 = -1;  // (roi_x0, roi_y0) is the coordinate of first corner
double roi_y0 = -1;
double roi_x1 = -1;  // (roi_x1, roi_y1) is the coordinate of second corner
double roi_y1 = -1;
float scale_width = 5;
float scale_height = 5;
vector<DPrecRect> objects;
char* input_dir   = "rawdata";
char* outputname  = "annotation.txt";
char* output_dir  = "cropped";
char* raw_image_ext = ".jpg";
char window_name[200] = "Object Marker";
bool FIXED_SCALE = true;
bool ACCEPT_POINT = false;


void printHelp();
void redrawImage();
void on_mouse(int, int, int, int, void*);
void saveObjects(FILE*, const char*);
int cropImages(const char*);


void printHelp() {
	printf("\nObject Marker: \n\ta tool to annotate the locations of objects in an image\n");
	printf("\tGunawan Herman, April 2006\n");
	printf("\tAdapted from ObjectMarker.cpp by A.Florian\n");
	printf("\n");
	printf("\tThis program searches for input images in dir '%s'\n", input_dir);
	printf("\tLocations of objects will be printed out to file '%s'\n", outputname);
	printf("\n");
	printf("------------------------------------------------------------\n");
	printf("|  btn  |               function                           |\n");
	printf("|-------|--------------------------------------------------|\n");
	printf("|Enter  | write currently marked objects to file           |\n");
	printf("|       | and proceed to the next image                    |\n");
	printf("|Space  | same                                             |\n");
	printf("|ESC    | close this program                               |\n");
	printf("|d      | delete the most recently added rect              |\n");
	printf("|8      | move recently added rect up by 1 px              |\n");
	printf("|9      | move recently added rect up by 10 pxs            |\n");
	printf("|2      | move recently added rect down by 1 px            |\n");
	printf("|3      | move recently added rect down by 10 pxs          |\n");
	printf("|4      | move recently added rect left by 1 px            |\n");
	printf("|5      | move recently added rect left by 10 pxs          |\n");
	printf("|6      | move recently added rect right by 1 px           |\n");
	printf("|7      | move recently added rect right by 10 pxs         |\n");
	printf("|w      | enlarge width of recently added rect by 1 px     |\n");
	printf("|W      | reduce width of recently added rect by 1 px      |\n");
	printf("|h      | enlarge height of recently added rect by 1 px    |\n");
	printf("|H      | reduce height of recently added rect by 1 px     |\n");
	printf("|z      | zoom in recently added rect by 2%%                |\n");
	printf("|Z      | zoom out recently added rect by 2%%               |\n");
	printf("|m      | switch between free-size and fixed-scale mode &  |\n");
	printf("|       | to lock-in the width-height ratio                |\n");
	printf("|s      | input the scale constant                         |\n");
	printf("|p      | objects can be marked with rect as well as with  |\n");
	printf("|       | points instead of rect                           |\n");
	printf("|t      | print this instruction                           |\n");
	printf("|j      | jump to image whose index is specified           |\n");
	printf("------------------------------------------------------------\n");
	printf("\n");
	printf("mode = %s\n", FIXED_SCALE ? "FIXED_SCALE" : "FREE_SIZE");
	if(FIXED_SCALE) {
		printf("Scale constant: %f x %f\n", scale_width, scale_height);	
	}
	printf("ACCEPT_POINT = %s\n", ACCEPT_POINT ? "YES" : "NO");
}


void redrawImage() {
        //printf("redrawImage()\n");
	image2 = cvCloneImage(image);

	// Display all rectangles
	int numOfRect = (int)(objects.size());
	for(int i = 0; i < numOfRect; i++) {
		DPrecRect rectangle = objects.at(i);
		if(rectangle.width > 0 || rectangle.height > 0) {
			cvRectangle(image2, cvPoint(rectangle.x, rectangle.y), 
				cvPoint(rectangle.x + (int)(rectangle.width), rectangle.y + (int)(rectangle.height)),
				CV_RGB(255, 0, 0), 1);
		} else {
			cvCircle(image2, cvPoint(rectangle.x, rectangle.y), 1, CV_RGB(255, 0, 0));
		}
	}

	if(roi_x0 > 0) {
		cvRectangle(image2, cvPoint((int)roi_x0,(int)roi_y0), cvPoint((int)roi_x1,(int)roi_y1), CV_RGB(255,0,0),1);
	}

	cvShowImage(window_name, image2);
	cvReleaseImage(&image2);
}


void on_mouse(int event,int x,int y,int flag, void*) {

	// If left mouse button is pressed, record the first coordinate
	if(event == CV_EVENT_LBUTTONDOWN) {
	        //printf("Left mouse button pressed\n");
		roi_x0 = roi_x1 = x;
		roi_y0 = roi_y1 = y;
	}
  
	// If mouse is moved while left mouse button is still pressed
#ifdef WIN32
	if(event == CV_EVENT_MOUSEMOVE && flag == CV_EVENT_FLAG_LBUTTON) {
#else
	if(event == CV_EVENT_MOUSEMOVE && flag == 33) {
#endif
	        //printf("Move moved with left button pressed\n");
		roi_x1 = x;
		
		if(!FIXED_SCALE) {
			roi_y1 = y;
		} else {
			roi_y1 = roi_y0 + (((roi_x1 - roi_x0)/scale_width) * scale_height);
		}

		redrawImage();
	}

	// If left mouse button is released
	if(event == CV_EVENT_LBUTTONUP) {
	        //printf("Left button released\n");
		int topleft_x = min((int)roi_x0, (int)roi_x1);
		int topleft_y = min((int)roi_y0, (int)roi_y1);
		double width  = abs((int)roi_x0 - (int)roi_x1);
		double height = abs((int)roi_y0 - (int)roi_y1);

		if((width == 0 || height == 0) && !ACCEPT_POINT) return;

		DPrecRect rectangle = {topleft_x, topleft_y, width, height};
		objects.push_back(rectangle);
		roi_x0 = -1;  // indicates that there's no temporary rectangle
		redrawImage();
	}
}


void saveObjects(FILE* output, const char* imagename) {
	int numOfRect = (int)(objects.size());
	if(numOfRect) {
		fprintf(output, "%s %d ", imagename, objects.size());
		for(int i = 0; i < (int)(objects.size()); i++) {
			DPrecRect rectangle = objects.at(i);
			fprintf(output, "%d %d %d %d ", rectangle.x, rectangle.y, (int)(rectangle.width), (int)(rectangle.height));
		}
		fprintf(output, "\n");
		fflush(output);
	}
}


int cropImages(const char* datafile) {
	static int index = 1;
	int counts = 0;

	FILE* in = fopen(datafile, "r");
	if(!in) {
		printf("Error: cannot open %s to read\n", datafile);
		exit(-1);
	}

	while(true) {
		char imagename[150], crop_output[20];
		int nrects, x, y, w, h;
		if(fscanf(in, "%s %d", imagename, &nrects) < 2) break;
		printf("%s %d\n", imagename, nrects);
		                                               
		IplImage* tmpimage = cvLoadImage(imagename, 0);
		IplImage* scaled_size = cvCloneImage(tmpimage);
		scaled_size->width = 24; //24;//16;
		scaled_size->height = 24; //12;//8;
		
		if(!tmpimage) {
			printf("Error: cannot load %s\n", imagename);
			exit(-1);
		}

		for(int i = 0; i < nrects; i++) {
			if(fscanf(in, "%d %d %d %d", &x, &y, &w, &h) < 4) {
				printf("%s %d %d\n", imagename, i, nrects);
				printf("Invalid structure in %s\n", datafile);
				exit(-1);
			}

			struct _IplROI roi = {0, x, y, w, h};
			tmpimage->roi = &roi;
			sprintf(crop_output, "%s/%05d.jpg", output_dir, index++);
			cvSaveImage(crop_output, tmpimage);
			
			// Load the object, and scale it to desired size
			IplImage* original_size = cvLoadImage(crop_output, 0);
			cvResize(original_size, scaled_size);
			// Overwrite the original object with scaled object
			cvSaveImage(crop_output, scaled_size);
			cvReleaseImage(&original_size);
			
			counts++;
			if(counts % 50 == 0) {
				printf("*");
			}
		}

		tmpimage->roi = NULL;
		cvReleaseImage(&tmpimage);
		cvReleaseImage(&scaled_size);
	}
	printf("\n");

	fclose(in);
	return counts;
}



#ifdef WIN32

int _tmain(int argc, _TCHAR* argv[]) {
	if(argc > 1) {
		int n = cropImages(outputname);
		printf("%d images has been produced", n);
		if(n > 0) {
			printf(": %s/%05d.jpg - %s/%05d.jpg", output_dir, 1, output_dir, n);
		}
		printf("\n");
		exit(0);
	}
	
	for(int i = 0; i < argc; i++) {
		printf("%s\n", argv[i]);
	}

	printHelp();

	enum KeyBindings {
		Key_Enter = 13,  Key_ESC = 27,  Key_Space = 32, Key_d = 100, 
		Key_8 = 56, Key_9 = 57, Key_2 = 50, Key_3 = 51, Key_4 = 52, Key_5 = 53, Key_6 = 54, Key_7 = 55,
		Key_w = 119, Key_W = 87, Key_h = 104, Key_H = 72, Key_z = 122, Key_Z = 90, 
		Key_m = 109, Key_s = 115, Key_e = 101, Key_p = 112, Key_t = 116, Key_j = 106
	};

	struct _finddata_t bmp_file;
	long hFile;
	int iKey = 0;
	FILE* output = fopen(outputname, "a");
	if(!output) {
		printf("Error: cannot open %s to write to\n", outputname);
		exit(-1);
	}

	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	fprintf(output, "\n");
	fprintf(output, "########################\n");
	fprintf(output, " %s ", asctime(timeinfo));
	fprintf(output, "########################\n");

	int targetImageIndex = 1;
	static int counter = 1;

	// get *.bmp files in directory
	char pattern[150];
	sprintf(pattern, "%s/*.jpg", input_dir);
	if((hFile = (long)_findfirst(pattern, &bmp_file)) ==-1L) {
		printf("no appropriate input files in directory 'rawdata'\n");
	}
	else {
		// init highgui
		cvAddSearchPath(input_dir);
		string strPrefix;
      
		// open every *.bmp file
		do {
			if(counter != targetImageIndex) {
				counter++;			
				continue;
			}

			objects.clear();
			strPrefix = string(input_dir) + string("/");
			strPrefix += bmp_file.name;
   
			image = cvLoadImage(strPrefix.c_str(), 1);
			//window_name = bmp_file.name;
			sprintf(window_name, "%d - %s", counter, bmp_file.name);
			cvNamedWindow(window_name, 1);
			cvSetMouseCallback(window_name, on_mouse);
			cvShowImage(window_name, image);
	
			bool cont = false;
			do {
				// Get user input
				iKey = cvWaitKey(0);

				switch(iKey) {
					// Press ESC to close this program, any unsaved changes will be discarded
					case Key_ESC:
						cvReleaseImage(&image);
						cvDestroyWindow(window_name);
						fclose(output);
						return 0;

					// Press Space or Enter to save marked objects on current image and proceed to the next image
					case Key_Space:
					case Key_Enter:
						cont = false;
						saveObjects(output, strPrefix.c_str());
						break;

					// Press d to remove the last added object
					case Key_d:
						cont = true;
						objects.pop_back();
						redrawImage();
						break;

					case Key_8:
					case Key_9:
						cont = true;
						if(!objects.empty()) {
							objects.back().y -= (iKey == Key_8) ? 1 : 10;
							redrawImage();
						}
						break;

					case Key_2:
					case Key_3:
						cont = true;
						if(!objects.empty()) {
							objects.back().y += (iKey == Key_2) ? 1 : 10;
							redrawImage();
						}
						break;

					case Key_4:
					case Key_5:
						cont = true;
						if(!objects.empty()) {
							objects.back().x -= (iKey == Key_4) ? 1 : 10;
							redrawImage();
						}
						break;

					case Key_6:
					case Key_7:
						cont = true;
						if(!objects.empty()) {
							objects.back().x += (iKey == Key_6) ? 1 : 10;
							redrawImage();
						}
						break;

					case Key_w:
					case Key_W:
						cont = true;
						if(!objects.empty()) {
							objects.back().width += (iKey == Key_w) ? 1 : ((objects.back().width > 5) ? -1 : 0);
							redrawImage();
						}
						break;

					case Key_h:
					case Key_H:
						cont = true;
						if(!objects.empty()) {
							objects.back().height += (iKey == Key_h) ? 1 : ((objects.back().height > 5) ? -1 : 0);
							redrawImage();
						}
						break;

					case Key_z:
					case Key_Z:
						cont = true;
						if(!objects.empty()) {
							objects.back().width *= (iKey == Key_z) ? 1.02 : ((objects.back().width > 5 && objects.back().height > 5) ? 0.98 : 1);
							objects.back().height *= (iKey == Key_z) ? 1.02 : ((objects.back().width > 5 && objects.back().height > 5) ? 0.98 : 1);
							redrawImage();
						}
						break;

					case Key_m:
						cont = true;
						FIXED_SCALE = !FIXED_SCALE;
						printf("mode = %s\n", FIXED_SCALE ? "FIXED_SCALE" : "FREE_SIZE");
						if(FIXED_SCALE) {
							if(!objects.empty()) {
								scale_width = objects.back().width;
								scale_height = objects.back().height;
							} else {
								scale_width = scale_height = 5;
							}

							printf("Scale constant: %f x %f\n", scale_width, scale_height);
						}
						break;

					case Key_s:
						cont = true;
						printf("Input the scale constant:\n");
						printf("\twidth : ");
						scanf("%f", &scale_width);
						printf("\theight: ");
						scanf("%f", &scale_height);
						printf("Scale constant: %f x %f\n", scale_width, scale_height);
						break;

					case Key_e:
						cont = true;
						for(int i = 0; i < (int)(objects.size()); i++) {
							printf("%d %d %f %f\n", objects.at(i).x, objects.at(i).y, objects.at(i).width, objects.at(i).height);	
						}
						break;

					case Key_p:
						cont = true;
						ACCEPT_POINT = !ACCEPT_POINT;
						printf("ACCEPT_POINT = %s\n", ACCEPT_POINT ? "YES" : "NO");
						break;

					case Key_t:
						cont = true;
						printHelp();
						break;

					case Key_j:
						cont = false;
						printf("Jump to image#: ");
						scanf("%d", &targetImageIndex);
						targetImageIndex--;  // because targetImageIndex++ below
						break;

					default:
						if(iKey >= 0 && iKey <= 127) {
							cont = true;
							printf("Unrecognised command\n");
						} else {
							cont = false;
						}
				}
			} while(cont);

			counter++;
			targetImageIndex++;
			cvDestroyWindow(window_name);
			cvReleaseImage(&image);
		} while(_findnext(hFile,&bmp_file)==0);
    
		fclose(output);
		_findclose( hFile );
	}
  
	return 0;
}

#else

int main(int argc, char* argv[]) {
	if(argc > 1) {
		int n = cropImages(outputname);
		printf("%d images has been produced", n);
		if(n > 0) {
			printf(": %s/%05d.jpg - %s/%05d.jpg", output_dir, 1, output_dir, n);
		}
		printf("\n");
		exit(0);
	}
	
	for(int i = 0; i < argc; i++) {
		printf("%s\n", argv[i]);
	}

	printHelp();

	enum KeyBindings {
		Key_Enter = 1048586,  Key_ESC = 1048603,  Key_Space = 1048608, Key_d = 1048676, 
		Key_8a = 1114040, Key_8b = 1048632, Key_9a = 1048633, Key_9b = 1114041, 
		Key_2a = 1048626, Key_2b = 1114034, Key_3a = 1048627, Key_3b = 1114035, 
		Key_4a = 1048628, Key_4b = 1114036, Key_5a = 1048629, Key_5b = 1114037, 
		Key_6a = 1048630, Key_6b = 1114038, Key_7a = 1048631, Key_7b = 1114039,
		Key_w = 1048695, Key_W = 1114199, Key_h = 1048680, Key_H = 1114184, 
		Key_z = 1048698, Key_Z = 1114202, Key_m = 1048685, Key_s = 1048691, 
		Key_e = 1048677, Key_p = 1048688, Key_t = 1048692, Key_j = 1048682
	};

	int iKey = 0;
	FILE* output = fopen(outputname, "a");
	if(!output) {
		printf("Error: cannot open %s to write to\n", outputname);
		exit(-1);
	}

	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	fprintf(output, "\n");
	fprintf(output, "########################\n");
	fprintf(output, " %s ", asctime(timeinfo));
	fprintf(output, "########################\n");

	int targetImageIndex = 1;
	static int counter = 1;

	// get *.bmp files in directory
	DIR* dir = opendir(input_dir);
	struct dirent* dp;
	
	// init highgui
	cvAddSearchPath(input_dir);
	string strPrefix;

	// open every *.bmp file
	while(dir) {
	  if(counter != targetImageIndex) {
	    counter++;			
	    continue;
	  }
	  
	  if(dp = readdir(dir)) {
	    if(strstr(dp->d_name, raw_image_ext)) {
	      //printf("%s\n", dp->d_name);
	    } else {
	      continue;
	    }
	  }
	  else {
	    closedir(dir);
	    break;
	  }

	  objects.clear();
	  strPrefix = string(input_dir) + string("/");
	  strPrefix += dp->d_name;
	  
	  image = cvLoadImage(strPrefix.c_str(), 1);
	  sprintf(window_name, "%d - %s", counter, dp->d_name);
	  cvNamedWindow(window_name, 1);
	  cvSetMouseCallback(window_name, on_mouse);
	  cvShowImage(window_name, image);
	  
	  bool cont = false;
	  do {
	    // Get user input
	    iKey = cvWaitKey(0);
	    
	    switch(iKey) {
	      // Press ESC to close this program, any unsaved changes will be discarded
	    case Key_ESC:
	      //printf("ESC is pressed\n");
	      cvReleaseImage(&image);
	      cvDestroyWindow(window_name);
	      fclose(output);
	      return 0;
	      
	      // Press Space or Enter to save marked objects on current image and proceed to the next image
	    case Key_Space:
	    case Key_Enter:
	      //printf("Enter or Space is pressed\n");
	      cont = false;
	      saveObjects(output, strPrefix.c_str());
	      break;
	      
	      // Press d to remove the last added object
	    case Key_d:
	      //printf("d is pressed\n");
	      cont = true;
	      objects.pop_back();
	      redrawImage();
	      break;
	      
	    case Key_8a:
	    case Key_8b:
	    case Key_9a:
	    case Key_9b:
	      //printf("Key 8 or 9 is pressed\n");
	      cont = true;
	      if(!objects.empty()) {
		objects.back().y -= (iKey == Key_8a || iKey == Key_8b) ? 1 : 10;
		redrawImage();
	      }
	      break;
	      
	    case Key_2a:
	    case Key_2b:
	    case Key_3a:
	    case Key_3b:
	      //printf("Key 2 or 3 is pressed\n");
	      cont = true;
	      if(!objects.empty()) {
		objects.back().y += (iKey == Key_2a || iKey == Key_2b) ? 1 : 10;
		redrawImage();
	      }
	      break;
	      
	    case Key_4a:
	    case Key_4b:
	    case Key_5a:
	    case Key_5b:
	      //printf("Key 4 or 5 is pressed\n");
	      cont = true;
	      if(!objects.empty()) {
		objects.back().x -= (iKey == Key_4a || iKey == Key_4b) ? 1 : 10;
		redrawImage();
	      }
	      break;
	      
	    case Key_6a:
	    case Key_6b:
	    case Key_7a:
	    case Key_7b:
	      //printf("Key 6 or 7 is pressed\n");
	      cont = true;
	      if(!objects.empty()) {
		objects.back().x += (iKey == Key_6a || iKey == Key_6b) ? 1 : 10;
		redrawImage();
	      }
	      break;
	      
	    case Key_w:
	    case Key_W:
	      //printf("Key w or W is pressed\n");
	      cont = true;
	      if(!objects.empty()) {
		objects.back().width += (iKey == Key_w) ? 1 : ((objects.back().width > 5) ? -1 : 0);
		redrawImage();
	      }
	      break;
	      
	    case Key_h:
	    case Key_H:
	      //printf("Key h or H is pressed\n");
	      cont = true;
	      if(!objects.empty()) {
		objects.back().height += (iKey == Key_h) ? 1 : ((objects.back().height > 5) ? -1 : 0);
		redrawImage();
	      }
	      break;
	      
	    case Key_z:
	    case Key_Z:
	      //printf("Key z or Z is pressed\n");
	      cont = true;
	      if(!objects.empty()) {
		objects.back().width *= (iKey == Key_z) ? 1.02 : ((objects.back().width > 5 && objects.back().height > 5) ? 0.98 : 1);
		objects.back().height *= (iKey == Key_z) ? 1.02 : ((objects.back().width > 5 && objects.back().height > 5) ? 0.98 : 1);
		redrawImage();
	      }
	      break;
	      
	    case Key_m:
	      //printf("Key m is pressed\n");
	      cont = true;
	      FIXED_SCALE = !FIXED_SCALE;
	      printf("mode = %s\n", FIXED_SCALE ? "FIXED_SCALE" : "FREE_SIZE");
	      if(FIXED_SCALE) {
		if(!objects.empty()) {
		  scale_width = objects.back().width;
		  scale_height = objects.back().height;
		} else {
		  scale_width = scale_height = 5;
		}
		
		printf("Scale constant: %f x %f\n", scale_width, scale_height);
	      }
	      break;
	      
	    case Key_s:
	      //printf("Key s is pressed\n");
	      cont = true;
	      printf("Input the scale constant:\n");
	      printf("\twidth : ");
	      scanf("%f", &scale_width);
	      printf("\theight: ");
	      scanf("%f", &scale_height);
	      printf("Scale constant: %f x %f\n", scale_width, scale_height);
	      break;
	      
	    case Key_e:
	      //printf("Key e is pressed\n");
	      cont = true;
	      for(int i = 0; i < (int)(objects.size()); i++) {
		printf("%d %d %f %f\n", objects.at(i).x, objects.at(i).y, objects.at(i).width, objects.at(i).height);	
	      }
	      break;
	      
	    case Key_p:
	      //printf("Key p is pressed\n");
	      cont = true;
	      ACCEPT_POINT = !ACCEPT_POINT;
	      printf("ACCEPT_POINT = %s\n", ACCEPT_POINT ? "YES" : "NO");
	      break;
	      
	    case Key_t:
	      //printf("Key t is pressed\n");
	      cont = true;
	      printHelp();
	      break;
	      
	    case Key_j:
	      //printf("Key j is pressed\n");
	      cont = false;
	      printf("Jump to image#: ");
	      scanf("%d", &targetImageIndex);
	      targetImageIndex--;  // because targetImageIndex++ below
	      break;
	      
	    default:
	      if(iKey >= 0) {
		printf("Unrecognised command\n");
		cont = true;
	      } else {
		cvReleaseImage(&image);
		cvDestroyWindow(window_name);
		fclose(output);
		return 0;
	      }
	    }
	  } while(cont);
	  
	  counter++;
	  targetImageIndex++;
	  cvDestroyWindow(window_name);
	  cvReleaseImage(&image);
	}
	
	fclose(output);
  
	return 0;
}

#endif
