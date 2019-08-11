#ifndef	__CAM_RGB_H__
#define __CAM_RGB_H__

#define RGB_Complete_Mask	(uint8_t)(1 << 0)

#define HRES_STR "640"
#define VRES_STR "480"

#define Deadline_Overhead_Factor	(float)1.01

#define PPM_Header_Max_Length	250

void *Cam_RGB_Func(void *para_t);

#endif
