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
        int s0,i;

        struct sockaddr_can addr0;
        struct ifreq ifr0;
        struct can_frame myFrame;
        char fl_bind;
        char bytes_wr;

        // system("sudo ip link set vcan0 down");
        // sleep(1);// sec
        // system("sudo ip link set vcan0 up type can bitrate 500000");
        // printf("sudo ip link set vcan0 up type can bitrate 500000 \n");
        // sleep(1);

        s0=socket(PF_CAN, SOCK_RAW, CAN_RAW);
        /*CAN0 interface set up*/
        strcpy(ifr0.ifr_name, "vcan0");
        ioctl(s0, SIOCGIFINDEX, &ifr0);
        addr0.can_family = AF_CAN;
        addr0.can_ifindex = ifr0.ifr_ifindex;
        fl_bind=bind(s0,(struct sockaddr*)&addr0, sizeof(addr0));
        if(fl_bind==0) {printf("bind ok\n");}

        // infinite loop
        while(1)
        {

                //define the CAN frame
                myFrame.can_id=rand() % 90;
                myFrame.can_dlc=rand()%9;
                for(i=0; i<myFrame.can_dlc; i++) myFrame.data[i]=rand() % 256;

                bytes_wr=write(s0,&myFrame,sizeof(myFrame));
                printf("bytes_wr: %d \n", bytes_wr);
                usleep(rand() % 10000);
        }
}
