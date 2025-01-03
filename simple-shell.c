#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX 1000

struct process{
    int pid;
    bool submitted;
    bool queued;
    bool completed;
    char* cmd;};

struct procTable{
    struct process processArray[MAX];
    int count;
    sem_t mutex;
};
char* historyArray[MAX];  
int historyCount=0;
double runtimeArray[MAX];
struct timeval startTime;
struct timeval endTime;
char* timeArray[MAX];
time_t currtime;
int pidArray[MAX];
char* NCPU;
char* TSLICE;
struct procTable* processTable;
int sharedMemory;
int schedulerPID;
int processSubmit(char *command);
pid_t mainPid;

void showHistory(){
            if (sem_wait(&((*processTable).mutex)) == -1) {
            perror("sem_wait");
            exit(1);
        }

        // Loop through the process table to print details
        for (int i = 0; i < (*processTable).count; i++) {
            struct process* proc = &(*processTable).processArray[i];
            printf("PID: %d | Command: %s | Submitted: %s | Queued: %s | Completed: %s\n",
                   proc->pid,
                   proc->cmd,
                   proc->submitted ? "Yes" : "No",
                   proc->queued ? "Yes" : "No",
                   proc->completed ? "Yes" : "No");
        }

        // Unlock shared memory after reading
        if (sem_post(&((*processTable).mutex)) == -1) {
            perror("sem_post");
            exit(1);
        }}
char* readInput() {
    char* buffer=(char*)malloc(MAX*sizeof(char*));
    printf("OSAssignment2@shell:~$ ");
    if (buffer==NULL){             
        perror("Memory allocation failed\n");
        exit(1);
    }

    if (fgets(buffer,MAX,stdin)==NULL){   
        printf("Could not read input\n");
        free(buffer);
        exit(1);
    }
    buffer[strcspn(buffer,"\n")]='\0';
    return buffer;
}
int parse(char* cmd,char** arr,char* delim){
    char* word=strtok(cmd,delim);
    int size=0;
    while (word!=NULL){
        arr[size++]=word;
        word=strtok(NULL,delim);}
    arr[size]=NULL;
    return size;}

bool isEmpty(const char* command) {
    while (*command == ' ') command++;
    return *command == '\0';           
}

