/*
*		File: Aux_Func.h
*		Purpose: The header file containing useful defines and function prototypes for related source file
*		Owner: Poorn Mehta
*		Last Modified: 8/11/2019
*/

#ifndef	__AUX_FUNC_H__
#define __AUX_FUNC_H__

// Used to check before attaching a thread to requested CPU core
#define No_of_Cores	(uint8_t)4

// Time stamp function modes
#define Mode_sec	(uint8_t)1
#define Mode_ms		(uint8_t)2
#define Mode_us		(uint8_t)3

// Supported resolutions
#define Res_960_720	(uint8_t)0
#define Res_800_600	(uint8_t)1
#define Res_640_480	(uint8_t)2
#define Res_320_240	(uint8_t)3
#define Res_160_120	(uint8_t)4

// Some converstion numbers
#define s_to_ns		(uint32_t)1000000000
#define s_to_us		(uint32_t)1000000
#define s_to_ms		(uint32_t)1000
#define ms_to_ns	(uint32_t)1000000
#define ms_to_us	(uint32_t)1000
#define us_to_ns	(uint32_t)1000
#define ns_to_us	(float)0.001
#define ns_to_ms	(float)0.000001
#define ns_to_s		(float)0.000000001

// Function prototypes
uint8_t Select_Resolution(uint8_t Res_Setting);
void Show_Analysis(void);
float Time_Stamp(uint8_t mode);
void Set_Logger(char *logname, int level_upto);
uint8_t Bind_to_CPU(uint8_t Core);
uint8_t Realtime_Setup(void);

#endif
