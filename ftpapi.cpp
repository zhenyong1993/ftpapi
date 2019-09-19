#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#include "ftpapi.h"

#include <map>
#include <vector>
#include <fstream>
#include <iostream>
#include <regex>
#include "json.h"
#include "interface.h"
using namespace std;


string G_user; //ftp username
string G_pwd; //ftp password
string G_ip; //ftp address
void _setAddr(const string& ip, const string& user, const string& pwd)
{
    G_ip = ip;
    G_user = user;
    G_pwd = pwd;
}

int _sendFileToCNC(void *handle, int type, const string&fileName, const string& content)
{
    std::ofstream ifs("send_tmp.txt", std::ofstream::trunc);
    ifs << content;
    ifs << endl;
    ifs.close();
    string destFile = "ftp://"+G_ip+"/"+fileName;
    cout << "destFile is " << destFile << endl;
    return ftpUploadFile(destFile.c_str(), "send_tmp.txt", G_user.c_str(), G_pwd.c_str());
}

int _getFileFromCNC(void *handle, int type, const string&fileName,string& content)
{
    string destFile = "ftp://"+G_ip+"/"+fileName;
    int ret = ftpDownloadFile(destFile.c_str(), "recv_tmp.txt", G_user.c_str(), G_pwd.c_str());
    if(0 != ret) return ret;

    std::ifstream ifs("recv_tmp.txt");

    char line[128]; int numBytes;
    cout << "before while loop" << endl;
    while (ifs.getline(line, 128))
    {
    //    ifs.getline(line, 128);
    //    numBytes = ifs.gcount();
        content += line;
        content += "\n";
    }
    cout << "after while loop" << endl;
    ifs.close();
    return 0;
}

int _listDir(void *handle,const   string&dir,string& content)
{
    string destFile = "ftp://"+G_ip+dir;
    ftpGetDirInfo(destFile.c_str(), "tmp_list.txt", G_user.c_str(), G_pwd.c_str());

    std::ifstream ifs("tmp_list.txt");
    char line[128]; int numBytes;
    string tmp = "";
    string destJson = "";
    Json::Value JsonRoot;

    while (!ifs.eof())
    {

        ifs.getline(line, 128);
        // numBytes = ifs.gcount();
        // content += line;
        // content += "\n";

        tmp = line;
        smatch res;
        bool flag = regex_match(tmp, res, regex("^([^\\s]*)\\s*([\\d]*)\\s*([^\\s]*)\\s*([^\\s]*)\\s*([\\d]*)\\s*([^\\s]*)\\s*([^\\s]*)\\s*([^\\s]*)\\s*([^\\s]*)\\s*$"));
        if(flag)
        {
            //fill to json
            Json::Value filename;
            string chTime = string(res[6]) + " " + string(res[7]) + " " + string(res[8]);
//            cout << "chTime is " << chTime << endl;
            filename["changeTime"] = Json::Value(chTime.c_str());
            filename["changeTime"] = Json::Value(chTime.c_str());

            filename["name"] = string(res[9]);
            filename["size"] = string(res[5]);
            filename["type"] = (string(res[1]).compare(0,1,"d")==0)?"dir":"file";

            if(!string(res[9]).empty())
                JsonRoot[res[9]] = filename;
            else
                cout << "empty match, ignore" << endl;
        }
        else
        {
            //perhaps we need another regex to fit specific content --xzy
            cout << "match failed" << endl;
        }

    }
    Json::FastWriter writer;
    content = writer.write(JsonRoot);

    // ostringstream os;
    // Json::StreamWriterBuilder builder;
    // builder["commentStyle"] = "None";
    // builder["indentation"] = "";
    // unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    // writer.write(JsonRoot, &os);
    // content = os.str();

}

int _deleteNcFile(void *handle,const string& programName)
{
    return ftpDeleteFile(("ftp://"+G_ip).c_str(), programName.c_str(), G_user.c_str(), G_pwd.c_str());
}
