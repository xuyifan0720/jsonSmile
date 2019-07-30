#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <set>
#include <map>

using namespace std;


#define ZIGZAG(n) (((n) >> 1) ^ (-((n) & 1)))

namespace decoder
{
	enum Content
	{
		Head,
		Key,
		Value
	};
}


class Decoder
{
public:
	// keep track of current state
	decoder::Content state;
	// keep track of the state after the current byte
	decoder::Content next;
	// read one byte at a time
	char buf[1];
	// array depth
	int arrD;
	// object depth
	vector<int> objD;
	// shared key reference
	vector<string> skeys;
	// shared value reference
	vector<string> svals;
	// shared long value reference 
	// currently not used in smile format, but good to have it here
	vector<string> slvals;
	// shared unicode reference
	// currently not used in smile format
	vector<string> unicode;
	// whether to write a comma before writing current content
	bool comma;
	// whether the next content would need comma
	bool nextComma;
	// escape characters
	map<char, string> escapes;
	Decoder();
	void decode(fstream& sm, ostream& js);
	void incObjD();
	void decObjD();
	void writeComma(ostream& js);
	void writeChar(ostream& js, char c);
	bool nextB(fstream& sm);
	void writeStr(fstream& sm, ostream& js, int length, 
		vector<string>& ref, bool c = true);
	void writeStr(ostream& js, string s, bool c = true);
	void writeNum(ostream& js, int n);
	void writeNum(ostream& js, long n);
	void writeNum(ostream& js, float n);
	void writeNum(ostream& js, double n);
	int vInt(fstream& sm);
	int zigzagDecode(fstream& sm);
	long vLong(fstream& sm);
	long zigzagLong(fstream& sm);
	double readD(fstream& sm);
	float readF(fstream& sm);

};