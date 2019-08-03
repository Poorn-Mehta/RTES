#include "main.h"

extern uint8_t data_buffer[No_of_Buffers][Big_Buffer_Size];
extern frame_p_buffer shared_struct;

static uint32_t frames, prev_frame, pix_cnt, diff_pix_cnt;
static uint8_t resp, buf_index, startup;
static int dumpfd, written, total;
static store_struct file_out[Test_Frames];
static char ppm_header[]="P6\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
static char ppm_dumpname[]="img/ts/test00000000.ppm";
static char pgm_header[]="P5\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
static char pgm_dumpname[]="img/ts/test00000000.pgm";
static float diff_percent;

static uint8_t cmp_pix(uint8_t *pix1, uint8_t *pix2)
{

	if(*pix1 >= (Pix_Max_Val - Pix_Allowed_Val))
	{
		if(*pix2 >= (*pix1 - Pix_Allowed_Val))
		{
			return 0;
		}

		else
		{
			return 1;
		}
	}

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

static void consecutive_cmp_all(void)
{

	prev_frame = 0;
	frames = 1;

	while(frames  < Test_Frames)
	{
		diff_pix_cnt = 0;
		for(pix_cnt = 0; pix_cnt < ((Big_Buffer_Size / 3) * 2); pix_cnt ++)
		{
			if(cmp_pix(&data_buffer[prev_frame][pix_cnt], &data_buffer[frames][pix_cnt]) != 0)
			{
				diff_pix_cnt += 1;
			}
		}

		diff_percent = (((float)diff_pix_cnt) / ((float)Big_Buffer_Size)) * 100;

		syslog(LOG_INFO, "<%.6fms>!!Cam_Filter!! Found %.3f percent different pixels between frame %d and %d", Time_Stamp(Mode_ms), diff_percent, prev_frame, frames);

		if((diff_percent >= Diff_Thr_Low) && (diff_percent <= Diff_Thr_High))
		{
			syslog(LOG_INFO, "<%.6fms>!!Cam_Filter!! ******Indicator Change Detected (%d and %d)******", Time_Stamp(Mode_ms), prev_frame, frames);
		}

		prev_frame += 1;
		frames += 1;
	}
}

static void store_all_test_frames(void)
{

	frames = 0;

	while(frames  < Test_Frames)
	{
		// Create the file
		dumpfd = open(&file_out[frames].filename[0], O_WRONLY | O_NONBLOCK | O_CREAT, 00666);
		

		// Write header in file
		written=write(dumpfd, &file_out[frames].header[0], file_out[frames].headersize);

		total=0;

		// Write the entire buffer having the image information to the file
		do
		{
			written=write(dumpfd, file_out[frames].dataptr, file_out[frames].filesize);
			total+=written;
		} while(total < file_out[frames].filesize);

		// Close file
		close(dumpfd);

		syslog(LOG_INFO, "<%.6fms>!!Cam_Filter!! Frame: %d Stored to Flash", Time_Stamp(Mode_ms), frames);

		frames += 1;
	}
}

static void dump_pgm(int size, unsigned int tag, struct timespec *time)
{
//    int written, total, dumpfd;

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

	syslog(LOG_INFO, "<%.6fms>!!Cam_Filter!! Frame Written to RAM Buffer", Time_Stamp(Mode_ms));

	buf_index += 1;	
	if(buf_index >= No_of_Buffers)
	{
		buf_index = 0;
	}
    
}

static void dump_ppm(int size, unsigned int tag, struct timespec *time)
{
//    int written, total, dumpfd;

	// Store timestamp and resolution in the header
	snprintf(&ppm_header[4], 11, "%010d", (int)time->tv_sec);
	strncat(&ppm_header[14], " sec ", 5);
	snprintf(&ppm_header[19], 11, "%010d", (int)((time->tv_nsec)/1000000));

	strncat(&ppm_header[29], " msec \n640 480\n255\n", 19);
	strncpy(ppm_dumpname, "img/ts/test00000000.ppm", sizeof(ppm_dumpname));	

	// Set name in a way that it reflects the frame number
	snprintf(&ppm_dumpname[11], 9, "%08d", tag);

	// Append .ppm at the last
	strncat(&ppm_dumpname[19], ".ppm", 5);

	strncpy(&file_out[frames].filename[0], ppm_dumpname, sizeof(ppm_dumpname));
	strncpy(&file_out[frames].header[0], ppm_header, sizeof(ppm_header));
	file_out[frames].headersize = sizeof(ppm_header);
	file_out[frames].filesize = size;
	file_out[frames].dataptr = &data_buffer[buf_index][0];

	syslog(LOG_INFO, "<%.6fms>!!Cam_Filter!! Frame Written to RAM Buffer", Time_Stamp(Mode_ms));

	buf_index += 1;	
	if(buf_index >= No_of_Buffers)
	{
		buf_index = 0;
	}
    
}

static void yuv2rgb(int y, int u, int v, unsigned char *r, unsigned char *g, unsigned char *b)
{
	int r1, g1, b1;

	// replaces floating point coefficients
	int c = y-16, d = u - 128, e = v - 128;       

	// Conversion that avoids floating point
	r1 = ((298 * c)           + (409 * e) + 128) >> 8;
	g1 = ((298 * c) - (100 * d) - (208 * e) + 128) >> 8;
	b1 = ((298 * c) + (516 * d)           + 128) >> 8;

	// Computed values may need clipping.

	*r = (r1 > 255) ? 255 : (r1 < 0 ? 0 : (uint8_t)r1);
	*g = (g1 > 255) ? 255 : (g1 < 0 ? 0 : (uint8_t)g1);
	*b = (b1 > 255) ? 255 : (b1 < 0 ? 0 : (uint8_t)b1);
}

static void process_image(const void *p, int size)
{
	int i, newi;
	struct timespec frame_time;
	unsigned char *pptr = (unsigned char *)p;
	int y_temp, y2_temp, u_temp, v_temp;

	// Absolute timestamp - store the current time in structure
	clock_gettime(CLOCK_REALTIME, &frame_time);

	// Handle the required image processing for YUYV capture
	if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
	{

//		for(i=0, newi=0; i<size; i=i+4, newi=newi+6)
		for(i=0, newi=0; i<size; i=i+4, newi=newi+2)
		{
			data_buffer[buf_index][newi] = (int)pptr[i];
			data_buffer[buf_index][newi+1] = (int)pptr[i+2];

//			y_temp=(int)pptr[i]; u_temp=(int)pptr[i+1]; y2_temp=(int)pptr[i+2]; v_temp=(int)pptr[i+3];
//			yuv2rgb(y_temp, u_temp, v_temp, &data_buffer[buf_index][newi], &data_buffer[buf_index][newi+1], &data_buffer[buf_index][newi+2]);
//			yuv2rgb(y2_temp, u_temp, v_temp, &data_buffer[buf_index][newi+3], &data_buffer[buf_index][newi+4], &data_buffer[buf_index][newi+5]);
		}

//		dump_ppm(((size*6)/4), frames, &frame_time);
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

void Cam_Filter(void)
{

	frames = 0;
	buf_index = 0;
	startup = 0;

	while(frames < Test_Frames) 
	{
		do
		{
			resp = read_frame();
		} while(resp != 0);

		process_image((void *)shared_struct.start, shared_struct.length);

		frames += 1;

/*		if((startup == 0) && (frames >= Useless_Frames))
		{
			frames = 0;
			startup = 1;
		}*/
	}
	
	store_all_test_frames();

	consecutive_cmp_all();

}
