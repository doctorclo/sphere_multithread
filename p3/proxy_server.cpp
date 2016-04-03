#include <iostream>
#include <algorithm>
#include <set>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <list>
#include <event.h>
#include <event2/dns.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/event.h>
#include <errno.h>
#include <string.h>
#include <map>
#include <ctime>
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

struct pocket
{
	struct bufferevent *event;
	struct event_base *base;
};

struct server

{
  	struct bufferevent *event;
	struct event ev_read;
	int user_socket;

};

struct client
{
	int id;
	struct event_base  *base;
	struct bufferevent *event;
 	struct bufferevent *server_event;
  	int need_to_close;
};

std::map<std::string,std::list<std::string>> config;
int port_num;
void  special_cb (struct bufferevent *event, short events, void *arg)
{
	std::cout<<"Error,new connection or  EOF  callback"<<std::endl;

  	if ( events & BEV_EVENT_EOF )
  	{
		std::cout<<"EOF was founded"<<std::endl;
    		struct client   *client = (struct client*) arg; //Try send data
    		struct evbuffer *buf_in  = bufferevent_get_input  (event);
    		struct evbuffer *buf_out;
		if (client->event==event)
			buf_out=bufferevent_get_output(client->server_event);
		else
			buf_out=bufferevent_get_output(client->event);
   		evbuffer_remove_buffer (buf_in, buf_out, evbuffer_get_length (buf_in));
 		int bev_fd = bufferevent_getfd (client->event);
    		shutdown (bev_fd, SHUT_WR);
		client->need_to_close=client->need_to_close+1;
		if ( client->need_to_close >= 2)
    		{
      			bufferevent_free (client->server_event);
      			bufferevent_free (client->event);
      			free (client);
    		}
  	}
  	else if ( events & BEV_EVENT_ERROR     ) 	
	{
		std::cout<<"Error was founded"<<std::endl;
    		struct client *client = (struct client*) arg;
    		bufferevent_free (client->server_event);
    		bufferevent_free (client->event);
   	 	free (client);
  	}
}

void  read_cb  (struct bufferevent *event, void *arg)
{
  	std::cout<<"It is  read callback"<<std::endl;
  	struct client  *client = (struct client*) arg;
  	struct evbuffer *buf_in  = bufferevent_get_input  (event);
  	struct evbuffer *buf_out;
	if (client->event==event)
		buf_out=bufferevent_get_output(client->server_event);
	else
		buf_out=bufferevent_get_output(client->event);
  	evbuffer_remove_buffer (buf_in, buf_out, evbuffer_get_length (buf_in));
}
void on_accept(int fd, short ev, void *arg)
{	int k=0;
	std::string ip_port;
	for(auto it = config.cbegin(); it != config.cend(); ++it)
	{
		if (k==port_num)
		{
			std::list<std::string> temp=it->second;
			std::list<std::string>::iterator i=temp.begin();
			std::srand(std::time(0)); 
   			int random_variable = std::rand()%temp.size();
			int num=0;
			for (i; i != temp.end(); ++i)
			{
	 		 	if (num==random_variable)
					ip_port=*i;
				num++;	
			}
		}
		k++;
	}
	std::string delimiter = ":";
	size_t pos = 0;
	std::string token;
	pos = ip_port.find(delimiter);
   	std::string ipp = ip_port.substr(0, pos);
    	ip_port.erase(0, pos + delimiter.length());
	std::string port=ip_port;
	struct client *client;
        client = (struct client *) calloc(1, sizeof(*client));
	std::cout<<"New user, welcome"<<std::endl;
	struct event_base *base = (struct event_base*) arg;
	static int num=0;
	int SlaveSocket = accept(fd, 0, 0);
	set_nonblock(SlaveSocket);
	int proxy_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	
    	if(proxy_socket == -1)
    	{
		std::cout << strerror(errno) << std::endl;
		return ;
    	}
	const  char *ip=ipp.c_str();
    	struct sockaddr_in SockAddr;
   	SockAddr.sin_family = AF_INET;
   	SockAddr.sin_port = htons(std::stoi(port));
   	if (inet_aton(ip,&(SockAddr.sin_addr))==0)
	{
    		std::cout<<"Can not convert add"<<std::endl;
   		return ;    
   	}
	int NewSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	
 	client->server_event = bufferevent_socket_new (base, NewSocket, BEV_OPT_CLOSE_ON_FREE);
 	bufferevent_setcb (client->server_event, read_cb, NULL, special_cb, client);
 
	 bufferevent_socket_connect (client->server_event, (struct sockaddr*) &SockAddr, sizeof (SockAddr)); 
  	client->event = bufferevent_socket_new (base, SlaveSocket, BEV_OPT_CLOSE_ON_FREE);
	num++;
  	bufferevent_setcb  (client->event, read_cb, NULL , special_cb, client);
  	bufferevent_enable (client->event,   EV_READ | EV_WRITE | EV_PERSIST);
  	bufferevent_enable (client->server_event, EV_READ | EV_WRITE | EV_PERSIST);
	return ;
}

