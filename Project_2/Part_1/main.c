#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <regex.h>

#define SERVER_PORT 21

#define MAX_LENGTH 100
#define FILENAME "Filename"
#define IP_ADDR "IP Address"
#define CRLF "\n"
#define USER "user"
#define PASS "PASS"
#define PASV "PASV"
#define RETR "RETR"

#define SRV_OK 220
#define NOT_LOGGED 530
#define USER_OK 331
#define LOGGED_IN 230
#define WRG_CMD 503
#define PASV_OK 227

void progress(int read) {
  for(int i =0; i <=50;i++)
    printf("\b");

  printf(">Bytes read: %d", read);
  
  fflush(stdout);
}

void printArgs(char *user, char* pass, char* host, char* path) {
  if(strcmp(user,"") != 0) {
    printf(">User: %s\n",user);
  }
  
  if(strcmp(pass,"") != 0) {
    printf(">Password: %s\n",pass);
  }
  
  if(strcmp(host,"") != 0) {
    printf(">Host: %s\n",host);
  }
  
  if(strcmp(path,"") != 0) {
    printf(">Path: %s\n",path);
  }
}

void printValue(char *value, char *title) {
  printf(">%s: %s\n",title,value);
}

void parseLogin(char *arg, char *user, char *pass, int *index) {
  int state = 0; //user
  int i = 0;
  while(arg[*index] != '@') {
    if(state == 0) {
      if(arg[*index] == ':') {
        state = 1; //pass
        i = 0;
      }
      else {
        user[i] = arg[*index];
        i++;
      }
    }
    else {
      pass[i] = arg[*index];
      i++;
    }
    (*index)++;
  }
  (*index)++;
}

void parseURL(char *arg, char *host, char *path, int index) {
  int state = 0; //host
  int i = 0;
  int j = 0;
  int length = strlen(arg) - index; //the remaining chars to parse
  while(j < length) {
    if(state == 0) { //host
      if(arg[index] == '/') {
        state = 1;
        i = 0;
      }
      else {
        host[i] = arg[index];
        i++;
      }
    }
    else { // path
      path[i] = arg[index];
      i++;
    }
    index++;
    j++;
  }
}

void parseFilename(char *path, char * filename) {
  
  int j = 0;
  for(int i = 0; i < strlen(path);i++) {
    if(path[i] == '/') {
      memset(filename,0,strlen(filename));
      j = 0;
    }
    else {
      filename[j] = path[i];
      j++;
    }
  }
}

int parseArg(char *arg, char *user, char *pass, char *host, char *path) {
  regex_t regex;
  int reti;
  reti = regcomp(&regex,"ftp://.*",REG_NOSUB);
  if(reti) {
    printf("Couldn't compile argument regex\n");
    regfree(&regex);
    return -1;
  }

  reti = regexec(&regex,arg,0,NULL,0);
  if(reti == REG_NOMATCH) {
    printf("Error in url: Must begin with \"ftp://\"\n");
    regfree(&regex);
    return -1;
  }

  regfree(&regex);
  reti = regcomp(&regex,"ftp://.*:.*@.*/.*",REG_NOSUB);
  if(reti) {
    printf("Couldn't compile argument regex\n");
    regfree(&regex);
    return -1;
  }
  
  int index = 6; //starts on index 6 because "ftp://" is checked before parsing
  reti = regexec(&regex,arg,0,NULL,0);
  if(!reti) {
    parseLogin(arg,user,pass,&index);
  }

  parseURL(arg,host,path,index);
  
  regfree(&regex);
  return 0;
}

//////////////////////getip.c/////////////////////////////////////////
int getip(char* host,char* ip_addr) {
  struct hostent *h;

  if ((h=gethostbyname(host)) == NULL) {  
      herror("gethostbyname");
      return -1;
  }
  
  sprintf(ip_addr,"%s",inet_ntoa(*((struct in_addr *)h->h_addr)));

  return 0;
}
//////////////////////////////////////////////////////////////////////

int sendSocketCommand(int socketfd,char *cmd, char* arg) {
  char command[MAX_LENGTH]="";
  sprintf(command,"%s",cmd);
  if(strcmp(arg,"") != 0) {
    sprintf(command,"%s %s",command,arg);
  }

  strcat(command,CRLF);
  return send(socketfd,command,strlen(command),0);
} 

int getServResp(char *reply) {
  char code[4] = " ";
  strncpy(code,reply,3);
  return atoi(code);
}

int readResponse(int socketFd){
  char repl2[MAX_LENGTH] = "";
  char b;
  int i = 0;
  while(1){
    if(read(socketFd,&b,1) <0) {
      printf("Failed to read server response");
      return -1;
    }
    if(b == '\n') {
      repl2[i] = b;
      if(repl2[3] == ' ') {
        return getServResp(repl2);
      }
      else {
        memset(repl2,0,MAX_LENGTH);
        i = 0;
      }
    }
    else {
      repl2[i] = b;
      i++;
    }
    //printf("%c",b);
  }
}

int getPort(int part1,int part2) {
  return part1 * 256 + part2;
}

