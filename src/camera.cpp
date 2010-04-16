#include "camera.h"

Camera::Camera(PlayerCc::PlayerClient &robot,
	       PlayerCc::BlobfinderProxy &blobProxy,
	       PlayerCc::CameraProxy &camProxy)
{
    this->robot = &robot;
    this->blobProxy = &blobProxy;
    this->camProxy = &camProxy;
}

/** Captures image from the Player Camera proxy and converts it
   to standard BGR IplImage format
 */
IplImage* Camera::captureImage(){
	
	robot->Read();

	int image_count = camProxy->GetImageSize();
	printf("image_count: %d\n", image_count);

	// Replace this buffer with the OpenCV data struct
	// that can be reused to avoid create - destroy every cycle
	imgBuffer = new uint8_t[image_count];

	// Set width and height
	int image_width = camProxy->GetWidth();
	int image_height = camProxy->GetHeight();

	// Create empty image of appropriate size
	frame = cvCreateImage(cvSize(image_width,image_height), 8, 3);

	// Get image from Camera Proxy from Player
	camProxy->GetImage(imgBuffer);

	// Copy data from the image buffer into frame & convert RGB to BGR
	// Look around for faster dense matrix twist transformation
	for (int i = 0; i < image_width; i++){
		for (int j = 0; j < image_height; j++){
			frame->imageData[image_width * j*3 + i*3 + 0] =
			       (char)imgBuffer[image_width * j*3 + i*3 + 2];
			frame->imageData[image_width * j*3 + i*3 + 1] =
			       (char)imgBuffer[image_width * j*3 + i*3 + 1];
			frame->imageData[image_width * j*3 + i*3 + 2] =
			       (char)imgBuffer[image_width * j*3 + i*3 + 0];
		}
	}

	delete[] imgBuffer;
	return frame;
}

/** Performs canny detection
  on input image and returns canny detected image.  Perform canny detection
  on new image so that it is non-destructive to original image.
 */
IplImage* Camera::doCanny(IplImage* in, double lowThresh, double highThresh, double aperture){
    if(in->nChannels != 1)
        return(0);
    IplImage* out = cvCreateImage(cvSize(in->width, in->height), IPL_DEPTH_8U, 1);
    cvCanny(in, out, lowThresh, highThresh, aperture);
    return(out);
}

/** Converts IplImage* in to 1-channel to use doCanny().
  Just a convenience function to perform canny detection
  on a 3-channel IplImage*.  The high and low thresholds
  determine the range and sensitivity of edge detection.
  Changing aperture from 3 casues segementation faults.
 */
IplImage* Camera::edgeDetection(IplImage* in, double lowThresh, double highThresh, double aperture){
	// Convert image to grayscale because Canny requires 1-channel
	// Copy to new image so it is a non-destructive change
	IplImage* gray = cvCreateImage( cvGetSize(in), 8, 1);
	cvCvtColor(in, gray, CV_BGR2GRAY);

	gray = doCanny(gray, lowThresh, highThresh, aperture);

	return gray;
}

/** Takes an image and performs blobdetection using the CMVision Player
   driver.  Requires information provided by the camera (duh) and
   blobfinder proxies.  Does not display blobs, merely draws rectangles
   around blobs identified by the blobfinder proxy.  If you want to see them
   try using the displayBlobs() function.
 */
void Camera::drawBlobs(IplImage* image){

	// global values for blobfinding
	int 	blobCount = 0;
	int	blobIndex = 0;
	int  	red = 0;
	int	green = 0;
	int 	blue = 0;

	CvScalar blobColor;

	blobCount = PLAYERCAM_MAX_BLOBS < blobProxy->GetCount() ? PLAYERCAM_MAX_BLOBS
						: blobProxy->GetCount();
	printf("blobs: %d  ", blobCount);

	for(int i = 0; i < blobCount; i++){
		blobs[i] = blobProxy->GetBlob(i);
	}

	for(int i = 0; i < blobCount; i++){
		// Need to get colors from blob to create box
		red = (blobs[i].color & 0x00FF0000) >> 16;
		green = (blobs[i].color  & 0x0000FF00) >> 8;
		blue = blobs[i].color & 0x000000FF;
		
		printf("blob color: %x %x %x %x\n", blobs[i].color, red, green, blue);

		cvRectangle(image, cvPoint(blobs[i].left, blobs[i].top),
				   cvPoint(blobs[i].right, blobs[i].bottom),
				   cvScalar(blue, green, red));
	}

	return;
}

