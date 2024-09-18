#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define TTYDEV "/dev/ttyUSB0"
#define PIPEF  "/tmp/RTSCTL"

//Maximum holding(on) time of PTT(secs.)
#define PTT_MAX_HOLD 10

int sfd;

//Control RTS PIN
int CTRL_VAL = TIOCM_RTS;
//If you want to use DTR,use following.
//int CTRL_VAL = TIOCM_DTR;
int mbits;

FILE *fp;

timer_t timerid;
struct itimerspec its;

void set_pin_on();
void set_pin_off();

//Set RTS Pin
void set_pin_on(){
    //Remove previouse timer
    if(timerid != NULL) timer_delete(timerid);
    //Create timer
    its.it_value.tv_sec = PTT_MAX_HOLD;
    its.it_value.tv_nsec = 0;
    timer_create(CLOCK_REALTIME, NULL, &timerid);

    printf("Set to on\n",mbits);
    ioctl(sfd, TIOCMBIS, &CTRL_VAL);

    //Arm the timer
    timer_settime(timerid, 0, &its, NULL);
}

//Reset RTS Pin
void set_pin_off(){
    printf("Set to off\n",mbits);
    ioctl(sfd, TIOCMBIC, &CTRL_VAL);
    //Disarm the timer
    if(timerid != NULL) timer_delete(timerid);
}

//Froce off Handler
void force_off_handler(int signum){
    printf("TIMER - ");
    printf("Set to off\n",mbits);
    ioctl(sfd, TIOCMBIC, &CTRL_VAL);
    //Disarm the timer
    if(timerid != NULL) timer_delete(timerid);
}

//Main
void main()
{
    char *ret;
    char buf[32];

    struct sigaction act, oldact;

    //Initialize some timer values
    memset(&act, 0, sizeof(act));
    memset(&oldact, 0, sizeof(oldact));
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    //Open TTY Device to IOCTL
    sfd = open(TTYDEV,O_RDWR | O_NOCTTY);

    if(sfd == -1){
        printf("%s : Open error.\n", TTYDEV);
        exit(1);
    } 

    //Unlink 1st,then make PIPE(FIFO)
    unlink(PIPEF);
    if(mkfifo(PIPEF, S_IRUSR|S_IWUSR) != 0){
        printf("PIPE(FIFO) Create error\n");
        close(sfd);
        exit(1);
    }
    chmod(PIPEF, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);

    //Prevent on when start this program
    set_pin_off();

    act.sa_handler = force_off_handler;
    act.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &act, &oldact);


    //Main Loop
    while(1){
        //Open PIPE(FIFO)
        fp = fopen(PIPEF, "r");

        //Wait for new line
        while(fgets(buf,32,fp) != NULL){
            if(strcmp(buf,"ON\n") == 0) set_pin_on();
            else if(strcmp(buf,"OFF\n") == 0) set_pin_off();
            else if(strcmp(buf,"ALT\n") == 0){
                ioctl(sfd, TIOCMGET, &mbits);
                if(mbits & CTRL_VAL) set_pin_off();
                else set_pin_on();
            }
            else if(strcmp(buf,"EXIT\n") == 0){
                sigaction(SIGALRM, &oldact, NULL);
                close(sfd);
                fclose(fp);
                exit(0);
            }
        }
        //After EOF,reopen and continue
    }

    sigaction(SIGALRM, &oldact, NULL);
    close(sfd);
    fclose(fp);
    exit(0);
}
