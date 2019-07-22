#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <set>
#include <map>

using namespace std;

enum Content
{
	Head,
	Key,
	Value
};

enum Next
{
	Free,
	Word,
	Number
};

class Encoder
{
public:
	Content state;
	Next next;
	set<string> skeys;
	map<string, int> kref;
	set<string> svals;
	map<string, int> vref;
	int kp;
	int vp;
	Encoder()
	{
		state = Head;
		next = Free;
		kp = 0;
		vp = 1;
	}

	void writeNum(ostream &sm, int value, int l)
	{
		//cout << "writing number " << value << endl;
		char* newV = reinterpret_cast<char *> (&value);
		sm.write(newV, l);
	}

	void encode(istream& js, ostream& sm)
	{
		char c;
		string s;
		int t;
		int arrD = 0;
		int objD = 0;
		bool done = true;
		while (done)
		{
			switch(state)
			{
				case Head:
				sm << ":)" << endl;
				writeNum(sm, 0x03, 1);
				state = Value;
				break;
				case Key:
				js >> c;
				switch(c)
				{
					case '\"':
					readKey(js, sm);
					state = Value;
					break;
					case ' ':
					break;
					case '\n':
					break;
					default:
					js.putback(c);
					state = Value;
					break;
				}
				break;
				case Value:
				switch(next)
				{
					case Free:
					js >> c;
					cout << "encountering " << c << endl;
					switch(c)
					{
						case '[':
						writeNum(sm, 0xf8, 1);
						state = Value;
						arrD += 1;
						break;
						case ']':
						writeNum(sm, 0xf9, 1);
						arrD -= 1;
						next = Free;
						state = Key;
						break;
						case '{':
						writeNum(sm, 0xfa, 1);
						objD += 1;
						next = Free;
						state = Key;
						break;
						case '}':
						writeNum(sm, 0xfb, 1);
						objD -= 1;
						if (objD <= 0)
							done = false;
						next = Free;
						state = Key;
						break;
						case '\"':
						readWord(js, sm, true);
						break;
						case ':':
						break;
						case ',':
						if (arrD == 0)
						{
							state = Key;
						}
						break;
						case ' ':
						break;
						case '\n':
						break;
						default:
						js.putback(c);
						readWord(js, sm, false);
						break;
					}
					break;
					case Word:
					{
					char cc = js.peek();
					if (cc == '{' || cc == '[')
					{
						next = Free;
						break;
					}
					if (cc == '\"')
					{
						next = Free;
						break;
					}
					else
						readWord(js, sm, false);
					next = Free;
					break;
					}
					default:
					break;
				}
				default:
				break;
			}
		}
	}

	void readKey(istream& js, ostream& sm)
	{
		char c; 
		vector <char> buf;
		int value;
		js >> c;
		cout << "reading key starting with " << c << endl;
		while (c != '\"')
		{
			if (c == '\\')
			{
				js >> noskipws >> c;
				switch(c)
				{
					case '\"':
					buf.push_back('\"');
					break;
					case '\\':
					buf.push_back('\\');
					break;
					case '\?':
					buf.push_back('\?');
					break;
					case '\'':
					buf.push_back('\'');
					break;
					case 'a':
					buf.push_back('\a');
					break;
					case 'b':
					buf.push_back('\b');
					break;
					case 'f':
					buf.push_back('\f');
					break;
					case 'n':
					buf.push_back('\n');
					break;
					case 'r':
					buf.push_back('\r');
					break;
					case 't':
					buf.push_back('\t');
					break;
					case 'v':
					buf.push_back('\v');
					break;
					default:
					buf.push_back('\\');
					buf.push_back(c);
					break;
				}
				js >> noskipws >> c;
			}
			else
			{
				buf.push_back(c);
				js >> noskipws >> c;
			}
		}
		string s(buf.begin(), buf.end());
		int sizes = s.size();
		if (sizes <= 64)
		{
			if (skeys.find(s) == skeys.end())
			{
				int encodedSize = sizes - 1;
				writeNum(sm, 0x80|encodedSize, 1);
				cout << "writing key " << s << endl;
				sm.write(s.c_str(),sizes);
				if (kp < 64)
				{
					skeys.insert(s);
					kref.insert(pair<string, int>(s, kp));
					kp ++;
				}
				return;
			}
			else
			{
				int pos = kref.find(s) -> second;
				writeNum(sm, 0x40|pos, 1);
				return;
			}
		}
		// long string, not yet implemented
		else
		{
			cout << "writing word " << s << endl;
			sm.write(s.c_str(),sizes);
		}
	}

