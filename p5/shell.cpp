#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <stddef.h>
#include <signal.h>
#include <fcntl.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstring>
#include <utility>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <cstdio>
#include <cerrno>
#include <regex>
#include <list>
#include <stdio.h>
 #include <stdlib.h>
#include "boost/regex.hpp"
void handler(int a);
void execute(std::list<std::string>,std::list<pid_t>);
void kill_all_processes();
void parse_cmd (std::list<std::string> &cmds, const std::string &line);
int run_programm(int *,char **,int ,int );
void parse(char *input, char **&cmd,std::list<std::string> &res);
std::list<pid_t> pids;
bool ismanaging(char c)
{
        return c == '>' || c == '<' || c == '|' || c == '&' || c == ';';
}
int main()
{
    signal(SIGINT,handler);
    std::string cmd;
    std::list<std::string> parse_cmds;
    std::list<pid_t> pids;
    std::getline(std::cin,cmd,'\n');
    while (!std::cin.eof())
    {
	
        parse_cmd(parse_cmds,cmd);  

        if ( parse_cmds.back () == "&" )
        {
             parse_cmds.pop_back ();
             pid_t  pid = fork ();
             if ( pid < 0 )
             {
                std::perror ("fork");
                exit (EXIT_FAILURE);
             }
             else if ( pid > 0 )
             {
                std::cerr << "Spawned child process " << pid << std::endl;
                parse_cmds.clear ();
             }
             else
             {
                 execute (parse_cmds,pids);
                 return EXIT_SUCCESS;
                                                                                             }
                                                                                       } 
       else
       {
           execute(parse_cmds,pids);
           parse_cmds.clear();
       }
        for ( auto pid : pids )
        {
            int child_status;
            waitpid (pid, &child_status, WNOHANG);
            if ( WIFEXITED (child_status) )
            {
                int ret = WEXITSTATUS (child_status);
                std::cerr << "Process " << pid << " exited: "<< ret << std::endl;
            } 
        }
    std::getline(std::cin,cmd,'\n');
    }
    return 0;
}
void parse_cmd(std::list<std::string> &cmds,const std::string &_s)
{
    std::string command;
    std::string str=_s;
    boost::regex rx("(&&|&|[|][|]|[|]|<|>|[^|&<>\\s]+)");
    boost::smatch res;
    while (boost::regex_search (str,res,rx))
    {
	cmds.push_back (res[0]);
        str = res.suffix().str();
    }
    /*   std::regex  re_cmds ("(&&|&|[|][|]|[|]|<|>|[^|&<>\\s]+)");
    std::regex  re_spcs ("\\s+");
    std::string cmds_parsed = std::regex_replace (cmd, re_spcs, " ");
    std::regex_iterator<std::string::iterator> re_end, re_it (cmds_parsed.begin (), cmds_parsed.end (), re_cmds);
    while ( re_it != re_end )
    {
        std::string s = re_it->str ();
        cmds.push_back (s);
        ++re_it;
    }
   // for (std::list<std::string>::iterator it=cmds.begin(); it != cmds.end(); ++it)
     //      std::cout <<"<<<" << *it;

      //std::cout << '\n';
    return ;*/
    return ;
}
void execute(std::list<std::string> cmds,std::list<pid_t> pids)
{
    char  **argv = new char* [cmds.size()+1];
    int argc=0;
    int input=0;
    int output=1;
    int or_code=0;
    int and_code=0;
    int fd[2];
    int code=-1;
     
    for (auto it=cmds.begin();it!=cmds.end();++it)
    
    {  
        pipe(fd);
        std::string s=*it;

        if ( s == ">" )
        {
            ++it;
            s = *it;
            output=open(s.c_str(),O_WRONLY|O_CREAT|O_TRUNC, 0666);
            if (it==cmds.end())
                break;
        }
        else if(s=="<"){
            ++it;
            s = *it;
            input=open(s.c_str(),O_RDONLY);
            if (it==cmds.end())
                break;
        }
        else if (s=="|")
        {
            int new_code;
            int new_output=fd[1];
            if  (and_code==1)
            {
                if (code==0)
                    new_code=run_programm(&argc,argv,input,new_output);
                else
                    new_code=1;
                and_code=1;
                
            }
            if (or_code==1)
            {
                if (code==1)
                    new_code=run_programm(&argc,argv,input,new_output);
                else
                    new_code=0;
                and_code=1;
            }
            if ((or_code!=1) &&(and_code!=1))
                new_code=run_programm(&argc,argv,input,new_output);
            if ((and_code==1) || (or_code==1))
            {
                input=0;
                output=1;
            }
            or_code=0;
            and_code=0;

            code=new_code;
               
            close(fd[1]);
            input=fd[0];
            argc=0;
            
        }
        else if (s=="&&")
        {
            int new_code;
            if ((code!=1) and (and_code==1))
            {
                new_code=run_programm(&argc,argv,input,output);
            }
            if ((code==1) and (and_code==1)) 
                new_code=1;
            if ((code!=0) and (or_code==1))
            {
                new_code=run_programm(&argc,argv,input,output);
            }
            if ((code==0) and (or_code==1))
                new_code=0;
            if ((or_code!=1) &&(and_code!=1))
                new_code=run_programm(&argc,argv,input,output);
            

            
            if ((and_code==1) || (or_code==1))
            {
                input=0;
                output=1;
            }
            or_code=0;
            and_code=1;

            code=new_code;
                
            argc=0;
        }
        else if (s=="||")
        {
            int new_code;
            if ((code!=1) and (and_code==1))
            {
                new_code=run_programm(&argc,argv,input,output);
            }
            if ((code==1) and (and_code==1)) 
                new_code=1;
            if ((code!=0) and (or_code==1))
            {
                new_code=run_programm(&argc,argv,input,output);
            }
            if ((code==0) and (or_code==1))
                new_code=0;
            if ((or_code!=1) &&(and_code!=1))
                new_code=run_programm(&argc,argv,input,output);
            
             if ((and_code==1) || (or_code==1))
            {
                input=0;
                output=1;
            }
            or_code=1;
            and_code=0;
            code=new_code;
            
            argc=0;
        }           
        
        else{
            s=*it;
            argv[argc] = (char*) s.c_str ();
            argc++;
        }

    }
    if ((code==0) && (and_code==1))
        code=run_programm(&argc,argv,input,output);
    else if ((code==1) && (or_code==1))
        code=run_programm(&argc,argv,input,output);
    else if((and_code==0)&&(or_code==0))
        code=run_programm(&argc,argv,input,output);
   argc=0;
    delete[] argv;
    return; 
}
int run_programm(int *argc,char **argv,int input,int output)
{
    argv[*argc] = NULL;
    pid_t  pid = fork ();
        if (pid==0){
        if (output!=1){
            dup2(output,fileno(stdout));
            close(output);
        }
        if (input!=0){
            dup2(input,fileno(stdin));
            close(input);
        }

        if ( execvp (argv[0], argv) == -1 )
        {
            perror ("execvp");
            exit (EXIT_FAILURE);
        }
    }
    if ( pid <  0 )
    {
        perror ("fork");
        exit (EXIT_FAILURE);
    }
    if (pid>0)
    {
         int child_status;
         //waitpid (pid, &child_status, 0);
         pid=wait(&child_status);
        //if ( WIFEXITED (child_status) )
       // {
            int ret = WEXITSTATUS (child_status);
            std::cerr << "Process " << pid << " exited: " << ret << std::endl;
            return ret;
        //}
        //lse
        //{
        //    return EXIT_SUCCESS;
        //}
    }
}
void kill_all_processes()
{
    for (auto it=pids.begin();it!=pids.end();++it)
    {
        kill(*it,SIGINT);
    }

}
void handler(int a)
{
    signal(SIGINT,handler);
    kill_all_processes();
}

