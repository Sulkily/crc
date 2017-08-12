#include <netinet/ip.h>			
#include <netinet/ip_icmp.h>		
#include <sys/socket.h>			
#include <stdlib.h>				
#include <stdio.h>				
#include <sys/types.h>
#include <errno.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <ifaddrs.h>

unsigned short checksum(unsigned short *buf, int nbytes)
{
	unsigned long sum;
	for (sum = 0; nbytes > 0; nbytes--)
		sum += *buf++;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return ~sum;
}





int main(int argc, char *argv[])
{
	int mysock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP); 
	int ttl = 0;					
	char buffr[4069] = { 0 };		
	struct ip *myIpHeader = (struct ip*)buffr;		
	struct hostent *h,*ownip;	
	struct ifaddrs *ifa;

	getifaddrs(&ifa);						
	ownip = gethostbyname(ifa->ifa_name);	
	
	if (argc < 2) {								
		printf("example use: './program fu-berlin.de'");
		return EXIT_FAILURE;
	}
	
	h = gethostbyname(argv[1]);		/
	if (h == 0) {
		printf("Unkown Host \n");
		exit(EXIT_FAILURE);
	}

	int on = 1;
	if (setsockopt(mysock, IPPROTO_IP, IP_HDRINCL, (const char *)&on, sizeof(on))< 0)	
		printf("setsocketopt failed. \n");													

	struct sockaddr_in cliAddr;	
	cliAddr.sin_port = htons(7);   
	cliAddr.sin_family = AF_INET;	
	inet_pton(AF_INET, argv[1], &(cliAddr.sin_addr)); 
	
	
	
	printf("%s: trace route zu '%s' (IP : %s) \n", argv[0], h->h_name, inet_ntoa(*(struct in_addr *) h->h_addr_list[0]));		

	while (1)		
	{
		//----------------------make own ip header---------------------

		myIpHeader->ip_v = 4;							
		myIpHeader->ip_hl = 5;						
		myIpHeader->ip_tos = 0;												
		myIpHeader->ip_len = 20+8;					// Total length 20 + 8 + 0 =  sizeof(struct ipheader) + sizeof(struct icmpheader) + strlen(data);
		myIpHeader->ip_off = 0;						
		myIpHeader->ip_p = IPPROTO_ICMP;				
		inet_pton(AF_INET, argv[1], &(myIpHeader->ip_dst));		
		inet_pton(AF_INET, ownip->h_name, &(myIpHeader->ip_src));		//set source address
		myIpHeader->ip_sum = 0;											
		myIpHeader->ip_id = htonl(12345);								
		myIpHeader->ip_ttl = ttl;										
		//----------------------make icmp header---------------------
			
		struct icmphdr *myicmphead = (struct icmphdr *) (buffr + 20);		 
		myicmphead->type = ICMP_ECHO;	
		myicmphead->code = 0;				
		myicmphead->checksum = 0;			
		//myicmphead->id = 0;
		//myicmphead->sequence = ttl + 1;
		
		//------------------------------------------------------------
		
		sendto(mysock, buffr , sizeof myIpHeader + sizeof myicmphead, 0, (struct sockaddr*)&cliAddr, sizeof cliAddr);
		
		char buff[4096] = { 0 };			
		struct sockaddr_in serverAddr;		
		socklen_t inlen = sizeof(struct sockaddr_in);	
		recvfrom(mysock, buff, sizeof(buff), 0, (struct sockaddr*) & serverAddr, &inlen);
		
		struct icmphdr *icmphd2 = (struct icmphdr *) (buff + 20);		
				
		if (icmphd2->type != 0)	{		
			printf("ttl limit: %d Address:%s\n", ttl, inet_ntoa(serverAddr.sin_addr));		
			if(ttl==500){
			exit(-1);
			}
		}
		else
		{
			printf("Reached destination:ttl limit: %d Address:%s\n", ttl, inet_ntoa(serverAddr.sin_addr));		
			exit(0);
		}
		ttl++;		
	}

	return 0;
}



