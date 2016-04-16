#include <iostream>
#include <cstdio>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <map>
#include <string>
#include <vector>
#include <dirent.h>
#include <fstream>
#include <cstdlib>
#include <climits>

using namespace std;

class CopyProcess
{
    vector<string> src; 
    string dest; 
    map<string, bool> args;
public:
    CopyProcess();
    void handleArgument(int argc, const char* argv[]);
    void run();
};

int main(int argc, const char *argv[])
{
    CopyProcess cmd;
    cmd.handleArgument(argc, argv);
    cmd.run();    
    return 0;
}

void save(DIR* pDir, string srcPath, string destPath)
{
    struct dirent* pDirent = NULL;
    while(NULL != (pDirent = readdir(pDir)))
    {
        string fileName = string(pDirent->d_name);
        if(fileName != "." && fileName != "..")
        {
            if(pDirent->d_type == DT_DIR)
            {
                DIR* childDir = opendir((srcPath + fileName).c_str());
                DIR* destDir = NULL;
                if((destDir = opendir((destPath + fileName).c_str())) == NULL)
                    mkdir((destPath + fileName).c_str(), 0775);
                if(NULL == childDir)
                {
                    cerr << "打开目录\"" << (srcPath + fileName) << "\" 失败" << endl;
                    return;
                }
                save(childDir, srcPath + fileName + "/", destPath + fileName + "/");
                closedir(childDir);
            }
            else
            {
                ifstream fin((srcPath + fileName).c_str(), ios::binary);
                ofstream fout((destPath + fileName).c_str(), ios::binary);
                if(!fin)
                {
                    cerr << "打开文件:" << (srcPath + fileName) << " 失败" << endl;
                    return;
                }
                if(!fout)
                {
                    cerr << "创建文件失败" << (destPath + fileName) << endl;
                    return;
                }
                char buffer[512];
                while(!fin.eof())
                {
                    fin.read(buffer, 512);
                    fout.write(buffer, fin.gcount());
                }
                fin.close();
                fout.close();
                struct stat srcStat;
                stat((srcPath + fileName).c_str(), &srcStat);
                chmod((destPath + fileName).c_str(), srcStat.st_mode);
            }
        }
    }
}

void CopyProcess::run()
{
    if(src.empty())
    {
        if(dest.empty())
        {
            cerr << "cp: 缺少了文件操作数" << endl;
            return;
        }
        cerr << "cp: 在\"" << dest << "\" 后缺少了要操作的目标文件" << endl;
        cerr << "Try 'cp --help' for more information." << endl;
        return;
    }
    else if(src.size() > 1)
    {
        struct stat fileStat;
        stat(dest.c_str(), &fileStat);
        if((fileStat.st_mode & S_IFDIR) == 0)
        {
            cerr << "cp: 目标\"" << dest << "\" 不是目录" << endl;
            return;
        }
    }


    for (size_t i = 0; i < src.size(); ++i)
    {
        struct stat fileStat;
        bool destIsDir = false;
        bool srcIsDir = false;
        if(src[i][src[i].size() - 1] == '/')
        {
            src[i].erase(src[i].size() - 1, 1);
            srcIsDir = true;
        }
        if(dest[dest.size() - 1] == '/')
        {
            dest.erase(dest.size() - 1, 1);
            destIsDir = true;
        }
        string fileName;
        string::size_type index = src[i].find_last_of('/');
        if(index != string::npos)
            fileName = src[i].substr(index + 1);
        else
            fileName = src[i];
        if(stat(src[i].c_str(), &fileStat) == -1)
        {
            if(srcIsDir)
            {
                cerr << "cp: 无法获取\"" << src[i] 
                     << "/\" 的文件状态(stat): 不是目录" << endl;
                return;
            }
            cerr << "cp: 无法获取\"" << src[i] 
                 << "\" 的文件状态(stat): 没有那个文件或目录" << endl;
            return;
        }
        if(fileStat.st_mode & S_IFDIR)
        {
            if(args["r"] == false)
            {
                cout << "cp: 略过目录\"" << src[i] << "\"" << endl;
                continue;
            }
            else
            {
                char srcRealPath[100];
                realpath(src[i].c_str(), srcRealPath);
                char desRealPath[100];
                realpath((dest + "/" + fileName).c_str(), desRealPath);
                if(strcmp(srcRealPath, desRealPath) == 0)
                {
                    cerr << "cp: \"" << src[i] << "\" 与\"" << dest + "/" + fileName << "\" 为同一文件" << endl;
                    return;
                }

                DIR* srcDir = opendir(src[i].c_str());
                DIR* destDir = NULL;
                if((destDir = opendir(dest.c_str())) == NULL)
                    mkdir(dest.c_str(), 0775);
                if((destDir = opendir((dest + "/" + fileName).c_str())) == NULL)
                    mkdir((dest + "/" + fileName).c_str(), 0775);
                if(NULL == srcDir)
                {
                    cerr << "打开目录\"" << src[i] << "\" 失败" << endl;
                    return;
                }

                save(srcDir, src[i] + "/", dest + "/" + fileName +  "/");
                closedir(srcDir);
            }
        }
        else
        {
            struct stat destStat;
            int status = stat(dest.c_str(), &destStat);
            string destPathName;
            if(status == -1)
            {
                if(destIsDir)
                {
                    cerr << "cp: 无法创建普通文件\"" << dest << "\": 不是目录" << endl;
                    return;
                }
                destPathName = dest;
            }
            else
            {
                if(destStat.st_mode & S_IFDIR)
                    destPathName = dest + "/" + fileName;
                else
                    destPathName = dest;
            }
            char srcRealPath[100];
            realpath(src[i].c_str(), srcRealPath);
            char desRealPath[100];
            realpath(destPathName.c_str(), desRealPath);
            if(strcmp(srcRealPath, desRealPath) == 0)
            {
                cerr << "cp: \"" << src[i] << "\" 与\"" << fileName << "\" 为同一文件" << endl;
                return;
            }
            ifstream fin(src[i].c_str(), ios::binary);
            ofstream fout(destPathName.c_str(), ios::binary);
            if(!fin)
            {
                cerr << "打开文件:" << src[i] << " 失败" << endl;
                return;
            }
            if(!fout)
            {
                cerr << "创建文件失败" << endl;
                return;
            }
            char buffer[512];
            while(!fin.eof())
            {
                fin.read(buffer, 512);
                fout.write(buffer, fin.gcount());
            }
            fin.close();
            fout.close();
            chmod(destPathName.c_str(), fileStat.st_mode);
        }
    }
}

void CopyProcess::handleArgument(int argc, const char* argv[])
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
                if(strcmp(argv[i], "--force") == 0)
                    args["f"] = true;
                else if(strcmp(argv[i], "--interactive") == 0)
                    args["i"] = true;
                else if(strcmp(argv[i], "--recursive") == 0)
                    args["r"] = true;
                else if(strcmp(argv[i], "--verbose") == 0)
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
            src.push_back(string(argv[i]));
        }
    }
    if(!src.empty())
    {
        dest = src.back();
        src.pop_back();
    }
}

CopyProcess::CopyProcess()
{
    args["f"] = false;
    args["i"] = false;
    args["p"] = false;
    args["no-preserve"] = false;
    args["r"] = false;
    args["u"] = false;
    args["v"] = false;
    args["help"] = false;
    args["version"] = false;
}

