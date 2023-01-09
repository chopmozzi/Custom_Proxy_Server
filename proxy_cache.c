/////////////////////////////////////////////////////////////////////////
// File Name    : proxy_cache.c                                        //
// Date         : 2022/06/08                                           //
// Os           : Ubuntu 16.04 LTS 64bits                              //
// Author       : Hwang Ji Ung                                         //
// Student ID   : 2018202020                                           //
// ------------------------------------------------------------------- //
// Title : System Programming Assignment #3-2 (proxy server)                     //
// Description : slog file을 기록하는 thread를 추가한다. critical section에서 접근할 때 //
// TID를 생성하고 log file작성을 완료하면 thread를 종료한다.                            //
//////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>      //sprintf
#include <string.h>     //strcpy, strcmp, strcat
#include <openssl/sha.h> //SHA1
#include <sys/types.h> //file and directory
#include <unistd.h> //access
#include <pwd.h> //getpwuid
#include <sys/stat.h> //file and directory
#include <fcntl.h> //creat
#include <dirent.h> //file information
#include <time.h>   //time function
#include <sys/socket.h> //socket
#include <netinet/in.h> //htonl, htons, inet_addr
#include <arpa/inet.h> //inet_ntoa
#include <stdlib.h> //exit function
#include <sys/wait.h> //waitpid function
#include <netdb.h>   //socket header
#include <sys/ipc.h> //semaphore header
#include <sys/sem.h> //semaphore header
#include <errno.h> //errno header
#include <pthread.h>

#define BUFFSIZE        1024
#define PORTNO          39999
int procount =0; //sub process count
//sha1_hash function -> url을 input으로 받고 hashed url로 변환하여 해당 hashed url을 return함
char *sha1_hash(char *input_url, char *hashed_url){
    unsigned char hashed_160bits[20]; //해시 된 160bits의 값.
    char hashed_hex[41];  //hashed_160bits를 1byte씩 16진수로 표현하고, 이를 문자열로 변환한 것
    int i;
    //SHA1 function Secure Hash Algorithm으로써 입력 한 test data를 160bits의 hased data로 반환함
    SHA1(input_url,strlen(input_url),hashed_160bits);
    //input_url->Hashing 할 데이터 strlen(input_url)->Hashing 할 데이터의 길이 hashed_160bits-> 해시 된 데이터를 저장할 배열
    //hashed_160bits의 데이터를 hashed_hex에 16진수로 변환하여 입력
    for(i=0;i<sizeof(hashed_160bits);i++)
        sprintf(hashed_hex + i*2, "%02x", hashed_160bits[i]);
    //hashed_url에 저장
    strcpy(hashed_url,hashed_hex);
    //return!!
    return hashed_url;
}
//getHomeDir function -> home directory path를 얻는 function
char *getHomeDir(char *home){
    struct passwd *usr_info = getpwuid(getuid());
    strcpy(home, usr_info->pw_dir);

    return home;
}
//handler function ->Sub process를 관리하는 function
//////////////////////////////////////////////////////////
// handler					                            //
// ==================================================== //
// Input : void					                        //
// Output : void					                    //
// Purpose: signal function에 사용하기 위해 사용            //
// 처음 main server을 열기 위해 bind를 하면 listen과 함께     //
// signal function과 호출됨 waitpid를 통해 sub process관리  //
/////////////////////////////////////////////////////////
static void handler()
{
    pid_t pid; //process id
    int status;//status
    while((pid=waitpid(-1, &status, WNOHANG))>0); //sub process wait!
}
//getIPAddr function ->domain을 IP Address로 변환
//////////////////////////////////////////////////////////
// getIPAddr					                        //
// ==================================================== //
// Input : char*					                    //
// Output : char*					                    //
// Purpose: domain을 IP Address로 변환하기 위해 사용         //
// request요청에서 받은 url(parsing한)을 받아 ip주소를 구함    //
// ip주소를 토대로 web server에 connect하기 위해 사용         //
/////////////////////////////////////////////////////////
char *getIPAddr(char *addr)
{
    struct hostent* hent;
    char* haddr;
    int len = strlen(addr);

    if((hent=(struct hostent*)gethostbyname(addr))!=NULL)
    {
        haddr=inet_ntoa(*((struct in_addr*)hent->h_addr_list[0]));
    }
    return haddr;
}
char mdir[4]={0,};  //cache내의 hash url값을 받아 만들어지는 directory의 이름 변수
char mfile[80]={0,}; //cache내 direcotry내의 hash url값을 받아 만들어지는 file의 이름 변수
char npurl2[BUFFSIZE]={0,}; //non parsing url(remove http://)
//thr_fn function ->MISS 시 로그 기록  thread
//////////////////////////////////////////////////////////
// thr_fn      					                        //
// ==================================================== //
// Input : void* (logpath)				                //
// Output : void*					                    //
// Purpose: thread 생성 시 로그를 작성하기 위한 함수(MISS)     //
/////////////////////////////////////////////////////////
void *thr_fn(void* logpath)
{
    //thread 생성 시 사용할 함수
    FILE*fp =fopen((char*)logpath,"at");
    time_t now; //프로그램 실행 시간을 재기 위한 변수
    struct tm *mtime;  //time variable  

    time(&now);
    mtime=localtime(&now);
    fprintf(fp,"[Miss]%s-[%d/%02d/%02d, %02d:%02d:%02d]\n",npurl2,mtime->tm_year+1900,mtime->tm_mon+1, mtime->tm_mday, mtime->tm_hour, mtime->tm_min, mtime->tm_sec);
    fclose(fp);
}
//thr_fn2 function ->HIT 시 로그 기록  thread
//////////////////////////////////////////////////////////
// thr_fn2      				                        //
// ==================================================== //
// Input : void* (logpath)				                //
// Output : char*					                    //
// Purpose: thread 생성 시 로그를 작성하기 위한 함수(HIT)     //
/////////////////////////////////////////////////////////
void *thr_fn2(void* logpath)
{
    FILE* fp=fopen((char*)logpath,"at");
    time_t now;
    struct tm *mtime;
    time(&now);
    mtime=localtime(&now);
    fprintf(fp,"[Hit]%s/%s-[%d/%02d/%02d, %02d:%02d:%02d]\n",mdir,mfile,mtime->tm_year+1900,mtime->tm_mon+1, mtime->tm_mday, mtime->tm_hour, mtime->tm_min, mtime->tm_sec);
    fprintf(fp,"[Hit]%s\n" ,npurl2);
    //print response message HIT
    fclose(fp);
}

