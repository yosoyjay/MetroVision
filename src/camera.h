/* 
 * File:   camera.h
 * Author: Jesse
 *
 * Created on March 18, 2010, 11:01 AM
 */

#ifndef _PLAYER_
#define _PLAYER_
#include <libplayerc++/playerc++.h>
#endif

#ifndef _CAMERA_H
#define	_CAMERA_H

#include "highgui.h"
#include "cv.h"

class Camera
{
public:
	Camera(PlayerCc::PlayerClient &robot,
	       PlayerCc::BlobfinderProxy &blobProxy,
	       PlayerCc::CameraProxy &camProxy);

	IplImage *edgeDetection(IplImage* in,
							double lowThresh,
							double highThresh,
							double aperture);

	IplImage *doCanny(IplImage* in,
					  double lowThresh,
					  double highThresh,
					  double aperture);
	IplImage *captureImage();

	void	drawBlobs(IplImage* in);
	void 	displayStream(char* windowName);
	void 	displayBlobs(char* windowName);
	void 	displayTetris(char* windowName);
    void 	displayHaar(char* windowName);
	void 	captureStream(char* windowName);
	double 	angle( CvPoint* pt1, CvPoint* pt2, CvPoint* pt0 );
	CvSeq 	*findTetris( IplImage* img, CvMemStorage* storage );
	void 	drawTetris( IplImage* img, CvSeq* tetrisPieces );
    void 	loadCascade(const char* cascadeName);
	void 	detectAndDrawHaar(IplImage* img, double scale = 1.3);

	//int findBlob();
	//CvPoint blobPosition(int i);
	~Camera();
private:
	/* Player related */
    PlayerCc::PlayerClient 		*robot;
	PlayerCc::BlobfinderProxy 	*blobProxy;
	PlayerCc::CameraProxy 		*camProxy;
    static const int 			PLAYERCAM_MAX_BLOBS = 256;
    player_blobfinder_blob_t 	blobs[PLAYERCAM_MAX_BLOBS];

	/* OpenCV related */
	CvHaarClassifierCascade  	*cascade;
	IplImage 		*frame;
	uint8_t 		*imgBuffer;
	CvMemStorage    *storage;
	int 			thresh;
	const char 		*cascadeName;
	CvMemStorage    *storageCascade;
};

#endif	/* _CAMERA_H */


