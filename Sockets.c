/*
 * lab5.c
 *
 *  Created on: Oct 27, 2016
 *      Author: Zhentao Xie
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <fcntl.h>
#define PORT 3030
#define TRUE 1

#ifndef BOOL
#define BOOL int
#endif

int sockID;
struct sockaddr_in client_addr;
//struct ifreq ifr;
int client_addr_len;
void *Thrd_func(void *ptr);


int main(void)
{
	char buff1[50];
	struct sockaddr_in broadcast_addr;
	broadcast_addr.sin_family = AF_INET;
	broadcast_addr.sin_port = htons(PORT);
	broadcast_addr.sin_addr.s_addr = inet_addr("10.3.52.255");
	client_addr_len = sizeof(struct sockaddr_in);
	int broadcast_addr_len = sizeof(struct sockaddr_in);
	int master = 0;
	int slave = 0;
	int randomNum = 0;
	//char Rndnum[3] = {" 0\n"};
	char buff2[256]={"Zhentao is the Master"};
	int pipe_TtoM;
	system("mkfifo TtoM");
	struct RMESSAGE
	{
		char read_msg[50];
	}rmsg;


	//////////////get my address ////////////////////////////////////////
	char hostName[50];
	gethostname(hostName,sizeof(hostName));
	struct hostent *host;
	struct in_addr **boardIPList;
	host = (struct hostent*)gethostbyname(hostName);
    boardIPList = (struct in_addr**)host->h_addr_list;
    char *myaddr;
    myaddr = inet_ntoa(*boardIPList[0]); //10.3.52.17
    printf("My address is %s\n",myaddr);
   ///////////////////////////////////////////////////////////////////

    char *cpymyaddr=malloc(50*sizeof(char));
    strcpy(cpymyaddr,myaddr);
    char *cpybuff1_1 = malloc(50*sizeof(char));
    char *cpybuff1_2 = malloc(50*sizeof(char));
    //char *myaddr_Rndnum = malloc(50*sizeof(char));
    char myaddr_Rndnum[50];

    ////////////////get my_PC////////////////////////////////////
    char *token1 = strtok(myaddr,".");
    int j;
    for(j=0; j<3; j++)
    {
    //	printf("token1 is %s\n",token1);
       token1 = strtok(NULL,".");
    }
    int my_PC = atoi(token1); //get my IP number 17
    printf("My_PC is %d\n",my_PC);



    /////////////////build a thread to receive from////////////////////////
    pthread_t thrd;
    pthread_create(&thrd, NULL, Thrd_func, NULL);


    ///////////////build FIFO TtoM thread to main/////////////////////////
    if((pipe_TtoM = open("TtoM", O_RDONLY))<0)
    {
    	printf("Pipe TtoM open in read side in main function error\n");
    	exit(-1);
    }
    else
    {
    	printf("pipe_TtoM open in read side in main function open successfully\n");
    }


    while(1)
    {
    	/////////// read from pipe TtoM //////////////////////////////////
    	memset(buff1,0,sizeof(buff1));
    	memset(rmsg.read_msg,0,50*sizeof(char));
    	printf("Waiting to read pipe TtoM\n");
    	if(read(pipe_TtoM,&rmsg,sizeof(rmsg))<0)
    	{
    		printf("pipe_TtoM read error\n");
    		exit(-1);
    	}
    	else
    	{
    		strcpy(buff1,rmsg.read_msg);
    		//printf("read pipe_TtoM in main successfully\n");
    		printf("The read buff1 is %s\n",buff1);
    	}

    	//////////// receive command WHOIS /////////////////////////////
    	if((strcmp(buff1,"WHOIS\n"))==0)
    	{
    		if(master==1)
    		{
    			//unicast
    			sendto(sockID, buff2, sizeof(buff2), 0, (struct sockaddr*)&client_addr, client_addr_len);
    			printf("The master=1\n");
    		}
    		else
    		{
    			printf("The master=0\n");
    		}
    	}

    	/////////// receive command VOTE ////////////////////////////////
    	else if((strcmp(buff1,"VOTE\n"))==0)
    	{
    		memset(myaddr_Rndnum,0,sizeof(myaddr_Rndnum));
    		master = 0;
    		slave = 0;
    		srand(time(NULL));
    		randomNum = (rand()%10)+1;
    		printf("randomNUM=%d\n",randomNum);
    		printf("My address is %s\n",cpymyaddr);
    		sprintf(myaddr_Rndnum,"# %s %d\n",cpymyaddr,randomNum);
    		printf("myaddr_Rndnum is %s\n",myaddr_Rndnum);

    		//char myaddr_Rndnum[50] = {"# 10.3.52.15 1"};

    		sendto(sockID, myaddr_Rndnum, sizeof(myaddr_Rndnum),0,(struct sockaddr*)&broadcast_addr,broadcast_addr_len);
    		printf("The message that I send to friends is %s\n",myaddr_Rndnum);
    	}

    	//////////// receive others address and number //////////////////////////////
    	else if(buff1[0]=='#')
    	{
    		printf("The message from friends is: %s\n",buff1);
    		strcpy(cpybuff1_1,buff1);
    		strcpy(cpybuff1_2,buff1);
    		//get friend_PC
    		char *token2 = strtok(cpybuff1_1,". ");
    		int i;
    		for(i=0;i<4;i++)
    		{
    		  token2 = strtok(NULL,". ");
    		}
    		int friend_PC = atoi(token2);
    		printf("friend PC is %d\n",friend_PC);

    		//get friend random number
    		char *token3 = strtok(cpybuff1_2,". ");
    		int k;
    		for(k=0;k<5;k++)
    		{
    		  token3 = strtok(NULL,". ");
    		}
    		int friend_randNum = atoi(token3); //get friend random number
    		printf("friend random number is %d\n",friend_randNum);

    		if(randomNum == friend_randNum)
    		{
    			if(my_PC >= friend_PC)
    			{
    				master = 1;
    			}
    			else
    			{
    				master = 0;
    				slave = 1;
    			}
    		}
    		else if(randomNum > friend_randNum)
    		{
    			master = 1;
    		}
    		else
    		{
    			master = 0;
    			slave = 1;
    		}

    		if(slave == 1)
    		{
    			master = 0;
    		}
    		printf("My current master is %d\n",master);
    	}

    	/////////////// receive garbage //////////////////////////////
    	else
    	{
    		printf("read garbage: %s\n",buff1);
    	}

    }
	return 0;
}

void *Thrd_func(void *ptr)
{
	struct WMESSAGE
	{
		char write_msg[50];
	}wmsg;

	struct sockaddr_in serv_addr;
	char buff_recv[50];
	if((sockID = socket(AF_INET,SOCK_DGRAM,0))==-1)
	{
		printf("Create socket error\n");
		exit(0);
	}
	bzero((char *)&serv_addr,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);
	if(bind(sockID,(struct sockaddr*)&serv_addr, sizeof(serv_addr))<0)
	{
		printf("Error on binding\n");
		exit(0);
	}
	BOOL bBroadcast = TRUE;
	setsockopt(sockID, SOL_SOCKET, SO_BROADCAST, (const char*)&bBroadcast, sizeof(BOOL));
	int pipe_TtoM;
	if((pipe_TtoM = open("TtoM",O_WRONLY))<0)
	{
		printf("pipe_TtoM in write side in thread function open error\n");
		exit(-1);
	}
	else
	{
		printf("pipe_TtoM open in write side in thread function open successfully\n");
	}
	while(1)
	{
		memset(wmsg.write_msg,0,50*sizeof(char));
		memset(buff_recv,0,sizeof(buff_recv));
		recvfrom(sockID, buff_recv, sizeof(buff_recv),0,(struct sockaddr*)&client_addr, &client_addr_len);
		printf("In thread function: Receive from %s\n",inet_ntoa(client_addr.sin_addr));
		printf("In thread function: The message received from client is:%s\n",buff_recv);

		strcpy(wmsg.write_msg, buff_recv);


		if(write(pipe_TtoM,&wmsg,sizeof(wmsg))!=sizeof(wmsg))
	    {
		   printf("pipe_TtoM in thread function write error\n");
		   exit(-1);
	    }
	    else
	    {
	    	//printf("write pipe_TtoM in thread successfully\n");
	    	printf("Thread write to main function is %s\n",wmsg.write_msg);
	    }
	}

}
