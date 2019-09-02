/*
*		File: Cam_Filter.h
*		Purpose: The header file containing useful defines and function prototypes for related source file
*		Owner: Poorn Mehta
*		Last Modified: 8/11/2019
*/

#ifndef	__CAM_FILTER_H__
#define __CAM_FILTER_H__

// The maximum number of frames that are allowed to be captured to find indicator shift
#define Test_Frames		(uint32_t)90

// The range of pixel value in both sides that is ignored (range = 2 * Pix_Allowed_Val)
#define Pix_Allowed_Val		(uint8_t)6

// Limits of uint8_t number
#define Pix_Max_Val		(uint8_t)255
#define Pix_Min_Val		(uint8_t)0

// Thresholds when the program is running this function at 1Hz
#define Diff_Thr_Low_1Hz	(float)(0.15) //0.15
#define Diff_Thr_High_1Hz	(float)(0.6) //0.6

// Thresholds when the program is running this function at 10Hz (Currently not supported)
#define Diff_Thr_Low_10Hz	(float)(0.45)
#define Diff_Thr_High_10Hz	(float)(0.75)

// Offset in milliseconds which is to be referred for further frame captures - 1Hz and 10Hz respectively
#define Offset_1Hz_ms		(uint32_t)(500)
#define Offset_10Hz_ms		(uint32_t)(50)

// The offset in seconds to simply give more time to program before starting continuous capture
#define Filter_Timestamp_Offset_s	(uint8_t)2

// Function prototype
void Cam_Filter(void);

#endif
