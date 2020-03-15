#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>
#include <getopt.h>

#include <time.h>
#include <pthread.h>

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
typedef struct {
        int s0;
        unsigned char id, dlc, counter;
        long sleep_ms;
} send_info;


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

void send_message(int s0, struct can_frame frame) {
        char nBytes = write(s0,&frame,sizeof(frame));
        timestamp();
        printf("WRITE  ");
        printf("nbytes: %d  ", nBytes);
        printf("can_id: %02X  ", frame.can_id);
        printf("can_dlc: %02X  data: ", frame.can_dlc);
        for (int i = 0; i < frame.can_dlc; i++)
                printf("%02X ", frame.data[i]);
        // printf("crc test: %d\n", get_crc(frame.data, frame.can_dlc));
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

        put_message(frame.data + 1, dlc -1);
        frame.data[dlc-1] = get_crc(frame.data, dlc-1);

        send_message(s0, frame);
}

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

void *send_message_thread(void *ptr){

        send_info *info = (send_info*)ptr;

        while(1) {
                pthread_mutex_lock( &mutex1 );
                create_and_send_message(info->s0, info->id, info->dlc, info->counter++);
                pthread_mutex_unlock( &mutex1 );
                usleep(info->sleep_ms*1000);
        }
}


int main() {
        int s0 = can_setup();

        send_info info[10] = {
                {.s0 = s0, .id = 1, .dlc = 4, .counter = 0, .sleep_ms = 1000},
                {.s0 = s0, .id = 2, .dlc = 5, .counter = 0, .sleep_ms = 333},
                {.s0 = s0, .id = 3, .dlc = 7, .counter = 0, .sleep_ms = 200},
                {.s0 = s0, .id = 5, .dlc = 8, .counter = 0, .sleep_ms = 100},
                {.s0 = s0, .id = 8, .dlc = 6, .counter = 0, .sleep_ms = 83},
                {.s0 = s0, .id = 13, .dlc = 5, .counter = 0, .sleep_ms = 125},
                {.s0 = s0, .id = 21, .dlc = 8, .counter = 0, .sleep_ms = 134},
                {.s0 = s0, .id = 34, .dlc = 7, .counter = 0, .sleep_ms = 87},
                {.s0 = s0, .id = 55, .dlc = 3, .counter = 0, .sleep_ms = 56},
                {.s0 = s0, .id = 89, .dlc = 6, .counter = 0, .sleep_ms = 76}
        };

        pthread_t threads[10];

        for(int i=0; i< 10; i++) {
                pthread_create(threads+i, NULL, send_message_thread, (void*)(info+i));
        }

        for(int i=0; i< 10; i++) {
                pthread_join(threads[i], NULL);
        }

        return 0;
}
