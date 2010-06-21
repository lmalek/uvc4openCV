/*******************************************************************************
$HeadURL$
$Author$
$Date$
$Revision$
*******************************************************************************/

#include <cv.h>
#include <highgui.h>
#include <cstdio>


#include "v4l2uvc.h"
#include "utils.h"
#include "color.h"

void RGB2IplImage(IplImage *img,
		  const unsigned char* rgbArray, 
		  unsigned int width, 
		  unsigned int height){
  int i,j;
  // zamienione sa kolory R i B
  for(i=0;i<height;i++) {
      for(j=0;j<width;j++) {
	  ((uchar *)(img->imageData + i*img->widthStep))[j*img->nChannels + 2]=
	    *(rgbArray+i*width*3+j*3+0);
	  ((uchar *)(img->imageData + i*img->widthStep))[j*img->nChannels + 1]=
	    *(rgbArray+i*width*3+j*3+1);
	  ((uchar *)(img->imageData + i*img->widthStep))[j*img->nChannels + 0]=
	    *(rgbArray+i*width*3+j*3+2);
	}
  }
}

struct vdIn *videoIn;

// A Simple Camera Capture Framework
int main(int argc, char* argv[]) {
  const char *mode = NULL; // nazwa formatu obrazu - do parsowania
			   // wywołania
  char *separateur; // serarator - do parsowania
  char *sizestring = NULL; // rozmiar obrazu - do parsowania
  char *fpsstring  = NULL; // klatki na sekunde - do parsowania
  int i; // iterator potrzebny do parsowania 
  
  const char *videodevice = NULL; // nazwa urzadzenia 

  IplImage * img=NULL; // obraz OpenCV
  int format = V4L2_PIX_FMT_YUYV; // format obrazu
  int grabmethod = 1; 
  int width = 640; 
  int height = 480;
  int fps = 30;

  short pan; // zmiana w poziomie 64 = jeden stopien
  short tilt; // zmiana w pionie 64 = jeden stopien
  int key; // cwisniety klawisz
  bool quit=false; // czy koniec programu
  
  printf("uvc2openCV \n\n");
  // parsowanie argumentow uruchomienia
  for (i = 1; i < argc; i++) {
    /* skip bad arguments */
    if (argv[i] == NULL || *argv[i] == 0 || *argv[i] != '-') { 
      continue; 
    }
    if (strcmp(argv[i], "-d") == 0) { // urzadzenie video
      if (i + 1 >= argc) {
	printf("No parameter specified with -d, aborting.\n");
	exit(1);
      }
      videodevice = strdup(argv[i + 1]);
    }
    if (strcmp(argv[i], "-f") == 0) { // format
      if (i + 1 >= argc) {
	printf("No parameter specified with -f, aborting.\n");
	exit(1);
      }
      mode = argv[i + 1];
      // jedynie YUYV jest w tym przykladzie obsluzony
      if (strcasecmp(mode, "yuv") == 0 || strcasecmp(mode, "YUYV") == 0) {
	format = V4L2_PIX_FMT_YUYV;
      } else if (strcasecmp(mode, "jpg") == 0 || strcasecmp(mode, "MJPG") == 0) {
	format = V4L2_PIX_FMT_MJPEG;
      } else {
	printf("Unknown format specified. Aborting.\n");
	exit(1);
      }
    }
    if (strcmp(argv[i], "-s") == 0) { // rozmiar obrazu np 640x480
      if (i + 1 >= argc) {
	printf("No parameter specified with -s, aborting.\n");
	exit(1);
      }
      sizestring = strdup(argv[i + 1]);
      width = strtoul(sizestring, &separateur, 10);
      if (*separateur != 'x') {
	printf("Error in size use -s widthxheight\n");
	exit(1);
      } else {
	++separateur;
	height = strtoul(separateur, &separateur, 10);
	if (*separateur != 0)
	  printf("hmm.. dont like that!! trying this height\n");
      }
    }
    if (strcmp(argv[i], "-i") == 0){  // FPS - klatki na sekunde
      if (i + 1 >= argc) {
	printf("No parameter specified with -i, aborting.\n");
	exit(1);
      }
      fpsstring = argv[i + 1];
      fps = strtoul(fpsstring, &separateur, 10);
      if(*separateur != '\0') {
	printf("Invalid frame rate '%s' specified with -i. "
	       "Only integers are supported. Aborting.\n", fpsstring);
	exit(1);
      }
    }
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      printf("usage: uvcview [-h -d -f -s -i]\n");
      printf("-h	print this message\n");
      printf("-d	/dev/videoX       use videoX device\n");
      printf("-f	choose video format (YUYV/yuv and MJPG/jpg are valid, MJPG is default)\n");
      printf("-i	fps           use specified frame interval\n");
      printf("-s	widthxheight      use specified input size\n");
      exit(0);
    }
  }
  // koniec parsowania parametrow


  if (videodevice == NULL || *videodevice == 0) {
    videodevice = "/dev/video0";  // domyslna kamera
  }

  cvNamedWindow( "uvc4openCV", CV_WINDOW_AUTOSIZE );

  videoIn = (struct vdIn *) calloc(1, sizeof(struct vdIn)); 
  // inicjalizacja wejscia wideo
  if (init_videoIn(videoIn, (char *) videodevice, width, height, fps, format,grabmethod) < 0)
    exit(1);
  // inicjalizacja obrazu potrzebnego do zmiany formatu z UYV422 do RBG24
  unsigned char *picture = (unsigned char *)malloc(width * height *3*sizeof(char));
  img = cvCreateImage(cvSize(width,height),IPL_DEPTH_8U,3);   // obraz OpenCV
  
  v4l2ResetPanTilt(videoIn);
  // Poberz obraz i wyswetl go 
  while( !quit ) {
    if (uvcGrab(videoIn) < 0) {     // pobierz obraz
      printf("Blad pobierania obrazu\n");
      break;
    }

    initLut(); // potrzebne wlaczenie LUT do konwersji UYV422 do RBG24
    // na pewno da sie przerobic aby dzialalo bez tego, ale juz mi sie
    // nie che
    if(picture){ // jesli jest zarezerwowana pamiec to konwertujemy do RGB24
      Pyuv422torgb24( videoIn->framebuffer, picture, videoIn->width, videoIn->height);
    }
    freeLut();

    RGB2IplImage(img,picture, width, height); // konwertujemy do OpenCV
    usleep(10000); // 10 [ms]
  
    if( !img ) {
      fprintf( stderr, "Blad konwersji na obraz OpenCV\n" );
      getchar();
      break;
    }

    cvShowImage( "uvc4openCV", img ); // wyswietl obraz w OpenCV


    // obsluga klawiatury
    //If ESC key pressed, Key=0x10001B under OpenCV 0.9.7(linux version),
    //remove higher bits using AND operator
    key=cvWaitKey(10)&255;
    pan=tilt=0;
    switch(key){
    case 27: quit=true; break; // klawisz = ESC
    case 81: pan=2*64;tilt=2*64; break; // strzalka lewa
    case 83: pan=-2*64;tilt=2*64; break; // strzalka prawa
    case 84: tilt=2*64;pan=-2*64; break; // strzalka gora
    case 82: tilt=-2*64;pan=2*64; break; // strzalka dol 
    case 114: v4l2ResetPanTilt(videoIn); break; // klawisz = r | RESET
    }
    if (key>80 && key<85) {// jesli ktoras ze strzalek to zmieniamy polozenie
      v4L2UpDownPanTilt(videoIn, pan, tilt);
    }
      
    //if( (key & 255) == 27 ) break;

  }

  // Release the capture device housekeeping
  cvDestroyWindow( "mywindow" );
  return 0;
}
