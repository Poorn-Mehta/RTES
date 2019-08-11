/*
*		File: Cam_RGB.h
*		Purpose: The header file containing useful defines and function prototypes for related source file
*		Owner: Poorn Mehta
*		Last Modified: 8/11/2019
*/

#ifndef	__CAM_RGB_H__
#define __CAM_RGB_H__

// Bit Mask to indicate whether RGB processing has been completed or not
#define RGB_Complete_Mask	(uint8_t)(1 << 0)

// String to be embedded inside the image file header
#define HRES_STR "640"
#define VRES_STR "480"

// The factor used to ignore a tiny fraction of overhead observed
#define Deadline_Overhead_Factor	(float)1.01

// The maximum possible length of image header
#define PPM_Header_Max_Length	250

// Function prototype
void *Cam_RGB_Func(void *para_t);

#endif
