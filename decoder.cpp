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


enum Content
{
	Head,
	Key,
	Value
};

class Decoder
{
public:
	Content state;
	Content next;
	char buf[1];
	int arrD;
	int objD;
	vector<string> skeys;
	vector<string> svals;
	vector<string> slvals;
	vector<string> unicode;
	bool comma;
	bool nextComma;

	Decoder()
	{
		state = Head;
		arrD = 0;
		objD = 0;
		comma = false;
		next = Value;
		nextComma = true;
	}

	void decode(fstream& sm, ostream& js)
	{
		while(nextB(sm))
		{
			int b = static_cast<int> (buf[0]) & 0xff;
			switch(state)
			{
				// assume header is right, need to change later
				case Head:
				nextB(sm);
				nextB(sm);
				nextB(sm);
				state = Value;
				break;
				case Key:
				next = Value;
				nextComma = true;
				if (b != 0xfb)
				{
					writeComma(js);
				}
				if (b == 0x20)
				{
					writeChar(js, '\"');
					writeChar(js, '\"');
				}
				else if (b >= 0x40 && b <= 0x7f)
				{
					int idx = b & 0x3f;
					if (idx < skeys.size())
					{
						writeStr(js, skeys[idx]);
					}
				}
				else if (b >= 0x80 && b <= 0xbf)
				{
					int l = b & 0x3f;
					writeStr(sm, js, l+1, skeys);
				}
				else if (b >= 0xc0 && b <= 0xf7)
				{
					// unicode, not implemented yet
					int l = b - 0xc0 + 2;
					writeStr(sm, js, l, unicode);
				}
				else if (b == 0xfb)
				{
					writeChar(js, '}');
					objD --;
					next = Key;
				}
				if (arrD == 0 && b!= 0xfb)
				{
					writeChar(js, ':');
				}
				state = next;
				comma = nextComma;
				break;

				// value case
				case Value:
				next = Key;
				nextComma = true;
				if (arrD >= 1 && b != 0xf9)
				{
					writeComma(js);
				}
				if (b < 0x20)
				{
					cout << "checking index " << b-1 << " for value" << endl;
					if (b-1 < svals.size())
					{
						writeStr(js, svals[b-1]);
					}
				}
				else if (b == 0x20)
				{
					writeChar(js, '\"');
					writeChar(js, '\"');
				}
				else if (b == 0x21)
				{
					writeStr(js, "null");
				}
				else if (b == 0x22)
				{
					writeStr(js, "false");
				}
				else if (b == 0x23)
				{
					writeStr(js, "true");
				}
				else if (b >= 0x24 && b <= 0x27)
				{
					// integral number, not implemented
					int l = b & 0x03;
					if (l == 0)
					{
						int n = zigzagDecode(sm);
						writeNum(js, n);
					}
				}
				else if (b >= 0x28 && b <= 0x2b)
				{
					// floating point number, not implemented
				}
				else if (b >= 0x2c && b <= 0x3f)
				{
					// reserved
				}
				else if (b >= 0x40 && b <= 0x5f)
				{
					int l = (b & 0x1f) + 1;
					writeStr(sm, js, l, svals);
				}
				else if (b >= 0x60 && b <= 0x7f)
				{
					int l = (b & 0x1f) + 33;
					writeStr(sm, js, l, svals);
				}
				else if (b >= 0x80 && b <= 0x9f)
				{
					// tiny unicode unimplemented
				}
				else if (b >= 0xa0 && b <= 0xbf)
				{
					// short unicode unimplemented
				}
				else if (b >= 0xc0 && b <= 0xdf)
				{
					// small integers unimplemented
					int original = (b - 0xc0);
					int written = ZIGZAG(original);
					writeNum(js, written);
				}
				else if (b == 0xe0)
				{
					cout << "writing long stuff " << endl;
					char b[1];
					sm.read(b, 1);
					string longS("");
					while((static_cast<unsigned int> (b[0]) & 0xff) != 0xfc)
					{
						longS += b[0];
						sm.read(b, 1);
					}
					writeStr(js, longS);
				}
				else if (b == 0xec)
				{
					// shared long string reference, not implemented
				}
				else if (b == 0xf8)
				{
					writeChar(js, '[');
					arrD += 1;
					nextComma = false;
					next = Value;
				}
				else if (b == 0xf9)
				{
					writeChar(js, ']');
					arrD -= 1;
				}
				else if (b == 0xfa)
				{
					writeChar(js, '{');
					objD += 1;
					nextComma = false;
				}
				else
				{
					cout << b << endl;
					cout << 0xfa << endl;
				}
				if (arrD >= 1)
				{
					next = Value;
				}
				state = next;
				comma = nextComma;
				break;
			}
		}
		return;
	}

	void writeComma(ostream& js)
	{
		if (comma)
		{
			writeChar(js, ',');
		}
	}

	void writeChar(ostream& js, char c)
	{
		string cs(1, c);
		js.write(cs.c_str(), 1);
	}

	bool nextB(fstream& sm)
	{
		sm.read(buf, 1);
		if (sm)
			return true;
		else
			return false;
	}

	void writeStr(fstream& sm, ostream& js, int length, 
		vector<string>& ref, bool c = true)
	{
		char temp[length+1];
		sm.read(temp, length);
		temp[length] = '\0';
		string s(temp);
		cout << "s is " << s << endl;
		ref.push_back(s);
		if (c)
		{
			writeChar(js, '\"');
			js.write(temp, length);
			writeChar(js, '\"');
		}
		else
		{
			js.write(temp, length);
		}
		return;
	}

	void writeStr(ostream& js, string s, bool c = true)
	{
		if (c)
		{
			writeChar(js, '\"');
			js.write(s.c_str(), s.size());
			writeChar(js, '\"');
		}
		else
		{
			js.write(s.c_str(), s.size());
		}
		return;
	}

	void writeNum(ostream& js, int n)
	{
		char temp[33];
		sprintf(temp, "%d", n);
		string s(temp);
		writeStr(js, s, false);
	}

	int zigzagDecode(fstream& sm)
	{
		int z = 0;
		nextB(sm);
		while(!(*buf & 0x80))
		{
			z <<= 7;
			z |= (static_cast<int>(*buf) & 0x7f);
			nextB(sm);
		}
		z <<= 6;
		z |= (static_cast<int>(*buf) & 0x3f);
		return ZIGZAG(z);
	}


	
};

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		return 0;
	}
	else
	{
		ofstream js;
		string address(argv[1]);
		fstream sm(address, ios::in | ios::binary);
		js.open(address + ".json", ios::binary);
		Decoder* dec = new Decoder();
		dec->decode(sm, js);
		js.close();
		sm.close();
		return 0;
	}
}