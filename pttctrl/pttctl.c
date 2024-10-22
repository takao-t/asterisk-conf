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
#include <sys/socket.h>
#include <netinet/in.h>

//PTTをホールドする最長時間
//特小機の場合は最長180秒で無線機側が解除
#define PTT_MAX_HOLD 170

//TTYデバイス用ディスクリプタ
int sfd;

//RTSでPTT制御する場合
int CTRL_VAL = TIOCM_RTS;
//DTRでPTT制御する場合
//int CTRL_VAL = TIOCM_DTR;

//制御線情報
int mbits;

//ホールド解除タイマ用
timer_t timerid;
struct itimerspec its;

void set_pin_on();
void set_pin_off();

//ピンをONにセット
void set_pin_on(){
    //以前にタイマが設定されていた場合は解除
    //ON-ONを繰り返すと最大PTTホールド時間を超えられるが
    //これはこれで利用価値がなくもないので対策しない
    if(timerid != NULL) timer_delete(timerid);
    //タイマを生成
    its.it_value.tv_sec = PTT_MAX_HOLD;
    its.it_value.tv_nsec = 0;
    timer_create(CLOCK_REALTIME, NULL, &timerid);

    printf("Set to on\n");
    ioctl(sfd, TIOCMBIS, &CTRL_VAL);

    //タイマをセット
    timer_settime(timerid, 0, &its, NULL);
}

//ピンをオフにセット
void set_pin_off(){
    printf("Set to off\n");
    ioctl(sfd, TIOCMBIC, &CTRL_VAL);
    //タイマを解除
    if(timerid != NULL) timer_delete(timerid);
}

//タイマによる強制オフハンドラ
void force_off_handler(int signum){
    printf("TIMER - ");
    printf("Set to off\n");
    ioctl(sfd, TIOCMBIC, &CTRL_VAL);
    //タイマを解除
    if(timerid != NULL) timer_delete(timerid);
}


//メイン
void main(int argc, char *argv[])
{
    char *ret;
    char ttydev[128];
    char strbuf[128];

    int sockfd, nsockfd;
    socklen_t c_len;
    struct sockaddr_in my_addr,c_addr; 
    char rec_buf[256];

    int myport;
    int sret;

    struct sigaction act, oldact;

    memset((char *)&my_addr, 0, sizeof(my_addr));

    if(argc != 3){
        printf("Usage: pttctl DEV PORT\n");
        exit(1);
    }

    //デバイス名
    strcpy(ttydev,argv[1]);
    //ポート番号
    myport = atoi(argv[2]);

    if(myport <= 0){
        printf("Usage: pttctl DEV PORT\n");
        exit(1);
    }

    //ソケット生成
    sockfd= socket(AF_INET, SOCK_STREAM, 0);

    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(myport);
    
    sret = bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr));
    if(sret < 0){
        perror("bind error\n");
        exit(1);
    }

    //タイマの初期値設定
    memset(&act, 0, sizeof(act));
    memset(&oldact, 0, sizeof(oldact));
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    //IOCTL対象のTTYデバイスオープン
    sfd = open(ttydev,O_RDWR | O_NOCTTY);

    if(sfd == -1){
        sprintf(strbuf, "%s", ttydev);
        perror(strbuf);
        exit(1);
    } 


    //開始時(TTYオープン)にオンになっているピンをオフする
    set_pin_off();

    //シグナルでの強制オフハンドラ登録
    act.sa_handler = force_off_handler;
    act.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &act, &oldact);


    listen(sockfd,5);

    //メインループ
    while(1){
        //accept
        c_len = sizeof(c_addr);
        nsockfd = accept(sockfd, (struct sockaddr*)&c_addr, &c_len);
        memset(rec_buf,0,sizeof(rec_buf));
        //socketから受信
        sret = recv(nsockfd, rec_buf, sizeof(rec_buf)-2, 0);
        //printf("%d\n", sret);
        //printf("%s\n", rec_buf);
        close(nsockfd);

        //受信した文字列によって処理
        //ONならピンをオンに
        if(strcmp(rec_buf,"NETPTT:ON\n") == 0) set_pin_on();
        //OFFならピンをオフに
        else if(strcmp(rec_buf,"NETPTT:OFF\n") == 0) set_pin_off();
        //ALTなら現在の状態を反転
        else if(strcmp(rec_buf,"NETPTT:ALT\n") == 0){
            ioctl(sfd, TIOCMGET, &mbits);
            if(mbits & CTRL_VAL) set_pin_off();
            else set_pin_on();
        }
        //EXITならプログラム終了
        else if(strcmp(rec_buf,"EXIT\n") == 0){
            sigaction(SIGALRM, &oldact, NULL);
            close(sockfd);
            close(sfd);
            exit(0);
        }
        //再度受信待ちへ
    }

    sigaction(SIGALRM, &oldact, NULL);
    close(sockfd);
    close(sfd);
    exit(0);
}