void launch(char* command){
    char** arr = (char**)malloc(MAX * sizeof(char*));
    if (arr == NULL) {
        perror("Memory allocation failed");
        free(command);
        exit(1);
    }
    /*if (historyCount<MAX){
        printf("hi %s \n",command);
        historyArray[historyCount]=strdup(command);
        printf("%u \n",historyCount);
            }
    else{
            perror("Command Limit Exceeded");
            free(arr);
            free(command);
            exit(1);    }*/
    int arrsize=parse(command,arr,"|");
    bool isSubmit = (strncmp(command, "submit", 6) == 0);
    if(isEmpty(command)){return;}
    else if (isSubmit){

        if (sem_wait(&((*processTable).mutex)) == -1){
            perror("sem_wait");
            exit(1);
        }
        if ((*processTable).count >= MAX) {
            perror("Process limit exceeded");
            sem_post(&((*processTable).mutex));
            exit(1);}

        (*processTable).processArray[(*processTable).count].submitted = true;
        (*processTable).processArray[(*processTable).count].completed = false;
        (*processTable).processArray[(*processTable).count].queued = false;
        (*processTable).processArray[(*processTable).count].cmd =strdup(command);
        (*processTable).processArray[(*processTable).count].pid=processSubmit(command);
        //start_time((*processTable).processArray[(*processTable).count].start);
        (*processTable).count++;
        
        if (sem_post(&((*processTable).mutex))==-1){
            perror("sem_post");
            exit(1);
        }
        return;}
    else{
    if (sem_wait(&((*processTable).mutex)) == -1) {
        perror("sem_wait");
        free(arr);
        free(command);
        exit(1);
    }
    if ((*processTable).count<MAX){
        int index=(*processTable).count;
        //(*processTable).processArray[index].pid=0; // Initialize PID as 0; updated after fork
        (*processTable).processArray[index].submitted=isSubmit;
        (*processTable).processArray[index].queued=false;  // queued only if 'submit' command
        (*processTable).processArray[index].completed=false;
        (*processTable).processArray[index].cmd=strdup(command); // Store the command in shared memory
        (*processTable).count++; // Increase count of processes
    } else {
        perror("Process table limit reached");
        free(arr);
        free(command);
        sem_post(&((*processTable).mutex));
        exit(1);
    }
    if (sem_post(&((*processTable).mutex)) == -1) {
        perror("sem_post");
        free(arr);
        free(command);
        exit(1);
    }}
    
    if (arrsize==1){
        int check=fork();
        if (check == 0){
            char** cmdLst=(char**)malloc(MAX * sizeof(char*));
            if (cmdLst==NULL) {
                perror("Memory allocation failed");
                free(command);
                free(arr);
                exit(1);
            }
            
            else{
            parse(arr[0],cmdLst," ");
            execvp(cmdLst[0],cmdLst);
            perror("execvp failed");
            free(command);
            free(cmdLst);
            free(arr);
            exit(1);}
        }
        else if (check>0) {
            /*gettimeofday(&startTime, NULL);
            currtime=time(NULL);
            timeArray[historyCount]=strdup(ctime(&currtime));
            if (timeArray[historyCount] == NULL) {
                perror("Memory allocation failed for timeArray");
                free(command);
                free(arr);
                exit(1);
            }
            pidArray[historyCount]=wait(NULL);
            fflush(stdout);
            gettimeofday(&endTime, NULL);
            runtimeArray[historyCount]=(endTime.tv_sec-startTime.tv_sec)+(endTime.tv_usec-startTime.tv_usec)/1000000.0;
            historyCount++;*/
            
            if (sem_wait(&((*processTable).mutex)) == -1) {
                perror("sem_wait");
                exit(1);
            }

            (*processTable).processArray[(*processTable).count-1].pid=check;

            if(sem_post(&((*processTable).mutex))==-1){
                perror("sem_post");
                exit(1);
            }
            wait(NULL);
            if(sem_wait(&((*processTable).mutex))==-1){
                perror("sem_wait");
                exit(1);
            }
            (*processTable).processArray[(*processTable).count-1].completed=true;
            if (sem_post(&((*processTable).mutex))==-1){
                perror("sem_post");
                exit(1);
            }
        } 
        else {
            perror("Forking error\n");
            free(command);
            free(arr);
            exit(1);
    }}
    else{
    int pipelines[arrsize][2];
        int i=0;
        while(i<arrsize){
            if(pipe(pipelines[i])==-1){
            perror("Error creating pipeline\n");
            exit(1);
            }        
    
            int check=fork();
            if(check==0){
                char** cmdLst=(char**)malloc(MAX*sizeof(char*));
                if(cmdLst==NULL){
                    perror("Memory allocation failed");
                    free(command);
                    free(arr);
                    exit(1);
                }
                parse(arr[i],cmdLst," ");

                if(i>0){
                    close(pipelines[i-1][1]);
                    dup2(pipelines[i-1][0],STDIN_FILENO);
                    close(pipelines[i-1][0]);
                }
                if(i<arrsize-1){
                    dup2(pipelines[i][1],STDOUT_FILENO);
                    close(pipelines[i][0]);
                    close(pipelines[i][1]);
                }

                execvp(cmdLst[0],cmdLst);
                perror("execvp failed");
                free(cmdLst);
                free(command);
                free(arr);
                exit(1);
            }
            else if(check>0){
                if(i>0){
                    close(pipelines[i-1][0]);
                    close(pipelines[i-1][1]);
                }

                if(i==arrsize-1){
                    pidArray[historyCount]=wait(NULL);
                    gettimeofday(&endTime,NULL);
                    runtimeArray[historyCount]=(endTime.tv_sec-startTime.tv_sec)+(endTime.tv_usec-startTime.tv_usec)/1000000.0;
                    historyCount++;
                }
                else{
                    wait(NULL);}
            }
            else{
                perror("Forking error\n");
                free(command);
                free(arr);
                exit(1);    
            }
            i++;
}}}
void handleSigint(int sig){
    if (mainPid==getpid()){
    printf("\nExiting...\n");
    printf("ProcessTable:\n");
    /*for (int i = 0; i<historyCount; i++) {
        printf("%s PID:%d | Runtime:%f seconds | Start Time: %s\n", historyArray[i], pidArray[i], runtimeArray[i],timeArray[i]);}*/
        
        if (sem_wait(&((*processTable).mutex)) == -1) {
            perror("sem_wait");
            exit(1);
        }

        // Loop through the process table to print details
        for (int i = 0; i < (*processTable).count; i++) {
            struct process* proc = &(*processTable).processArray[i];
            printf("PID: %d | Command: %s | Submitted: %s | Queued: %s | Completed: %s\n",
                   proc->pid,
                   proc->cmd,
                   proc->submitted ? "Yes" : "No",
                   proc->queued ? "Yes" : "No",
                   proc->completed ? "Yes" : "No");
        }

        // Unlock shared memory after reading
        if (sem_post(&((*processTable).mutex)) == -1) {
            perror("sem_post");
            exit(1);
        }
        
        if (sem_destroy(&((*processTable).mutex)) == -1){
            perror("shm_destroy");
            exit(1);
            }
            if (munmap(processTable, sizeof(struct procTable)) < 0){
              printf("Error unmapping\n");
              perror("munmap");
              exit(1);
            }
            if (close(sharedMemory) == -1){
              perror("close");
              exit(1);
            }
            if (shm_unlink("/shm26") == -1){
            perror("shm_unlink");
            exit(1);
            }}
        exit(0);
    }