/** Displays images from the Player camera proxy.

 */
void Camera::displayStream(char* windowName){

	cvNamedWindow(windowName,0);

	// Esc breaks loop.  Highgui is responding strangly to close window button
	while (1){
		char c = cvWaitKey(33);
		frame = captureImage();
		cvShowImage(windowName, frame);
		cvReleaseImage(&frame);
		if( c == 27) break;
	}

	cvDestroyWindow(windowName);
	return;
}

/** Displays images from the Player camera proxy.

 */
void Camera::captureStream(char* windowName){

	cvNamedWindow(windowName,0);
	int count=0;
	char imageName[] = "image000.bmp";
	// Esc breaks loop.  Highgui is responding strangly to close window button
	while ( 1 ){

		frame = captureImage();
	    char c = cvWaitKey(33);
		if(c == 112){
			IplImage* tmpImg;
			// Clone frame as destination for saving needs
			// to be the exact same size as source.
			// Perhaps there is a less expensive way?
			// Like just allocating the space. Make that a to do.
			tmpImg = cvCloneImage(frame);
			cvCvtColor(frame, tmpImg, CV_BGR2RGB);
			//name += 2*11;
			imageName[7] = (char)(count % 10+48);
			if(count >= 10 )
				imageName[6] =(char)(count / 10+48);
			if(count >= 100 )
				imageName[5] =(char)(count / 100+48);
			count++;
			if(cvSaveImage(imageName, frame) == 0){
				printf("Error saving image");
			}
			printf("Saved %s\n", imageName);
		}
		cvShowImage(windowName, frame);
		cvReleaseImage(&frame);
		if(c == 27) break;
	}

	cvDestroyWindow(windowName);
	return;
}

/** Displays images that have had blobdetection performed on them.

 */
void Camera::displayBlobs(char* windowName){

	cvNamedWindow(windowName,0);
	// Esc breaks loop.  Highgui is responding strangly to close window button
	while (1) {

		frame = captureImage();
		drawBlobs(frame);
		cvShowImage(windowName, frame);
		cvReleaseImage(&frame);

		char c = cvWaitKey(33);
		if( c == 27 ) break;
	}

	cvDestroyWindow(windowName);
	return;
}

/** Displays images that have had tetrisdetection performed on them.

 */

void Camera::displayTetris(char* windowName){

    // create memory storage that will contain all the dynamic data
   	storage = cvCreateMemStorage(0);

	cvNamedWindow(windowName,0);
	// Esc breaks loop.  Highgui is responding strangly to close window button
	while ( cvWaitKey(33) != 27 ){
		frame = captureImage();
        drawTetris(frame,findTetris(frame, storage));
		cvShowImage(windowName, frame);
		cvReleaseImage(&frame);
	}
	cvDestroyWindow(windowName);
	return;
}

/** Displays images from the Player camera proxy.

 */
void Camera::displayHaar(char* windowName){

	cvNamedWindow(windowName,0);

	// Esc breaks loop.  Highgui is responding strangly to close window button
	while ( cvWaitKey(33) != 27 ){

		frame = captureImage();
		detectAndDrawHaar(frame, 1.3);
		cvShowImage(windowName, frame);
		cvReleaseImage(&frame);
	}

	cvDestroyWindow(windowName);
	return;
}


/** This helper function finds the cosine of an angle between two vectors
   from pt0 to pt1 and then pt0 to pt2. Used by findTetris() to identify
   right angles.
   From OpenCV examples.
 */
