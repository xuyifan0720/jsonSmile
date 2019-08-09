#include "encoder.h"

using namespace std;
using namespace encoder;


// initialize state. kp and vp are key position and value position(in the reference)
// arrD is how many nested arrays we are in. 
// objD is how many nested objects we are in for each given arrD
Encoder::Encoder()
{
	state = Head;
	kp = 0;
	vp = 1;
	arrD = 0;
	objD.push_back(0);
}

// write a number that is l bytes long
void Encoder::writeNum(ostream &sm, int value, int l)
{
	char* newV = reinterpret_cast<char *> (&value);
	sm.write(newV, l);
}

void Encoder::encode(istream& js, ostream& sm)
{
	// used for input
	char c;
	string s;
	int t;
	bool done = true;
	// keep looping until we reach the end } or ]
	while (done)
	{
		switch(state)
		{
			// write smile format header, then switch to value
			case Head:
			sm << ":)" << endl;
			writeNum(sm, 0x03, 1);
			state = Value;
			break;
			// if we encounter a key
			case Key:
			// first read one character
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
				// we get out of one nested object
				case '}':
				writeNum(sm, 0xfb, 1);
				decObjD();
				// if we're out of all arrays and objects, we stop reading
				if (objD[0] <= 0 && arrD <= 0)
					done = false;
				// if we are in the middle of an array and an outmost object ended,
				// the next thing we'll encounter is a value
				if (objD[arrD] < 1 && arrD > 0)
				{
					state = Value;
				}
				// otherwise it's a key
				else
				{
					state = Key;
				}
				break;
				// key needs to be a string, if it's not, reinterpret it as a value
				default:
				js.putback(c);
				state = Value;
				break;
			}
			break;
			case Value:
			js >> c;
			cout << "encountering " << c << endl;
			switch(c)
			{
				// starts an array, increase array depth and the next thing we encounter
				// will be a value
				case '[':
				writeNum(sm, 0xf8, 1);
				state = Value;
				arrD += 1;
				break;
				// ends an array, decrease array depth, and if we are not in an array 
				// or an object, we stop reading from the file.
				// The next thing we encounter is a key.
				case ']':
				writeNum(sm, 0xf9, 1);
				arrD -= 1;
				if (objD[0] <= 0 && arrD <= 0)
					done = false;
				state = Key;
				break;
				// starts an object, increase the object depth of current array level
				// the next thing we encounter is a key
				case '{':
				writeNum(sm, 0xfa, 1);
				incObjD();
				state = Key;
				break;
				// end of object marker is a key, and shouldn't be here
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
				// if we read a comma, then if we're in an object, the next thing is 
				// a key
				// if we're not in an object, we're in an array, so the next thing is
				// a value
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
			default:
			break;
		}
	}
}

// increase the object depth at current array depth
void Encoder::incObjD()
{
	while (arrD >= objD.size())
	{
		objD.push_back(0);
	}
	objD[arrD]++;
}

// decrease the object depth at current array depth 
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
		// if we see a \, read one byte further to see if it's a escape character.
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
		// collects the characters into a vector
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
	// convert char vector into a string
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
		// if we cannot find the key in the reference list, we have to write the key
		if (skeys.find(s) == skeys.end())
		{
			writeNum(sm, msb|encSize, 1);
			cout << "writing key " << s << endl;
			sm.write(s.c_str(),sizes);
			// put the key into the reference list
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
	// keep track of whether all characters are ascii
	if (!isascii(c))
	{
		allAscii = false;
	}
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
				// zigzag encode
				int k = (n >> 31) ^ (n << 1);
				// set msb to 0xc0
				writeNum(sm, (0xc0|k), 1);
			}
			// if converting the string to an integer is the same as converting it 
			// to a long, then we have an integer
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
				// try convert to a float and a double
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
			// ascii and unicode have different msb
			if (allAscii)
			{
				// short ascii and medium length ascii have different msb and encoded
				// size in smile format
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
			// unicode
			else
			{
				// short unicode
				if (sizes <= 33)
				{
					encSize = sizes - 2;
					msb = 0x80;
				}
				// medium sized unicode
				else
				{
					encSize = sizes - 34;
					msb = 0xa0;
				}
			}

			// check whether it's in reference, if it's not in reference
			if (svals.find(s) == svals.end())
			{
				writeNum(sm, msb|encSize, 1);
				sm.write(s.c_str(),sizes);
				// store the value into reference
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
	// write 0x24 to denote we have an integer next in smile
	writeNum(sm, 0x24, 1);
	// zigzag encode our integer
	int k = (n >> 31) ^ (n << 1);
	// a vector to keep track of what we will write
	vector<int> words;
	// the first thing to write is the 6 lsb of our integer
	words.push_back(k&0x3f);
	k >>= 6;
	// then we write our integer 7 bits at a time
	while (k > 0)
	{
		words.push_back(k&0x7f);
		k >>= 7;
	}
	// actually writing the digits
	for (int i = words.size() - 1; i >= 1; i --)
	{
		writeNum(sm, words[i]&0x7f, 1);
	}
	// for the last 6 digit, set msb to 1 so that we know it's the end of current number
	writeNum(sm, (words[0]&0x3f)|0x80, 1);
}

// write a long (zigzag encoded), similar to how we write an int
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
	// first write 0x28 to indicate we have a floating point next
	writeNum(sm, 0x28, 1);
	// reinterpret it as an int 
	assert(sizeof(float) == sizeof(int));
	int k = reinterpret_cast<int&>(f);
	vector<int> words;
	// write 7 bits at a time, set msb to 0
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

// write a double, similar to writing a float.
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

