#define	_SIGNAL_QUEUE_SIZE	100
struct	_SIGNAL_HANDLER {
	int	pid;	/* my process id */
	int	queue[_SIGNAL_QUEUE_SIZE],
		int queue_index;
};

int	set_signal_handler(void);
void	catch_signal(int /*signal*/);
void	reset_signal_handler(void);
