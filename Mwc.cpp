#include <sys/unistd.h>
#include <sys/types.h>
#include <iostream>
#include <sys/stat.h>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <set>

using namespace std;

int main(int argc, const char *argv[])
{
    set<char> options;
    vector<string> files;
    bool isFirst = true;
    for (int i = 1; i < argc; ++i)
    {
        if(argv[i] == NULL)
            continue;
        if(argv[i][0] == '-')
        {
            for (size_t j = 1; j < strlen(argv[i]); ++j)
            {
                options.insert(argv[i][j]);
            }    
        }
        else
        {
            files.push_back(string(argv[i]));
        }
    }
    if(files.empty())
    {
        int byteCount = 0;
        int charCount = 0;
        int lineCount = 0;
        int wordCount = 0;
        int maxLineLength = 0;
        if(options.size() == 0)
        {
            options.insert('l');
            options.insert('w');
            options.insert('c');
        }
        string tempLine;
        while(getline(cin, tempLine))
        {
            byteCount += sizeof(char) * (strlen(tempLine.c_str()) + 1);
            string tempWord;
            lineCount++;
            //cout << tempLine.size() << endl;
            maxLineLength = maxLineLength > tempLine.size() ? maxLineLength : tempLine.size();
            stringstream ss(tempLine);
            while(ss >> tempWord)
                wordCount++;
        }
        if(isFirst)
        {
            if(options.find('l') != options.end())
                fprintf(stdout, "line\t");
            if(options.find('w') != options.end())
                fprintf(stdout, "word\t");
            if(options.find('c') != options.end())
                fprintf(stdout, "bytes\t");
            if(options.find('L') != options.end())
                fprintf(stdout, "max line");
            fprintf(stdout, "\n");
            isFirst = false;
        }
        if(options.find('l') != options.end())
            fprintf(stdout, "%d\t", lineCount);
        if(options.find('w') != options.end())
            fprintf(stdout, "%d\t", wordCount);
        if(options.find('c') != options.end())
            fprintf(stdout, "%d\t", byteCount);
        if(options.find('L') != options.end())
            fprintf(stdout, "%d\t", maxLineLength);
        fprintf(stdout, "\n");
    }

    for (int i = 0; i < files.size(); ++i)
    {
        ifstream fin;
        fin.open(files.at(i).c_str(), ios::in);
        if(!fin.good())
        {
            cerr << "open file error";
            return 0;
        }
        int byteCount = 0;
        int charCount = 0;
        int lineCount = 0;
        int wordCount = 0;
        int maxLineLength = 0;
        if(options.size() == 0)
        {
            options.insert('l');
            options.insert('w');
            options.insert('c');
        }
        string tempLine;
        while(getline(fin, tempLine))
        {
            string tempWord;
            lineCount++;
            //cout << tempLine.size() << endl;
            maxLineLength = maxLineLength > tempLine.size() ? maxLineLength : tempLine.size();
            stringstream ss(tempLine);
            while(ss >> tempWord)
                wordCount++;
        }
        fin.close();
        FILE* fp = fopen(files.at(i).c_str() ,"r");
        if(fp == NULL)
        {
            cerr << "open file error";
            return 0;
        }
        fseek(fp, 0, SEEK_SET);
        int start = ftell(fp);
        fseek(fp, 0, SEEK_END);
        byteCount = ftell(fp) - start;
        if(isFirst)
        {
            if(options.find('l') != options.end())
                fprintf(stdout, "line\t");
            if(options.find('w') != options.end())
                fprintf(stdout, "word\t");
            if(options.find('c') != options.end())
                fprintf(stdout, "bytes\t");
            if(options.find('L') != options.end())
                fprintf(stdout, "max line");
            fprintf(stdout, "\n");
            isFirst = false;
        }
        if(options.find('l') != options.end())
            fprintf(stdout, "%d\t", lineCount);
        if(options.find('w') != options.end())
            fprintf(stdout, "%d\t", wordCount);
        if(options.find('c') != options.end())
            fprintf(stdout, "%d\t", byteCount);
        if(options.find('L') != options.end())
            fprintf(stdout, "%d\t", maxLineLength);
        fprintf(stdout, "%s", files.at(i).c_str());
        fprintf(stdout, "\n");
    }
        return 0;
}