void parsePort(char *codes,int * port) {
  char port_part1 [5] = "";
  char port_part2 [5]= "";
  int i = 0;

  char *ptr = strtok(codes,",");
  while(ptr != NULL) {
    if (i == 4) {
      strcpy(port_part1,ptr);
    }
    else if( i == 5) {
      strcpy(port_part2,ptr);
    }
    ptr = strtok(NULL,",");
    i++;
  }

  *port = getPort(atoi(port_part1),atoi(port_part2));
}

int readPassiveResponse(int socketFd,int *port){
  char repl2[MAX_LENGTH] = "";
  char code[4] = "";
  char b;
  int i = 0;
  int state = 0;
  
  while(1){
    if(read(socketFd,&b,1) <0) {
      printf("Failed to read server response");
      return -1;
    }
    switch(state) {
      case 0:
        if(b == '(') {
          state = 1;
          i = 0;
        }
        else if( i < 3) {
          code[i] = b;
          i++;
        }
        break;
      case 1:
        if(b == ')') {
          parsePort(repl2,port);
          state = 2;
        }
        else {
          repl2[i] = b;
          i++;
        }
        break;
      case 2:
        if(b == '\n') {
          return atoi(code);
        }
        break;
    }
  }
}

int downloadFile(int retr_sockfd, char *filename){

  FILE * fPtr; 
  if((fPtr = fopen(filename, "w+")) == NULL){
    return -1;
  } 
  
  char buffer;
  int readBytes = 0;
  
  while(1){
    int a;
    if((a = read(retr_sockfd, &buffer, 1)) < 0) return -1;
    if(a == 0){
       break;
    }
    readBytes++;

    progress(readBytes);
    fputc(buffer, fPtr);
  }

  fclose(fPtr);
  return 0;
}

int main(int argc, char** argv){
  if(argc != 2) {
    printf("Wrong arguments. Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
    return -1;
  }

  char user[MAX_LENGTH] = "";
  char pass[MAX_LENGTH] = "";
  char host[MAX_LENGTH] = "";
  char path[MAX_LENGTH] = "";
  if(parseArg(argv[1],user,pass,host,path) < 0)
    return -1;
  printArgs(user,pass,host,path);

  char filename[MAX_LENGTH] = "";
  parseFilename(path,filename);
  printValue(filename,FILENAME);

  char ip_addr[MAX_LENGTH] = "";
  if(getip(host,ip_addr)<0) {
    printf("Failed to get IP address.\n");
    return -1;
  }
  printValue(ip_addr,IP_ADDR);
  
  ///////////////////////////////clientTCP.c///////////////////////////////////////////////////
	int	sockfd;
  int retr_sockfd;
	struct	sockaddr_in server_addr;
	char	buf[MAX_LENGTH] = "";  
	int	bytes;
  int resp_code;
	
	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip_addr);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(SERVER_PORT);		/*server TCP port must be network byte ordered */
    
	/*open an TCP socket*/
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    		perror("socket()");
        	exit(0);
  }
	/*connect to the server*/
  if(connect(sockfd, (struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
    perror("connect()");
		exit(0);
	}
  
  resp_code = readResponse(sockfd);
  if(resp_code != SRV_OK) {
    printf("Service not ready.Response code: %d\n",resp_code);
    return -1;
  }

  if(strcmp(user,"") == 0) {
    strcpy(user,"anonymous");
    char easter[] = {79,32,74,79,71,79,0};
    strcpy(pass,easter);
  }

  if(sendSocketCommand(sockfd,USER,user) <0) {
    printf("Failed to send socket command.\n");
    return -1;
  }

  
  resp_code = readResponse(sockfd);
  if(resp_code != USER_OK) {
    printf("Log in failed. Response code: %d\n",resp_code);
    return -1;
  }

  printf(">User: %s\n",user);
  
  if(sendSocketCommand(sockfd,PASS,pass) <0) {
    printf("Failed to send socket command.\n");
    return -1;
  }

  resp_code = readResponse(sockfd);
  if(resp_code != LOGGED_IN) {
    printf("Log in failed. Response code: %d\n",resp_code);
    return -1;
  }

  printf(">Password: %s\n",pass);

  if(sendSocketCommand(sockfd,PASV,"") <0) {
    printf("Failed to send socket command.\n");
    return -1;
  }
  
  int port;
  resp_code = readPassiveResponse(sockfd,&port);
  if(resp_code != PASV_OK) {
    printf("Failed to enter passive mode. Response code: %d\n",resp_code);
    return -1;
  }
  printf(">Access port: %d\n",port);

  server_addr.sin_port = htons(port);
  if ((retr_sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    		perror("socket()");
        	exit(0);
  }
	/*connect to the server*/
  if(connect(retr_sockfd, (struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
    perror("connect()");
		exit(0);
	}
  
  if(sendSocketCommand(sockfd,RETR,path) <0) {
    printf("Failed to send socket command.\n");
    return -1;
  }

  printf(">Downloading file %s\n", filename);
  if(downloadFile(retr_sockfd,filename) <0) {
    printf("Failed to download file: %s\n",filename);
    return -1;
  }
  printf("\n>Finished downloading %s\n", filename);

  close(retr_sockfd);
	close(sockfd);
	exit(0);
}