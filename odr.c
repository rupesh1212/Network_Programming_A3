#include    "unp.h"
#include    "hw_addrs.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include    <net/if.h>

#define     ARPHRD_ETHER   1
#define     TTL            10
#define server_port 6820
#define odr_port 9512	
#define odr_path "/tmp/odr6820"
#define server_path "/tmp/serv6820"

extern struct hwa_info  *Get_hw_addrs();
extern void free_hwa_info(struct hwa_info *);

struct mapping_table
{
    char filename[20];
    int  port;
    int type;
    unsigned long long int timestamp; 
    //int broadcast_id;
    struct mapping_table *next;
}*map_head;

struct routing_table
{
    char destination_addr[50];
    char next_node_mac[50];
    int out_interface_index;
    int hop_count;
    unsigned long long int timestamp;
    struct routing_table *next;
}*rt_head;



struct odr_packet 
{
    int type;
    char source[20];
    char destination[20];
    int broadcast_id;
    int hop_count;
    int force_flag;
    int rrep_sent;	
    int source_port;
    int destination_port;
    int application_msg_size;
    
};

struct ethernet_frame
{
   unsigned char source_mac[ETH_ALEN];
   unsigned char destination_mac[ETH_ALEN];
    int protocol_field;
    struct odr_packet *odr_pack;
};


struct hw_addr
{
    char name[50];                          
    char ip_addr[20];                    
   unsigned char mac_addr[ETH_ALEN];                      
    int index;                          
}ifi_info[10];

void error(char *msg)
{
    fprintf(stderr,"%s:  %s\n",msg,strerror(errno));
remove("tmp/odr6820");
    exit(1);
}


void error_wo_exit(char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
}


unsigned long long int get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long int timestamp = (unsigned long long int)((unsigned long long int)tv.tv_usec)/1000000;
    timestamp += (unsigned long long int) ((unsigned long long int)tv.tv_sec);
    return timestamp;
} 

void mapping_entry(char *file_name,int port,int type)
{
	
	struct mapping_table *current,*current1;
	current=map_head;
	if(current==NULL)
	{
		current=(struct mapping_table*)malloc(sizeof(struct mapping_table));
		map_head=current;
		current1=current;
		current->next=NULL;
	}
	else	{
			while(current->next!=NULL)
			current=current->next;
			current1=(struct mapping_table*)malloc(sizeof(struct mapping_table));(struct mapping_table*)malloc(sizeof(struct mapping_table));
			current->next=current1;
			current1->next=NULL;
		}
	memcpy(current1->filename,file_name,sizeof(current->filename));
	
	current1->type=type;
	if(type==1)
	{
		current1->timestamp=0;	
		current1->port=port;
	}
	else{
		 current1->timestamp=get_time();
		 current1->port=port+565;
	    }
}

struct routing_table* check_routing_entry(char *dest_name)
{
   	struct routing_table *current;                 
  	// p=NULL;
   	current=rt_head;   
	if(current==NULL)
	{

		return(current);
	}												
   	while(current!=NULL)
   	{	
       		if(memcmp(&current->destination_addr,dest_name,sizeof(current->destination_addr))==0)
 		return(current);
       		current=current->next;    
		
   	}


}

