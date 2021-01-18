#include<iostream>
#include<list>
using namespace std;

int main(int argc, char *argv[])
{
	list<int> array;
	list<int>::iterator itor;
	int *buf;
	int order_buf;
	int val, i, cur_size = 0;
	int size, data_size;

	cout<<"please input you size data, 0 is end"<<endl;
	cin>>size;

	while(size) {
		int val;
		for (i = 0; i < size; ++i) {
			cin>>val;
			itor = array.begin();
			itor = itor + array.size()/2;
			array.push_back(val);
		}
		cout<<"please input you size data, 0 is end"<<endl;
		cin>>size;
	}

	
	for (i = 0; i < array.size(); i++){
		cout<<buf[i];
	}
	//delete buf;

	return 0;
}
