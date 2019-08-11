#include "main.h"
#include "Aux_Func.h"
#include "Cam_Func.h"
#include "Cam_Monitor.h"

int fd;
uint32_t n_buffers;
uint32_t HRES, VRES, Monitor_Deadline;
uint8_t Terminate_Flag;
frame_p_buffer *frame_p;
strct_analyze Analysis;
frame_p_buffer shared_struct;
sem_t Monitor_Sem;
float Monitor_Start_Stamp, Monitor_Stamp_1;

static float Monitor_End_Stamp, Monitor_Stamp_2;
static uint32_t mon_index;
static int resp;

// Declare set to use with FD_macros
static fd_set fds;

// Structure to store timing values
static struct timeval tout;

//mmap
uint8_t read_frame(void)
{
	// https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#v4l2-buffer
	struct v4l2_buffer buf;

	// Set the entire structure to 0
	CLEAR(buf);

	// Set proper type
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	// Indicate that the method being used is memory mapping
	buf.memory = V4L2_MEMORY_MMAP;

	// Dequeue a filled buffer from driver's outgoing queue
	if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
	{
		switch (errno)
		{
			case EAGAIN:
			{
				return 1;
			}

			case EIO:
			/* Could ignore EIO, see spec. */

			/* fall through */

			default:
			{
				syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! VIDIOC_DQBUF Error", Time_Stamp(Mode_ms));

				errno_exit("VIDIOC_DQBUF");
			
				break;
			}
		}
	}

	assert(buf.index < n_buffers);

	shared_struct.start = (void *)frame_p[buf.index].start;
	shared_struct.length = buf.bytesused;

	// Enqueue back the empty buffer to driver's incoming queue
	if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
	{
		syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! VIDIOC_QBUF Error", Time_Stamp(Mode_ms));

		errno_exit("VIDIOC_QBUF");
	}

	return 0;
}

// Following function implements writer Thread
void *Cam_Monitor_Func(void *para_t)
{

	mon_index = 0;
	syslog(LOG_INFO, "<%.6fms>!!Cam_Monitor!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

	while(1)
	{

		if(sem_wait(&Monitor_Sem) != 0)
		{
			syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Couldn't wait on Monitor_Sem", Time_Stamp(Mode_ms));
			continue;
		}

		syslog(LOG_INFO, "<%.6fms>!!RMA!! Cam_Monitor Launched (Iteration: %d)", Time_Stamp(Mode_ms), mon_index);
		
		if(Terminate_Flag != 0)
		{
			break;
		}

		// Clear the set
		FD_ZERO(&fds);

		// Store file descriptor of camera in the set
		FD_SET(fd, &fds);

		// Set timeout of communication with Camera to 25 ms

		tout.tv_sec = 0;
		tout.tv_usec = 0;

		// http://man7.org/linux/man-pages/man2/select.2.html
		// Select() blockingly monitors given file descriptors for their availibity for I/O operations
		// First parameter: nfds should be set to the highest-numbered file descriptor in any of the three sets, plus 1
		// Second parameter: file drscriptor set that should be monitored for read() availability
		// Third parameter: file drscriptor set that should be monitored for write() availability
		// Fourth parameter: file drscriptor set that should be monitored for exceptions
		// Fifth parameter: to specify time out for select()
		resp = select(fd + 1, &fds, NULL, NULL, &tout);

		// Error handling
		if(resp == -1)
		{
			// If a signal made select() to return, then skip rest of the loop code, and start again
			if(EINTR == errno)
			{
				continue;
			}

			syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! Select() Error", Time_Stamp(Mode_ms));

			errno_exit("select");
		}

		// If select() indicated that device is ready for reading from it, then grab a frame
		else if(read_frame() != 0)
		{
			syslog(LOG_ERR, "<%.6fms>!!Cam_Monitor!! read_frame() Error", Time_Stamp(Mode_ms));
		}

		Monitor_Stamp_2 = Time_Stamp(Mode_ms);

		Analysis.Exec_Analysis.Monitor_Exec[mon_index] = Monitor_Stamp_2 - Monitor_Stamp_1;

		syslog(LOG_INFO, "<%.6fms>!!RMA!! Cam_Monitor Completed (Iteration: %d)", Time_Stamp(Mode_ms), mon_index);

		mon_index += 1;
	}

	Monitor_End_Stamp = Time_Stamp(Mode_ms);

	Analysis.Jitter_Analysis.Overall_Jitter[Monitor_TID] = Monitor_End_Stamp - (Monitor_Start_Stamp + (Monitor_Loop_Count * Monitor_Deadline));

	// Exit and log the termination event
	syslog(LOG_INFO, "<%.6fms>!!Cam_Monitor!! Exiting...", Time_Stamp(Mode_ms));

	pthread_exit(0);
}
