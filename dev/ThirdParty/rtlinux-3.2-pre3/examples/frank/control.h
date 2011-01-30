#define COMMAND_FIFO 3

#define START_TASK	1
#define STOP_TASK	2

struct my_msg_struct {
	int command;
	int task;
	int period;
};
