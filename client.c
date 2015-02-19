#include	"unp.h"
#include	"hw_addrs.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
//#include <linux/if_arp.h>
#include	<net/if.h>


void error(char *msg)
{
	fprintf(stderr,"%s:  %s\n",msg,strerror(errno));
	exit(1);
}


void error_wo_exit(char *msg)
{
	fprintf(stderr, "%s: %s\n", msg, strerror(errno));
}

int checkifcorrect(char *input)
{
	if(strlen(input)<3 || strlen(input)>4)
		return 0;	
	if(input[0]=='v' && input[1]=='m' && isdigit(input[2]) && input[2]!='0')
	{
		if(strlen(input)==4)
		{
			if(input[3]=='0') return 1;
			else return 0;
		}
		else return 1;
	}
}

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
		error_wo_exit("Error in msg_send");
}

void msg_recv(int s,char* recvline,char* recv_source,int* recv_port)
{
	struct sockaddr odraddr;
	int n, i=0;
	socklen_t         	len;
	len=sizeof(odraddr);
	char recvline1[MAXLINE];
	bzero(&odraddr,sizeof(odraddr));
	if((n=recvfrom(s,recvline1,MAXLINE,0,(SA*)&odraddr, &len))<0)
		error("Error in msg_recv");
	recvline1[n]=0;
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

int main(int argc, char **argv)
{

	int 			i, s, j, *recv_port, force_flag=0;
	struct in_addr 		addr_list;
	struct hostent 		*he;
	struct sockaddr_un	cliaddr, servaddr, tempcli;
	char 			input[10], recvline[MAXLINE], *recv_source, buf[100];
	socklen_t         	len;
	unsigned char canonicalIP[255];
	fd_set rset;
	time_t ticks;

	s = socket(AF_LOCAL, SOCK_DGRAM, 0);
	if (s == -1) { error("Error in socket creation"); }
	bzero(&cliaddr,sizeof(cliaddr));
	bzero(&servaddr,sizeof(servaddr));
	cliaddr.sun_family=AF_LOCAL;
	strcpy(cliaddr.sun_path,"/tmp/temp6820XXXXXX");
	int filed=mkstemp(cliaddr.sun_path);
	if(filed==-1)
	error("File not created by mkstemp \n");
	close(filed);
	unlink(cliaddr.sun_path);
	if((bind(s, (SA *) &cliaddr, sizeof(cliaddr)))<0)
		error("Error in bind");
   	len = sizeof(tempcli);
    	if((getsockname(s, (SA *) &tempcli, &len))<0)
		error_wo_exit("Error in getsockname");
    	printf("bound to %s\n", tempcli.sun_path);
	
	 //our MAC address 
	//unsigned char *src_mac = hwa->if_haddr;
	unsigned char src_mac[255];
	while(1)
	{
	printf("Enter the server node (from vm1 to vm10): ");
	//fgets(input,5,stdout);
	scanf("%s",&input);
	if(!(checkifcorrect(input)))
	{
		printf("Incorrect option!\nPlease provide the server node from the given choices\n");
		continue;
	}
	if ((he = gethostbyname(input)) <0) 
		{  // get the host info
			error_wo_exit("Error in gethostbyname");
			
	   	 }
	

	    // print information about this host:
	    //printf("VM name is: %s\n", he->h_name);
	    printf("Canonical IP address of server node: ");
		fflush(stdout);
	 //addr_list = *(struct in_addr *)(he->h_addr);
	  for (i = 0; he->h_addr_list[i] != NULL; i++)
		   { 
			strcpy(canonicalIP,inet_ntoa(*(struct in_addr *)he->h_addr_list[i]));
			printf("%s \n",canonicalIP); 
			fflush(stdout);
		   }
	if(gethostname(src_mac, sizeof(src_mac))<0)
		error_wo_exit("Error in gethostname");

	//unsigned char dest_mac[6] = {0x00, 0x0c, 0x29, 0x49, 0x3f, 0x65};
	char mesg[30]="Request for time from server";	
send:	printf("Client at node %s sending request to server at node %s\n", src_mac, input);
	msg_send( s, canonicalIP, 6820,  mesg, force_flag);
	printf("Request sent!\n");
	FD_ZERO(&rset);
	FD_SET(s,&rset);
	struct timeval tv;
	tv.tv_sec=10L;
	tv.tv_usec=0;
	int ready=select(s+1,&rset,NULL,NULL,&tv);
	if(ready<0)
		error("Error in select");
	if(FD_ISSET(s,&rset))
	{
        	msg_recv(s,recvline,recv_source,recv_port);
		ticks=time(NULL);
       		memset(buf,0,sizeof(buf));
 		snprintf(buf,sizeof(buf),"%.24s\r\n",ctime(&ticks));

		printf("Client at node %s received from %s %s\n", src_mac, input, buf);
		
	}
	if(ready==0)
	{
		if(force_flag==1)
		{
			printf("Client at node %s exiting due to no response from server at node %s\n",src_mac, input);
			continue;
		}		
		printf("Timeout\nRetransmitting request...\n");
		force_flag=1;
		goto send;
		
	}
	}
	//unlink(cliaddr.sun_path);
exit(0);

}