void broadcast(int sockraw,char *sendline,int a,char *dest_ip,int broadcast_id,int force_flag,int rrep_sent)
	{
	
	printf("Starting Broadcast...\n");fflush(stdout);
	int i; char ip1[20];
	struct odr_packet       *opack=(struct odr_packet*)malloc(sizeof(struct odr_packet));
	void* buffer = (void*)malloc(ETH_FRAME_LEN);
	unsigned char* etherhead = buffer;
	void* data = buffer + 14;
	unsigned char destination_mac[6];
	unsigned char source_mac[6],src[10];

	struct ethhdr *eh = (struct ethhdr *)etherhead;

	
	memset(opack,0,sizeof(struct odr_packet));

	//memset(buffer,0,sizeof(struct odr_packet));

	struct sockaddr *p;
	char *ip;
	struct sockaddr_ll broadcast;           
	bzero(&broadcast,sizeof(broadcast));
	broadcast.sll_family = PF_PACKET;
	broadcast.sll_ifindex = ifi_info[a].index;
	broadcast.sll_protocol=htons(3243);
	broadcast.sll_hatype = ARPHRD_ETHER;
	broadcast.sll_pkttype = PACKET_BROADCAST;
	broadcast.sll_halen = ETH_ALEN;
	broadcast.sll_addr[0] = 0xff;
	broadcast.sll_addr[1] = 0xff;
	broadcast.sll_addr[2] = 0xff;
	broadcast.sll_addr[3] = 0xff;
	broadcast.sll_addr[4] = 0xff;
	broadcast.sll_addr[5] = 0xff;
	broadcast.sll_addr[6] = 0x00;
	broadcast.sll_addr[7] = 0x00;

	destination_mac[0]=0xff;
	destination_mac[1]=0xff;
	destination_mac[2]=0xff;
	destination_mac[3]=0xff;
	destination_mac[4]=0xff;
	destination_mac[5]=0xff;

	source_mac[0]=ifi_info[a].mac_addr[0];
	source_mac[1]=ifi_info[a].mac_addr[1];
	source_mac[2]=ifi_info[a].mac_addr[2];
	source_mac[3]=ifi_info[a].mac_addr[3];
	source_mac[4]=ifi_info[a].mac_addr[4];
	source_mac[5]=ifi_info[a].mac_addr[5];

	memcpy((void*)buffer, (void*)destination_mac, ETH_ALEN);
	memcpy((void*)(buffer+ETH_ALEN), (void*)source_mac, ETH_ALEN);
	eh->h_proto = htons(3243);

	memcpy(&opack->source,&ifi_info[a].ip_addr,sizeof(opack->source));
	strcpy(ip1,dest_ip);
	memcpy(&opack->destination,&ip1,strlen(ip1));

	opack->broadcast_id=htonl(broadcast_id);
	opack->hop_count=htonl(0);

	opack->force_flag=htonl(force_flag);
	opack->rrep_sent=htonl(rrep_sent);

	opack->type=htonl(0);

	/*printf("source adr %s\n",(opack->source));
	printf("dest ip %s\n",(opack->destination));
	printf("broadcast id %i\n",ntohl(opack->broadcast_id));
	printf("flg %i\n",ntohl(opack->force_flag));
	printf("rrep sent no. %i\n",ntohl(opack->rrep_sent));*/

	memcpy(data,opack,sizeof(struct odr_packet));
	if(sendto(sockraw,buffer,ETH_FRAME_LEN,0,(struct sockaddr*)&broadcast,sizeof(broadcast))<0)
		error_wo_exit("Error in sendto of broadcast");   

/*	char *srcname,src1[10],destname[10];
	struct hostent *he;
	struct in_addr ipv4addr;
	printf("1\n");
        bzero(&ipv4addr,sizeof(struct in_addr));
	printf("2\n");
	printf("4\n");
	//memcpy(srcname,&he->h_name,10);
	printf("5\n");fflush(stdout);
	

	/*if(gethostname(src1, sizeof(src1))<0)
		error_wo_exit("Error in gethostname");
	printf("ODR at node %s : broadcasting   frame hdr  src %s",src1,src1);
	/*for(i=6;i>0;i--)
		printf("%.2x%s", destination_mac[i] & 0xff, (i == 1) ? " " : ":");*/
/*	printf("\n\t\t\tODR msg   type 0   src ");fflush(stdout);
	inet_pton(AF_INET, &opack->source, &ipv4addr);
	printf("3\n");
	he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	printf("4\n");
	printf("%s  ",he->h_name);
	printf("5\n");
	inet_pton(AF_INET, &opack->destination, &ipv4addr);
	he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	printf("dest %s\n",he->h_name);
	/*if(gethostname(src, sizeof(src))<0)
		error_wo_exit("Error in gethostname");
	printf("ODR at node %s : sending   frame hdr  src %s   dest ",src,src);
	for(i=6;i>0;i--)
		printf("%.2x%s", destination_mac[i] & 0xff, (i == 1) ? " " : ":");
	printf("\n\t\t\tODR msg   type 0   src %s   dest %s\n",srcname,destname);*/
	broadcast_id++;
	free(buffer);                 

		             
}

int check_map_entry(char *client_name)
{
    struct mapping_table *current; 
    current=map_head;
    while(current!=NULL)
    {
        if((memcmp(&current->filename,client_name,sizeof(current->filename)))==0)
            return 1;
        else current=current->next;
    }
    return 0;
}



int getmsgcomponents(char *recvline,char *dest_ip,int *dest_port,char *mesg)
{

	int i=0, j=0, k=0, force_flag=0;
	char *token;
	char temp[4][28];
	int n = strlen(recvline);

	memset(temp,0,sizeof(temp));
	memset(mesg,0,strlen(mesg));
    
	while(recvline[i]!='\0')
	{
		if(recvline[i]!='|')
		{
		temp[j][k]=recvline[i];
		k++;
		i++;
		}
		else {	i++;
			j++;
			k=0;
		     }
		
	}
	strcpy(dest_ip,temp[0]);
	dest_port=atoi(temp[1]);
	strcpy(mesg,temp[2]);
	force_flag=atoi(temp[3]);

return force_flag;
}

unsigned char* retrieveMacFromIndex( int recv_index ,int a)
{
	unsigned char *source_mac=NULL;int x;
	int i;
	for(i=0;i<a;i++)
	{
		if(recv_index==ifi_info[i].index)
			for(x=6;x>0;x--)
			 source_mac[x]=&ifi_info[i].mac_addr[x];
	}
return source_mac;
}

int get_dest_port()
{
    return(map_head->port);
}


int check_stale(struct routing_table *current,unsigned long long int stale)
{
	unsigned long long int temp;                //check staleness 
	temp=get_time();
    	if((temp-current->timestamp)>stale)
       		return 0;
   
    return 1;
}

