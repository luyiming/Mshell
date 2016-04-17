#include <iostream>
#include <cstdio>
#include <string>
#include <map>
#include <cstring>
#include <fstream>
#include <cerrno>
#include <vector>

using namespace std;

class CatProcess
{
    vector<string> files;
    map<string, bool> args;
public:
    CatProcess();
    void handleArgument(int argc, const char* argv[]);
    void exec();
};

int main(int argc, const char *argv[])
{
    CatProcess cmd;
    cmd.handleArgument(argc, argv);
    cmd.exec();
    return 0;
}
void CatProcess::exec()
{
    for(int i = 0; i < files.size(); ++i)
    {
        int lineCount = 1;
        string temp;
        string ends;
        bool flag = false;
        if(args["E"])
            ends = "$";
        if(files[i] == "-")
        {
            while(cin >> temp)
            {
                if(args["T"])
                {
                    for(int i = 0; i < temp.size(); ++i)
                    {
                        if(temp[i] == '\t')
                        {
                            temp.erase(i, 1);
                            temp.insert(i, "^I");
                        }
                    }
                }
                if(temp.empty() && args["s"])
                {
                    if(flag)
                        continue;
                    else
                        flag = true;
                }
                if(!temp.empty())
                    flag = false;
                if(args["b"])
                {
                    if(!temp.empty())
                        cout << lineCount++ << "  " << temp << ends << endl;
                    else
                        cout << ends << endl;
                }
                else if(args["n"])
                    cout << lineCount++ << "  " << temp << ends << endl;
                else
                    cout << temp << ends << endl;
            }
        }
        else
        {
            ifstream fin(files[i].c_str());
            if(!fin)
            {
                cerr << "文件不存在" << files[i] << endl;
                return;
            }
            while(getline(fin, temp))
            {
                if(args["T"])
                {
                    for(int i = 0; i < temp.size(); ++i)
                    {
                        if(temp[i] == '\t')
                        {
                            temp.erase(i, 1);
                            temp.insert(i, "^I");
                        }
                    }
                }
                if(temp.empty() && args["s"])
                {
                    if(flag)
                        continue;
                    else
                        flag = true;
                }
                if(!temp.empty())
                    flag = false;
                if(args["b"])
                {
                    if(!temp.empty())
                        cout << lineCount++ << "  " << temp << ends << endl;
                    else
                        cout << ends << endl;
                }
                else if(args["n"])
                    cout << lineCount++ << "  " << temp << ends << endl;
                else
                    cout << temp << ends << endl;
            }
            fin.close();
        }
    }
}
void CatProcess::handleArgument(int argc, const char* argv[])
{
    if(argc == 1)
        files.push_back("-");
    for (int i = 1; i < argc; i++)
    {
        if(argv[i] == NULL)
        {
            fprintf(stderr, "参数为空不合法");
            return;
        }
        if(argv[i][0] == '-')
        {
            if(argv[i][1] == '-')
            {
                if(strcmp(argv[i], "--show-all") == 0)
                    args["A"] = true;
                else if(strcmp(argv[i], "--number-nonblank") == 0)
                    args["b"] = true;
                else if(strcmp(argv[i], "--number") == 0)
                    args["n"] = true;
                else if(strcmp(argv[i], "--squeeze-blank") == 0)
                    args["s"] = true;
                else if(strcmp(argv[i], "--show-tabs") == 0)
                    args["T"] = true;
                else if(strcmp(argv[i], "--show-nonprinting") == 0)
                    args["v"] = true;
                else if(strcmp(argv[i], "--version") == 0)
                    args["version"] = true;
                else if(strcmp(argv[i], "--help") == 0)
                    args["help"] = true;
                else
                {
                    fprintf(stderr, "%s 参数不合法", argv[i]);
                    return;
                }
            }
            else
            {
                if(argv[i][1] == '\0')
                    files.push_back("-");
                for (int j = 1; argv[i][j] != '\0'; j++)
                {
                    if(args.find(string(1, argv[i][j])) != args.end())
                        args[string(1, argv[i][j])] = true;
                    else
                    {
                        fprintf(stderr, "-%c 参数不合法", argv[i][j]);
                        return;
                    }
                }
            }
        }
        else
        {
            files.push_back(string(argv[i]));
        }
    }
}

CatProcess::CatProcess()
{
    args["A"] = false;
    args["b"] = false;
    args["E"] = false;
    args["n"] = false;
    args["s"] = false;
    args["T"] = false;
    args["v"] = false;
    args["help"] = false;
    args["version"] = false;
}