//sig_alr function ->Check alarm signal function
//////////////////////////////////////////////////////////
// sig_alr					                            //
// ==================================================== //
// Input : signo					                    //
// Output : void					                    //
// Purpose: alarm에 따른 함수 실행을 하기 위해 사용            //
// request요청을 받았을 때 10초동안 response를 받지 못하면 실행  //
// No response 출력                                      //
/////////////////////////////////////////////////////////
static void sig_alr(int signo)
{   
    pthread_t tid;
    void *tret;
    //print no response
    printf("=========No response=========\n");
    exit(0);
}
int servertime=0;
time_t sstart; //서버 시간을 재기 위한 변수
time_t ssend;   //end time
//sig_int function ->Check sigint signal function
//////////////////////////////////////////////////////////
// sig_int					                            //
// ==================================================== //
// Input : signo					                    //
// Output : void					                    //
// Purpose: Ctrl+c 신호를 받았을 때 프로그램을 종료            //
// logfile에 sup process count와 프로그램실행 시간 출력      //
/////////////////////////////////////////////////////////
static void sig_int(int signo)
{   //Ctrl+c
    time(&ssend);
    //calculate server process time
    servertime=ssend-sstart;
    
        if(signo==SIGINT)
        {   //setting logpath
            char logpath[180]={0,}; //logfile directory의 경로
            char logfpath[180]={0,}; //logfile file의 경로
            char homepath[180]={0,}; //home directory path
            getHomeDir(homepath); //Execute getHomeDir    time_t now; //프로그램 실행 시간을 재기 위한 변수
            char* log="/logfile";  //logfile directory path
            char* logf="/logfile.txt"; //logfile.txt path
            strcpy(logpath, homepath);
            strcat(logpath,log);
            strcpy(logfpath, logpath);
            strcat(logfpath, logf);
            DIR* dir_ptr=NULL; //directory poionter for opendir, readdir
            dir_ptr=opendir(logpath);
            if(dir_ptr==NULL) //log directory가 있는지
                mkdir(logpath,S_IRWXU|S_IRWXG|S_IRWXO);
        
            FILE *fp=fopen(logfpath,"at");
            //print terminate message
            fprintf(fp,"**SERVER** [Terminated] run time: %d sec. #sub process: %d\n",servertime,procount);
            fclose(fp);
        } 
    exit(0); //terminated
}
//sig_chi function ->Check child process sigint signal function
//////////////////////////////////////////////////////////
// sig_chi					                            //
// ==================================================== //
// Input : signo					                    //
// Output : void					                    //
// Purpose: Ctrl+c 신호를 받았을 때 child process 처리      //
// terminate child process                              //
/////////////////////////////////////////////////////////
static void sig_chi(int signo)
{
    exit(0);
}

