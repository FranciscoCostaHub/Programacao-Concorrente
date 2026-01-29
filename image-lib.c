#include "image-lib.h"
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include <time.h>

/******************************************************************************
 * smooth_image()
 *
 * Arguments: in - pointer to image
 * Returns: out - pointer to smoother image, or NULL in case of failure
 * Side-Effects: none
 *
 * Description: creates clone of image smoother
 *
 *****************************************************************************/
gdImagePtr  blur_image(gdImagePtr in_img){
	
	gdImagePtr out_img;
	

	out_img = gdImageCopyGaussianBlurred(in_img, 20, -1);


	if (!out_img) {
		return NULL;
	}

	return(out_img);		
} 


/******************************************************************************
 * contrast_image()
 *
 * Arguments: in - pointer to image
 * Returns: out - pointer to high contrast  image, or NULL in case of failure
 * Side-Effects: none
 *
 * Description: creates clone of image but its contrast reduced
 *
 *****************************************************************************/
gdImagePtr  contrast_image(gdImagePtr in_img){
	
	gdImagePtr out_img;
	
	out_img =  gdImageClone (in_img);
	if (!out_img) {
		return NULL;
	}

	gdImageContrast(out_img, -20);


	return(out_img);		
} 


/******************************************************************************
 * sepia_image()
 *
 * Arguments: in - pointer to image
 * Returns: out - pointer to sepia image, or NULL in case of failure
 * Side-Effects: none
 *
 * Description: creates clone of image but with sepia color
 *
 *****************************************************************************/
gdImagePtr  sepia_image(gdImagePtr in_img){
	
	gdImagePtr out_img;
	
	out_img =  gdImageClone (in_img);
	if (!out_img) {
		return NULL;
	}

	gdImageColor(out_img, 120, 70, 0, 0);


	return(out_img);		
} 


/******************************************************************************
 * thumb_image()
 *
 * Arguments: in - pointer to image
 * Returns: out - pointer to thumbnail image, or NULL in case of failure
 * Side-Effects: none
 *
 * Description: creates clone of image but smaller
 *
 *****************************************************************************/
gdImagePtr  thumb_image(gdImagePtr in_img){
	
	gdImagePtr out_img;
	
	int width,heigth;

	width = in_img->sx / 5;
	heigth = in_img->sy / 5;

	out_img = gdImageScale(in_img, width, heigth);
	if (!out_img) {
		return NULL;
	}

	return(out_img);		
}


/******************************************************************************
 * gray_image()
 *
 * Arguments: in - pointer to image
 * Returns: out - pointer to grayscale image, or NULL in case of failure
 * Side-Effects: none
 *
 * Description: creates clone of image but in grayscale
 *
 *****************************************************************************/
gdImagePtr  gray_image(gdImagePtr in_img){
	
	gdImagePtr out_img;
	
	out_img =  gdImageClone (in_img);
	if (!out_img) {
		return NULL;
	}

	gdImageGrayScale(out_img);
	return(out_img);
}


/******************************************************************************
 * read_jpeg_file()
 *
 * Arguments: file_name - name of file with data for JPEG image
 * Returns: img - the image read from file or NULL if failure to read
 * Side-Effects: none
 *
 * Description: reads a JPEG image from a file
 *
 *****************************************************************************/
gdImagePtr read_jpeg_file(char * file_name){

	FILE * fp;
	gdImagePtr read_img;

	fp = fopen(file_name, "rb");
   	if (!fp) {
        fprintf(stderr, "Can't read image %s\n", file_name);
        return NULL;
    }
    read_img = gdImageCreateFromJpeg(fp);
    fclose(fp);
  	if (read_img == NULL) {
    	return NULL;
    }

	return read_img;
}

/******************************************************************************
 * write_jpeg_file()
 *
 * Arguments: img - pointer to image to be written
 *            file_name - name of file where to save PNG image
 * Returns: (bool) 1 in case of success, 0 in case of failure to write
 * Side-Effects: none
 *
 * Description: writes a JPEG image to a file
 *
 *****************************************************************************/
int write_jpeg_file(gdImagePtr write_img, char * file_name){
	FILE * fp;

	fp = fopen(file_name, "wb");
	if (fp == NULL) {
		return 0;
	}
	gdImageJpeg(write_img, fp, 70);
	fclose(fp);

	return 1;
}


/******************************************************************************
 * create_directory()
 *
 * Arguments: file_name - name of directory to be created
 * Returns: (bool) 1 if the directory already exists or succesfully created
 *                 0 in case of failure to create
 * Side-Effects: none
 *
 * Description: Create a directory. 
 *
 *****************************************************************************/
int create_directory(char * dir_name){

	DIR * d = opendir(dir_name);
	if (d == NULL){
		if (mkdir(dir_name, 0777)!=0){
			return 0;
		}
	}else{
		fprintf(stderr, "%s directory already existent\n", dir_name);
		closedir(d);
	}
	return 1;
}



/******************************************************************************
 * diff_timespec()
 *
 * Arguments: time 1 - structure containg the time seasure with  clock_gettime
 *            time 2 - structure containg the time seasure with  clock_gettime
 * Returns: (struct timespec) 
 * Side-Effects: none
 *
 * Description: This functino subtracts the two received times and returns the result
 *
 *****************************************************************************/
struct timespec diff_timespec(const struct timespec *time1, const struct timespec *time0) {
  assert(time1);
  assert(time0);
  struct timespec diff = {.tv_sec = time1->tv_sec - time0->tv_sec, //
      .tv_nsec = time1->tv_nsec - time0->tv_nsec};
  if (diff.tv_nsec < 0) {
    diff.tv_nsec += 1000000000; // nsec/sec
    diff.tv_sec--;
  }
  return diff;
}
