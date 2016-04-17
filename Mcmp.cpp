#include <iostream>
#include <cstdio>
#include <string>
#include <map>
#include <cstring>
#include <fstream>
#include <cerrno>

using namespace std;


class CompareProcess
{
    string file1;
    string file2;
    map<string, bool> args;
public:
    CompareProcess();
    void handleArgument(int argc, const char* argv[]);
    void exec();  
};

int main(int argc, const char* argv[])
{
    CompareProcess cmp;
    cmp.handleArgument(argc, argv);
    cmp.exec();
    return 0;
}

void CompareProcess::exec()
{
    FILE *fp1 = NULL, *fp2 = NULL;
    if((fp1 = fopen(file1.c_str(), "r")) == NULL)
    {
        if(errno == 18)
            cerr << "权限不够" << endl;
        else
            cerr << "无法打开文件" << file1 << endl;
    }
    if((fp2 = fopen(file2.c_str(), "r")) == NULL)
    {
        if(errno == 18)
            cerr << "权限不够" << endl;
        else
            cerr << "无法打开文件" << file2 << endl;
    }
    int lineCount = 1, byteCount = 1;
    int ch1, ch2;
    while((ch1 = fgetc(fp1)) != EOF)
    {
        if((ch2 = fgetc(fp2)) == EOF)
            break;
        if(ch1 == '\n' && ch2 == '\n')
            lineCount++;
        if(ch1 != ch2)
            break;
        byteCount++;
    }
    if(ch1 != ch2)
    {
        if(args["b"] && !args["l"])
            fprintf(stdout, "1 2 不同 : 第 %d 行 , 第 %d 字节为 %o %c\t%o %c\n", lineCount, byteCount, ch1, ch1, ch2, ch2);
        else if(args["l"] && !args["b"])
            fprintf(stdout, "%d\t%o\t%o\n", byteCount, ch1, ch2);
        else if(args["l"] && args["b"])
            fprintf(stdout, "%d %o %c\t%o %c\n", byteCount, ch1, ch1, ch2, ch2);
        else
            fprintf(stdout, "1 2不同 : 第 %d 字节 , 第 %d 行\n", byteCount, lineCount);
    }
}

void CompareProcess::handleArgument(int argc, const char* argv[])
{
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
                if(strcmp(argv[i], "--print-bytes") == 0)
                    args["b"] = true;
                else if(strcmp(argv[i], "--verbose") == 0)
                    args["l"] = true;
                else if(strcmp(argv[i], "--quiet") == 0 || strcmp(argv[i], "--silent") == 0)
                    args["s"] = true;
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
            if(file1.empty())
                file1 = argv[i];
            else if(file2.empty())
                file2 = argv[i];
            else
                cerr << "参数太多" << endl;
        }
    }
}

CompareProcess::CompareProcess()
{
    args["b"] = false;
    args["i"] = false;
    args["l"] = false;
    args["n"] = false;
    args["s"] = false;
    args["help"] = false;
    args["version"] = false;
}
