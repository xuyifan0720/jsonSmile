#include "encoder.h"
#include "decoder.h"
using namespace std;
bool hasEnding (string const &fullString, string const &ending);
void testOne(string address);
void testAll();

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		testAll();
	}
	else
	{
		string address(argv[1]);
		testOne(address);
	}
	return 0;
}

void testOne(string address)
{
	cout << "testing " << address << endl;
	if (hasEnding(address, "smile"))
	{
		ofstream js;
		fstream sm(address, ios::in | ios::binary);
		js.open(address + ".json", ios::binary);
		Decoder* dec = new Decoder();
		dec->decode(sm, js);
		js.close();
		sm.close();
		return;
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
		return;
	}
}

void testAll()
{
	vector<string> addresses;
	addresses.push_back("json-org-sample1");
	addresses.push_back("json-org-sample2");
	addresses.push_back("json-org-sample3");
	addresses.push_back("json-org-sample4");
	for (int i = 0; i < addresses.size(); i++)
	{
		string s = addresses[i];
		string jsonAdd = "testdata/json/" + s + ".jsn";
		string smileAdd = "testdata/smile/"+ s + ".smile";
		testOne(jsonAdd);
		cout << "test finished for " << jsonAdd << endl;
		testOne(smileAdd);
		cout << "test finished for " << smileAdd << endl;
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