void handleSigchld(int signum, siginfo_t *info, void *context){
    if(signum == SIGCHLD){
        pid_t sender_pid=(*info).si_pid;
        if (sender_pid!=schedulerPID){
            if (sem_wait(&(*processTable).mutex)==-1){
                perror("sem_wait");
                exit(1);
            }
            for (int i=0; i<(*processTable).count; i++){
                if ((*processTable).processArray[i].pid == sender_pid){ 
                    (*processTable).processArray[i].completed = true;
                    (*processTable).processArray[i].queued=false;
                    break;
                }
            }
            if (sem_post(&(*processTable).mutex) == -1){
                perror("sem_post");
                exit(1);
            }
        }
    }
}
void mainloop(){
    int repeat=1;
    while (repeat!=0){
        char* cmd=readInput();
        if(cmd==NULL || strlen(cmd)==0){
            free(cmd);
            continue;
        }
        if ((strcmp(cmd, "history")) != 0 /*&& (strcmp(cmd,"stop")!=0)*/){

            launch(cmd);}
        /*else if(strcmp(cmd,"stop")==0){
        if (sem_wait(&(*processTable).mutex) == -1){
                perror("sem_wait");
                exit(1);
            }
            (*processTable).running=false;
            if (sem_post(&(*processTable).mutex) == -1){
                perror("sem_post");
                exit(1);
            }
            waitpid(schedulerPID,NULL,0);
            
            
        }
        
        else if(strcmp(cmd,"schedule")==0){
          int pid=fork();
          if (pid<0){
            perror("forking error");
            exit(1);
            }
          else if (pid==0){
            if (execl("./simple-scheduler", "SimpleScheduler", NCPU, TSLICE, NULL)==-1){
            perror("execl error");
            exit(1);}
            if (sem_destroy(&((*processTable).mutex)) == -1){
            perror("shm_destroy");
            exit(1);
            }
            if (munmap(processTable, sizeof(struct procTable)) < 0){
              printf("Error unmapping\n");
              perror("munmap");
              exit(1);
            }
            if (close(sharedMemory) == -1){
              perror("close");
              exit(1);
            }
            if (shm_unlink("/shm26") == -1){
            perror("shm_unlink");
            exit(1);
            }
        exit(0);
        }
        else{
          schedulerPID=pid;}
        }*/
        else if ((strcmp(cmd, "history") == 0)){ 
            int check = fork();
            if (check==0){
                showHistory();
                exit(0);
            }
            else if (check>0){
                currtime=time(NULL);
                char* timestr=ctime(&currtime);
                timeArray[historyCount]=strdup(timestr);

                gettimeofday(&startTime, NULL);
                historyArray[historyCount]="history"; 
                pidArray[historyCount]=wait(NULL);
                gettimeofday(&endTime, NULL);
                runtimeArray[historyCount]=(endTime.tv_sec-startTime.tv_sec)+(endTime.tv_usec-startTime.tv_usec)/1000000.0;
                historyCount+=1;
            }
            else{
                perror("Forking error.\n");
                free(cmd);
                exit(1);
            }
        }
        else{
            perror("error");
            free(cmd);
            //exit(1);
            continue;
        }
        free(cmd);
    }}
    

