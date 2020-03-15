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


#define OK 0
#define NOT_OK 0xFF


typedef unsigned char crc;
typedef struct {
        unsigned char id, dlc, counter;
        long long int last_ms;
        long sleep_ms;
} receive_info;

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

void print_frame(struct can_frame frame){
        timestamp();
        printf("READ  ");
        printf("can_id: %02X  ", frame.can_id);
        printf("can_dlc: %02X  data: ", frame.can_dlc);
        for (int i = 0; i < frame.can_dlc; i++)
                printf("%02X ", frame.data[i]);
        printf("  ");
}

receive_info* check_id(struct can_frame frame, receive_info* infos){
        for(int i=0; i<10; i++)
                if(infos[i].id == frame.can_id)
                        return infos + i;

        printf("id %02X not OK", frame.can_id);
        return NULL;
}

int check_dlc(struct can_frame frame, unsigned char dlc){
        if(frame.can_dlc == dlc)
                return OK;
        printf("dlc %02X not OK, expected: %02X", frame.can_dlc, dlc);
        return NOT_OK;
}

int check_count(struct can_frame frame, unsigned char count){
        if(frame.data[0] == count)
                return OK;
        printf("count %02X not OK, expected: %02X", frame.data[0], count);
        return NOT_OK;
}

long long current_timestamp_ms() {
        struct timeval te;
        gettimeofday(&te, NULL); // get current time
        long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
        // printf("milliseconds: %lld\n", milliseconds);
        return milliseconds;
}

int check_timing(receive_info*info, long period){
        int ok = NOT_OK;
        long long int current_ms = current_timestamp_ms();

        long err = period / 10;

        long long int expected_ms = info->last_ms + period;

        if(info->last_ms == 0)
                ok = OK;
        else if(current_ms > expected_ms - err && current_ms < expected_ms + err)
                ok = OK;


        if(ok == OK) {
                info->last_ms = current_timestamp_ms();
                return OK;
        }
        printf("period not OK, expected message at: %lld but got it at: %lld\n",
               expected_ms, current_ms);
        return NOT_OK;
}

void check_and_print_message(struct can_frame frame, receive_info* infos){
        print_frame(frame);

        receive_info* info = check_id(frame, infos);
        if(info == NULL)
                return;

        int dlc_ok = check_dlc(frame, info->dlc);
        if(dlc_ok == NOT_OK)
                return;

        int counter_ok = check_count(frame, info->counter);
        if(counter_ok == NOT_OK)
                return;

        int crc_ok = get_crc(frame.data, frame.can_dlc);
        if(crc_ok != OK) {
                printf("crc not OK");
                return;
        }

        int timing_ok = check_timing(info, info->sleep_ms);
        if(timing_ok == NOT_OK)
                return;

        info->counter++;
}

int main() {
        int s0 = can_setup();

        receive_info info[10] = {
                {.id = 1, .dlc = 4, .counter = 0,.last_ms = 0, .sleep_ms = 1000},
                {.id = 2, .dlc = 5, .counter = 0,.last_ms = 0, .sleep_ms = 333},
                {.id = 3, .dlc = 7, .counter = 0,.last_ms = 0, .sleep_ms = 200},
                {.id = 5, .dlc = 8, .counter = 0,.last_ms = 0, .sleep_ms = 100},
                {.id = 8, .dlc = 6, .counter = 0,.last_ms = 0, .sleep_ms = 83},
                {.id = 13, .dlc = 5, .counter = 0,.last_ms = 0, .sleep_ms = 125},
                {.id = 21, .dlc = 8, .counter = 0,.last_ms = 0, .sleep_ms = 134},
                {.id = 34, .dlc = 7, .counter = 0,.last_ms = 0, .sleep_ms = 87},
                {.id = 55, .dlc = 3, .counter = 0,.last_ms = 0, .sleep_ms = 56},
                {.id = 89, .dlc = 6, .counter = 0,.last_ms = 0, .sleep_ms = 76}
        };

        while(1) {
                struct can_frame frame;
                unsigned char nbytes = read(s0, &frame, sizeof(struct can_frame));

                check_and_print_message(frame, info);
        }
        return 0;
}
