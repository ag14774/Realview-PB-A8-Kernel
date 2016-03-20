#include "shell.h"

typedef struct {
    int jobnumber;
    int pid;
    char name[30];
} proc_t;

typedef struct {
    proc_t proc[50];
    int freeJobID[50];
    int freeJobIDpointer;
    int nextJobID;
} proc_table_t;

proc_table_t proc_table;

void add_proc(int pid, char* name){
    int index;
    if(proc_table.freeJobIDpointer >= 0){
        index = proc_table.freeJobID[proc_table.freeJobIDpointer--];
    }
    else {
        index = proc_table.nextJobID++;
    }
    proc_table.proc[index].jobnumber = index;
    proc_table.proc[index].pid = pid;
    memcpy(proc_table.proc[index].name, name, 30);
}

void remove_proc(int jobID){
    proc_table.proc[jobID].jobnumber = 0;
    proc_table.freeJobID[++proc_table.freeJobIDpointer] = jobID;
}

void update_table(){
    for(int i = 0;i<proc_table.nextJobID;i++){
        if(i == proc_table.proc[i].jobnumber){ //a process is valid only if the index matches the jobnumber
            int stat = procstat(proc_table.proc[i].pid);
            if(stat<0)
                remove_proc(i);
        }
    }
}

char *trimwhitespace(char *str){
    char *end;

    // Trim leading space
    while(isspace(*str)) str++;

    if(*str == 0)  // All spaces?
        return str;

    //Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace(*end)) end--;

    // Write new null terminator
    *(end+1) = 0;

    return str;
}

void parse_command(char* line, char** argbuff){
    int i = 0;
    int len = strlen(line);
    int j = 0;
    argbuff[j++] = line;
    for(i = 0;i<len;i++){
        if(line[i]==' '){
            line[i] = '\0';
            if(line[i+1]!='\0')
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
    printF("%s%s%s%s%s%s%s%s%s",
            "Shell version 0.1\n",
            "These shell commands are defined internally. Type 'help' to see this list.\n\n",
           " bg [job_num]\t\t\tBrings a program to the background\n",
           " fg [job_num]\t\t\tBrings a program to the foregorund\n",
           " ps\t\t\t\tPrints the list of running programs and their status\n",
           " nice [job_num] [priority]\tChange the priority of a program\n",
           " exec [name] [param1 param2...]\tStart a new program\n\n",
           " Press the '>' key to terminate the current process\n",
           " Press the '<' key to pause the current process\n");
}

void ps(){
    printF("\nJOB\tNAME\tSTATE\n");
    for(int i = 0;i<proc_table.nextJobID;i++){
        if(i == proc_table.proc[i].jobnumber){ //a process is valid only if the index matches the jobnumber
            int stat = procstat(proc_table.proc[i].pid);
            if(stat<0)
                remove_proc(i);
            switch(stat){
            case 0:
            case 1:
                printF("%d\t%s\t%s\n",i,proc_table.proc[i].name, "RUNNING");
                break;
            case 2:
                printF("%d\t%s\t%s\n",i,proc_table.proc[i].name, "BLOCKED");
                break;
            case 3:
                printF("%d\t%s\t%s\n",i,proc_table.proc[i].name, "STOPPED");
                break;
            default:
                break;
            }
        }
    }
    printF("\n");
}

void shell() {
    proc_table.nextJobID = 0;
    proc_table.proc[0].jobnumber=-1;
    proc_table.freeJobIDpointer = -1;

    char line[50];
    char* argbuff[50];

    char *cmd_list[5];
    cmd_list[0] = "bg";
    cmd_list[1] = "exec";
    cmd_list[2] = "fg";
    cmd_list[3] = "help";
    cmd_list[4] = "nice";
    cmd_list[5] = "ps";

    printF("\n  ___   _  _   ___   _      _    \n");
    printF(" / __| | || | | __| | |    | |   \n");
    printF(" \\__ \\ | __ | | _|  | |__  | |__ \n");
    printF(" |___/ |_||_| |___| |____| |____|\n\n");

    while( 1 ) {
        printF(">");
        int n = read_line(line,50);
        line[n-2] = '\0';
        char *line_new = trimwhitespace(line);
        parse_command(line_new, argbuff);
        char *cmd = argbuff[0];//parse here
        char first_letter = cmd[0];
        switch (first_letter) {
            case 'b':
                if(strcmp(cmd,cmd_list[0]) == 0){
                    int job = string2int(argbuff[1]);
                    if(job != proc_table.proc[job].jobnumber)
                        break;
                    int proc_pid = proc_table.proc[job].pid;
                    kill(proc_pid,SIGCONT);
                    //printF("Not Implemented\n");
                    break;
                }
            case 'e':
                if(strcmp(cmd,cmd_list[1]) == 0){
                    int pid = fork();
                    char** argv = &argbuff[1];
                    if(pid==0){
                        int success = exec(argbuff[1], argv);
                        if(success<0)
                            exit();
                    }
                    else{
                        if(proc_table.nextJobID>=50 && proc_table.freeJobIDpointer<0)
                            update_table();
                        add_proc(pid, argbuff[1]);
                        waitpid(pid);
                    }
                    //printF("Not Implemented\n");
                    break;
                }
            case 'f':
                if(strcmp(cmd,cmd_list[2]) == 0){
                    int job = string2int(argbuff[1]);
                    if(job != proc_table.proc[job].jobnumber)
                        break;
                    int proc_pid = proc_table.proc[job].pid;
                    waitpid(proc_pid);
                    //printF("Not Implemented\n");
                    break;
                }
            case 'h':
                if(strcmp(cmd,cmd_list[3]) == 0){
                    help();
                    break;
                }
            case 'n':
                if(strcmp(cmd,cmd_list[4]) == 0){
                    int job = string2int(argbuff[1]);
                    int priority = string2int(argbuff[2]);
                    if(job != proc_table.proc[job].jobnumber)
                        break;
                    int proc_pid = proc_table.proc[job].pid;
                    nice(proc_pid, priority);
                    break;
                }
            case 'p':
                if(strcmp(cmd,cmd_list[5]) == 0){
                    ps();
                    //printF("Not Implemented\n");
                    break;
                }
            default:
                if(first_letter)
                    printF("Command not recognised\n");
                break;
        }
    }

    return;
}

void (*entry_shell)() = &shell;
