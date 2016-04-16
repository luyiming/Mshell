#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <math.h> 
#include <unistd.h>
#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>


using namespace std;

int main(int argc, char* argv[])
{
    string path;
    cin >> path;
    int fout = open(path.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    cout << path << endl;
    cout << fout << endl;
    char buf[512] = {'a'};
    cout << buf << endl;
    write(fout, &buf, 512);
    close(fout);
    return 0;
}
