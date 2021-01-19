#include<iostream>
#include<list>
#include<map>
using namespace std;
void test_string_order(void)
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
			itor = array.end();
			while(itor != array.begin()) {
				if (*itor < val) {
					array.insert(itor, val);
				}
				itor++;	
		
			}
			//itor = itor + array.size()/2;
			//array.push_back(val);
		}
		cout<<"please input you size data, 0 is end"<<endl;
		cin>>size;
	}

	itor = array.begin();
	while(itor != array.end()) {
		cout<<*itor++<<endl;
	}
	//for (i = 0; i < array.size(); i++){
	//	cout<<buf[i];
	//}
	//delete buf;
}

void test_string_8(void)
{
	list <string> str_list;
	list <string>::iterator itor;
	string str;
	int cnt = 0;
	int front_len, back_len;

	while(cnt++ < 2) {
		cin>>str;
		str_list.push_back(str);
	}

	itor = str_list.begin();
	while(itor != str_list.end()) {
		int front_len, back_len;
		int i;
		front_len = itor->length()/8;
		back_len = itor->length()%8;

		for( i = 0; i < front_len; i++) {
			cout<<itor->substr(i * 8, 8)<<endl;
		}

		cout<<itor->substr(i * 8, back_len);

		for(i=0; i< 8 - back_len; i++)
			cout<<"0";
		cout<<endl;
		itor++;
	}

	
}

void string_format_output(string str)
{
	const char *buf = str.c_str();
	int len = str.length();
	int i;
	int offset = 0;
	unsigned long val = 0;

	for(i=len-1;i>=0;i--) {
		char ch = buf[i];
		if (ch >= '0' && ch <= '9') {
			val += (ch - '0')<<(4*offset);

		} else if (ch >= 'A' && ch <= 'F') {
			val += (ch - 'A' + 10)<<(4*offset);
		} else if (ch == 'x') {
			if (i == 1 && buf[i-1] == '0') {
				cout<<val<<endl;
				return;
			}
				
			break;
		}
		offset++;
	}
	cout<<"data error"<<endl;
}

void int_zhi(void)
{
	unsigned long val;
	list<unsigned long> outlist;
	list<unsigned long>::iterator itor;
	cin>>val;

	unsigned long index = 2;

	while(1) {
		if (val%index == 0) {
			outlist.push_back(index);
			if ( val == index) {
				//outlist.push_back(index);
				break;
			}
			val = val/index;
		} else {
			index++;
		}
	}

	itor = outlist.begin();
	while(itor != outlist.end()) {

		cout<<*itor++<< " ";
		
	}
	cout<<endl;	
}

void map_test(void)
{
	int size ;
	int key,val;
	map<int,int> hashmap;
	map<int,int>::iterator itor;

	cin>>size;

	while(size--) {
		cin>>key>>val;
		if (hashmap.count(key)) {
			hashmap[key] += val;
		} else 
			hashmap.insert(make_pair(key,val));
	}

	itor = hashmap.begin();
	while(itor != hashmap.end()) {
		cout<<itor->first<<itor->second<<endl;
		itor++;
	}


}

void itergne_order(void)
{
	int val;
	int record[10] ={0};
	int add = 1;
	list<int> outlist;
	list<int>::iterator itor;
	cin>>val;

	while(val/add) {
		int key = (val/add)%10;
		add *= 10;
		if (record[key])
			continue;
	
		record[key] = 1;
		outlist.push_back(key);
	}

	itor = outlist.begin();
	while(itor != outlist.end()) {
		cout<<*itor++;
	}
	
	


int main(int argc, char *argv[])
{
	
	return 0;
}