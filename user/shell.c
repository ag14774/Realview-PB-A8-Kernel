#include "shell.h"

#define HT_SIZE 50
#define HT_BUCKET 4

typedef struct {
    int pid;
    char name[30];
} hashentry_t;

hashentry_t ht[HT_SIZE][HT_BUCKET];

char current_cmd[50];

uint32_t hash_pid(int pid){
    return ((uint32_t)(282563095*pid+29634029) % 993683819) % HT_SIZE ; //best so far
}

int isStillAlive(int pid){
    int stat = procstat(pid);
    if(!stat)
        return 0;
    else
        return 1;
}

hashentry_t* find_proc(int pid){
    uint32_t index = hash_pid(pid);
    uint32_t j = 0;
    while(j<HT_BUCKET){
        if(ht[index][j].pid == pid)
            return &ht[index][j];
        j = j + 1;
    }
    return NULL;
}

void add_proc(int pid, char* name){
    uint32_t index = hash_pid(pid);
    uint32_t j = 0;
    while(ht[index][j].pid != 0 && isStillAlive(ht[index][j].pid) && j<HT_BUCKET){
        j = j + 1;
    }
    if(j==HT_BUCKET)
        return;
    ht[index][j].pid = pid;
    memcpy(ht[index][j].name, name, 30);
}

char *trimwhitespace(char *str){
    char *end;

    // Trim leading space
    while(isspace(*str)||*str=='|') str++;

    if(*str == 0)  // All spaces?
        return str;

    //Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && (isspace(*end)||*end=='|')) end--;

    // Write new null terminator
    *(end+1) = 0;

    return str;
}

void parse_command(char* line, char** argbuff){
    int i = 0;
    int len = strlen(line);
    memcpy(current_cmd,line,len+1);
    line = current_cmd;
    int j = 0;
    argbuff[j++] = line;
    for(i = 0;i<len;i++){
        if(line[i]=='|'){
            line[i] = '\0';
            if(line[i+1]!='\0'){
                argbuff[j++] = (char*)3; //signal to detect end of first command
            }
            for(i=i;i<len && (line[i+1]==' '||line[i+1]=='|');i++);
            if(i!=len && line[i+1]!='\0'){
                argbuff[j++] = &line[i+1];
            }
        }
        else if(line[i]==' '){
            line[i] = '\0';
            for(i=i;i<len && line[i+1]==' ';i++);
            if(line[i+1]!='\0' && line[i+1]!='|')
                argbuff[j++] = &line[i+1];
        }
    }
    argbuff[j] = (char*)0;
}

//only for positive
int string2int(char* string){
    int acc = 0;
    for(int i=0;string[i]!='\0';i++){
        if(string[i]<'0' || string[i]>'9')
            return -1;
        acc = 10*acc + string[i] - '0';
    }
    return acc;
}

//implement parser here

void help(){
    printF("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
            "Shell version 0.1\n",
            "These shell commands are defined internally. Type 'help' to see this list.\n\n",
           " bg [job_num]\t\t\tBrings a program to the background\n",
           " fg [job_num]\t\t\tBrings a program to the foreground\n",
           " ps\t\t\t\tPrints the list of running programs and their status\n",
           " nice [job_num] [priority]\tChange the priority of a program\n",
           " exec [name] [param1 param2...]\tStart a new program\n",
           " shutdown \t\t\tPrepare OS for shutdown\n\n",
           " cd [path]\t\t\tChange current directory\n",
           " ls \t\t\t\tList all files in the current directory\n",
           " pwd \t\t\t\tPrint current directory path\n",
           " rm [name]\t\t\tRemove a file\n",
           " touch [name]\t\t\tCreate a file\n",
           " mkdir [name]\t\t\tCreate a new folder\n",
           " rmdir [name]\t\t\tRemove a folder\n\n",
           " Press the '>' key to terminate the current process\n",
           " Press the '<' key to pause the current process\n");
}

void ps(){
    int pids[50];
    char* shellname = "shell";
    int num = procs(pids);
    printF("\nJOB\tNAME\t\tSTATE\n");
    for(int i = 0;i<num;i++){
        if(pids[i]==1)
            continue;
        int stat = procstat(pids[i]);
        hashentry_t* hentry = find_proc(pids[i]);
        int parpid = pids[i];
        char* name;
        while(!hentry){
            parpid = getppid(parpid);
            if(parpid <= 1)
                break;
            hentry = find_proc(parpid);
        }
        if(parpid<=1){
            name = shellname;
        }
        else{
            name = hentry->name;
        }
        switch(stat){
        case 0:
        case 1:
            if(parpid == pids[i])
                printF("%d\t%s\t\t%s\n",pids[i],name, "RUNNING");
            else
                printF("%d\t%s%s\t\t%s\n",pids[i],name,"_child", "RUNNING");
            break;
        case 2:
            if(parpid == pids[i])
                printF("%d\t%s\t\t%s\n",pids[i],name, "BLOCKED");
            else
                printF("%d\t%s%s\t\t%s\n",pids[i],name,"_child", "BLOCKED");
            break;
        case 3:
            if(parpid == pids[i])
                printF("%d\t%s\t\t%s\n",pids[i],name, "STOPPED");
            else
                printF("%d\t%s%s\t\t%s\n",pids[i],name,"_child", "STOPPED");
            break;
        default:
            break;
        }
    }
    printF("\n");
}

