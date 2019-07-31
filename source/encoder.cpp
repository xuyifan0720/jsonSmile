#include "encoder.h"

using namespace std;
using namespace encoder;



Encoder::Encoder()
{
	state = Head;
	next = Free;
	kp = 0;
	vp = 1;
	arrD = 0;
	objD.push_back(0);
}

// write a number that is l bytes long
void Encoder::writeNum(ostream &sm, int value, int l)
{
	//cout << "writing number " << value << endl;
	char* newV = reinterpret_cast<char *> (&value);
	sm.write(newV, l);
}

void Encoder::encode(istream& js, ostream& sm)
{
	char c;
	string s;
	int t;
	bool done = true;
	// keep looping until we reach the end }
	while (done)
	{
		switch(state)
		{
			// write smile format header
			case Head:
			sm << ":)" << endl;
			writeNum(sm, 0x03, 1);
			state = Value;
			break;
			case Key:
			js >> c;
			switch(c)
			{
				// read string in the quotation mark
				case '\"':
				readKey(js, sm);
				state = Value;
				break;
				// skip whitespace and newline
				case ' ':
				break;
				case '\n':
				break;
				case '}':
				writeNum(sm, 0xfb, 1);
				decObjD();
				if (objD[0] <= 0 && arrD <= 0)
					done = false;
				if (objD[arrD] < 1 && arrD > 0)
				{
					state = Value;
				}
				else
				{
					state = Key;
				}
				next = Free;
				break;
				// key needs to be a string
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
					// structural markers. ] and } may denote the end of the file
					case '[':
					writeNum(sm, 0xf8, 1);
					state = Value;
					arrD += 1;
					break;
					case ']':
					writeNum(sm, 0xf9, 1);
					arrD -= 1;
					if (objD[0] <= 0 && arrD <= 0)
						done = false;
					next = Free;
					state = Key;
					break;
					case '{':
					writeNum(sm, 0xfa, 1);
					incObjD();
					next = Free;
					state = Key;
					break;
					case '}':
					js.putback(c);
					state = Key;					
					break;
					// a quotation mark means the value is a string
					case '\"':
					readWord(js, sm, true);
					break;
					case ':':
					break;
					case ',':
					if (objD[arrD] >= 1)
					{
						state = Key;
					}
					break;
					// skip white space and newline
					case ' ':
					break;
					case '\n':
					break;
					// otherwise, it's a number, float, or a literal (true, false, null)
					default:
					js.putback(c);
					readWord(js, sm, false);
					break;
				}
				break;
				// this case is actually never used, but i just left it there
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

void Encoder::incObjD()
{
	while (arrD >= objD.size())
	{
		objD.push_back(0);
	}
	objD[arrD]++;
}

void Encoder::decObjD()
{
	while (arrD >= objD.size())
	{
		objD.push_back(0);
	}
	objD[arrD]--;
}

// read a key
void Encoder::readKey(istream& js, ostream& sm)
{
	char c; 
	vector <char> buf;
	int value;
	bool allAscii = true;
	js >> c;
	//cout << "reading key starting with " << c << endl;
	// if we encounter any non-ascii character, it's a unicode. 
	if (!isascii(c))
	{
		allAscii = false;
	}
	// read all characters before the end quotation mark
	while (c != '\"')
	{
		// handle escape characters
		if (c == '\\')
		{
			js >> noskipws >> c;
			if (!isascii(c))
			{
				allAscii = false;
			}
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
			if (!isascii(c))
			{
				allAscii = false;
			}
		}
		// collects the characters in a vector
		else
		{
			buf.push_back(c);
			js >> noskipws >> c;
			if (!isascii(c))
			{
				allAscii = false;
			}
		}
	}
	string s(buf.begin(), buf.end());
	int sizes = s.size();
	int encSize = 0;
	int msb = 0;
	// use the size of the string and whether it's character or unicode to decide its smile token
	if (allAscii)
	{
		encSize = sizes - 1;
		msb = 0x80;
	}
	else
	{
		encSize = sizes - 2;
		msb = 0xc0;
	}
	// small strings
	if (sizes <= 64)
	{
		if (skeys.find(s) == skeys.end())
		{
			writeNum(sm, msb|encSize, 1);
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
	// long string, I dont think smile format supports long key string
	else
	{
		cout << "writing word " << s << endl;
		sm.write(s.c_str(),sizes);
	}
}

// read a value
void Encoder::readWord(istream& js, ostream& sm, bool quotation)
{
	char c; 
	vector <char> buf;
	int value;
	bool allAscii = true;
	js >> c;
	if (!isascii(c))
	{
		allAscii = false;
	}
	//cout << "reading word starting with " << c << endl; 
	if (c == '\"')
	{
		writeNum(sm, 0x20, 1);
		return;
	}
	// if the string starts with a quotation mark, read until the end quotation mark
	if (quotation)
	{
		while (c != '\"')
		{
			// handle escape characters
			if (c == '\\')
			{
				js >> noskipws >> c;
				if (!isascii(c))
				{
					allAscii = false;
				}
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
				if (!isascii(c))
				{
					allAscii = false;
				}
			}
			// check if we have an ascii string or a unicode string
			else
			{
				buf.push_back(c);
				js >> noskipws >> c;
				if (!isascii(c))
				{
					allAscii = false;
				}
			}
		}
	}
	// if there's no quotation mark, read until we hit a , or structural marker
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
	// compare out string to literals
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
		// if we can convert it to an integer
		try
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
		// if we can't convert it to an integer, it's a decimal number
		catch (invalid_argument& e)
		{
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
			catch (invalid_argument& ia)
			{
				// if we can't convert it to decimal either, handle it like a normal string
				goto normalString;
			}
		}
	}
	else
	{
normalString:
		int msb = 0;
		int encSize = 0;
		int sizes = s.size();
		// determine the smile format token initializer 
		if (sizes <= 64)
		{
			if (allAscii)
			{
				if (sizes <= 32)
				{
					encSize = sizes - 1;
					msb = 0x40;
				}
				else
				{
					encSize = sizes - 33;
					msb = 0x60;
				}
			}
			else
			{
				if (sizes <= 33)
				{
					encSize = sizes - 2;
					msb = 0x80;
				}
				else
				{
					encSize = sizes - 34;
					msb = 0xa0;
				}
			}

			// check whether it's in reference
			if (svals.find(s) == svals.end())
			{
				writeNum(sm, msb|encSize, 1);
				//cout << "writing word " << s << endl;
				sm.write(s.c_str(),sizes);
				if (vp < 32)
				{
					svals.insert(s);
					vref.insert(pair<string, int>(s, vp));
					vp ++;
				}
				return;
			}
			// write the reference number instead
			else
			{
				int pos = vref.find(s)->second;
				writeNum(sm, pos, 1);
				return;
			}
		}
		// write a long string
		else
		{
			cout << "writing word " << s << endl;
			writeNum(sm, 0xe0, 1);
			sm.write(s.c_str(),sizes);
			writeNum(sm, 0xfc, 1);
		}
	}
}

// write an integer (zigzag encoded)
void Encoder::writeNum(ostream& sm, int n)
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

// write a long (zigzag encoded)
void Encoder::writeNum(ostream& sm, long n)
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

// write a floating point
void Encoder::writeNum(ostream& sm, float f)
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

// write a double
void Encoder::writeNum(ostream& sm, double d)
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

