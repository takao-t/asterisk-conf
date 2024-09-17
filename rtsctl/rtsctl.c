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
int RTS_VAL = TIOCM_RTS;
//
FILE *fp;

void set_rts_on();
void set_rts_off();

//Set RTS Pin
void set_rts_on(){
    printf("Set to on\n");
    ioctl(sfd, TIOCMBIS, &RTS_VAL);
}

//Reset RTS Pin
void set_rts_off(){
    printf("Set to off\n");
    ioctl(sfd, TIOCMBIC, &RTS_VAL);
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

    //Main Loop
    while(1){
        //Open PIPE(FIFO)
        fp = fopen(PIPEF, "r");

        //Wait for new line
        while(fgets(buf,32,fp) != NULL){
            if(strcmp(buf,"ON\n") == 0) set_rts_on();
            else if(strcmp(buf,"OFF\n") == 0) set_rts_off();
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