void shell() {
    char line[50];
    char* argbuff[50];

    char *cmd_list[20];
    cmd_list[0]  = "bg";
    cmd_list[1]  = "cd";
    cmd_list[2]  = "exec";
    cmd_list[3]  = "fg";
    cmd_list[4]  = "help";
    cmd_list[5]  = "ls";
    cmd_list[6]  = "mkdir";
    cmd_list[7]  = "nice";
    cmd_list[8]  = "ps";
    cmd_list[9]  = "pwd";
    cmd_list[10] = "rmdir";
    cmd_list[11] = "rm";
    cmd_list[12] = "shutdown";
    cmd_list[13] = "touch";

    printF("\n  ___   _  _   ___   _      _    \n");
    printF(" / __| | || | | __| | |    | |   \n");
    printF(" \\__ \\ | __ | | _|  | |__  | |__ \n");
    printF(" |___/ |_||_| |___| |____| |____|\n\n");

    while( 1 ) {
        printF(">");
        int n = read_line(line,50);
        line[n-1] = '\0';
        char *line_new = trimwhitespace(line);
        parse_command(line_new, argbuff);
        char *cmd = argbuff[0];//parse here
        char first_letter = cmd[0];
        switch (first_letter) {
            case 'b':
                if(strcmp(cmd,cmd_list[0]) == 0){
                    int proc_pid = string2int(argbuff[1]);
                    kill(proc_pid,SIGCONT);
                    //printF("Not Implemented\n");
                    break;
                }
            case 'c':
                if(strcmp(cmd,cmd_list[1]) == 0){
                    int res = chdir(argbuff[1]);
                    if(res != 0){
                        printF("An error occurred!\n");
                    }
                    break;
                }
            case 'e':
                if(strcmp(cmd,cmd_list[2]) == 0){
                    int i = 1;
                    int j = 1;
                    int firstpid = 0;
                    int prevpipe = -1;
                    while(1){
                        if(argbuff[i] == (char*)3){
                            argbuff[i] = (char*)0;
                            int pipe = getpipe();
                            fcntl(pipe, READ_WRITE|CLOSE_ON_EXEC);
                            int pid = fork();
                            if(pid == 0){
                                redir(1, pipe);
                                if(prevpipe >= 0)
                                    redir(0, prevpipe); 
                                int success = exec(argbuff[j], &argbuff[j]);
                                if(success<0){
                                    exit(-1);
                                }
                            }
                            else{
                                if(prevpipe>=0)
                                    close(prevpipe);
                                prevpipe = pipe;
                                if(j == 1)
                                    firstpid = pid;
                                add_proc(pid, argbuff[j]);
                                j = i + 1;
                            }
                        }
                        else if(argbuff[i] == (char*)0){
                            int pid = fork();
                            if(pid == 0){
                                if(prevpipe>=0)
                                    redir(0, prevpipe);
                                int success = exec(argbuff[j], &argbuff[j]);
                                if(success<0){
                                    exit(-1);
                                }
                            }
                            else{
                                if(prevpipe>=0)
                                    close(prevpipe);
                                if(j == 1)
                                    firstpid = pid;
                                add_proc(pid, argbuff[j]);
                                waitpid(firstpid);
                            }
                            break;
                        }
                        i++;
                    }
                    break;
                }
            case 'f':
                if(strcmp(cmd,cmd_list[3]) == 0){
                    int proc_pid = string2int(argbuff[1]);
                    waitpid(proc_pid);
                    //printF("Not Implemented\n");
                    break;
                }
            case 'h':
                if(strcmp(cmd,cmd_list[4]) == 0){
                    help();
                    break;
                }
            case 'l':
                if(strcmp(cmd,cmd_list[5]) == 0){
                    char buff[200];
                    int res = getdents(buff);
                    int count = 0;
                    char* dir = buff;
                    for(int i=0;buff[i]!='\0';i++){
                        if(buff[i] == '|'){
                            count++;
                            buff[i] = '\0';
                            printF("%s\n",dir);
                            if(count == res)
                                break;
                            dir = &buff[i+1];
                        }
                    }
                    break;
                }
            case 'm':
                if(strcmp(cmd,cmd_list[6]) == 0){
                    int res = mkdir(argbuff[1]);
                    if(res != 0){
                        printF("An error occurred!\n");
                    }
                    break;
                }
            case 'n':
                if(strcmp(cmd,cmd_list[7]) == 0){
                    int proc_pid = string2int(argbuff[1]);
                    int priority = string2int(argbuff[2]);
                    nice(proc_pid, priority);
                    break;
                }
            case 'p':
                if(strcmp(cmd,cmd_list[8]) == 0){
                    ps();
                    //printF("Not Implemented\n");
                    break;
                }
                else if(strcmp(cmd, cmd_list[9]) == 0){
                    char buff[100];
                    buff[0] = '\0';
                    int r = getcwd(buff);
                    if(r==0){
                        printF("%s\n",buff);
                    }
                    else{
                        printF("An error occurred!\n");
                    }
                    break;
                }
            case 'r':
                if(strcmp(cmd,cmd_list[10]) == 0){
                    int res = rmdir(argbuff[1]);
                    if(res != 0){
                        printF("An error occurred!\n");
                    }
                    break;
                }
                else if(strcmp(cmd, cmd_list[11]) == 0){
                    int res = unlink(argbuff[1]);
                    if(res != 0){
                        printF("An error occurred!\n");
                    }
                    break;
                }
            case 's':
                if(strcmp(cmd,cmd_list[12]) == 0){
                    clear_cache();
                    printF("You can shutdown your computer now!\n");
                    break;
                }
            case 't':
                if(strcmp(cmd,cmd_list[13]) == 0){
                    int res = creat(argbuff[1]);
                    if(res != 0){
                        printF("An error occurred!\n");
                    }
                    break;
                }
            default: {
                if(first_letter)
                    printF("Command not recognised\n");
            }
        }
    }

    return;
}

void (*entry_shell)() = &shell;
