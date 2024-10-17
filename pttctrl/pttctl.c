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

//使用するTTYデバイス名
#define TTYDEV "/dev/ttyUSB0"
//インタフェース用パイプ(FIFO)
#define PIPEF  "/tmp/PTTCTL"

//PTTをホールドする最長時間
//特小機の場合は最長180秒で無線機側が解除
#define PTT_MAX_HOLD 170

//TTYデバイス用ディスクリプタ
int sfd;

//パイプ用ファイルポインタ
FILE *fp;

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
void main()
{
    char *ret;
    char buf[32];
    char strbuf[128];

    struct sigaction act, oldact;

    //タイマの初期値設定
    memset(&act, 0, sizeof(act));
    memset(&oldact, 0, sizeof(oldact));
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    //IOCTL対象のTTYデバイスオープン
    sfd = open(TTYDEV,O_RDWR | O_NOCTTY);

    if(sfd == -1){
        sprintf(strbuf, "%s", TTYDEV);
        perror(strbuf);
        exit(1);
    } 

    //パイプを一旦unlinkしてから作成
    unlink(PIPEF);
    if(mkfifo(PIPEF, S_IRUSR|S_IWUSR) != 0){
        perror("PIPE");
        close(sfd);
        exit(1);
    }
    //パイプの権限を設定
    //権限をグループ等に制限する場合はここを修正
    chmod(PIPEF, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);

    //開始時(TTYオープン)にオンになっているピンをオフする
    set_pin_off();

    //シグナルでの強制オフハンドラ登録
    act.sa_handler = force_off_handler;
    act.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &act, &oldact);


    //メインループ
    while(1){
        //パイプをオープン(RO)
        fp = fopen(PIPEF, "r");

        //パイプからの1行待ち
        while(fgets(buf,32,fp) != NULL){
            //ONならピンをオンに
            if(strcmp(buf,"ON\n") == 0) set_pin_on();
            //OFFならピンをオフに
            else if(strcmp(buf,"OFF\n") == 0) set_pin_off();
            //ALTなら現在の状態を反転
            else if(strcmp(buf,"ALT\n") == 0){
                ioctl(sfd, TIOCMGET, &mbits);
                if(mbits & CTRL_VAL) set_pin_off();
                else set_pin_on();
            }
            //EXITならプログラム終了
            else if(strcmp(buf,"EXIT\n") == 0){
                sigaction(SIGALRM, &oldact, NULL);
                close(sfd);
                fclose(fp);
                exit(0);
            }
        }
        //パイプからEOFが来たら再度待機へ
    }

    sigaction(SIGALRM, &oldact, NULL);
    close(sfd);
    fclose(fp);
    exit(0);
}
