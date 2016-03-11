#include "shell.h"

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

//implement parser here

void help(){
    printF("%s%s%s%s%s%s%s%s",
           "Shell version 0.1\n",
           "These shell commands are defined internally. Type 'help' to see this list.\n\n",
           " bg [job_spec]\tBrings a program to the background\n",
           " fg [job_spec]\tBrings a program to the foregorund\n",
           " ps\t\tPrints the list of running programs and their status\n",
           " exec [name]\tStart a new program\n\n",
           " Press the '>' key to terminate the current process\n",
           " Press the '<' key to pause the current process\n");
}

void shell() {
    char line[50];

    char *cmd_list[5];
    cmd_list[0] = "bg";
    cmd_list[1] = "exec";
    cmd_list[2] = "fg";
    cmd_list[3] = "help";
    cmd_list[4] = "ps";

    printF("\n  ___   _  _   ___   _      _    \n");
    printF(" / __| | || | | __| | |    | |   \n");
    printF(" \\__ \\ | __ | | _|  | |__  | |__ \n");
    printF(" |___/ |_||_| |___| |____| |____|\n\n");

    while( 1 ) {
        printF(">");
        int n = read_line(line,50);
        line[n-2] = '\0';
        char *line_new = trimwhitespace(line);
        char *cmd = line_new;//parse here
        char first_letter = cmd[0];
        switch (first_letter) {
            case 'b':
                if(strcmp(cmd,cmd_list[0]) == 0){
                    printF("Not Implemented\n");
                    break;
                }
            case 'e':
                if(strcmp(cmd,cmd_list[1]) == 0){
                    printF("Not Implemented\n");
                    break;
                }
            case 'f':
                if(strcmp(cmd,cmd_list[2]) == 0){
                    printF("Not Implemented\n");
                    break;
                }
            case 'h':
                if(strcmp(cmd,cmd_list[3]) == 0){
                    help();
                    break;
                }
            case 'p':
                if(strcmp(cmd,cmd_list[4]) == 0){
                    printF("Not Implemented\n");
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