	void readWord(istream& js, ostream& sm, bool quotation)
	{
		char c; 
		vector <char> buf;
		int value;
		js >> c;
		cout << "reading word starting with " << c << endl; 
		if (c == '\"')
		{
			writeNum(sm, 0x20, 1);
			return;
		}
		if (quotation)
		{
			while (c != '\"')
			{
				if (c == '\\')
				{
					js >> noskipws >> c;
					switch(c)
					{
						case '\"':
						buf.push_back('\"');
						break;
						case '\\':
						buf.push_back('\\');
						break;
						case '\?':
						buf.push_back('\?');
						break;
						case '\'':
						buf.push_back('\'');
						break;
						case 'a':
						buf.push_back('\a');
						break;
						case 'b':
						buf.push_back('\b');
						break;
						case 'f':
						buf.push_back('\f');
						break;
						case 'n':
						buf.push_back('\n');
						break;
						case 'r':
						buf.push_back('\r');
						break;
						case 't':
						buf.push_back('\t');
						break;
						case 'v':
						buf.push_back('\v');
						break;
						default:
						buf.push_back('\\');
						buf.push_back(c);
						break;
					}
					js >> noskipws >> c;
				}
				else
				{
					buf.push_back(c);
					js >> noskipws >> c;
				}
			}
		}
		else
		{
			while (c != '}' && c!= ']' && c!=',')
			{
				buf.push_back(c);
				js >> c;
			}
			js.putback(c);
		}
		string s(buf.begin(), buf.end());
		cout << "reading " << s << endl;
		if (s.compare("false")  == 0|| s.compare("False") == 0)
		{
			writeNum(sm, 0x22, 1);
			return;
		}
		if (s.compare("true")  == 0|| s.compare("True") == 0)
		{
			writeNum(sm, 0x23, 1);
			return;
		}
		if (s.compare("null")  == 0|| s.compare("Null") == 0)
		{
			writeNum(sm, 0x21, 1);
			return;
		}
		// without a quotation mark, it's either an integer or a decimal number
		if (!quotation) 
		{
			// if we can convert it to a double
			try
			{
				double d = stod(s);
				float f = static_cast<float>(d);
				double dd = static_cast<double>(f);
				// can be expressed with a float
				if (d == dd)
				{
					writeNum(sm, f);
				}
				else
				{
					writeNum(sm, d);
				}
			}
			catch (invalid_argument& e)
			{
				long l = stol(s);
				int n = static_cast<int>(l);
				long ll = static_cast<long>(n);
				// small integer
				if (l >= -16 && l <= 15)
				{
					int k = (n >> 31) ^ (n << 1);
					writeNum(sm, (0xc0|k), 1);
				}
				else if (l == ll)
				{
					// can be expressed with an int
					writeNum(sm, n);
				}
				else
				{
					writeNum(sm, l);
				}
			}
		}
		else
		{
			int sizes = s.size();
			// assume it's short ascii, other cases not implemented
			if (sizes <= 32)
			{
				if (svals.find(s) == svals.end())
				{
					int encodedSize = sizes - 1;
					writeNum(sm, 0x40|encodedSize, 1);
					cout << "writing word " << s << endl;
					sm.write(s.c_str(),sizes);
					if (vp < 32)
					{
						svals.insert(s);
						vref.insert(pair<string, int>(s, vp));
						vp ++;
					}
					return;
				}
				else
				{
					int pos = vref.find(s)->second;
					writeNum(sm, pos, 1);
					return;
				}

			}
			else if (sizes <= 64)
			{
				int encodedSize = sizes - 33;
				writeNum(sm, 0x60|encodedSize, 1);
				cout << "writing word " << s << endl;
				sm.write(s.c_str(),sizes);
				return;
			}
			else
			{
				cout << "writing word " << s << endl;
				writeNum(sm, 0xe0, 1);
				sm.write(s.c_str(),sizes);
				writeNum(sm, 0xfc, 1);
			}
		}
	}

	void writeNum(ostream& sm, int n)
	{
		writeNum(sm, 0x24, 1);
		int k = (n >> 31) ^ (n << 1);
		vector<int> words;
		words.push_back(k&0x3f);
		k >>= 6;
		while (k > 0)
		{
			words.push_back(k&0x7f);
			k >>= 7;
		}
		for (int i = words.size() - 1; i >= 1; i --)
		{
			writeNum(sm, words[i]&0x7f, 1);
		}
		writeNum(sm, (words[0]&0x3f)|0x80, 1);
	}

	void writeNum(ostream& sm, long n)
	{
		writeNum(sm, 0x25, 1);
		long k = (n >> 63) ^ (n << 1);
		vector<long> words;
		words.push_back(k&0x3f);
		k >>= 6;
		while (k > 0)
		{
			words.push_back(k&0x7f);
			k >>= 7;
		}
		for (int i = words.size() - 1; i >= 1; i --)
		{
			int next = static_cast<int>(words[i]&0x7f);
			writeNum(sm, next, 1);
		}
		int next = static_cast<int>((words[0]&0x3f)|0x80);
		writeNum(sm, next, 1);
	}

	void writeNum(ostream& sm, float f)
	{
		writeNum(sm, 0x28, 1);
		assert(sizeof(float) == sizeof(int));
		int k = reinterpret_cast<int&>(f);
		vector<int> words;
		for (int i = 0; i < 5; i ++)
		{
			words.push_back(k&0x7f);
			k >>= 7;
		}
		for (int i = words.size() - 1; i >= 0; i --)
		{
			writeNum(sm, words[i]&0x7f, 1);
		}
	}

	void writeNum(ostream& sm, double d)
	{
		writeNum(sm, 0x29, 1);
		assert(sizeof(double) == sizeof(long));
		long k = reinterpret_cast<long&>(d);
		vector<int> words;
		for (int i = 0; i < 10; i ++)
		{
			words.push_back(static_cast<int>(k&0x7f));
			k >>= 7;
		}
		for (int i = words.size() - 1; i >= 0; i --)
		{
			writeNum(sm, words[i]&0x7f, 1);
		}
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
		ifstream js;
		ofstream sm;
		string address(argv[1]);
		js.open(address);
		sm.open(address + ".smileTemp", ios::binary);
		Encoder* enc = new Encoder();
		enc->encode(js, sm);
		js.close();
		sm.close();
		return 0;
	}
}