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

void timestamp()
{
        time_t ltime; /* calendar time */
        ltime=time(NULL); /* get current cal time */
        char *timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = '\0';
        printf("\n%s  ", timestamp);
}

void send_message(int s0, struct can_frame frame) {
        char nBytes = write(s0,&frame,sizeof(frame));
        timestamp();
        printf("WRITE  ");
        printf("nbytes: %d  ", nBytes);
        printf("can_id: %02X  ", frame.can_id);
        printf("can_dlc: %02X  data: ", frame.can_dlc);
        for (int i = 0; i < frame.can_dlc; i++)
                printf("%02X ", frame.data[i]);
        printf("crc test: %d\n", get_crc(frame.data, frame.can_dlc));
}


char random_byte(){
        return rand() %256;
}

void put_message(char* message, char n){
        for(char i=0; i<n-1; i++)
                message[i] = random_byte();
}

void create_and_send_message(int s0, unsigned char id, unsigned char dlc, unsigned char counter){
        struct can_frame frame;

        frame.can_id=id;
        frame.can_dlc=dlc;
        frame.data[0] = counter;

        put_message(frame.data + 1, dlc -2);
        frame.data[dlc-1] = get_crc(frame.data, dlc-1);

        send_message(s0, frame);
}

int main() {

        int s0 = can_setup();

        // infinite loop
        unsigned char i=0;
        while(1)
        {
                create_and_send_message(s0, 0x10, 8, i++);
                sleep(1);
        }
        return 0;
}
