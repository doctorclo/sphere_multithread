#include <iostream>
#include <list>
#include <algorithm>
#include <ctime>
#include <omp.h>
#include <time.h>
std::list<int>::iterator take_mid(std::list<int>::iterator start, std::list<int>::iterator end) {
	std::list<int>::iterator  middle;
	for (std::list<int>::iterator it = middle = start; it != end; ++it) {
		if (++it == end)
			break;
		++middle;
	}
	return middle;
}

void fast_sort(std::list<int>::iterator start, std::list<int>::iterator end,int num_threads) {
	if ( std::next(start) == end || start == end )
		return;
	std::list<int>::iterator middle = take_mid(start, end);
	#pragma omp parallel num_threads(num_threads)
	{
		if (omp_get_thread_num() != 0)
			fast_sort(middle, end,num_threads);
		else
			fast_sort(start, middle,num_threads);
	}

	std::inplace_merge(start, middle, end);
}

void parall_sort(std::list<int>::iterator start, std::list<int>::iterator end,int num_threads) {
	omp_set_nested(1);
	fast_sort(start, end,num_threads);
}

int main() {
    int num_threads=2;
    unsigned int start_time =  clock(); 
    std::list<int> sorted_list;
	int list_size=1000;
    srand (time(NULL));
    for (int i=0;i<list_size;i++)
        sorted_list.push_back(rand() %list_size +1);
    parall_sort(sorted_list.begin(), sorted_list.end(),num_threads);
	for (std::list<int>::iterator it=sorted_list.begin(); it != sorted_list.end(); ++it)
		std::cout << *it << " ";
	std::cout << std::endl;
	return 0;
}
