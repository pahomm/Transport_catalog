#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>

#include "json.h"

using namespace std;

#include "transport_guide.h"

int main() {
	Json::Document inputJson(Json::Load(cin));
	TransportGuide tg;
	Json::Document outputJson = tg.ProcessingJson(inputJson);
	Json::Upload(cout, outputJson);
	return 0;
}