void send_rrep(int sockraw, struct odr_packet *opack,struct routing_table *routing_row,unsigned char *source_mac,int recv_index, int a)
{
	printf("Sending RREP...\n");fflush(stdout);
	int i;
	struct sockaddr_ll  rrepaddr;

	struct odr_packet *opack_rrep=(struct odr_packet*)malloc(sizeof(struct odr_packet));
	
	unsigned char mymac[6];
	char* buffer = (char*)malloc(ETH_FRAME_LEN);
	unsigned char* etherhead = buffer;
	char* data = buffer + 14;
	memset(buffer,0,sizeof(buffer));
	memset(&rrepaddr,0,sizeof(rrepaddr));
	memset(opack_rrep,0,sizeof(opack_rrep));
	printf("opack source%s\n",opack->source);
	struct ethhdr *eh = (struct ethhdr *)etherhead;
	memcpy(buffer, (char*)routing_row->next_node_mac, ETH_ALEN);
	memcpy((buffer+ETH_ALEN),(char*)source_mac, ETH_ALEN);
	eh->h_proto = htons(3243);
	memcpy(&opack_rrep->source,&opack->source,sizeof(opack->source));
	memcpy(&opack_rrep->destination,&opack->destination,sizeof(opack->destination));
	opack_rrep->hop_count=htonl(routing_row->hop_count+1);
	opack_rrep->force_flag=htonl(opack->force_flag);
	opack_rrep->type=htonl(1);
	memcpy(data,opack,sizeof(data));
	//rrepaddr.sll_family   = PF_PACKET;  
	rrepaddr.sll_protocol = htons(3243);     
	rrepaddr.sll_ifindex  = (routing_row->out_interface_index);
	//rrepaddr.sll_hatype   = ARPHRD_ETHER;
	//rrepaddr.sll_pkttype  = PACKET_OTHERHOST;
	rrepaddr.sll_halen    = ETH_ALEN;       
/*
	rrepaddr.sll_addr[0]=routing_row->next_node_mac[0];       
	rrepaddr.sll_addr[1]=routing_row->next_node_mac[1];       
	rrepaddr.sll_addr[2]=routing_row->next_node_mac[2];       
	rrepaddr.sll_addr[3]=routing_row->next_node_mac[3];       
	rrepaddr.sll_addr[4]=routing_row->next_node_mac[4];       
	rrepaddr.sll_addr[5]=routing_row->next_node_mac[5];      
	rrepaddr.sll_addr[6]=0x00;       
	rrepaddr.sll_addr[7]=0x00;  */
	rrepaddr.sll_addr[0]=0xf6;
	rrepaddr.sll_addr[1]=0x08;
	rrepaddr.sll_addr[2]=0xd9;
	rrepaddr.sll_addr[3]=0x29;
	rrepaddr.sll_addr[4]=0x0c;
	rrepaddr.sll_addr[5]=0x00;
	rrepaddr.sll_addr[6]=0x00;       
	rrepaddr.sll_addr[7]=0x00;
	/*printf("source mac addr ");
	for(i=6;i>0;i--)
	printf("%.2x",source_mac[i]&0xff);fflush(stdout);
	printf("index %i\n",rrepaddr.sll_ifindex);
	printf("mac addr ");
	for(i=6;i>0;i--)
	printf("%.2x",rrepaddr.sll_addr[i]&0xff);fflush(stdout);*/
	//printf("size %i\n",strlen(buffer));
	if((sendto(sockraw,buffer,ETH_FRAME_LEN,0,(struct sockaddr*)&rrepaddr,sizeof(rrepaddr)))<0)
		error_wo_exit("Error in send");

/*	char src[10],srcname[10],destname[10];
	struct hostent *he;
	struct in_addr ipv4addr;
        bzero(&ipv4addr,sizeof(struct in_addr));
	inet_pton(AF_INET, &opack->source, &ipv4addr);
	he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	strcpy(srcname, he->h_name);
	inet_pton(AF_INET, &opack->destination, &ipv4addr);
	he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	strcpy(destname, he->h_name);

	if(gethostname(src, sizeof(src))<0)
		error_wo_exit("Error in gethostname");
	printf("ODR at node %s : sending   frame hdr  src %s   dest ",src,src);
	for(i=6;i>0;i--)
		printf("%.2x%s", routing_row->next_node_mac[i] & 0xff, (i == 1) ? " " : ":");
	printf("\n\t\t\tODR msg   type 0   src %s   dest %s\n",srcname,destname);*/

	free(buffer);






}


