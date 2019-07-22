// Logger.c

#include "main.h"

static mqd_t logger_queue;

static int q_recv_resp;

static void signal_function(int value)
{
	if((value == SIGUSR1) || (value == SIGUSR2))
	{
	    syslog(LOG_INFO, "<%.6fms>Logger Exiting...", Time_Stamp(Mode_ms));
        mq_close(logger_queue);
        mq_unlink(logger_q_name);
        pthread_exit(0);
	}
}

static void sig_setup(void)
{
		// Configuring timer and signal action
		struct sigaction custom_signal_action;

		// Set all initial values to 0 in the structure
		memset(&custom_signal_action, 0, sizeof (custom_signal_action));

		// Set signal action handler to point to the address of the target function (to execute on receiving signal)
		custom_signal_action.sa_handler = &signal_function;

		// Setting the signal action to kick in the handler function for these 2 signals
		sigaction(SIGUSR1, &custom_signal_action, 0);
		sigaction(SIGUSR2, &custom_signal_action, 0);
}

static uint8_t Logger_Q_Setup(void)
{
    // Queue setup
	struct mq_attr logger_queue_attr;
	logger_queue_attr.mq_maxmsg = queue_size;
	logger_queue_attr.mq_msgsize = sizeof(log_struct);

	logger_queue = mq_open(logger_q_name, O_CREAT | O_RDONLY | O_CLOEXEC, 0666, &logger_queue_attr);
	if(logger_queue == -1)
	{
		syslog(LOG_ERR, "<%.6fms>Queue opening error for Logger", Time_Stamp(Mode_ms));
		return 1;
	}
	return 0;
}

void *Logger_Func(void *para_t)
{

    mq_unlink(logger_q_name);

    if(Logger_Q_Setup() != 0)
    {
        syslog (LOG_ERR, "<%.6fms>Logger Thread Setup Failed", Time_Stamp(Mode_ms));
        pthread_exit(0);
    }

    sig_setup();

    syslog (LOG_INFO, "<%.6fms>Logger Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

    log_struct incoming_msg;
    char src_string[Source_Name_Len];

    while(1)
    {
        q_recv_resp = mq_receive(logger_queue, &incoming_msg, sizeof(log_struct), 0);

        if(q_recv_resp < 0)
        {
            incoming_msg.log_level = LOG_ERR;
            incoming_msg.source = Log_Logger;
            strncpy(incoming_msg.message, "Queue Receiving Failed", Logging_Msg_Len);
        }

        switch(incoming_msg.source)
        {
            case Log_Main:
            {
                strncpy(src_string, "Main", Source_Name_Len);
                break;
            }

            case Log_Logger:
            {
                strncpy(src_string, "Logger", Source_Name_Len);
                break;
            }

            case Log_Monitor:
            {
                strncpy(src_string, "Cam_Monitor", Source_Name_Len);
                break;
            }

            case Log_RGB:
            {
                strncpy(src_string, "Cam_RGB", Source_Name_Len);
                break;
            }

            case Log_Compress:
            {
                strncpy(src_string, "Cam_Compress", Source_Name_Len);
                break;
            }

            case Log_Grey: 
            {
                strncpy(src_string, "Cam_Grey", Source_Name_Len);
                break;
            }

            case Log_Socket:
            {
                strncpy(src_string, "Cam_Socket", Source_Name_Len);
                break;
            }

            case Log_Scheduler:
            {
                strncpy(src_string, "Log_Scheduler", Source_Name_Len);
                break;
            }

            default:
            {
                strncpy(src_string, "UNKNOWN", Source_Name_Len);
                break;
            }
        }

        syslog (incoming_msg.log_level, "<%.6fms>!!Source Thread: %s!! Msg: %s", Time_Stamp(Mode_ms), src_string, incoming_msg.message);
    }
}