int processSubmit(char *command){
    int pid;
    char** arr = (char**)malloc(MAX * sizeof(char*));
    char *space=strchr(command,' ');
    if (space != NULL){
        memmove(command,space+1,strlen(command));
    }
    parse(command,arr," ");
    pid=fork();
    if (pid<0){
        printf("forking error.\n");
        exit(1);
    }
    else if (pid==0){
        if (execvp(arr[0],arr) == -1) {
            perror("execvp");
            printf("Not a valid/supported command.\n");
            exit(1);
        }
        exit(0);
    }
    else{
        if (kill(pid, SIGSTOP)==-1){
            perror("kill");
            exit(1);
        }
        return pid;
    }}
int main(int argc, char** argv){
    mainPid=getpid();
    struct sigaction signal_chld;
    if(memset(&signal_chld,0,sizeof(signal_chld))==0){
      perror("memset fault");
      exit(1);}
    signal_chld.sa_flags=SA_SIGINFO|SA_NOCLDSTOP|SA_RESTART;
    signal_chld.sa_sigaction=handleSigchld;
    if(sigaction(SIGCHLD,&signal_chld,NULL)==-1){
      perror("sigaction fault");
      exit(1);}
      
    struct sigaction signal_int;
    if(memset(&signal_int,0,sizeof(signal_int))==0){
      perror("memset fault");
      exit(1);}
    signal_int.sa_handler=handleSigint;
  
    if(sigaction(SIGINT,&signal_int,NULL)==-1){
      perror("sigaction fault");
      exit(1);}
    if (argc!=3){
        printf("Input format: %s <NCPU> <TIME_QUANTUM>\n",argv[0]);
        exit(1);
    }

    sharedMemory=shm_open("/shm26", O_CREAT|O_RDWR, 0666);
    if (sharedMemory==-1){
        perror("shm_open error");
        exit(1);
    }
    //printf("Shared Memory FD: %d\n", sharedMemory);
    if (ftruncate(sharedMemory,sizeof(struct procTable))==-1){
        perror("ftruncate error");
        exit(1);
    }
    processTable=mmap(NULL,sizeof(struct procTable),PROT_READ|PROT_WRITE, MAP_SHARED,sharedMemory,0);
    if (processTable==MAP_FAILED){
        perror("mmap");
        exit(1);
    }
    (*processTable).count=0;
    NCPU=argv[1];
    if (atoi(NCPU)<=0){
        printf("invalid ncpu value");
    }
    TSLICE=argv[2];
    if (atoi(TSLICE)<=0){
        printf("invalid time slice value");
        
    }
    
    if (sem_init(&((*processTable).mutex),1,1)==-1){
        perror("semaphore initialize error");
        exit(1);
    }
    int pid=fork();
          if (pid<0){
            perror("forking error");
            exit(1);
            }
          else if (pid==0){
            if (execl("./simple-scheduler", "SimpleScheduler", NCPU, TSLICE, NULL)==-1){
            perror("execl error");
            exit(1);}}
          else{
          schedulerPID=pid;}
        
        mainloop();
        if (sem_destroy(&((*processTable).mutex)) == -1){
        perror("shm_destroy");
        exit(1);
    }
        if (munmap(processTable, sizeof(struct procTable)) < 0){
            printf("Error unmapping\n");
            perror("munmap");
            exit(1);
        }
        if (close(sharedMemory) == -1){
            perror("close");
            exit(1);
        }
        if (shm_unlink("/shm26") == -1){
        perror("shm_unlink");
        exit(1);}
    
        
}
