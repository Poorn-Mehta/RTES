/*
*		File: Cam_Filter.c
*		Purpose: The source file containing functions to search and mark a blurry frame
*		Owner: Poorn Mehta
*		Last Modified: 8/11/2019
*
*		A few of the functions in this source file have been developed with reference to the
*		code developed by Professor Sam Siewert. The source code can be found in the below link.
*		http://ecee.colorado.edu/~ecen5623/ecen/ex/Linux/computer-vision/simple-capture/
*
*/

#include "main.h"
#include "Aux_Func.h"
#include "Cam_Func.h"
#include "Cam_Filter.h"
#include "Cam_RGB.h"
#include "Cam_Monitor.h"

// Shared Variables
uint8_t data_buffer[No_of_Buffers][Big_Buffer_Size], Operating_Mode;
frame_p_buffer shared_struct;
struct timespec prev_t;

// Local Variables
static uint32_t frames, pix_cnt, diff_pix_cnt, Offset_ms, Grey_Buffer_Size;
static uint8_t resp, buf_index;
static int dumpfd, written, total;
static store_struct file_out[Test_Frames];
static char pgm_header[]="P5\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
static char pgm_dumpname[]="img/ts/test00000000.pgm";
static float diff_percent, Diff_Thr_Low, Diff_Thr_High;
static struct timespec test_timestamps[Test_Frames];

// Function to store the given frame number
// Parameter1: uint32_t frame_no - the number of frame which is to be stored
// Return: void
static void store_test_frame(uint32_t frame_no)
{
	// Create the file
	dumpfd = open(&file_out[frame_no].filename[0], O_WRONLY | O_NONBLOCK | O_CREAT, 00666);
	

	// Write header in file
	written=write(dumpfd, &file_out[frame_no].header[0], file_out[frame_no].headersize);

	total=0;

	// Write the entire buffer having the image information to the file
	do
	{
		written=write(dumpfd, file_out[frame_no].dataptr, file_out[frame_no].filesize);
		total+=written;
	} while(total < file_out[frame_no].filesize);

	// Close file
	close(dumpfd);

	syslog(LOG_INFO, "<%.6fms>!!Cam_Filter!! Frame: %d Stored to Flash", Time_Stamp(Mode_ms), frame_no);
}

// Function to store the given frame number
// Parameter1: uint8_t *pix1 - pointer to first pixel which is to be compared
// Parameter2: uint8_t *pix2 - pointer to second pixel which is to be compared
// Return: uint8_t result - 0: pixels are not found to be different (the values lie in the given threshold)
static uint8_t cmp_pix(uint8_t *pix1, uint8_t *pix2)
{

	// Check first if pixel 1's value is close enough to the Maximum possible value
	// And that if the full threshold is added, it will overflow
	if(*pix1 >= (Pix_Max_Val - Pix_Allowed_Val))
	{
		// If it is the case, and if pixel 2's value is in range
		// The elements are same
		if(*pix2 >= (*pix1 - Pix_Allowed_Val))
		{
			return 0;
		}

		// Otherwise they're out of range and different
		else
		{
			return 1;
		}
	}

	// Similar logic to above, only reverse in a way
	else if(*pix1 <= (Pix_Min_Val + Pix_Allowed_Val))
	{
		if(*pix2 <= (*pix1 + Pix_Allowed_Val))
		{
			return 0;
		}

		else
		{
			return 1;
		}
	}

	// If none of those conditions met, then check if they're within tolerable distance from each other
	else
	{
		if((*pix2 <= (*pix1 + Pix_Allowed_Val)) && (*pix2 >= (*pix1 - Pix_Allowed_Val)))
		{
			return 0;
		}

		else
		{
			return 1;
		}
	}

}

// Function to compare two consecutive frames - pixel by pixel
// And if the difference if found, then to provide a reference time for scheduler
// Parameter1: uint32_t frame1 - first frame number
// Parameter2: uint32_t frame2 - second frame number
// Return: uint8_t result - 0: match found, and reference time provided
static uint8_t consecutive_cmp(uint32_t frame1, uint32_t frame2)
{
	// Reset the number of different pixels found
	diff_pix_cnt = 0;

	// Compare 2 frames pixel by pixel, and keeping track of number of pixels that were found to be different
	for(pix_cnt = 0; pix_cnt < (Grey_Buffer_Size); pix_cnt ++)
	{
		if(cmp_pix(&data_buffer[frame1][pix_cnt], &data_buffer[frame2][pix_cnt]) != 0)
		{
			diff_pix_cnt += 1;
		}
	}

	// Convert the number into percentages 
	diff_percent = (((float)diff_pix_cnt) / ((float)Grey_Buffer_Size)) * 100;

	syslog(LOG_INFO, "<%.6fms>!!Cam_Filter!! Found %.3f percent different pixels between frame %d and %d", Time_Stamp(Mode_ms), diff_percent, frame1, frame2);

	// When the indicator position change is found, log it, set the time stamp, and store those 2 frames for manual inspection
	if((diff_percent >= Diff_Thr_Low) && (diff_percent <= Diff_Thr_High))
	{
		syslog(LOG_INFO, "<%.6fms>!!Cam_Filter!! ******Indicator Change Detected (%d and %d)******", Time_Stamp(Mode_ms), frame1, frame2);

		// Adjusting reference time for the scheduler
		prev_t.tv_sec = test_timestamps[frame2].tv_sec + Filter_Timestamp_Offset_s;
		prev_t.tv_nsec = test_timestamps[frame2].tv_nsec;

		if(prev_t.tv_nsec < (s_to_ns - (Offset_ms * ms_to_ns)))
		{
			prev_t.tv_sec = prev_t.tv_sec;
			prev_t.tv_nsec = (uint32_t)(prev_t.tv_nsec + (Offset_ms * ms_to_ns));
		}

		else
		{
			prev_t.tv_sec = prev_t.tv_sec + 1;
			prev_t.tv_nsec = (uint32_t)((Offset_ms * ms_to_ns) - (s_to_ns - prev_t.tv_nsec));
		}

		syslog(LOG_INFO, "<%.6fms>!!Cam_Filter!! ******Chosen Time: %ldsec %dmsec******", Time_Stamp(Mode_ms), prev_t.tv_sec, (uint32_t)(prev_t.tv_nsec * ns_to_ms));

		// Store both frames for difference
		store_test_frame(frame1);
		store_test_frame(frame2);

		return 0;
	}

	return 1;
}

