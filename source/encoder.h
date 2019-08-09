#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <set>
#include <map>

using namespace std;


namespace encoder
{
	enum Content
	{
		Head,
		Key,
		Value
	};
}




class Encoder
{
public:
	encoder::Content state;
	set<string> skeys;
	map<string, int> kref;
	set<string> svals;
	map<string, int> vref;
	int arrD;
	vector<int> objD;
	int kp;
	int vp;
	Encoder();
	void writeNum(ostream &sm, int value, int l);
	void incObjD();
	void decObjD();
	void encode(istream& js, ostream& sm);
	void readKey(istream& js, ostream& sm);
	void readWord(istream& js, ostream& sm, bool quotation);
	void writeNum(ostream& sm, int n);
	void writeNum(ostream& sm, long n);
	void writeNum(ostream& sm, float f);
	void writeNum(ostream& sm, double d);

};