double Camera::angle( CvPoint* pt1, CvPoint* pt2, CvPoint* pt0 )
{
    double dx1 = pt1->x - pt0->x;
    double dy1 = pt1->y - pt0->y;
    double dx2 = pt2->x - pt0->x;
    double dy2 = pt2->y - pt0->y;
    return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

/** Returns a CvSeq (An OpenCV sequence) of Tetris pieces detected in an image.
   Based on the OpenCV example of identifying a square.  Modified to detect
   L-shaped Tetris pieces.  Effectiveness dependent upon thresholds of edge
   dectection and camera being positioned orthogonal to the Tetris piece.
 */
CvSeq* Camera::findTetris( IplImage* img, CvMemStorage* storage )
{
	thresh = 50;
    CvSeq* contours;
    int i, c, l, N = 11;
    CvSize sz = cvSize( img->width & -2, img->height & -2 );

	/// Copy of image so that the detection is non-destructive
    IplImage* timg = cvCloneImage( img );

	/// Gray scale needed
	IplImage* gray = cvCreateImage( sz, 8, 1 );

	/// Smaller version to do scaling
    IplImage* pyr = cvCreateImage( cvSize(sz.width/2, sz.height/2), 8, 3 );
    IplImage* tgray;
    CvSeq* result;
    double s, t;

    // create empty sequence that will contain points -
    /// 6 points per tetris piece (the vertices)
    CvSeq* tetrisPieces = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvPoint), storage );

    // select the maximum region of interest (ROI) in the image
    // with the width and height divisible by 2.  What is the biggest
    // size of the object.
    cvSetImageROI( timg, cvRect( 0, 0, sz.width, sz.height ));

    // down-scale and upscale the image to filter out the noise
	// I get the filter, but why down and upscale?
    cvPyrDown( timg, pyr, 7 );
    cvPyrUp( pyr, timg, 7 );
    tgray = cvCreateImage( sz, 8, 1 );

    /// find pieces in every color plane of the image
    for( c = 0; c < 3; c++ )
    {
        /// extract the c-th color plane
        cvSetImageCOI( timg, c+1 );
        cvCopy( timg, tgray, 0 );

        /// try several threshold levels
        for( l = 0; l < N; l++ )
        {
            /// hack: use Canny instead of zero threshold level.
            /// Canny helps to catch tetrisPieces with gradient shading
            if( l == 0 )
            {
                // apply Canny. Take the upper threshold from slider
                // and set the lower to 0 (which forces edges merging)
                cvCanny( tgray, gray, 50, 120, 5 );
                // dilate canny output to remove potential
                // holes between edge segments
                cvDilate( gray, gray, 0, 1 );
            }
            else
            {
                // apply threshold if l!=0:
                //     tgray(x,y) = gray(x,y) < (l+1)*255/N ? 255 : 0
                cvThreshold( tgray, gray, (l+1)*255/N, 255, CV_THRESH_BINARY );
            }

            // find contours and store them all as a list
            cvFindContours( gray, storage, &contours, sizeof(CvContour),
                CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );

            // test each contour
            while( contours )
            {
                // approximate contour with accuracy proportional
                // to the contour perimeter
                result = cvApproxPoly( contours, sizeof(CvContour), storage,
                    CV_POLY_APPROX_DP, cvContourPerimeter(contours)*0.02, 0 );

				/* Tetris pieces have 6 vertices.  The approximation of large
				 * area is used to filter out "noisy contours."
                // Note: absolute value of an area is used because
                // area may be positive or negative - in accordance with the
                // contour orientation*/
                if( result->total == 6 &&
                    fabs(cvContourArea(result,CV_WHOLE_SEQ)) > 1000 &&
					fabs(cvContourArea(result,CV_WHOLE_SEQ)) < 10000 )
                {
                    s = 0;

                    for( i = 0; i < 7; i++ )
                    {
                        // find minimum angle between joint
                        // edges (maximum of cosine)
                        if( i >= 2 )
                        {
                            t = fabs(angle(
                            (CvPoint*)cvGetSeqElem( result, i ),
                            (CvPoint*)cvGetSeqElem( result, i-2 ),
                            (CvPoint*)cvGetSeqElem( result, i-1 )));
                            s = s > t ? s : t;
                        }
                    }

                    // if cosines of all angles are small
                    // (all angles are ~90 degree) then write quandrange
                    // vertices to resultant sequence
                    if( s < 0.3 )
                        for( i = 0; i < 6; i++ )
                            cvSeqPush( tetrisPieces,
                                (CvPoint*)cvGetSeqElem( result, i ));
                }

                // take the next contour
                contours = contours->h_next;
            }
        }
    }

    // release all the temporary images
    cvReleaseImage( &gray );
    cvReleaseImage( &pyr );
    cvReleaseImage( &tgray );
    cvReleaseImage( &timg );

    return tetrisPieces;
}