int main(int argc, char **argv)
{
	FILE *fp;
	char buff[255];
	if (argc > 1)
	{	
   		fp = fopen(argv[1], "r");
		if (fp==NULL)
		{
			std::cout<<"Can t open this file"<<std::endl;
			return 0;
		}
	}
	else
	{
		std::cout<<"Insert file name"<<std::endl;
		return 0;
	}
	while (fgets(buff, 255, fp) != NULL) {
		printf("%s", buff);
		std::string s = buff;
		std::string delimiter = ",";
		size_t pos = 0;
		std::string token;
		int k=0;
		std::string listen_port;
		std::list<std::string> dst;
		std::list<std::string>::iterator it;
		while ((pos = s.find(delimiter)) != std::string::npos) 	     {
		
    			token = s.substr(0, pos);
    		
    			if (token[0]==' ')
			{	
				token.erase(0,1);	
			}
			s.erase(0, pos + delimiter.length());
			if (k>0)
			{
				dst.push_back(token);
			}
			if (k==0)
			{
				listen_port=token;
			}
			k++;
		}	
		if (s[0]==' ')
		{	
			s.erase(0,1);	
		}
		if (s[s.length()-1]=='\n')
		{
			s.erase(s.length()-1,1);	
		}
		dst.push_back(s);
		config.insert(std::pair<std::string,std::list<std::string>>(listen_port,dst));
 	}
	struct event_base  *base = event_base_new ();
	int MasterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(MasterSocket == -1)
	{
		std::cout << strerror(errno) << std::endl;
		return 1;
	}
	int k=0;
	int listen_port;
	std::srand(std::time(0)); 
   	int random_variable = std::rand()%config.size();
	port_num=random_variable;
	for(auto it = config.cbegin(); it != config.cend(); ++it)
	{
		if (k==random_variable)
		{	std::cout<<"Listen port is "<<it->first<<std::endl;
			listen_port=std::stoi(it->first);
		}
		k++;
	}	
	struct sockaddr_in SockAddr;
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_port = htons(listen_port);
	SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	int Result = bind(MasterSocket, (struct sockaddr *)&SockAddr, sizeof(SockAddr));
	if(Result == -1)
	{
		std::cout << strerror(errno) << std::endl;
		return 1;
	}

	set_nonblock(MasterSocket);
	Result = listen(MasterSocket, SOMAXCONN);
	if(Result == -1)
	{
		std::cout << strerror(errno) << std::endl;
		return 1;
	}
	struct event ev_accept;
	event_set (&ev_accept, MasterSocket, EV_READ | EV_PERSIST, on_accept, base);
	event_base_set (base, &ev_accept);
	event_add(&ev_accept, NULL);
	event_base_loop(base,0);
	return 0;
}
