#ifndef	__CAM_FILTER_H__
#define __CAM_FILTER_H__

#define Test_Frames		(uint32_t)90

#define Pix_Allowed_Val		(uint8_t)6
#define Pix_Max_Val		(uint8_t)255
#define Pix_Min_Val		(uint8_t)0

#define Diff_Thr_Low_1Hz		(float)(0.1)
#define Diff_Thr_High_1Hz		(float)(0.4)

#define Diff_Thr_Low_10Hz		(float)(0.3)
#define Diff_Thr_High_10Hz		(float)(0.5)

#define Offset_1Hz_ms		(uint32_t)(500)
#define Offset_10Hz_ms		(uint32_t)(50)

#define Filter_Timestamp_Offset_s	(uint8_t)2

void Cam_Filter(void);

#endif