void send_rreq(int sockraw, struct odr_packet *opack,struct routing_table *routing_row,int rrep_sent, int recv_index,int a)
{
    printf("Sending RREQ...\n");fflush(stdout);
    struct sockaddr_ll  rreqaddr;
    struct odr_packet *opack_rreq=(struct odr_packet*)malloc(sizeof(struct odr_packet));
    char mymac[6];
    void* buffer = (void*)malloc(ETH_FRAME_LEN);
    unsigned char* etherhead = buffer;
    void* data = buffer + 14;
    struct ethhdr *eh = (struct ethhdr *)etherhead;

    memcpy(&opack_rreq->source,&opack->source,sizeof(opack->source));
    memcpy(&opack_rreq->destination,&opack->destination,sizeof(opack->destination));
    opack_rreq->broadcast_id=htonl(opack->broadcast_id);//////////
    opack_rreq->hop_count=htonl(routing_row->hop_count);
    opack_rreq->force_flag=htonl(opack->force_flag);
    opack_rreq->rrep_sent=htonl(rrep_sent);
    opack_rreq->type=htonl(0);
    memcpy((void*)buffer, (void*)routing_row->next_node_mac, ETH_ALEN);
    memcpy((void*)(buffer+ETH_ALEN),(void*)mymac, ETH_ALEN);
    eh->h_proto = htons(3243);
    data=(void*)opack;
    rreqaddr.sll_family   = PF_PACKET;  
    rreqaddr.sll_protocol = htons(3243);
    rreqaddr.sll_ifindex  = routing_row->out_interface_index;
    rreqaddr.sll_hatype   = ARPHRD_ETHER;
    rreqaddr.sll_pkttype  = PACKET_OTHERHOST;
    rreqaddr.sll_halen    = ETH_ALEN;       
 
    rreqaddr.sll_addr[0]=routing_row->next_node_mac[0];       
    rreqaddr.sll_addr[1]=routing_row->next_node_mac[1]; 
    rreqaddr.sll_addr[2]=routing_row->next_node_mac[2]; 
    rreqaddr.sll_addr[3]=routing_row->next_node_mac[3]; 
    rreqaddr.sll_addr[4]=routing_row->next_node_mac[4]; 
    rreqaddr.sll_addr[5]=routing_row->next_node_mac[5]; 
    rreqaddr.sll_addr[6]=0x00; 
    rreqaddr.sll_addr[7]=0x00; 

    if((sendto(sockraw,buffer,ETH_FRAME_LEN,0,(SA*)&rreqaddr,sizeof(rreqaddr)))<0)
    	error_wo_exit("Error in send");

/*	char src[10],srcname[10],destname[10];
	struct hostent *he;
	struct in_addr ipv4addr;
        bzero(&ipv4addr,sizeof(struct in_addr));
	inet_pton(AF_INET, opack->source, &ipv4addr);
	he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	strcpy(srcname, he->h_name);
	inet_pton(AF_INET, opack->destination, &ipv4addr);
	he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	strcpy(destname, he->h_name);
	int i;
	if(gethostname(src, sizeof(src))<0)
		error_wo_exit("Error in gethostname");
	printf("ODR at node %s : sending   frame hdr  src %s  dest ",src,src);
	for(i=6;i>0;i--)
		printf("%.2x%s", routing_row->next_node_mac[i] & 0xff, (i == 1) ? " " : ":");
	printf("\n\t\t\tODR msg   type 0   src %s   dest %s\n",srcname,destname);*/

 free(buffer);
}

void send_appl_payload(int sockraw, struct odr_packet *opack,struct routing_table *routing_row,int recv_index, int a)
{
    printf("Sending application payload...\n");fflush(stdout);
    struct sockaddr_ll  appaddr;
    struct odr_packet *opack_app;
    char mymac[6];
    void* buffer = (void*)malloc(ETH_FRAME_LEN);
    unsigned char* etherhead = buffer;
    void* data = buffer + 14;
    struct ethhdr *eh = (struct ethhdr *)etherhead;

    memcpy(&opack_app->source,&opack->source,sizeof(opack->source));
    memcpy(&opack_app->destination,&opack->destination,sizeof(opack->destination));
    char dest[20];
    strcpy(dest,(opack->destination));
    opack_app->destination_port= get_dest_port();///////////////////////////////////////
    opack_app->hop_count=htonl(0);///////////////////
  //  app_pack->application_msg_size=sizeof();///////////////////////////////
    opack_app->type=htonl(2);
    
    memcpy((void*)buffer, (void*)routing_row->next_node_mac, ETH_ALEN);
    memcpy((void*)(buffer+ETH_ALEN),(void*)mymac, ETH_ALEN);
    eh->h_proto = htons(3243);
    data=(void*)opack;
    appaddr.sll_family   = PF_PACKET;  
    appaddr.sll_protocol = htons(3243);    
    appaddr.sll_ifindex  = routing_row->out_interface_index;
    appaddr.sll_hatype   = ARPHRD_ETHER;
    appaddr.sll_pkttype  = PACKET_OTHERHOST;
    appaddr.sll_halen    = ETH_ALEN;       

    appaddr.sll_addr[0]=routing_row->next_node_mac[0];       
    appaddr.sll_addr[1]=routing_row->next_node_mac[1];
    appaddr.sll_addr[2]=routing_row->next_node_mac[2];
    appaddr.sll_addr[3]=routing_row->next_node_mac[3];
    appaddr.sll_addr[4]=routing_row->next_node_mac[4];
    appaddr.sll_addr[5]=routing_row->next_node_mac[5];
    appaddr.sll_addr[6]=0x00;
    appaddr.sll_addr[7]=0x00;

    if((sendto(sockraw,buffer,ETH_FRAME_LEN,0,(SA*)&appaddr,sizeof(appaddr)))<0)
    	error_wo_exit("Error in send");

/*	char src[10],srcname[10],destname[10];
	struct hostent *he;
	struct in_addr ipv4addr;
        bzero(&ipv4addr,sizeof(struct in_addr));
	inet_pton(AF_INET, opack->source, &ipv4addr);
	he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	strcpy(srcname, he->h_name);
	inet_pton(AF_INET, opack->destination, &ipv4addr);
	he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	strcpy(destname, he->h_name);
	int i;
	if(gethostname(src, sizeof(src))<0)
		error_wo_exit("Error in gethostname");
	printf("ODR at node %s : sending   frame hdr  src %s  dest ",src,src);
	for(i=6;i>0;i--)
		printf("%.2x%s", routing_row->next_node_mac[i] & 0xff, (i == 1) ? " " : ":");
	printf("\n\t\t\tODR msg   type 0   src %s   dest %s\n",srcname,destname);*/

 free(buffer);
}


