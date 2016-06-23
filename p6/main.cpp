#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <cassert>
#include <stdio.h>
#include <string.h>
#include <assert.h>




#define     MAX_KEY     32
#define     MAX_VAL     256
#define     MAX_CACHE   4096


std::hash<const char*> my_hash;
using namespace boost::asio;
void new_accept(boost::shared_ptr <boost::asio::ip::tcp::acceptor> acceptor, boost::shared_ptr <boost::asio::ip::tcp::socket> socket);

struct server_client
{
    boost::shared_ptr <boost::asio::ip::tcp::socket> socket;
    boost::shared_ptr <boost::asio::streambuf> buffer;
};
struct hash_table_item
{
    bool is_2;
    char value[MAX_VAL];
    int TLL;
    bool is_1;
    char key[MAX_KEY];

};

static hash_table_item *my_hash_table;
static std::vector <server_client> all_serv_clients;

static boost::asio::io_service service;
bool set(const char *key, const char *value, int TLL)
{
    int start_position =(int)(my_hash(key) %MAX_CACHE);
    int position = start_position;
    
    while (1) {
        if (!my_hash_table[position].is_1)
        {
            break;
        }
        position++;

        if (position == start_position)
            break;
    } 

    if (my_hash_table[position].is_1) {

        return false;

    }
    memcpy(my_hash_table[position].value, value, strlen(value) + 1);
    memcpy(my_hash_table[position].key, key, strlen(key) + 1);
    my_hash_table[position].is_2 = false;
    my_hash_table[position].TLL = TLL;
    my_hash_table[position].is_1 = true;
    
    return true;
}

bool get(const char *key, char *value)
{
    int start_position = (int)(my_hash(key) % MAX_CACHE);
    int position = (int)start_position;

    while (1) {

        if (!my_hash_table[position].is_1) {
            break;
        }

        if (strcmp(key, my_hash_table[position].key) == 0) {
            break;
        }

        position++;

        if (position == start_position)
            break;

    } 
    
    if (!  (my_hash_table[position].is_1 && strcmp(key, my_hash_table[position].key) == 0)  ) {
        return false;
    }
    memcpy(value, my_hash_table[position].value, strlen(my_hash_table[position].value) + 1);
    return true;


}


void need_to_write(server_client &c, const boost::system::error_code &error, size_t size)
{
    return ;
}

void need_to_read(server_client &c, const boost::system::error_code &error, size_t size)
{
    size_t new_size =size;
    new_size ++;
    std::istream new_stream(c.buffer.get());
    std::string cmd, key, value, result;
    int TLL;

    new_stream >> cmd;

    if (cmd == "get") {
        new_stream >> key;
        getline(new_stream, value);

        if (key.size() >= MAX_KEY) {
            result = "[ERROR 1] key is large\n";
        } else {
            char res[MAX_VAL];
            bool temp = get(key.data(), res);

            if (temp) {
                result = "[CORRECT] " + key + ' ' + std::string(res) + '\n';
            } else {
                result = "[ERROR 2] no this key\n";
            }
        }
    } else if (cmd == "set") {

        new_stream >> TLL >> key;
        getline(new_stream, value);


        int ptr = 0;

        while (ptr < int(value.size()) && isspace(value[ptr])) {
            ptr++;
        }

        value = value.substr(ptr);
        if (key.size() >= MAX_KEY) {
            result =  "[ERROR 3] key is large\n";
        } else if (value.size() >= MAX_VAL) {
            result =  "[ERROR 4] valueue is large\n";

        } else {
            bool temp = set(key.data(), value.data(), TLL);

            if (temp) {

                result = "[CORRECT] " + key + ' ' + value + '\n';
            } 
            else {

                result = "[ERROR 5] no memory\n";
            }

        }

    } else {

        getline(new_stream, value);
        result = "[ERROR 0] no this command\n";
    }

    async_write(*c.socket, buffer(result), boost::bind(need_to_write, c, _1, _2));

    if (error) {

        boost::system::error_code new_temp;
        c.socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, new_temp);
        c.socket->close(new_temp);
        std::cout << "Connection has been lost" << std::endl;
        return;

    }


    async_read_until(*c.socket, *c.buffer, "\n", boost::bind(need_to_read, c, _1, _2));
}


void handle_new_accept(boost::shared_ptr <boost::asio::ip::tcp::acceptor> acceptor, boost::shared_ptr <ip::tcp::socket> socket, const boost::system::error_code &error)
{
    if (!error) {
        std::cout << "We accept new connection!" << std::endl;
        server_client new_temp;
        new_temp.socket = socket;
        new_temp.buffer = boost::shared_ptr <boost::asio::streambuf> (new boost::asio::streambuf);
        all_serv_clients.push_back(new_temp);
        async_read_until(*new_temp.socket, *new_temp.buffer, "\n", boost::bind(need_to_read, new_temp, _1, _2));
        boost::shared_ptr <boost::asio::ip::tcp::socket> newsocket(new boost::asio::ip::tcp::socket(service));
        new_accept(acceptor, newsocket);
    }
}

void new_accept(boost::shared_ptr <ip::tcp::acceptor> acceptor, boost::shared_ptr <ip::tcp::socket> socket)
{
    acceptor->async_accept(*socket, boost::bind(handle_new_accept, acceptor, socket, _1));
}

int main()
{
    std::cout<<"Starting new hash server "<<std::endl;

    my_hash_table = (hash_table_item *) calloc(MAX_CACHE, sizeof(*my_hash_table));
    boost::shared_ptr <boost::asio::ip::tcp::acceptor> acceptor(new boost::asio::ip::tcp::acceptor(service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 6000)));
    boost::shared_ptr <boost::asio::ip::tcp::socket> socket(new boost::asio::ip::tcp::socket(service));
    new_accept(acceptor, socket);
    service.run();
    return 0;
}
