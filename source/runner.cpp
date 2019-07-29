#include "encoder.h"
#include "decoder.h"
using namespace std;
bool hasEnding (string const &fullString, string const &ending);
int main(int argc, char** argv)
{
	if (argc != 2)
	{
		return 0;
	}
	else
	{
		string address(argv[1]);
		if (hasEnding(address, "smile"))
		{
			ofstream js;
			fstream sm(address, ios::in | ios::binary);
			js.open(address + ".json", ios::binary);
			Decoder* dec = new Decoder();
			dec->decode(sm, js);
			js.close();
			sm.close();
			return 0;
		}
		else
		{
			ifstream js;
			ofstream sm;
			js.open(address);
			sm.open(address + ".smile", ios::binary);
			Encoder* enc = new Encoder();
			enc->encode(js, sm);
			js.close();
			sm.close();
			return 0;
		}
	}
}


bool hasEnding (string const &fullString, string const &ending) 
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}