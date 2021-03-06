
#include "main.h"

extern uint8_t Terminate_Flag;
extern uint8_t data_buffer[No_of_Buffers][Big_Buffer_Size];

static store_struct file_in;
static mqd_t storage_queue;
static struct timespec curr_time;
static int q_recv_resp, dumpfd, total_bytes, written_bytes;
static uint32_t frames;

static uint8_t Storage_Q_Setup(void)
{
    // Queue setup
	struct mq_attr storage_queue_attr;
	storage_queue_attr.mq_maxmsg = queue_size;
	storage_queue_attr.mq_msgsize = sizeof(store_struct);

	storage_queue = mq_open(storage_q_name, O_CREAT | O_RDONLY | O_CLOEXEC, 0666, &storage_queue_attr);
	if(storage_queue == -1)
	{
		syslog(LOG_ERR, "<%.6fms>Queue opening error for Storage", Time_Stamp(Mode_ms));
		return 1;
	}
	return 0;
}


void *Storage_Func(void *para_t)
{

	if(Storage_Q_Setup() != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Storage!! Thread Setup Failed", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	frames = 0;

	syslog(LOG_INFO, "<%.6fms>!!Storage!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

	while(frames < No_of_Frames)
	{
		clock_gettime(CLOCK_REALTIME, &curr_time);
		curr_time.tv_sec += 1;

		q_recv_resp = mq_timedreceive(storage_queue, (char *)&file_in, sizeof(store_struct), 0, &curr_time);

		// Check the result of timed receive
		if((q_recv_resp < 0) && (errno != ETIMEDOUT))
		{
			syslog(LOG_ERR, "<%.6fms>!!Storage!! Queue receiving Error", Time_Stamp(Mode_ms));
			errno_exit("queue receive");
		}

		else if(q_recv_resp == sizeof(store_struct))
		{
			//open, write, close

			// Create the file
			dumpfd = open(&file_in.filename[0], O_WRONLY | O_CREAT, 00666);

			// Write header in file
			written_bytes = write(dumpfd, &file_in.header[0], file_in.headersize);

			total_bytes = 0;

			// Write the entire buffer having the image information to the file
			do
			{
				written_bytes = write(dumpfd, file_in.dataptr, file_in.filesize);
				total_bytes += written_bytes;
			} while(total_bytes < file_in.filesize);

			// Close file
			close(dumpfd);

			syslog(LOG_INFO, "<%.6fms>!!Storage!! Successfully Stored Frame: %d", Time_Stamp(Mode_ms), frames);

			frames += 1;
		}
	}

	syslog (LOG_INFO, "<%.6fms>!!Storage!! Exiting...", Time_Stamp(Mode_ms));
	mq_close(storage_queue);
	mq_unlink(storage_q_name);
	pthread_exit(0);
}
