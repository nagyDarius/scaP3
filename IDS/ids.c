#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>
#include <getopt.h>

#include <time.h>

#include <sched.h>
#include <limits.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>


typedef unsigned char crc;

#define POLYNOMIAL 0xDA
#define WIDTH  (8 * sizeof(crc))
#define TOPBIT (1 << (WIDTH - 1))


crc get_crc(unsigned char const message[], int nBytes)
{
        crc remainder = 0;


        /*
         * Perform modulo-2 division, a byte at a time.
         */
        for (int byte = 0; byte < nBytes; ++byte)
        {
                /*
                 * Bring the next byte into the remainder.
                 */
                remainder ^= (message[byte] << (WIDTH - 8));

                /*
                 * Perform modulo-2 division, a bit at a time.
                 */
                for (unsigned char bit = 8; bit > 0; --bit)
                {
                        /*
                         * Try to divide the current data bit.
                         */
                        if (remainder & TOPBIT)
                        {
                                remainder = (remainder << 1) ^ POLYNOMIAL;
                        }
                        else
                        {
                                remainder = (remainder << 1);
                        }
                }
        }

        /*
         * The final remainder is the CRC result.
         */
        return (remainder);
}


int can_setup(){
        int s0;
        s0=socket(PF_CAN, SOCK_RAW, CAN_RAW);

        struct ifreq ifr0;
        strcpy(ifr0.ifr_name, "vcan0");
        ioctl(s0, SIOCGIFINDEX, &ifr0);

        struct sockaddr_can addr0;
        addr0.can_family = AF_CAN;
        addr0.can_ifindex = ifr0.ifr_ifindex;
        char fl_bind=bind(s0,(struct sockaddr*)&addr0, sizeof(addr0));
        if(fl_bind==0) {
                printf("Bind ok\n");
        }else {
                printf("Binding failed\n");
                exit(1);
        }

        return s0;
}

static char* get_timestamp(char* buf) {
    struct timeb start;
    char append[10];
    if (buf) {
        ftime(&start);
        strftime(buf, 100, "%H:%M:%S", localtime(&start.time));

        /* append milliseconds */
        sprintf(append, ":%03u", start.millitm);
        strcat(buf, append);
    }
    return buf;
}

void timestamp()
{
        char timestamp[50];
        printf("\n%s  ", get_timestamp(timestamp));
}

int main() {
        int s0 = can_setup();

        while(1) {
                struct can_frame frame;
                unsigned char nbytes = read(s0, &frame, sizeof(struct can_frame));

                check_and_print_message(frame);
        }
        return 0;
}
