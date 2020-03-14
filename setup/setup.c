#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>
#include <getopt.h>

#include <sched.h>
#include <limits.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>

int main()
{
		system("sudo ip link set vcan0 down");
		sleep(1);
		system("sudo ip link set vcan0 up type can bitrate 500000");
		printf("sudo ip link set vcan0 up type can bitrate 500000 \n");
		sleep(1);
}
