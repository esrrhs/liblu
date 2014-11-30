#include "lu.h"
#include <iostream>

int main(int argc, char *argv[])
{
	inilu();
	lu * pl = newlu();
	dellu(pl);
	char c;
	std::cin >> c;
	return 0;
}
