#include <fstream>
#include <iostream>
#include <stdint.h>
#include <map>
#include <list>
#include <string>
#include <algorithm>    
#include <vector>
#include <set>
#define MAX_MEM 10000000
#define MAX_N 5000
void write_to_file(std::map <int64_t,std::list<int64_t>> &rev_index,std::string file_name,int *pos);
void read_from_file(std::string file_name);
void create_index(std::list <std::string> file_names);
void read_final_index();
std::set<int64_t> all_words;
int main(int argc, char **argv)
{
    std::cout<<"Now writing forward index from file"<<std::endl;
    std::ifstream bytefile("test_large"); 
    std::map <int64_t,std::list<int64_t>> rev_index;
    std::list <int> positions;
    std::list <std::string> names;
    char *buff1 = new char[8];
    char *buff2 = new char[4];
    std::list<int64_t>::iterator it1;
    std::map <int64_t,std::list<int64_t>>::iterator it; 
    int size=0;
    int pos1=0;
    int N=0;
    int *pos=&pos1;   
    int num_docs=0;
    while (bytefile.read(buff1, 8) != 0)
    {
        num_docs+=1;
        bytefile.read(buff2, 4);
        size+=8;
        //size+=4;
        int64_t *d_id = reinterpret_cast<int64_t*>(buff1);
        int32_t *n = reinterpret_cast<int32_t*>(buff2);
	    int64_t doc_id=*d_id;
        //std::cout << *d_id << ' ' << *n << std::endl;
        for (int32_t i = 0; i < *n; i++)
        {
            size+=8;
            bytefile.read(buff1, 8);
            int64_t *w_id = reinterpret_cast<int64_t*>(buff1);
            all_words.insert(*w_id);
            if (rev_index.find(*w_id) == rev_index.end()) {
                std::list<int64_t> new_index;
                new_index.push_back (doc_id);
                rev_index.insert ( std::pair<int64_t,std::list<int64_t>>(*w_id,new_index) );

            } else {
                 rev_index.at(*w_id).push_back(doc_id);
            }
            if (size>=MAX_MEM){
                std::cout<<"docs READ"<<num_docs<<std::endl;
               // std::cout<<"WRITE TO FILE"<<std::endl;
                positions.push_back(*pos);
                std::string s = std::to_string(N);
                names.push_back(s);
                write_to_file(rev_index,s,pos);
                size=0;
                N++;
                rev_index.clear();
            }
       //     std::cout << *w_id << ' ';
        }
     //   std::cout << std::endl;
    }
    bytefile.close();
   
    std::cout<<"Writing reverse index"<<std::endl;
  

    std::string s = std::to_string(N);
    write_to_file(rev_index,s,pos);
    names.push_back(s);
    s=std::to_string(2);
    create_index(names);
    //read_from_file(s);
    read_final_index();
    return 0;
}
void write_to_file(std::map <int64_t,std::list<int64_t>> &rev_index,std::string file_name,int*pos)
{
    std::ofstream outfile;
    outfile.open (file_name);
    int t=0;
    std::list<int64_t>::iterator it1;
    std::map <int64_t,std::list<int64_t>>::iterator it;

    for (it=rev_index.begin(); it!=rev_index.end(); ++it){
        int32_t temp=(it->second.size());
        char *list_size=reinterpret_cast<char*>(&temp);
        int64_t temp2=it->first;
        char *d_id = reinterpret_cast<char*>(&temp2);
        outfile.write(d_id,8);
        outfile.write (list_size,4);
        *pos+=12;
        t++;
        std::list<int64_t> new_list=it->second;
        for (it1=new_list.begin(); it1!=new_list.end(); ++it1){
            outfile.write(reinterpret_cast<char*>(&*it1),8);
            *pos+=8;
            t++;
        }
    }
   
  
  
}
void create_index(std::list <std::string> file_names)
{
    int64_t word_num=0;
    int64_t docs_offset=0;
    std::ofstream outfile;
    outfile.open ("final_index");
    char *buff1 = new char[8];
    char *buff2 = new char[4];
    std::ifstream myFiles[MAX_N];
    int need_to_read[MAX_N];
    for (int i=0;i<MAX_N;i++)
        need_to_read[i]=1;
    int64_t read_word[MAX_N];
    int32_t docs_len[MAX_N];
    std::list<std::string>::iterator it1;
    int i=0;
    for (it1=file_names.begin(); it1!=file_names.end(); ++it1){
        myFiles[i].open(*it1);
        i++;
    }
    i=0;
    int64_t min;
    while (1){
        for (i=0; i<file_names.size(); i++){
            if (need_to_read[i]==1){
                if (myFiles[i].read(buff1, 8)!=0){
                    myFiles[i].read(buff2, 4);
                    int64_t *w_id = reinterpret_cast<int64_t*>(buff1);
                    int32_t *n = reinterpret_cast<int32_t*>(buff2);
                    read_word[i]=*w_id;
                    docs_len[i]=*n;
                    min=read_word[i];
                    need_to_read[i]=0;
                } else 
                    need_to_read[i]=-1;           
            }            
        }
        for (i=0;i<file_names.size();i++)
            if (need_to_read[i]==0)
            {
                if (read_word[i]<min)
                {
                    min=read_word[i];
                }
            }
        std::vector <int64_t> all_docs;
        for (i=0;i<file_names.size();i++)
            if (need_to_read[i]==0)
                if (read_word[i]==min)
                {
                    need_to_read[i]=1;
                    for (int32_t j = 0; j < docs_len[i]; j++)
                    {
                        myFiles[i].read(buff1, 8);
                        int64_t *d_id = reinterpret_cast<int64_t*>(buff1);
                        all_docs.push_back(*d_id);
                    }
                }
        std::sort (all_docs.begin(), all_docs.end());
        char *w_id = reinterpret_cast<char*>(&min);
        int32_t nn_docs=(int32_t)(all_docs.size());
        if (nn_docs==0)
            break;
        char *n_docs = reinterpret_cast<char*>(&nn_docs);
        outfile.seekp(word_num*8*2);
        outfile.write(w_id,8);
        word_num+=1;
        int64_t offset=16*(all_words.size())+16+docs_offset;
        char *new_offset = reinterpret_cast<char*>(&offset);
        outfile.write(new_offset,8);
        outfile.seekp(offset);
        outfile.write(n_docs,4);
        docs_offset+=4;
        for (std::vector<int64_t>::iterator it=all_docs.begin(); it!=all_docs.end(); ++it){
            outfile.write(reinterpret_cast<char*>(&*it),8);
            docs_offset+=8;
        }
        int thats_all=1;
        for (i=0;i<file_names.size();i++)
            if (need_to_read[i]!=-1)
            {
                thats_all=0;
                break;
            }
        if (thats_all==1)
            break;
 
    }
    i=0;
    int64_t zero=0;
    char *zeroes = reinterpret_cast<char*>(&zero);
    outfile.seekp(all_words.size()*8*2);
    outfile.write(zeroes,8);
    outfile.write(zeroes,8);
    outfile.close();

}

