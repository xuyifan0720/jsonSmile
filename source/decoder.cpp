#include "decoder.h"
using namespace decoder;

Decoder::Decoder()
{
	state = Head;
	arrD = 0;
	comma = false;
	next = Value;
	nextComma = true;

	for (int i = 0; i < 1; i ++)
		objD.push_back(0);

	// all the escape characters we'll use
	escapes.insert(pair<char, string>('\'', "\\\'"));
	escapes.insert(pair<char, string>('\"',"\\\""));
	escapes.insert(pair<char, string>('\?',"\\\?"));
	escapes.insert(pair<char, string>('\\',"\\\\"));
	escapes.insert(pair<char, string>('\a',"\\a"));
	escapes.insert(pair<char, string>('\b',"\\b"));
	escapes.insert(pair<char, string>('\f',"\\f"));
	escapes.insert(pair<char, string>('\n',"\\n"));
	escapes.insert(pair<char, string>('\r',"\\r"));
	escapes.insert(pair<char, string>('\t',"\\t"));
	escapes.insert(pair<char, string>('\v',"\\v"));
}

void Decoder::decode(fstream& sm, ostream& js)
{
	while(nextB(sm))
	{
		int b = static_cast<int> (buf[0]) & 0xff;
		switch(state)
		{
			// assume header is right, need to change later
			// read 3 more bytes
			case Head:
			nextB(sm);
			nextB(sm);
			nextB(sm);
			state = Value;
			break;
			// when it's a key, by default the next item is a value
			case Key:
			next = Value;
			nextComma = true;
			// if it's not }, then test whether we have to write comma before this key
			if (b != 0xfb)
			{
				writeComma(js);
			}
			// empty string
			if (b == 0x20)
			{
				writeChar(js, '\"');
				writeChar(js, '\"');
			}
			// shared string reference
			else if (b >= 0x40 && b <= 0x7f)
			{
				int idx = b & 0x3f;
				if (idx < skeys.size())
				{
					writeStr(js, skeys[idx]);
				}
			}
			// ascii string
			else if (b >= 0x80 && b <= 0xbf)
			{
				int l = b & 0x3f;
				writeStr(sm, js, l+1, skeys);
			}
			// unicode string
			else if (b >= 0xc0 && b <= 0xf7)
			{
				int l = b - 0xc0 + 2;
				writeStr(sm, js, l, unicode);
			}
			else if (b == 0xfb)
			{
				writeChar(js, '}');
				decObjD();
				if (objD[arrD] < 1)
					next = Value;
				else
				{
					next = Key;
				}
			}
			else
			{
				sm.putback(buf[0]);
			}
			// if we're not in an array and it's not }, write a :
			if (b!= 0xfb)
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
			if (objD[arrD] < 1 && b != 0xf9)
			{
				writeComma(js);
			}
			// short string reference
			if (b < 0x20)
			{
				//cout << "checking index " << b-1 << " for value" << endl;
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
			// integral number
			else if (b >= 0x24 && b <= 0x27)
			{
				int l = b & 0x03;
				if (l == 0)
				{
					int n = zigzagDecode(sm);
					writeNum(js, n);
				}
				else if (l == 1)
				{
					long n = zigzagLong(sm);
					writeNum(js, n);
				}
				else
				{
					// big integers not implemented
				}
			}
			// floating point number
			else if (b >= 0x28 && b <= 0x2b)
			{
				int l = b & 0x03;
				if (l == 0)
				{
					float n = readD(sm);
					writeNum(js, n);
				}
				else if (l == 1)
				{
					double n = readD(sm);
					writeNum(js, n);
				}
				else
				{
					// big floating numbers not implemented
				}
			}
			else if (b >= 0x2c && b <= 0x3f)
			{
				// reserved
			}
			// short ascii
			else if (b >= 0x40 && b <= 0x5f)
			{
				int l = (b & 0x1f) + 1;
				writeStr(sm, js, l, svals);
			}
			// middle-length ascii
			else if (b >= 0x60 && b <= 0x7f)
			{
				int l = (b & 0x1f) + 33;
				writeStr(sm, js, l, svals);
			}
			// short unicode
			else if (b >= 0x80 && b <= 0x9f)
			{
				int l = (b & 0x1f) + 2;
				writeStr(sm, js, l, unicode);
			}
			// middle-length unicode
			else if (b >= 0xa0 && b <= 0xbf)
			{
				int l = (b & 0x1f) + 34;
				writeStr(sm, js, l, unicode);
			}
			// small integers
			else if (b >= 0xc0 && b <= 0xdf)
			{
				int original = (b - 0xc0);
				int written = ZIGZAG(original);
				writeNum(js, written);
			}
			// long string
			else if (b == 0xe0)
			{
				//cout << "writing long stuff " << endl;
				char b[1];
				sm.read(b, 1);
				string longS("");
				// read characters until 0xfc is encountered
				while((static_cast<unsigned int> (b[0]) & 0xff) != 0xfc)
				{
					longS += b[0];
					sm.read(b, 1);
				}
				slvals.push_back(longS);
				writeStr(js, longS);
			}
			// shared long string
			else if (b == 0xec)
			{
				nextB(sm);
				int lsb = static_cast<int> (buf[0]) & 0xff;
				int idx = (0x03 << 8) + lsb;
				if (idx < skeys.size())
				{
					writeStr(js, skeys[idx]);
				}
			}
			// structural markers
			else if (b == 0xf8)
			{
				writeChar(js, '[');
				arrD += 1;
				while(arrD >= objD.size())
					objD.push_back(0);
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
				incObjD();
				nextComma = false;
			}
			else
			{
				cout << b << endl;
				cout << 0xfa << endl;
			}
			if (objD[arrD] < 1)
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

void Decoder::incObjD()
{
	while (arrD >= objD.size())
	{
		objD.push_back(0);
	}
	objD[arrD]++;
}

void Decoder::decObjD()
{
	while (arrD >= objD.size())
	{
		objD.push_back(0);
	}
	objD[arrD]--;
}

void Decoder::writeComma(ostream& js)
{
	if (comma)
	{
		writeChar(js, ',');
	}
}

// write one character
void Decoder::writeChar(ostream& js, char c)
{
	string cs(1, c);
	js.write(cs.c_str(), 1);
}

// read one byte
bool Decoder::nextB(fstream& sm)
{
	sm.read(buf, 1);
	if (sm)
		return true;
	else
		return false;
}

// read length bytes from instream and write them as a string to outstream
// save the reference to ref
// c is whether to write quotation mark around it
void Decoder::writeStr(fstream& sm, ostream& js, int length, 
	vector<string>& ref, bool c)
{
	char temp[length+1];
	sm.read(temp, length);
	temp[length] = '\0';
	string s = "";
	int tempLength = length;
	for (int i = 0; i < tempLength; i ++)
	{
		char t = temp[i];
		// replace escape character 
		if (escapes.find(t) == escapes.end())
		{
			s += t;
		}
		else
		{
			//cout << "in dict " << t << endl;
			string replace = escapes.find(t) -> second;
			//cout << "replacement " << replace << endl;
			s += replace;
			length += replace.size() - 1;
		}
	}
	//cout << "writing s " << s << endl;
	ref.push_back(s);
	if (c)
	{
		writeChar(js, '\"');
		js.write(s.c_str(), length);
		writeChar(js, '\"');
	}
	else
	{
		js.write(s.c_str(), length);
	}
	return;
}

// write a constant string s
void Decoder::writeStr(ostream& js, string s, bool c)
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

// write an integer
void Decoder::writeNum(ostream& js, int n)
{
	char temp[33];
	sprintf(temp, "%d", n);
	string s(temp);
	writeStr(js, s, false);
}

// write a long
void Decoder::writeNum(ostream& js, long n)
{
	char temp[65];
	sprintf(temp, "%ld", n);
	string s(temp);
	writeStr(js, s, false);
}

// write a float
void Decoder::writeNum(ostream& js, float n)
{
	char temp[33];
	sprintf(temp, "%f", n);
	string s(temp);
	writeStr(js, s, false);
}

// write a double
void Decoder::writeNum(ostream& js, double n)
{
	char temp[65];
	sprintf(temp, "%f", n);
	string s(temp);
	writeStr(js, s, false);
}

// read one integer that is not zigzag encoded
int Decoder::vInt(fstream& sm)
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
	return z;
}

// read one integer and zigzag decode it
int Decoder::zigzagDecode(fstream& sm)
{
	int z = vInt(sm);
	return ZIGZAG(z);
}

// read one long
long Decoder::vLong(fstream& sm)
{
	long z = 0;
	nextB(sm);
	while(!(*buf & 0x80))
	{
		z <<= 7;
		z |= (static_cast<long>(*buf) & 0x7f);
		nextB(sm);
	}
	z <<= 6;
	z |= (static_cast<long>(*buf) & 0x3f);
	return z;
}

long Decoder::zigzagLong(fstream& sm)
{
	long z = vLong(sm);
	return ZIGZAG(z);
}

// read one double
double Decoder::readD(fstream& sm)
{
	nextB(sm);
	long l = static_cast<long>(buf[0])&0xff;
	for (int i = 0; i < 9; i ++)
	{
		nextB(sm);
		long k = static_cast<long>(buf[0])&0xff;
		l <<= 7;
		l |= k;
	}
	assert(sizeof(long) == sizeof(double));
	double d = reinterpret_cast<double&>(l);
	return d;
}

// read one float
float Decoder::readF(fstream& sm)
{
	nextB(sm);
	int n = static_cast<int>(buf[0])&0xff;
	for (int i = 0; i < 5; i ++)
	{
		nextB(sm);
		int k = static_cast<int>(buf[0])&0xff;
		n <<= 7;
		n |= k;
	}
	assert(sizeof(float) == sizeof(int));
	float f = reinterpret_cast<float&>(n);
	return f;
}