//p -> wait()
/////////////////////////////////////////////////////////////////
// p         					                               //
// ====================================================        //
// Input : semid(semaphore id)					               //
// Output : 성공과 실패 여부     					              //
// Purpose: semaphore를 critical section에 넣고 sem_op를 -1로 하여//
// critical section에 하나밖에 못 들어오게 함                       //
// (들어올 수 있는 개수를 1로 하였기 때문)                            //
/////////////////////////////////////////////////////////////////
int p(int semid)
{
    struct sembuf pbuf;

    pbuf.sem_num=0;
    pbuf.sem_op=-1;
    pbuf.sem_flg=SEM_UNDO;

    if(semop(semid, &pbuf,1)==-1){
        printf("p() operation is failed\n");
        return 0;
    }else{
        return 1;
    }
}
//v -> signal()
/////////////////////////////////////////////////////////////////
// v         					                               //
// ====================================================        //
// Input : semid(semaphore id)					               //
// Output : 성공과 실패 여부     					              //
// Purpose: semaphore를 critical section에서 탈출 시킴            //
// sem_op를 1로 하여 다시 하나의 process가 접근할 수 있도록 함          //
// (들어올 수 있는 개수를 1로 하였기 때문)                            //
/////////////////////////////////////////////////////////////////
int v(int semid)
{
    struct sembuf vbuf;
    vbuf.sem_num=0;
    vbuf.sem_op=1;
    vbuf.sem_flg=SEM_UNDO;

    if(semop(semid,&vbuf,1)==-1){
        printf("v() operation is failed\n");
        return 0;
    }else{
        return 1;
    }
}
//initsem -> semaphore init
/////////////////////////////////////////////////////////////
// initsem   					                           //
// ====================================================    //
// Input : semaphore key(port num)		                   //
// Output : semaphore id     					           //
// Purpose: semget을 통해 semid를 설정함                      //
/////////////////////////////////////////////////////////////
int initsem(key_t skey)
{
    int status = 0, semid;
    //skey== port number, 커널안에 없으면 sem을 만듦, 이미 존재하면 실패
    if((semid=semget(skey,1,IPC_CREAT|IPC_EXCL|0666))==-1){
        if(errno=EEXIST)
            semid=semget(skey,1,0);
    }else //critical section안의 프로세스 개수는 한개로 제어
        status=semctl(semid,0,SETVAL,1);
        if(semid==-1||status==-1){
            printf("initsem is failed\n");
            exit(1);
        }else //success return semaphore
            return semid;
}