void read_from_file(std::string file_name)
{


    std::ifstream bytefile1(file_name);
    char *buff1 = new char[8];
    char *buff2 = new char[4];
    while (bytefile1.read(buff1, 8) != 0)
    {
        bytefile1.read(buff2, 4);
        int64_t *d_id = reinterpret_cast<int64_t*>(buff1);
        int32_t *n = reinterpret_cast<int32_t*>(buff2);
        std::cout << *d_id << ' ' << *n << std::endl;
        for (int32_t i = 0; i < *n; i++)
        {
            bytefile1.read(buff1, 8);
            int64_t *w_id = reinterpret_cast<int64_t*>(buff1);
            std::cout << *w_id << ' ';
        }
        std::cout<<std::endl;
    }

 


    bytefile1.close();    
}
void read_final_index()
{
    std::ifstream bytefile1("final_index");
    char *buff1 = new char[8];
    char *buff11=new char[8];
    char *buff2 = new char[4];
    for (int i=0;i<all_words.size();i++){
        bytefile1.seekg(16*i);
        bytefile1.read(buff1, 8);
        int64_t *w_id = reinterpret_cast<int64_t*>(buff1);
        bytefile1.read(buff11, 8);
        int64_t *offset = reinterpret_cast<int64_t*>(buff11);
        std::cout << *w_id << ' ' << *offset << std::endl;
        bytefile1.seekg(*offset);
        bytefile1.read(buff2, 4);
        int32_t *n = reinterpret_cast<int32_t*>(buff2);
        for (int32_t j = 0; j < *n; j++)
        {
            bytefile1.read(buff1, 8);
            int64_t *d_id = reinterpret_cast<int64_t*>(buff1);
            std::cout << *d_id << ' ';
        }
        std::cout<<std::endl;
    }
    bytefile1.seekg(16*all_words.size());
    bytefile1.read(buff1, 8);
    int64_t *zero1 = reinterpret_cast<int64_t*>(buff1);
    bytefile1.read(buff11, 8);
    int64_t *zero2 = reinterpret_cast<int64_t*>(buff11);
    std::cout<<"This is zero after all word id"<<std::endl;
    std::cout << *zero1 << ' ' << *zero2 << std::endl;

    std::cout << "END WRITING"<<std::endl;
    bytefile1.close();    
}

