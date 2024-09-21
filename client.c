#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <unistd.h>
	FILE *trace,*data;
	char buffer[1024];
	int client_sock;
	char *ip;
	int port;
	struct sockaddr_in client_addr;
	socklen_t size_addr;
	int e;
	
	void* function(void* arg)
	{
    client_sock = socket(AF_INET,SOCK_STREAM,0);
    if(client_sock<0)
    {
        fprintf(trace,"Error create server socket\n");
        exit(1);
    }
    fprintf(trace,"client socket created successfully\n");
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(port);
    client_addr.sin_addr.s_addr= inet_addr(ip);
    e = connect(client_sock,(struct sockaddr*)&client_addr,sizeof(client_addr));
    if(e<0)
    {
        fprintf(trace,"Error coonecting\n");
        exit(1);
    }
    fprintf(trace,"client connected to the server successfully\n");

    strcpy(buffer,"Hello server\n");
    send(client_sock,buffer,sizeof(buffer),0);
	close(client_sock);
    
	}
int main(int argc, int ** argv)
{
	char service[10];
	int n_port;
	char ip_add[15];
	trace = fopen("trace.txt","a");
	if(trace==NULL){
		printf("Impossible d'ouvrir le fichier trace.txt\n");
	}
	else
	{
		data = fopen("config.txt","r");
		if(data!=NULL)
		{
		//fgets(service,10,data);
		fscanf(data,"%s %d %s",service,&n_port,ip_add);
			if(strcmp(service,"client")==0)
			{
				//fscanf(data,"%d",&n_port);
				//fgets(ip_add,15,data);
				port = 8080;
				ip = "127.0.0.1";
				pthread_t t[2];
				int i;
				
				for(i=0;i<2;i++)
				{
					pthread_create(&t[i],NULL,&function,NULL);
					
				}
				for(i=0;i<2;i++)
				{
					pthread_join(t[i],NULL);
				}
			}
		}
    }
    return 0;
    
}
