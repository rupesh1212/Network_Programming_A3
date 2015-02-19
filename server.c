#include	"unp.h"
#include	<time.h>
#include<linux/if_ether.h>
#define  USID_PROTO 3243 


struct information_rcvd
{
	char 		*canon_ip;
	int		portno;
	char		*body;
}info;



/*mesg_recv(int sockfd,char *buff,char *canon_name,int port,char *body,int flag)
{
	printf("Waiting for client\n"); fflush(stdout);
	int n;
	n=recvfrom(sockfd,buff,sizeof(buff),0,NULL,NULL);
	buff[n]=0;
	memcpy(&info,&buff,);	
	strcpy(canon_name,info.canon_ip);
	port=info.portno;
	strcpy(body,info.body);
	printf("Client's canonical address is %s with port no. %i",canon_name,port);
	printf("The information sent by the client is %s\n",body);
}

mesg_send(int sockfd,char *buff,char*canon_name,int port,int flag)
{
	struct sockaddr_un		cliaddr;
	bzero(&cliaddr,sizeof(cliaddr));
	cliaddr.sun_family=AF_LOCAL;
	strcpy(cliaddr.sun_path,"/tmp/odr");
	strcpy(info.canon_ip,canon_name);
	info.portno=port;
	strcpy(info.body,body);
	memcpy()
	sendto(sockfd,buff,sizeof(buff),0,(SA*)&cliaddr,sizeof(cliaddr));
}*/

void msg_send( int s, char *canonicalIP, int portno, char *mesg, int flag)
{

    char sendline[MAXLINE], portnum[20],force_flag[2];
    sprintf(portnum,"%d",portno);
    sprintf(force_flag,"%d",flag);   
    sprintf(sendline,"%s|%s|%s|%s\n", canonicalIP, portnum, mesg, force_flag);
    struct sockaddr_un  odraddr; 
    bzero(&odraddr, sizeof(odraddr)); /* fill in server's address */
    odraddr.sun_family = AF_LOCAL;
    strcpy(odraddr.sun_path, "/tmp/odr6820");
    printf("Sent data is %s\n", sendline);
    if((sendto(s,sendline,strlen(sendline),0,&odraddr,sizeof(odraddr)))<0)
        printf("Error in msg_send:%\n",strerror(errno));
}

void msg_recv(int s,char* recvline,char* recv_source,int* recv_port)
{
    struct sockaddr odraddr;
    int n, i=0;
    socklen_t             len;
    char recvline1[MAXLINE];
    bzero(&odraddr,sizeof(odraddr));
    if((n=recvfrom(s,recvline1,MAXLINE,0,(SA*)&odraddr, &len))<0)
        error("Error in msg_recv");
    recvline[n]=0;
    char *token;
    char temp[3][MAXLINE];
    token = strtok(recvline1, '|');
  
       /* walk through other tokens */
       while( token != NULL )
       {
          //printf( " %s\n", token );
        strcpy(temp[i],token);
        i++;
            token = strtok(NULL, s);
       }
    strcpy(recv_source,temp[0]);
    recv_port=atoi(temp[1]);
    strcpy(recvline,temp[2]);
    

}

int
main(int argc, char **argv)
{
	int				sockfd,port,flag;
	char				*canon_name,*body;
	struct sockaddr_un		servaddr;
	time_t				ticks;
	void* buff=(void*)malloc(ETH_FRAME_LEN);
	printf("\n****************Welcome to the server*********************\n\n");
	

	if((sockfd= socket(AF_LOCAL, SOCK_DGRAM,htons(0)))<0)
		printf("error in bind: %s\n",strerror(errno));

	unlink("/tmp/serv6820");
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path,"/tmp/serv6820");
	
	if((bind(sockfd, (SA *) &servaddr, sizeof(servaddr)))<0)
		printf("error in bind: %s\n",strerror(errno));
	printf("Socket created and binded and is ready for communication!\n");
	remove(servaddr.sun_path);
	while(1)
		{
			msg_recv(sockfd,buff,canon_name,port);
			printf("Recived msg from %s and index %i\n",canon_name,port);
		
			ticks = time(NULL);
       			snprintf(body, sizeof(body), "%.24s\r\n", ctime(&ticks));
			printf("Value to be sent: %s\n",body);

			msg_send(sockfd,canon_name,port,buff,flag);
		}
		
}