/** Draws the identified Tetris pieces on the given image.
 */
void Camera::drawTetris( IplImage* img, CvSeq* tetrisPieces )
{
    CvSeqReader reader;
    int i;

    // initialize reader of the sequence
    cvStartReadSeq( tetrisPieces, &reader, 0 );

    // read the pieces sequence elements at a time (all vertices of the piece)
    for( i = 0; i < tetrisPieces->total; i += 6 )
    {
        CvPoint pt[6], *rect = pt;
        int count = 6;

        // read 6 vertices
        CV_READ_SEQ_ELEM( pt[0], reader );
        CV_READ_SEQ_ELEM( pt[1], reader );
        CV_READ_SEQ_ELEM( pt[2], reader );
        CV_READ_SEQ_ELEM( pt[3], reader );
        CV_READ_SEQ_ELEM( pt[4], reader );
        CV_READ_SEQ_ELEM( pt[5], reader );


        // draw the piece as a closed polyline
        cvPolyLine( img, &rect, &count, 1, 1, CV_RGB(255,0,0), 3, CV_AA, 0 );
    }

	return;
}
/** Loads the .xml file that holds the cascade information determined
 	after performing Haar training.
 */
void Camera::loadCascade(const char* cascadeName){
	cascade = (CvHaarClassifierCascade*) cvLoad(cascadeName, 0, 0, 0);
	// Need to add error checking for the file
	storageCascade = cvCreateMemStorage(0);
}


/** Detects and draws The Objects that have been defined in the .xml
 	cascade file.
 */
void Camera::detectAndDrawHaar(IplImage* img, double scale){
	/* In an faux Ict-T voice "Colors" */
    static CvScalar colors[] = {
        {{0,0,255}}, {{0,128,255}}, {{0,255,255}}, {{0,255,0}},
        {{255,128,0}}, {{255,255,0}}, {{255,0,0}}, {{255,0,255}} 
    };

	/* Image needs to be 1-channel (grayscale) and scaled down. I'm
	 * using the scale values used in the OpenCV book as a starting point.
     */
    IplImage* gray = cvCreateImage( cvSize(img->width, img->height), 8,1);
    IplImage* small_img = cvCreateImage( cvSize( cvRound(img->width/scale),
            					 		 cvRound(img->height/scale)), 8, 1);
    cvCvtColor( img, gray, CV_BGR2GRAY);
    cvResize( gray, small_img, CV_INTER_LINEAR);
    cvEqualizeHist( small_img, small_img);

	/* Use built in function to detect if there are any objects */
    cvClearMemStorage(storageCascade);
    CvSeq* objects = cvHaarDetectObjects(
            small_img,  
            cascade,    
            storageCascade,
            1.1,
            2,
            0,
            cvSize(30,30)
    );

    /* Iterate through all objects and draw a box around them */
    for( int i = 0; i < (objects ? objects->total: 0); i++){
        CvRect* r = (CvRect*) cvGetSeqElem( objects, i);
        cvRectangle(
        	img,
            cvPoint(r->x,r->y),
            cvPoint(r->x+r->width, r->y+r->height),
            colors[i%8]
        );
    }
    cvReleaseImage( &gray);
    cvReleaseImage( &small_img);
   
}





/*
CvPoint Camera::blobPosition(int i){

	CvPoint centroid;

	centroid.x = blobs[i].x;
	centroid.y = blobs[i].y;
	
    return centroid;
    
}
  Return true if there is a blob of color c.
int Camera::findBlob(){

	// global values for blobfinding
	int 	blobCount = 0;
	int	blobIndex = 0;
	int  	red 	  = 0;
	int	green     = 0;
	int 	blue      = 0;

	player_blobfinder_blob_t blobs[PLAYERCAM_MAX_BLOBS];

	CvScalar blobColor;

	blobCount = PLAYERCAM_MAX_BLOBS < blobProxy->GetCount() ? PLAYERCAM_MAX_BLOBS
						: blobProxy->GetCount();

	for(int i = 0; i < blobCount; i++){
		blobs[i] = blobProxy->GetBlob(i);
	}
	
	for(int i = 0; i < blobCount; i++){
		if(blobColor == blobs[i].color){
			return 1;
		}
	}
}*/