//main function
int main()
{
    pthread_t tid; //thread variable
    int err;        //error variable
    void *tret;
    int semid, i; //semaphore id
    struct sockaddr_in server_addr, client_addr; //proxy server address, client(web browser) server address
    int socket_fd, client_fd; //proxy fd, client fd
    struct sockaddr_in web_addr; //web server address
    int web_fd; //web file descriptor
    int len, len_out;
    pid_t pid; //process id
    int status; //status integer
    //time_t sstart, send; //서버 시간을 재기 위한 변수
    time_t now; //프로그램 실행 시간을 재기 위한 변수
    char homepath[180]={0,}; //home directory path
    getHomeDir(homepath); //Execute getHomeDir function -> return home directory path
    char path[180]={0,};  //실제 directory와 file이 생성될 path
    char logpath[180]={0,}; //logfile directory의 경로
    char logfpath[180]={0,}; //logfile file의 경로
    char* cache="/cache";  //cache directory 경로를 strcat을 통해 붙여주기 위한 변수
    char* log="/logfile";  //logfile directory path
    char* logf="/logfile.txt"; //logfile.txt path
    int processtime=0; //프로그램 실행 시간
    //int servertime=0; //서버 실행 시간
    int count=0;
    char prif[180]={0,};
    //int procount=0; //sub process count!
    struct tm *mtime;  //time variable  
    char returnurl[80]={0,}; //sha1_hash function을 사용하기 위해 input으로 들어가는 변수
    char returnurl2[80]={0,}; //sha1_hash function을 사용하기 위해 output으로 받는 변수
    char curfile[180]={0,};  //비교하는 currentfile path
    char verpath[180]={0}; //폴더 내의 파일 목록을 비교하기 위한 기준 변수
    int option=0; //for using setsocket
    DIR* dir_ptr=NULL; //directory poionter for opendir, readdir
    struct dirent *file = NULL; //file information
    FILE *fp; //fopen, fclose, File function
    //logpath and cache path
    strcpy(logpath, homepath);
    strcat(logpath,log);
    strcat(homepath,cache);
    strcpy(logfpath, logpath);
    strcat(logfpath, logf);
    time(&sstart);
    //open main server
    if((socket_fd=socket(PF_INET, SOCK_STREAM,0))<0)
    {   //실패 시 예외처리
        printf("Server : Can't open stream socket\n");
        return 0;
    }
    option=1; //port를 중복 사용할 수 있도록 함
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    bzero((char*)&server_addr, sizeof(server_addr)); //init
    server_addr.sin_family=AF_INET; //setting proxy_server addr
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    server_addr.sin_port=htons(PORTNO);
    if(bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))<0)
    {   //실패 시 예외처리
        printf("Server : Can't bind local address\n");
        close(socket_fd);
        return 0;
    }
    //queue wait
    listen(socket_fd,5);
    //signal function
    signal(SIGCHLD, (void*)handler); //child process wait
    signal(SIGALRM,sig_alr); //alarm siganl function(no response)
    signal(SIGINT,sig_int);  //Ctrl+c signal function(process terminate)
    while(1)
    {
        //HTTP의 header, message를 받기 위한 변수
        struct in_addr inet_client_address;
        char buf[BUFFSIZE]={0,};
        char* bufarray[10]={NULL,};
        char response_header[BUFFSIZE]={0,};
        char response_message[BUFFSIZE]={0,};
        char tmp[BUFFSIZE]={0,};
        char method[20]={0,};
        char npurl[BUFFSIZE]={0,}; //non parsing url
        char url[BUFFSIZE]={0,}; //parsing url
        char *tok=NULL;
        len=sizeof(client_addr);
      
        //client에서 연결 요청
        client_fd=accept(socket_fd,(struct sockaddr*)&client_addr, &len);
        if(client_fd<0)
        {   //실패 시 예외 처리
            printf("Server : accept failed\n");
            close(socket_fd);
            return 0;
        }

        pid=fork(); //create sub process
        procount++;
     
        if(pid==-1)
        {   //실패 시 예외 처리
            close(client_fd);
            close(socket_fd);
            continue;
        }
        if(pid==0)
        {   
            //child process
            signal(SIGINT,sig_chi); //child process에서의 sig int처리
            inet_client_address.s_addr=client_addr.sin_addr.s_addr;
            //web browser request message
            int k=0;

            while(read(client_fd,buf,BUFFSIZE)!=0){
            
            //read(client_fd, buf, BUFFSIZE);
            //puts(buf);
            //read(client_fd, buf, BUFFSIZE);
            //puts(buf);
            //url 예외처리
            strcpy(tmp,buf);
            tok=strtok(tmp," ");
            strcpy(method, tok);
            if(strcmp(method, "GET")==0)
            {
                tok=strtok(NULL," ");
                strcpy(npurl, tok);
            }
            else
                continue;
            //firefox에서 들어오는 값들 예외처리
            if(strcmp(&npurl[strlen(npurl)-11], "favicon.ico") == 0||strcmp(&npurl[strlen(npurl)-11], "success.txt") == 0)
            {
                close(client_fd);
                exit(0);
            }
            if(strcmp(&npurl[strlen(npurl)-4],"ipv4")==0||strcmp(&npurl[strlen(npurl)-4],"ipv6")==0)
            {
                close(client_fd);
                exit(0);
            }
            if((semid=initsem(PORTNO))<0)
                exit(1);
            printf("*PID# %d is waiting for the semaphore.\n",getpid());
            p(semid); //semaphore control(only one process access)
            printf("*PID# %d is in the critical zone.\n",getpid());
            sleep(10);
            //url parshing
            for(int i=0; i<strlen(npurl);i++)
            {
                if(npurl[i]=='/'&&npurl[i+1]!='/')
                {
                    strcpy(url,npurl+i+1);
                    strcpy(npurl2,npurl+i+1);
                    break;
                }
            }
            //getIPAddr을 사용하기 위해 슬래시들을 다 떼줌
            for(int i=0; i<strlen(url);i++)
            {
                if(url[i]=='/')
                {
                    url[i]='\0';
                }
            }
            //print request message
            /*printf("[%s : %d] client was connected\n", inet_ntoa(inet_client_address), client_addr.sin_port);
            puts("===============================\n");
            printf("Request from [%s : %d]\n",inet_ntoa(inet_client_address), client_addr.sin_port);
            puts(buf);
            puts("===============================\n");*/
            if(count ==0) //first url input
            {   //create cache direcotry and logfile directory
                umask(0);
                mkdir(homepath,S_IRWXU|S_IRWXG|S_IRWXO);
                mkdir(logpath,S_IRWXU|S_IRWXG|S_IRWXO);
                strcat(homepath,"/");
                //path에 cache 경로를 포함한 값을 복사
                strcpy(path,homepath);
                count++;
            }
            //returnrul2변수에 sha1_hash로 변환한 input url의 값을 복사
            strcpy(returnurl2,sha1_hash(npurl2,returnurl));
            //directory의 이름은 3글자이므로 해당 부분만큼 복사
            strncpy(mdir,returnurl2,3);
            //쓰레기 값 방지를 위해 null 삽입
            mdir[3]='\0';
            //해당 경로로 mkdir 실행
            strcat(path,mdir);
            dir_ptr=opendir(path);  //path에 있는 directory의 값을 불러움
            if(dir_ptr==NULL)       //해당 폴더가 없을 경우 폴더 생성
            {
                umask(0);
                mkdir(path,S_IRWXU|S_IRWXG|S_IRWXO);
                dir_ptr=opendir(path); //해당 경로의 데이터를 불러옴
            }
            strcat(path,"/");
            memset(verpath,0, sizeof(verpath)); //initialization verpath
            strcpy(verpath, path); //비교할 때 기준이 되는 verpath 값 초기화
            //4번째 글자부터 file의 이름으로 하기 위해 복사
            strcpy(mfile,returnurl2+3);
            strcat(path,mfile);
            for(file=readdir(dir_ptr);file;file=readdir(dir_ptr)) //directory내의 파일 목록을 검색함
            {   //pass . or ..
                if(strcmp(file->d_name, ".")==0||strcmp(file->d_name, "..")==0)
                {
                    continue;
                }
                strcpy(curfile,verpath); //파일목록의 내용을 curfile 값에 복사
                strcat(curfile,file->d_name); //파일 이름을 붙여줌
                if(strcmp(curfile,path)==0) //동일한 파일 이름을 찾았을 경우(기존 파일이 존재할 경우) 
                {   
                    //HIT case
                    //fp=fopen(logfpath,"at");
                    strcpy(prif,mdir);
                    strcat(prif,"/");
                    strcat(prif,mfile);
                    //time(&now);
                    //mtime=localtime(&now);
                    //wriet hit logfile
                    err=pthread_create(&tid, NULL, thr_fn2,logfpath); //thread create
                    printf("*PID# %d create the *TID# %u.\n",getpid(),(unsigned int)tid);
                    //fprintf(fp,"[Hit]%s/%s-[%d/%02d/%02d, %02d:%02d:%02d]\n",mdir,mfile,mtime->tm_year+1900,mtime->tm_mon+1, mtime->tm_mday, mtime->tm_hour, mtime->tm_min, mtime->tm_sec);
                    //fprintf(fp,"[Hit]%s\n" ,npurl2);
                    //print response message HIT
                    //fclose(fp);
                    printf("*TID# %u is exited.\n",(unsigned int)tid);
                    pthread_join(tid, &tret); //thread exit
                    //HIT일 경우 web server를 거치지 않고 proxy에서 반환
                    fp = fopen(curfile,"rt");
                    //cache파일에 있는 response data를 읽어옴
                    long lSize;
                    fseek(fp, 0 , SEEK_END);
                    lSize = ftell(fp);
                    rewind(fp);
                    char *pLine;
                    pLine = (char*)malloc(sizeof(char)*lSize);
                    fread(pLine,1,lSize,fp);
                    //response message write!
                    write(client_fd,pLine,lSize);
                    //close(client_fd);
                    free(pLine);
                    fclose(fp);
                }
            }//기존 파일이 존재하지 않을 경우
            if(access(path,F_OK)==-1)
            {   //MISS
                if(access(logfpath,F_OK)==-1)
                {
                    creat(logfpath,0777);
                }
                //logfile MISS write
                //fp=fopen(logfpath,"at");
                err=pthread_create(&tid, NULL, thr_fn,logfpath); //thread create
                printf("*PID# %d create the *TID# %u.\n",getpid(),(unsigned int)tid);
				//fprintf(fp,"[Miss]%s-[%d/%02d/%02d, %02d:%02d:%02d]\n",npurl2,mtime->tm_year+1900,mtime->tm_mon+1, mtime->tm_mday, mtime->tm_hour, mtime->tm_min, mtime->tm_sec);
                //fclose(fp);
                printf("*TID# %u is exited.\n",(unsigned int)tid);
                pthread_join(tid, &tret); //thread exit
                if((web_fd=socket(PF_INET, SOCK_STREAM,0))<0)
                {
                    printf("Server : Can't open stream socket\n");
                    return 0;
                }
                //web_addr 초기화
                bzero((char*)&web_addr, sizeof(web_addr));
                web_addr.sin_family=AF_INET;
                //도메인을 ip로 변환
                char* IPAddr=getIPAddr(url);
                //web addr setting
                web_addr.sin_addr.s_addr=inet_addr(IPAddr);
                web_addr.sin_port=htons(80);
                //proxy와 web server를 연결
                if(connect(web_fd, (struct sockaddr*)&web_addr, sizeof(web_addr))<0)
                {
		            printf("can't connect.\n");
		            return -1;
	            }
                //transfer request message to web server from proxy
                write(web_fd,buf,BUFFSIZE);
                //start alarm!!
                alarm(60);
                //init buf
                //bzero(buf,sizeof(buf));
                memset(buf,0,BUFFSIZE);
                //read response message from web server
                FILE *file = NULL;
                int buf2[BUFFSIZE]={0,};
                ssize_t nbyte=1;
                //create cache file
                creat(path,0777);
                file = fopen(path,"wb");
                while(nbyte!=0)
                {   //write response message to cache file
                    nbyte=read(web_fd,buf,1024);
                    fwrite(buf,sizeof(char),nbyte,file);
                }
                fclose(file);
                sleep(30);
                //10초안에 response message를 읽으면 알람끔
                alarm(0);
                //read response message to cache file
                file = fopen(path,"rb");
                long lSize;
                fseek(file, 0 , SEEK_END);
                lSize = ftell(file);
                rewind(file);
                char *pLine;
                pLine = (char*)malloc(sizeof(char)*lSize);
                fread(pLine,1,lSize,file);
                //write response message
                write(client_fd,pLine,lSize);
                //puts(pLine);
                free(pLine);
                fclose(file);
                //printf("chop\n");
                
                //read(client_fd,buf, BUFFSIZE);
                /*if(buf==NULL)
                    printf("mojji\n");
                else
                    puts(buf);*/
            }
            bzero(buf,sizeof(buf));
            strcpy(path,homepath);
            memset(curfile,0,sizeof(curfile));
            //print disconnect message
            //printf("[%s : %d] client was disconnected\n", inet_ntoa(inet_client_address), client_addr.sin_port);
            //close client, web

            printf("*PID# %d exited the critical zone.\n",getpid());
            
            v(semid); //deallocation semaphore
            
            }
            
            close(client_fd);
            close(web_fd);
            exit(0);

        }
    }
    //close socket
    close(socket_fd);
    return 0;
}