// Originally written by Professor Sam Siewert as per mentioned on Top of this File
// Function to storage frame information, including timestamp, in a local circular buffer
// Parameter1: int size - size of the converted image
// Parameter2: uint32_t tag - frame number
// Parameter3: struct timespec *time - pointer to structure which holds the capture time of the image
// Return: void
static void dump_pgm(int size, uint32_t tag, struct timespec *time)
{

	test_timestamps[tag].tv_sec = time->tv_sec;
	test_timestamps[tag].tv_nsec = time->tv_nsec;

	// Store timestamp and resolution in the header
	snprintf(&pgm_header[4], 11, "%010d", (int)time->tv_sec);
	strncat(&pgm_header[14], " sec ", 5);
	snprintf(&pgm_header[19], 11, "%010d", (int)((time->tv_nsec)/1000000));

	strncat(&pgm_header[29], " msec \n640 480\n255\n", 19);
	strncpy(pgm_dumpname, "img/ts/test00000000.pgm", sizeof(pgm_dumpname));	

	// Set name in a way that it reflects the frame number
	snprintf(&pgm_dumpname[11], 9, "%08d", tag);

	// Append .pgm at the last
	strncat(&pgm_dumpname[19], ".pgm", 5);

	strncpy(&file_out[frames].filename[0], pgm_dumpname, sizeof(pgm_dumpname));
	strncpy(&file_out[frames].header[0], pgm_header, sizeof(pgm_header));
	file_out[frames].headersize = sizeof(pgm_header);
	file_out[frames].filesize = size;
	file_out[frames].dataptr = &data_buffer[buf_index][0];

	buf_index += 1;	
	if(buf_index >= No_of_Buffers)
	{
		buf_index = 0;
	}
    
}

// Originally written by Professor Sam Siewert as per mentioned on Top of this File
// Function to process raw frame to convert a greyscale image
// Parameter1: const void *p - pointer to memory mapped region where raw frame data is stored
// Parameter2: int size - size of the raw frame
// Return: void
static void process_image(const void *p, int size)
{
	int i, newi;
	struct timespec frame_time;
	unsigned char *pptr = (unsigned char *)p;

	// Absolute timestamp - store the current time in structure
	clock_gettime(CLOCK_REALTIME, &frame_time);

	// Handle the required image processing for YUYV capture
	if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
	{
		for(i=0, newi=0; i<size; i=i+4, newi=newi+2)
		{
			data_buffer[buf_index][newi] = (int)pptr[i];
			data_buffer[buf_index][newi+1] = (int)pptr[i+2];
		}
		dump_pgm((size/2), frames, &frame_time);
	}

	// Error handling
	else
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Filter!! Unknown Format", Time_Stamp(Mode_ms));
	}

	fflush(stderr);
	//fprintf(stderr, ".");
	fflush(stdout);
}

// Function to capture a number of frames, and stop when either
// The maximum number of test frames have been reached, or indicator position change has been detected
// Parameter1: void
// Return: void
void Cam_Filter(void)
{
	// Setting up initial values of variables
	frames = 0;
	buf_index = 0;
	Grey_Buffer_Size = (Big_Buffer_Size / 3) * 2;

	// Select appropriate values according to the running mode (currently 10Hz not supported)
	if((Operating_Mode & Mode_FPS_Mask) == Mode_10_FPS_Val)
	{
		Offset_ms = Offset_10Hz_ms;
		Diff_Thr_High = Diff_Thr_High_10Hz;
		Diff_Thr_Low = Diff_Thr_Low_10Hz;
	}

	else
	{
		Offset_ms = Offset_1Hz_ms;
		Diff_Thr_High = Diff_Thr_High_1Hz;
		Diff_Thr_Low = Diff_Thr_Low_1Hz;
	}

	while(frames < Test_Frames)
	{
		// Keep on polling camera until a new frame arrives
		do
		{
			resp = read_frame();
		} while(resp != 0);

		// Process the image
		process_image((void *)shared_struct.start, shared_struct.length); 

		// Look for difference
		if(frames > 0)
		{
			if(consecutive_cmp((frames - 1), frames) == 0)
			{
				break;
			}
		}

		frames += 1;
	}

	// Log message if no frame difference was found
	if(frames == Test_Frames)
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Filter!! Couldn't Find The Targeted Difference", Time_Stamp(Mode_ms));
		clock_gettime(CLOCK_REALTIME, &prev_t);
		prev_t.tv_sec += Filter_Timestamp_Offset_s;
	}

}
