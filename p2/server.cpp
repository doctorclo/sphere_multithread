#define MAX_EVENTS 10
#define PORT 1489
#include <iostream>
#include <algorithm>
#include <set>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/epoll.h>
#include <map>
#include <utility>
#include <functional>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
char Buffer[1024];
void clean()
{
    for (int i=0;i<1024;i++)
        Buffer[i]='\0';
}
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
{
    struct epoll_event ev, events[MAX_EVENTS];
    int listen_sock, conn_sock, nfds, epollfd;
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    int enable = 1;
    std::set<int> sockets;
    std::map <int,std::string> users_names;
    if (listen_sock == -1)
    {
	std::cout << strerror(errno) << std::endl;
        return 1;
    }

    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(PORT);
    SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        std::cout<<"setsockopt(SO_REUSEADDR) failed";
    int Result = bind(listen_sock, (struct sockaddr *)&SockAddr, sizeof(SockAddr));

    if (Result == -1)
    {   std::cout<<"Can not bind"<<std::endl;
	std::cout << strerror(errno) << std::endl;
	return 1;
    }
    std::cout<<"Bind is created"<<std::endl;
	//set_nonblock(listen_sock);
    int members=1;
    Result = listen(listen_sock, SOMAXCONN);	
    if(Result == -1)
    {   std::cout<<"Can not listen"<<std::endl;
	std::cout << strerror(errno) << std::endl;
	return 1;
    }
    std::cout<<"Listen is created"<<std::endl; 
    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

     ev.events = EPOLLIN;
     ev.data.fd = listen_sock;
     if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
     }

    for (;;) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listen_sock) {
		std::cout<<"accepted connection"<<std::endl;
                conn_sock = accept(listen_sock,
                                          0, 0);
                if (conn_sock == -1) {
                    perror("accept");
                    exit(EXIT_FAILURE);
                 }
                      
                char name[50];
	        for (int i=0;i<50;i++)
                    name[i]='\0';
		send(conn_sock,"Welcome\n\0",9, MSG_NOSIGNAL);
	        set_nonblock(conn_sock);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_sock;
                sockets.insert(conn_sock);
		std::string new_name=std::string(name);           	
                users_names.insert ( std::pair<int,std::string>(conn_sock,"anon"));
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock,
                                   &ev) == -1) {
                    perror("epoll_ctl: conn_sock");
                    exit(EXIT_FAILURE);
                }
            }
	    else if((events[n].events & EPOLLERR)||(events[n].events & EPOLLHUP)){        
	        shutdown(events[n].data.fd, SHUT_RDWR);
	        close(events[n].data.fd);
            } else {			
		     
		clean();
		int RecvSize = recv(events[n].data.fd, Buffer, 1024, MSG_NOSIGNAL);
		      
		if (RecvSize>0){
                    int speak_socket=events[n].data.fd;
                    char speaker_name[128];
                    if (users_names.find(speak_socket)->second.compare(std::string("anon"))==0)
                    {  
                       std::map<int,std::string>::iterator it;
                        it = users_names.find(events[n].data.fd);
                        if (it != users_names.end())
                             users_names.erase (it);
			users_names.insert ( std::pair<int,std::string>(speak_socket,Buffer));
                    } else {
                        strncpy(speaker_name, users_names.find(speak_socket)->second.c_str(), sizeof(speaker_name));
                        speaker_name[sizeof(speaker_name) - 1] = 0; 
                        char final_buffer[1024];
                        int temp=strlen(speaker_name);
	                if (speaker_name[temp-1] == '\n'){
			    speaker_name[temp-1] = '\0';
		

                        }
                        
                        sprintf (final_buffer, "<%s> : %s",speaker_name,Buffer);
 
                        for(auto Iter = sockets.begin(); Iter != sockets.end(); Iter++)
			    if ((*Iter!=speak_socket)){
                               send(*Iter, final_buffer, RecvSize+strlen(speaker_name)+5, MSG_NOSIGNAL);
                             }
                    }      
                } else {
                    std::cout<<"connection terminated"<<std::endl;
                    shutdown(events[n].data.fd, SHUT_RDWR);
                    close(events[n].data.fd);
                    sockets.erase(events[n].data.fd);
	            std::map<int,std::string>::iterator it;
                    it = users_names.find(events[n].data.fd);
  	            if (it != users_names.end())
                        users_names.erase (it);
		                                    
                } 
            }
        }
    }
    return 0;
}