void add_routing_entry(char *dest_ip, unsigned char *mac,int index,int hop_count)
{
	int i;char ip[16];
	struct routing_table *current, *current1;
	current=rt_head;
   
   	if(current==NULL)
	{
		current=(struct routing_table*)malloc(sizeof(struct routing_table));
		rt_head=current;
		current1=current;
		current->next=NULL;
			
	}
	else 	{
    			while(current->next!=NULL)
			{
				current=current->next;
				
  			}
			current1=(struct routing_table*)malloc(sizeof(struct routing_table));
			current->next=current1;
			current1->next=NULL;
		}
	memset(current1->next_node_mac,0,sizeof(current1->next_node_mac));
	memset(current1,0,sizeof(current));
	memcpy(current1->destination_addr,dest_ip,sizeof(current->destination_addr));
	memcpy(ip,current1->destination_addr,16);

	current1->next_node_mac[0]=mac[0];
	current1->next_node_mac[1]=mac[1];
	current1->next_node_mac[2]=mac[2];
	current1->next_node_mac[3]=mac[3];
	current1->next_node_mac[4]=mac[4];
	current1->next_node_mac[5]=mac[5];
	current1->out_interface_index=(index);

	current1->hop_count=(hop_count);

	current1->timestamp=get_time();
    
}

void delete_routing_entries(char ip_addr)
{
        struct routing_table *current,*current1;
        current=rt_head;
        if(strcmp(current->destination_addr,ip_addr)==0)
        {
            rt_head=current->next;
            free(current);
        }    
        else    {
                    while(current->next!=NULL)
                    {
                        current1=current->next;
                        if(strcmp(current1->destination_addr,ip_addr)==0)
                        {
                            current=current1->next;
                            free(current1);
                        }   
                        current=current->next;
                    }
                }       
}

void update_mapping_table(char *name)
{
	struct mapping_table *current;
	current=map_head;
	while(current!=NULL)
	{
		if(memcmp(&current->filename,name,sizeof(current->filename))==0)
		{
			current->timestamp=get_time();
			break;
		}
		current=current->next;
	}	
}

void update_routing_table(char *ip,int hop_count,int flag)
{
    struct routing_table *current;
    current=rt_head;
    while(current!=NULL)
    {
        if(memcmp(&current->destination_addr,ip,sizeof(current->destination_addr))==0)
        {
            if(flag==1)
            {
                current->hop_count=hop_count;
            }
            else if(current->hop_count>hop_count)
            {
                current->hop_count=hop_count;
                current->timestamp=get_time();
            }
        }
        current=current->next;
    }
}



int checkifdest(char *ip_addr,int count)
{	
    int i;
    for(i=0;i<count;i++)
    {
        if(memcmp(&ifi_info[i].ip_addr,ip_addr,sizeof(ifi_info[i].ip_addr))==0)
        return 1;
    }
    return 0;
}

void delete_mapping_entries()
{
    struct mapping_table *current,*current1;
    current=map_head;
    //current1=current->next;
    while(current->next!=NULL)
    {
	current1=current->next;
	if(current1->type!=1)
        if((get_time()-current1->timestamp)>TTL)
        {
	    if(current1->next!=NULL)	
            current=current1->next;
            //current=current1->next;
		free(current1);
		break;	
        }
        current=current->next;
    }current->next=NULL;
}


void displaymap()
{
	struct mapping_table *current;
	current=map_head;
	printf("\n**************MAPPING TABLE FOR SUNPATH-PORT ENTRIES************\n");fflush(stdout);
	printf("Sun-path Name\t\tPort Number\tEntry Type\t\tTimestamp\n");fflush(stdout);
	while(current!=NULL)	
	{
		
		printf("%s\t\t",current->filename);
		printf("%i\t\t",current->port);
		if(current->type==1)
		printf("Permanent entry\t\t");
		else printf("Temporary entry\t\t");
		printf("%d\n",current->timestamp);
		current=current->next;
	}
	printf("-------------------------------------------------------------------\n");fflush(stdout);
}

