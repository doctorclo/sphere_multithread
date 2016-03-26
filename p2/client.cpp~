#include <iostream>
#include <algorithm>
#include <set>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#define PORT 1489
int set_nonblock(int fd)
{
	int flags;
#if defined(O_NONBLOCK)
	if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
		flags = 0;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
	flags = 1;
	return ioctl(fd, FIOBIO, &flags);
#endif
} 

int main(int argc, char **argv)
{   int first_message=0;	
    char name[256];
    int MasterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	
    if(MasterSocket == -1)
    {
	std::cout << strerror(errno) << std::endl;
	return 1;
    }
    const  char *ip="127.0.0.1";
    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(PORT);
    if (inet_aton(ip,&(SockAddr.sin_addr))==0){
    std::cout<<"Can not convert add"<<std::endl;
    return 0;    
    }
    if (0!=connect(MasterSocket,(struct sockaddr*)&SockAddr,sizeof(SockAddr))){
        std::cout<<"Connect is not established"<<std::endl;
	return 0; 
    } 
    set_nonblock(MasterSocket);
    std::cout<<"Connect is established"<<std::endl;
    int std_in=0;
    set_nonblock(std_in);
    int Max;
    while(true)
    {
	fd_set Set;
	FD_ZERO(&Set);
	FD_SET(0,&Set);
	FD_SET(MasterSocket,&Set);
	   if (MasterSocket>std_in)
		Max=MasterSocket;
	   else
		Max=std_in;
	if(select(Max+1, &Set, NULL, NULL, NULL) <= 0)
	{
	    std::cout << strerror(errno) << std::endl;
	    return 1;
        }		
        if(FD_ISSET(MasterSocket, &Set))
	{                       
                        
            static char Buffer[1024];
	    int RecvSize = recv(MasterSocket, Buffer, 1024, MSG_NOSIGNAL);
	    std::cout<<Buffer;
	    if((RecvSize == 0) && (errno != EAGAIN))
	    {    std::cout<<"Bye"<<std::endl;
	         shutdown(MasterSocket, SHUT_RDWR);
		 close(MasterSocket);
		 break;
	    }
	    if (first_message==0)
		 std::cout<<"To exit write <exit>.\nPlease,insert your name"<<std::endl;	
	}
	if(FD_ISSET(0, &Set)){
	    if (first_message==0)
		fputs("\x1b[1A\x1b[2K", stdout);  
            char message[1024];
            fgets(message,1024,stdin);
            fputs("\x1b[1A\x1b[2K", stdout);                   
            if (strcmp(message,"exit\n")==0){
		send(MasterSocket,NULL,0,0);
		std::cout<<"Goodbye, see you later"<<std::endl;
		shutdown(MasterSocket, SHUT_RDWR);
                close(MasterSocket);
	        break;
            }
	    if (first_message!=0){
		printf("<You> : %s",message);
            }
            first_message=1; 
            send(MasterSocket,message,248,0);
            fflush(stdout);   
	}
    }
    return 0;
}
