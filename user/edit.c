#include "edit.h"

char file[1000];
int lineIndices[100];
int lines;

void find_lines(){
    int j = 0;
    lines = 0;
    int i = 0;
    for(i=0;i<1000 && file[i]!=0xFF;i++){
        if(file[i]=='\n'){
            lineIndices[lines++] = j;
            j = i+1;
        }
    }
    file[i] = '\0';
    lineIndices[lines++] = j;
    lineIndices[lines] = i+1;
}

void editMode(int line){
    int start = lineIndices[line];
    int end = lineIndices[line+1];
    for(int i=start;i<end;i++){
        if(file[i]!='\0' && file[i]!='\n' && file[i]!=0xFF){
            simkey(file[i]);
        }
    }
    char buf[100];
    int r=read_line(buf,100);
    file[lineIndices[lines]-1] = 0xFF;
    memmove(&file[start+r],&file[end], lineIndices[lines]-end);
    for(int i=0;i<r;i++){
        file[start+i] = buf[i];
    }
    if(line==lines-1){
        file[start+r-1]=0xFF;
    }
    find_lines();
}

void deleteLine(int line){
    int start = lineIndices[line];
    int end = lineIndices[line+1];
    file[lineIndices[lines]-1] = 0xFF;
    memmove(&file[start],&file[end], lineIndices[lines]-end);
    if(line==lines-1){
        file[start-1] = 0xFF;
    }
    find_lines();
}

void printline(int i){
    if(i<lines){
        int start = lineIndices[i];
        int end   = lineIndices[i+1];
        char* line = &file[start];
        write(1,line,end-start-1);
    }
    printF("\n");
}

void insertLine(int after){
    if(after>=lines)
        after=lines-1;
    char buf[100];
    int r = read_line(buf,100);
    file[lineIndices[lines]-1] = 0xFF;
    int end = lineIndices[after+1];
    memmove(&file[end+r],&file[end],lineIndices[lines]-end);
    for(int i=0;i<r;i++){
        file[end+i] = buf[i];
    }
    if(after==lines-1){
        file[end-1] = '\n';
        file[end+r-1] = 0xFF;
    }
    find_lines();
}

char *trimspace(char *str){
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

int s2in(char* string){
    if(string==NULL)
        return -1;
    int acc = 0;
    for(int i=0;string[i]!='\0';i++){
        if(string[i]<'0' || string[i]>'9')
            return -1;
        acc = 10*acc + string[i] - '0';
    }
    return acc;
}

void edit(int argc, char** argv) {
  printF("Amazing editor 2016!\n");

  int fd = open(argv[1],READ_ONLY);
  read(fd, file, 1000);
  find_lines();
  close(fd);
  
  while(1){
    printF(">>>");
    char c[20];
    int r = read_line(c, 20);
    c[r-1] = '\0';
    char* line = trimspace(c);
    char* com = strtok(line, " \n");
    if(strcmp(com,"print")==0){
        char* arg2 = strtok(NULL, " \n");
        int n = s2in(arg2);
        if(n == -1){
            printF("%s\n",file);
        }
        else{
            printline(n);
        }
    }
    else if(strcmp(com,"edit")==0){
        char* arg2 = strtok(NULL, " \n");
        int n = s2in(arg2);
        if(n>=0){
            editMode(n);
        }
    }
    else if(strcmp(com,"delete")==0){
        char* arg2 = strtok(NULL, " \n");
        int n = s2in(arg2);
        if(n>=0){
            deleteLine(n);
        }
    }
    else if(strcmp(com, "save")==0){
        int fd = open(argv[1],READ_WRITE|CLEAR_FILE);
        write(fd, file, strlen(file));
        close(fd);
    }
    else if(strcmp(com, "insert")==0){
        char* arg2 = strtok(NULL, " \n");
        int n = s2in(arg2);
        if(n>=0){
            insertLine(n);
        }
        else{
            insertLine(lines-1);
        }
    }
    else if(strcmp(com, "exit")==0){
        break;
    }
  }
  return;
}

void (*entry_edit)() = &edit;