void display_routing_table()
{
	char ip[16];int i;
	struct routing_table *current;
	current=rt_head;
	printf("\n***********************ROUTING TABLE***********************\n");fflush(stdout);
	while(current!=NULL)
	{
		printf("Destination IP ADDR\tNext Node MAC Addr\tOutgoing Interface Index\tHop Count\tLast Updated Time\n");fflush(stdout);
		printf("%s\t\t",rt_head->destination_addr);fflush(stdout);
		for(i=6;i>0;i--)
		printf("%.2x:",current->next_node_mac[i]&0xff);fflush(stdout);
		printf("\t\t");
		printf("%i\t\t\t",current->out_interface_index);fflush(stdout);
		printf("%i\t\t",current->hop_count);fflush(stdout);
		printf("%d\n",current->timestamp);fflush(stdout);
		current=current->next;
	}
	printf("-------------------------------------------------------------------\n");fflush(stdout);
}
/***********************************************************************************************************************************************/
int main(int argc, char **argv)
{

    int         sockraw,sockunix, j, a,n, dest_port,    				force_flag=0,rrep_sent=0,broadcast_id=1,hop_count=0,recv_index=3,check,staleness,checkdest,var=5145;
    struct in_addr      addr_list;
    struct hostent      *he;
    struct sockaddr_un  cliaddr,odraddr;
    struct sockaddr_ll  odraddr_recv, rrepaddr;
    struct odr_packet       *opack=(struct odr_packet*)malloc(sizeof(struct odr_packet));	
    struct ethernet_frame   *ethf=(struct ethernet_frame*)malloc(sizeof(struct ethernet_frame)); 
    struct ethernet_frame   *ethf1=(struct ethernet_frame*)malloc(sizeof(struct ethernet_frame));
    struct ethernet_frame   *ethframe=(struct ethernet_frame*)malloc(sizeof(struct ethernet_frame));	
    struct payload          *pl;
    
    rt_head=NULL;
    map_head=NULL;
    struct routing_table    *routing_row;
    int             map_entry=0;
    char            input[10], recvline[MAXLINE],sendline[1518], dest_ip[20], mesg[MAXLINE];

    void* buffer1 = (void*)malloc(ETH_FRAME_LEN);

    struct hwa_info *hwa, *hwahead;
    struct sockaddr *sa;
    char   *ptr;
    int    i, prflag;

    printf("\n");
    int x=0;
    printf("-------------------------NETWORK HARDWARE DETAILS-----------------------------\n");fflush(stdout);
    for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) 
    {
       
        printf("%s :%s", hwa->if_name, ((hwa->ip_alias) == IP_ALIAS) ? " (alias)\n" : "\n");
        if ( (sa = hwa->ip_addr) != NULL)
        printf("         IP addr = %s\n", sock_ntop(sa, sizeof(*sa)));
        prflag = 0;
        i = 0;
        do {
            if (hwa->if_haddr[i] != '\0') 
            {
                prflag = 1;
                break;
            }
        } while (++i < IF_HADDR);
        if (prflag) 
        {
            printf("         HW addr = ");
            ptr = hwa->if_haddr;
            i = IF_HADDR;
            do {
		ifi_info[x].mac_addr[i]=*ptr & 0xff; 
                printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
            } while (--i > 0);
        }
	
        printf("\n         interface index = %d\n\n", hwa->if_index);
       
	
        strcpy(ifi_info[x].name,hwa->if_name);
	strcpy(ifi_info[x].ip_addr,sock_ntop(sa, sizeof(*sa)));
        strcpy(ifi_info[x].mac_addr,hwa->if_haddr);
        ifi_info[x].index=hwa->if_index;
        x++;
    }
    free_hwa_info(hwahead);
    printf("\n----------------------------------------------------------------------------\n\n");fflush(stdout);
    sockunix = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (sockunix == -1) { error("Error in socket creation"); }
    //int sockraw[10];
    int sockopt=1;
    for(i=0;i<x;i++)
    {
	sockraw = socket(PF_PACKET, SOCK_RAW, htons(3243));
    	if (sockraw == -1) { error("Error in socket creation"); }
 	if(strcmp(ifi_info[i].name,"lo") && strcmp(ifi_info[i].name,"eth0"))
	{
	    if((setsockopt(sockraw, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt)))<0)
		error("Error in setsock");
	    if(setsockopt(sockraw, SOL_SOCKET, SO_BINDTODEVICE, ifi_info[i].name,IFNAMSIZ-1)<0)
		error("Error in bind");
        }
    }
    bzero(&cliaddr,sizeof(cliaddr));
    bzero(&odraddr,sizeof(odraddr));
    bzero(&odraddr_recv,sizeof(odraddr_recv));
    remove("/tmp/odr6820");
    odraddr.sun_family = AF_LOCAL;
    strcpy(odraddr.sun_path, "/tmp/odr6820");
    
    if((bind(sockunix, (SA *) &odraddr, SUN_LEN(&odraddr)))<0)
        error("Error in bind");
    printf("Adding permanent entries in sunpath-port mapping table..\n");fflush(stdout);
	mapping_entry(server_path,server_port,1);
	mapping_entry(odr_path,odr_port,1);
	
    

