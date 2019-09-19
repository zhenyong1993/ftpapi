#include "ftpapi.h"
#include <iostream>
using namespace std;

int main(int argc, char** argv)
{
	cout << "main start" << endl;
	_setAddr("127.0.0.1", "llj", "llj");
	void* p_fd;

	string buffer = "";
	//getFileFromCNC(p_fd, 0, "mount.exp", buffer);
	_listDir(p_fd, "/", buffer);
	cout << "buffer is " << endl;
	cout << buffer << endl;

	return 0;
}
