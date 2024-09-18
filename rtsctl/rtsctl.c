#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define TTYDEV "/dev/ttyUSB0"
#define PIPEF  "/tmp/RTSCTL"

int sfd;

//Control RTS PIN
int CTRL_VAL = TIOCM_RTS;
//If you want to use DTR,use following.
//int CTRL_VAL = TIOCM_DTR;
int mbits;

FILE *fp;

void set_pin_on();
void set_pin_off();

//Set RTS Pin
void set_pin_on(){
    printf("Set to on\n",mbits);
    ioctl(sfd, TIOCMBIS, &CTRL_VAL);
}

//Reset RTS Pin
void set_pin_off(){
    printf("Set to off\n",mbits);
    ioctl(sfd, TIOCMBIC, &CTRL_VAL);
}

//Main
void main()
{
    char *ret;
    char buf[32];

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
                close(sfd);
                fclose(fp);
                exit(0);
            }
        }
        //After EOF,reopen and continue
    }

    close(sfd);
    fclose(fp);
    exit(0);
}