while(1)
{
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(sockunix,&rset);
    //int maxd=sockraw[0];
    FD_SET(sockraw,&rset);
	
    int maxfd=sockraw>sockunix?sockraw:sockunix;
    printf("waiting for request...\n");fflush(stdout);
    int ready=select(maxfd+1,&rset,NULL,NULL,NULL);
    if(ready<0)
        error("Error in select");

    if(FD_ISSET(sockunix,&rset))
    {

        int             i=0;
        char            client_filename[50];
        socklen_t           len;
        len=sizeof(cliaddr);
	
        if((n=recvfrom(sockunix,recvline,MAXLINE,0,(SA*)&cliaddr, &len))<0)
            error("Error in recvfrom");
        recvline[n]=0;
	printf("Received request on DGRAM socket\n");fflush(stdout);
	strcpy(client_filename,cliaddr.sun_path);
        force_flag=getmsgcomponents(recvline,&dest_ip,&dest_port,&mesg);
	if(map_head!=NULL)
        printf("Deleting stale entries from sunpath-port mapping table\n");fflush(stdout);
        delete_mapping_entries();
        check=check_map_entry(&client_filename);
	
        if(check==0)
        {	printf("Adding temporary client entry in the sunpath-port mapping table\n");fflush(stdout);
            	mapping_entry(client_filename,var++,0);
	 }
        else    {
		    printf("Updating temporary client entry in the sunpath-port mapping table\n");fflush(stdout);
                    update_mapping_table(client_filename);
                }   
        displaymap();
        if(force_flag==1)
        {
		printf("Force path recovery flag set...\nBroadcasting...\n");fflush(stdout);
            for(a=0;a<x;a++)
            {
                if((strcmp(ifi_info[a].name,"lo"))&&(strcmp(ifi_info[a].name,"eth0")))
             	{       
                	broadcast(sockraw,sendline,a,dest_ip,broadcast_id,force_flag,rrep_sent);
			
		} 
            }//continue;
        }
          
   	 routing_row=check_routing_entry(&dest_ip);
        if(routing_row!=NULL)
        {
	    printf("Destination path details available in routing table\n");fflush(stdout);
            staleness=check_stale(routing_row,atoi(argv[1]));
            if(staleness==1)        //fresh
            {
		printf("Entry isn't stale!\n");fflush(stdout);
               send_rreq(sockraw,opack,routing_row,rrep_sent,recv_index,x); //send to next odr
            }
            else    {
			printf("Entry is stale!\nDeleting the entry from the routing table\n");fflush(stdout);
                         delete_routing_entries(dest_ip);           //delete entry
                         for(a=0;a<x;a++)
                        {
                             if((strcmp(ifi_info[a].name,"lo"))&&(strcmp(ifi_info[a].name,"eth0")))
                            broadcast(sockraw,sendline,a,dest_ip,broadcast_id,force_flag,rrep_sent);     //broadcast
                        }        
                    }
    
        }
        else    {
		    printf("Destination path details NOT available in routing table\n");fflush(stdout);
                    for(a=0;a<x;a++)
                    {
                        if((strcmp(ifi_info[a].name,"lo"))&&(strcmp(ifi_info[a].name,"eth0")))
                        broadcast(sockraw,sendline,a,dest_ip,broadcast_id,force_flag,rrep_sent);
			
		     }
	
                }
     	
}

	
    if(FD_ISSET(sockraw,&rset))
    {
	
        socklen_t           len1;
        len1=sizeof(odraddr_recv);
	unsigned char neighbour_addr[8];
	unsigned char destination_mac[6];
	unsigned char source_mac[6];
	int flag,bid,rps,srcport,destport,asize;

        memset(opack,0,sizeof(struct odr_packet));
	memset(buffer1,0,ETH_FRAME_LEN);
	memset(&neighbour_addr,0,sizeof(neighbour_addr));

        if((n=recvfrom(sockraw,buffer1,ETH_FRAME_LEN,0,(SA*)&odraddr_recv, &len1))<0)
            error("Error in recvfrom");
        recvline[n]=0;

        recv_index=odraddr_recv.sll_ifindex;
	memcpy(&neighbour_addr , &odraddr_recv.sll_addr,sizeof(neighbour_addr));
	int protocol=ntohs(odraddr_recv.sll_protocol);
	//printf("neighbour addr:");
	//for(i=6;i>0;i--)
	//printf("%.2x",neighbour_addr[i]);
	memcpy( (void*)destination_mac,(void*)buffer1, ETH_ALEN);
	memcpy( (void*)source_mac,(void*)(buffer1+ETH_ALEN), ETH_ALEN);
	
	struct odr_packet *data =(struct odr_packet*)(buffer1+14);
	//opack=(struct odr_packet*)data;
	char src[16],dest[16];
	int opt=ntohl(data->type);
	
	memcpy(src,data->source,16);
	memcpy(dest,data->destination,16);
	printf("dest ip %s\n",dest);fflush(stdout);
	int hpc=ntohl(data->hop_count);
	if(opt!=2)
	{
		 flag=ntohl(data->force_flag);
		 bid=ntohl(data->broadcast_id);
		 rps=ntohl(data->rrep_sent);
	}
	if(opt==2)
	{
		 srcport=ntohl(data->source_port);
		 destport=ntohl(data->destination_port);
		 asize=ntohl(data->application_msg_size);
	}
	
        if(opt==0)
            {
		
            //RREQ
	    printf("Received RREQ\n");fflush(stdout);
            checkdest=checkifdest(&dest,x);
	
            if(checkdest!=1)
                {
                    //This is not the dest ODR
                    //check in routing table
		    printf("This is not the destination node\n");fflush(stdout);
                    routing_row=check_routing_entry(&dest_ip);
                    if(routing_row!=NULL)
                    {
			 printf("Entry present in the routing table\n");fflush(stdout);
                         staleness=check_stale(routing_row,argv[1]);
                         if(staleness==1)        //fresh

                         {
                            //fresh
                            //sending RREP
      			    printf("Entry isn't stale!\n");fflush(stdout);
                            if(flag!=1)
                            {
                                send_rrep(sockraw,data,routing_row,&neighbour_addr,recv_index,x);              
                                rrep_sent=1;

                            }   
                            //sending RREQ
                            send_rreq(sockraw,&opack,&routing_row,rrep_sent,recv_index,x);
                         }
                    
                         else    {
                                        //expired entry
                                         //broadcast
				    printf("Entry is stale!\nDeleting the entry from the routing table\n");fflush(stdout);
                                     delete_routing_entries((dest));           //delete entry
                                     for(a=0;a<x;a++)
                                    {
                                         if((strcmp(ifi_info[a].name,"lo"))&&(strcmp(ifi_info[a].name,"eth0"))&&(ifi_info[a].index!=recv_index))
                                        broadcast(sockraw,sendline,a,(dest),bid,flag,rps);     //broadcast
                                    }
                                 }
                        }
			else	{
					printf("Entry NOT available in the routing table\n");fflush(stdout);
					for(a=0;a<x;a++)
					{
						if((strcmp(ifi_info[a].name,"lo"))&&(strcmp(ifi_info[a].name,"eth0"))&&(ifi_info[a].index!=recv_index))
			       			broadcast(sockraw,sendline,a,(dest),bid,flag,rps);     //broadcast
					}
            			}
                }
            else    {
                                    //This is the dest ODR
			printf("This is the destination node\n");fflush(stdout);
                        routing_row=check_routing_entry(&(dest));
			
                        if(routing_row==NULL)
                        {
				printf("Entry NOT available in the routing table\nAdding entry to the routing table...\n");fflush(stdout);	
                            	add_routing_entry(&(dest),source_mac,recv_index,hpc);            //addentry
				display_routing_table();
				routing_row=check_routing_entry(&(dest));
				send_rrep(sockraw,data,routing_row,&neighbour_addr,recv_index,x);	
				rrep_sent=1;
				
                        }
                    else    {
				printf("Entry available in the routing table\n");fflush(stdout);
                                 staleness=check_stale(routing_row,argv[1]);
                                 if(staleness==1)        //fresh
    
                                 {
				    printf("Entry isn't stale!\n");fflush(stdout);
                                    //fresh
                                    //sending RREP
				    printf("Checking if any update is needed in the routing table...\n");fflush(stdout);
				     update_routing_table(dest,hpc,flag);
				    printf("Update completed!\n");fflush(stdout);
				     routing_row=check_routing_entry(&(dest));
                                     send_rrep(sockraw,data,routing_row,&neighbour_addr,recv_index,x);	
                                        rrep_sent=1;
                                 }
                                 else
                                 {
                                     //stale
                                     //addentry
				     printf("Entry is stale!\n");fflush(stdout);
				     printf("Deleting stale entry from routing table\n");fflush(stdout);
                                     delete_routing_entries(dest);
				     printf("Adding new entry to the routing table\n");fflush(stdout);
                                     add_routing_entry(&(dest),source_mac,recv_index,hpc);            //addentry
        			     routing_row=check_routing_entry(&(dest));
	  			     send_rrep(sockraw,data,routing_row,&neighbour_addr,recv_index,x);	
				     rrep_sent=1;
                                 }
                                    
                
                            }
                    }
        }
        if(opt==1)
        {
                //RREP
		printf("Received RREP\n");fflush(stdout);
                checkdest=checkifdest(&dest,x);
                if(checkdest!=1) // not client odr
                {
		    printf("This is not the destination node\n");fflush(stdout);
                    if(opack->force_flag==1)
                    {
                        //update
			printf("Force Flag is 1\nUpdating the routing table...\n");fflush(stdout);
                        update_routing_table(dest,hpc,flag);
			routing_row=check_routing_entry(&(dest));
			send_rrep(sockraw,data,routing_row,&neighbour_addr,recv_index,x);	
                    }
                     else
                        {   
                            routing_row=check_routing_entry(&(dest));
                             if(routing_row==NULL)
                            {
				printf("Entry NOT available in the routing table\Adding entry to the routing table\n");fflush(stdout);
                                add_routing_entry(&(dest),source_mac,recv_index,hpc);            //addentry
				routing_row=check_routing_entry(&(dest));
				send_rrep(sockraw,data,routing_row,&neighbour_addr,recv_index,x);
                            }
                             else    
                                {
				     printf("Entry available in the routing table\n");fflush(stdout);
                                     staleness=check_stale(routing_row,argv[1]);
                                     if(staleness==1)        
        
                                     {
                                        //fresh
					printf("Entry isn't stale entry!\nUpdating routing table...\n");fflush(stdout);
                                        update_routing_table(dest,hpc,flag);
					routing_row=check_routing_entry(&(dest));
					send_rrep(sockraw,data,routing_row,&neighbour_addr,recv_index,x);
                                     }
                                     else
                                     {
                                         //expired
					printf("Entry is stale\nDeleting the entry from the routing table...\n");fflush(stdout);
                                        delete_routing_entries(dest);
					printf("Adding the new entry in the routing table\n");fflush(stdout);
                                        add_routing_entry(&(dest),source_mac,recv_index,hpc);            //addentry
					routing_row=check_routing_entry(&(dest));
					send_rrep(sockraw,data,routing_row,&neighbour_addr,recv_index,x);
                                     }
                                }
                        }   
                }
                else
                {
                    //addentry
		   printf("This is the destination node\nAdding the new entry in the routing table\n");fflush(stdout);
                    add_routing_entry(&(dest),source_mac,recv_index,hpc);            //addentry
                    //send appl payload
                    routing_row=check_routing_entry(&(dest));
                    send_appl_payload(sockraw,&opack,routing_row,recv_index,x);
                }
            
        }
        if(opt==2)
        {
		printf("PAYLOAD TIME\n");    //APPL PAYLOAD
		
                checkdest=checkifdest(&dest,x);
                if(checkdest!=1) // not client odr
                {
             		 //update
			update_routing_table(dest,hpc,flag);
                        send_appl_payload(sockraw,&opack,routing_row,recv_index,x);
	        }
                else
                {
		 		
		         //send appl payload
		         routing_row=check_routing_entry(&(dest));
		         send_appl_payload(sockraw,&opack,routing_row,recv_index,x);

		
        	}    	
        }
    }

}
remove("tmp/odr6820");
exit(0);
}



