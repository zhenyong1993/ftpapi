#include "interface.h"
//#include "json.h"
#include <map>
#include "curl/curl.h"
//#include "md5file.h"
#include <algorithm>
#include <wchar.h>
#include <string>
#include <iostream>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <sys/stat.h>
//#include "io.h"

using namespace std;
//#include "log.h"

#pragma comment(lib,"ws2_32.lib")

#define UPLOADFILEDIR		"/program"
#define DOWNLOADFILEDIR		"/api/program/upload"
#define CHECKFILEURL		"/api/program/check"
#define DELETEFILEURL		"/api/program/delete"
#define UPLOADURL			"http://127.0.0.1:8080/program/"
#define DOWNLOADURL			"http://127.0.0.1:8080/program/"


//#define UPLOAD_FILE_AS  "while-uploading.txt"
//#define REMOTE_URL      "ftp://example.com/"  UPLOAD_FILE_AS
//#define RENAME_FILE_TO  "renamed-and-fine.txt"

//#define FTPBODY "ftp-list"
#define FTPHEADERS "ftp-responses"


int write_string_to_file(const std::string & file_string, const std::string str);

map<string,string> packFileUpload(const string& machineNo,const string& path,const string & url);

map<string,string> packModifyPara(const string& machineNo,const string& paraName,const string & val);

string getLastMessage(const string& ip,const string& machineNo);

map<string,string> packFileDownload(const string& machineNo,const string& path,const string & url,const string mdCode);

int process(string &str, map<string,string> &tmp);


int getFile(char* url, char* destFile);
int waitForFile(const char *url);
void Split(const string &str, const string &delimiter, vector<string> &dest);

void ConvertUtf8ToGBK(string &strUtf8);


//file name
string splitFilename(char* str);

//ftpget
struct FtpFile {
	const char *filename;
	FILE *stream;
};

static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream);

//ftpupload
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream);

//ftpdir
static size_t write_response(void *ptr, size_t size, size_t nmemb, void *data);


string splitFilename(char* str)
{
	string ret = "";
	cout << "Splitting: " << str << '\n';
	string tmp = str;
	size_t found = tmp.find_last_of("/\\");
	ret = tmp.substr(found + 1);
	cout << " file: " << ret << '\n';

	return ret;
}

static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
	struct FtpFile *out = (struct FtpFile *)stream;
	if (!out->stream) {
		/* open file for writing */
		out->stream = fopen(out->filename, "wb");
		if (!out->stream)
			return -1; /* failure, can't open file to write */
	}
	return fwrite(buffer, size, nmemb, out->stream);
}

int ftpDownloadFile(char* ftp_addr, char* path, const char *username, const char *password)
{
	int iret = 0;
	CURL *curl;
	CURLcode res;

	string fileName = splitFilename(ftp_addr);
	struct FtpFile ftpfile = {
		path, /* name to store the file as if successful */
		NULL
	};
	//struct FtpFile ftpfile = {
	//	"curl.tar.gz", /* name to store the file as if successful */
	//	NULL
	//};

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();

	bool isSetUser = true;
	if (strcmp(username, "") == 0 && strcmp(password, "") == 0)
		isSetUser = false;
	// get user_key pair
	char user_key[1024] = { 0 };
	if (isSetUser)
		sprintf(user_key, "%s:%s", username, password);

	printf("isSetUser : %d , username : %s, password: %s , user_key : %s \n\n", isSetUser, username, password, user_key);

	if (curl) {
		/*
		* You better replace the URL with one that works!
		*/

		curl_easy_setopt(curl, CURLOPT_URL, ftp_addr);
		//curl_easy_setopt(curl, CURLOPT_URL,
		//	"ftp://ftp.openssl.org/source/openssl-1.0.2q.tar.gz");
		if (isSetUser)
			curl_easy_setopt(curl, CURLOPT_USERPWD, user_key); // set user:password

		/* Define our callback to get called when there's data to be written */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
		/* Set a pointer to our struct to pass to the callback */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

		/* Switch on full protocol/debug output */
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		res = curl_easy_perform(curl);

		/* always cleanup */
		curl_easy_cleanup(curl);

		if (CURLE_OK != res) {
			/* we failed */
			fprintf(stderr, "curl told us %d\n", res);
			iret = res;
		}
	}
	else
	{
		iret = -1;
	}

	if (ftpfile.stream)
		fclose(ftpfile.stream); /* close the local file */

	curl_global_cleanup();

	return iret;
}


static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	curl_off_t nread;
	/* in real-world cases, this would probably get this data differently
	as this fread() stuff is exactly what the library already would do
	by default internally */
	FILE *filestream = (FILE *)stream;
	size_t retcode = fread(ptr, size, nmemb, filestream);

	nread = (curl_off_t)retcode;

	fprintf(stderr, "*** We read %" CURL_FORMAT_CURL_OFF_T
		" bytes from file\n", nread);
	return retcode;
}

int ftpUploadFile(char* ftp_addr, char* filePath, const char *username, const char *password)
{
	int iret = 0;
	CURL *curl;
	CURLcode res;
	FILE *hd_src;
	struct stat file_info;
	curl_off_t fsize;

	//struct curl_slist *headerlist = NULL;
	//static const char buf_1[] = "RNFR " UPLOAD_FILE_AS;
	//static const char buf_2[] = "RNTO " RENAME_FILE_TO;

	/* get the file size of the local file */
	if (stat(filePath, &file_info)) {
		printf("Couldn't open '%s': %s\n", filePath, strerror(errno));
		return -1;
	}else
	{
		printf("open %s success\n", filePath);
	}
	fsize = (curl_off_t)file_info.st_size;

//	printf("Local file size: %" CURL_FORMAT_CURL_OFF_T " bytes.\n", fsize);
	printf("Local file size: %d bytes.\n", fsize);

	/* get a FILE * of the same file */
	hd_src = fopen(filePath, "rb");

	/* In windows, this will init the winsock stuff */
	curl_global_init(CURL_GLOBAL_ALL);

	/* get a curl handle */
	curl = curl_easy_init();

	bool isSetUser = true;
	if (strcmp(username, "") == 0 && strcmp(password, "") == 0)
		isSetUser = false;
	// get user_key pair
	char user_key[1024] = { 0 };
	if (isSetUser)
		sprintf(user_key, "%s:%s", username, password);

	if (curl) {
		/* build a list of commands to pass to libcurl */
		//headerlist = curl_slist_append(headerlist, buf_1);
		//headerlist = curl_slist_append(headerlist, buf_2);

		/* we want to use our own read function */
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

		/* enable uploading */
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

		/* specify target */
		curl_easy_setopt(curl, CURLOPT_URL, ftp_addr);

		if (isSetUser)
			curl_easy_setopt(curl, CURLOPT_USERPWD, user_key); // set user:password

		/* pass in that last of FTP commands to run after the transfer */
		//curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist);

		/* now specify which file to upload */
		curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);

		/* Set the size of the file to upload (optional).  If you give a *_LARGE
		option you MUST make sure that the type of the passed-in argument is a
		curl_off_t. If you use CURLOPT_INFILESIZE (without _LARGE) you must
		make sure that to pass in a type 'long' argument. */
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
			(curl_off_t)fsize);

		//xzy:
		curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);
		//curl_easy_setopt(curl, CURLOPT_FTPPORT, "-");

		/* Now run off and do what you've been told! */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
		{
			cout << "res is " << res << endl;
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
			iret = -1;
		}

		/* clean up the FTP commands list */
		//curl_slist_free_all(headerlist);

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	else
	{
		iret = -1;
	}

	fclose(hd_src); /* close the local file */

	curl_global_cleanup();
	return iret;
}

static size_t write_response(void *ptr, size_t size, size_t nmemb, void *data)
{
	FILE *writehere = (FILE *)data;
	return fwrite(ptr, size, nmemb, writehere);
}


int ftpGetDirInfo(char* ftp_addr, char* dirpath, const char *username, const char *password)
{
	int iret = 0;
	CURL *curl;
	CURLcode res;
	FILE *ftpfile;
	FILE *respfile;

	/* local file name to store the file as */
	ftpfile = fopen(dirpath, "wb"); /* b is binary, needed on win32 */

									/* local file name to store the FTP server's response lines in */
	respfile = fopen(FTPHEADERS, "wb"); /* b is binary, needed on win32 */

	curl = curl_easy_init();

	bool isSetUser = true;
	if (strcmp(username, "") == 0 && strcmp(password, "") == 0)
		isSetUser = false;
	// get user_key pair
	char user_key[1024] = { 0 };
	if (isSetUser)
		sprintf(user_key, "%s:%s", username, password);

	if (curl) {
		/* Get a file listing from sunet */
		curl_easy_setopt(curl, CURLOPT_URL, ftp_addr);
		//curl_easy_setopt(curl, CURLOPT_URL, "ftp://ftp.openssl.org/source/");
		if (isSetUser)
			curl_easy_setopt(curl, CURLOPT_USERPWD, user_key); // set user:password
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, ftpfile);
		/* If you intend to use this on windows with a libcurl DLL, you must use
		CURLOPT_WRITEFUNCTION as well */
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_response);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, respfile);

//		cout << "ftpGetDirInfo: before curl_easy_perform" << endl;
		res = curl_easy_perform(curl);
//		cout << "after " << endl;
		/* Check for errors */
		if (res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
			iret = -1;
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	else
	{
		iret = -1;
	}

	fclose(ftpfile); /* close the local file */
	fclose(respfile); /* close the response file */

	return iret;
}

int ftpDeleteFile(char* ftp_addr, char* filePath, const char *username, const char *password)
{
	int iret = 0;
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();

	// get user_key pair
	char user_key[1024] = { 0 };
	sprintf(user_key, "%s:%s", username, password);

	struct curl_slist* headerlist = NULL;
	char szComand[1024] = { 0 };
	sprintf(szComand, "DELE %s", filePath);
	headerlist = curl_slist_append(headerlist, szComand);

	if (curl) {
		/* Get a file listing from sunet */
		curl_easy_setopt(curl, CURLOPT_URL, ftp_addr);
		//curl_easy_setopt(curl, CURLOPT_PORT, 21);
		curl_easy_setopt(curl, CURLOPT_USERPWD, user_key);

		curl_easy_setopt(curl, CURLOPT_QUOTE, headerlist);

		curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);

		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
			iret = -1;
		}

		if (headerlist != NULL)
			curl_slist_free_all(headerlist); //free the list again

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	else
	{
		iret = -1;
	}

	return iret;
}


string urlEncode(const string& str)
{
	CURL *curl = curl_easy_init();
	string ret = str;
	if(curl) 
	{
	  char *output = curl_easy_escape(curl, str.c_str(), str.length());
	  if(output) 
	  {
	  	ret = output;
	  	//cout<<"before is "<<str<<endl<<"after is "<<ret<<endl;
		//printf("before is %s after is %s\n", str.c_str(),output);		
		curl_free(output);
	  }
	}
	curl_easy_cleanup(curl);
	return ret;
}

int write_string_to_file(const std::string & file_string, const std::string str)
{
	std::ofstream	OsWrite(file_string.c_str(), std::ofstream::out);
	OsWrite << str;
	OsWrite << std::endl;
	OsWrite.close();
	return 0;
}

// reply of the requery
#if 0
size_t req_reply(void *ptr, size_t size, size_t nmemb, void *stream)
{
	#if 0
	cout << "----->reply" << endl;
	string *str = (string*)stream;
	cout << *str << endl;
	(*str).append((char*)ptr, size*nmemb);
	return size * nmemb;
	#else
	long sizes = size * nmemb;
    string temp;
    temp = string((char*)data,sizes);
    content += temp;
    return sizes;
	#endif
}
#else
static size_t req_reply(void *data, size_t size, size_t nmemb, string &content)
{
    long sizes = size * nmemb;
    string temp;
    temp = string((char*)data,sizes);
    content += temp;
    return sizes;
}
#endif


// http GET
CURLcode curl_get_req(const std::string &url, std::string &response)
{
	// init curl
	CURL *curl = curl_easy_init();
	// res code
	CURLcode res;
	struct curl_slist *head = NULL;
	if (curl)
	{		
		#if 1
		head = curl_slist_append(head, "cache-control: no-cache");
		head = curl_slist_append(head, "content-type: application/x-www-form-urlencoded;charset=UTF-8");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, head);
		#endif

		// set params
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // url
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false); // if want to use https
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false); // set peer and host verify false
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, req_reply);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_HEADER, 1);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3); // set transport and time out time
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
		//curl_easy_setopt(curl, CURLOPT_PROXYPORT, 8080L);
		// start req
		
		res = curl_easy_perform(curl);
	}
	// release curl
	if(head)
	{
		curl_slist_free_all(head);
	}
	curl_easy_cleanup(curl);
	return res;
}

// http POST
CURLcode curl_post_req(const string &url, const string &postParams, string &response)
{
	// init curl
	CURL *curl = curl_easy_init();
	// res code
	CURLcode res;
	if (curl)
	{
		// set params
		curl_easy_setopt(curl, CURLOPT_POST, 1); // post req
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // url
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postParams.c_str()); // params
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false); // if want to use https
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false); // set peer and host verify false
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, req_reply);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_HEADER, 1);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
		//curl_easy_setopt(curl, CURLOPT_PROXYPORT, 8080L);
		// start req
		res = curl_easy_perform(curl);
	}
	// release curl
	curl_easy_cleanup(curl);
	return res;
}


int process(string &str, map<string,string> &tmp)
{
	map<string,string>::iterator m_it=tmp.begin();
	#if 0
	for (;m_it!=tmp.end(); m_it++)
	{
		if(m_it!=tmp.begin())
		{
			str+="&";
		}
		str=str+m_it->first;
		str+="=";
		str+=m_it->second;
	}	
	#else
	string newStr = "";
	for (;m_it!=tmp.end(); m_it++)
	{
		if(m_it!=tmp.begin())
		{
			newStr+="&";
		}
		newStr=newStr+urlEncode(m_it->first);
		newStr+="=";
		newStr+=urlEncode(m_it->second);
	}	
	
	str = str + newStr;
	#endif

	string getResponseStr;
	auto res = curl_get_req(str, getResponseStr);
	if (res != CURLE_OK)
	{
		#ifdef _DEBUG
		cerr << "curl_easy_perform() failed: " + string(curl_easy_strerror(res)) << endl;
		#endif
		return res;
	}
	else
	{
		#ifdef _DEBUG
		//cout << getResponseStr << endl;	
		#endif
		return 0;
	}
}



void replacen(string& str)
{
	str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
	str.erase(std::remove(str.begin(), str.end(), '\t'), str.end());
	#if 0
	#ifdef _DEBUG
	cout << str << endl;
	#endif
	//str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
	int index = str.find(":",0);
	while(index != -1)
	{
		if (str[index - 1] == ' ' && str[index + 1] == ' ' && str[index - 2] == '\"' && str[index + 2] == '\"')
		{
			str.erase(index-1,1);
			str.erase(index,1);
		}

		index = str.find_first_of(" : ",index+1);
	}
	#endif
	#ifdef _DEBUG
	cout << str << endl;
	#endif
}
/*
void test(const string &str)
{
	Json::Value root;
	Json::Reader reader;
	bool parsingSuccessful = reader.parse(str, root);	
	if(parsingSuccessful)
	{
		//cout<<"parse json ok"<<endl;
		//cout<<"Md5 is "<<root["md5"].asString()<<endl;
	}
	else
	{
		cout<<"parse json fail"<<endl;
	}
}



map<string,string> packFileDownload(const string& machineNo,const string& path,const string & url,const string mdCode)
{
	//xzy
	map<string,string> ret;
	string tmp = "";
	Json::Value contentData;
	contentData["path"] = "program";
	contentData["downloadUrl"] = url + "/" + path;
	contentData["id"] = "id";
	contentData["md5"] = mdCode;
	contentData["content"] = "content";
	tmp = contentData.toStyledString();
	cout<<"download order data is "<<tmp<<endl;
	replacen(tmp);
	test(tmp);
	ret["orderData"] = tmp;
	
	ret["cmdId"] = "cmdId";
	ret["messageType"] = "10";
	ret["orderType"] = "91";
	ret["level"] = "7";
	ret["frequence"] = "1";
	ret["switchs"] = "on";
	ret["destination"] = machineNo;
	ret["dest"] = machineNo;
	ret["source"] = "192.168.1.100";

	return ret;
}


map<string,string>  packFileUpload(const string& machineNo,const string& path,const string & url)
{
	map<string,string> ret;
	string tmp = "";
//xzy
	Json::Value contentData;
	contentData["filePath"] = path;
	contentData["uploadUrl"] = url;
	tmp = contentData.toStyledString();
	cout<<"upload order data is "<<tmp<<endl;
	//replacen(tmp);
	ret["orderData"] = tmp;
	
	ret["cmdId"] = "cmdId";
	ret["messageType"] = "10";
	ret["orderType"] = "102";
	ret["level"] = "7";
	ret["frequence"] = "1";
	ret["switchs"] = "on";
	ret["destination"] = machineNo;
	ret["dest"] = machineNo;
	ret["source"] = "cloudMqtt.10_1_196_73";

	return ret;
}


map<string,string> packModifyPara(const string& machineNo,const string& paraName,const string & val)
{
	map<string,string> ret;
	string tmp = "";

	Json::Value contentData;
	//string key = paraName;
	//string keyval = val; 
	//key = urlEncode(key);
	//keyval = urlEncode(keyval);
	contentData[paraName] = val;
	ret["orderData"]=contentData.toStyledString();
	tmp = contentData.toStyledString();
	cout<<"modify para order data is "<<tmp<<endl;
	//replacen(tmp);
	ret["orderData"] = tmp;

	ret["cmdId"] = "cmdId";
	ret["messageType"] = "10";
	ret["orderType"] = "89";
	ret["level"] = "7";
	ret["frequence"] = "1";
	ret["switchs"] = "on";
	ret["destination"] = machineNo;
	ret["dest"] = machineNo;
	ret["source"] = "";

	return ret;
}
*/
string getLastMessage(const string& ip,const string& machineNo)
{
	string str = "http://" + ip + "/agentServer/verify/getLastMessage?machineNo=" + machineNo;
	return str;
}

int libInit()
{
	return curl_global_init(CURL_GLOBAL_ALL);
}

int uploadFile(char* cnc_addr, char* machineNo, char* filePath)
{
	//map<string, string> tmp = packFileUpload(machineNo, filePath, UPLOADURL);
	string ipAddr = cnc_addr;
	ipAddr = ipAddr + ":8080";
	string str = "http://" + ipAddr + "/agentServer/order/jsonOrder?";
/*
	if (process(str, tmp))
	{
		return -1;
	}
*/
	string file = GetFileName(filePath,"/");
	string tmpPath = file;
	ConvertUtf8ToGBK(tmpPath);
#ifdef _DEBUG
	cout<<"gbk is "<<tmpPath<<endl;
	cout<<"utf name is "<<file<<endl;
#endif

	string fileUrl = ipAddr + UPLOADFILEDIR + "/" + file;
	#ifdef _DEBUG
	cout<<"file url is "<<fileUrl<<endl;
	#endif
	if (waitForFile(fileUrl.c_str()))
	{
		return -1;
	}

	return getFile((char *)fileUrl.c_str(), (char *)tmpPath.c_str());
}

string& trim(string &s) 
{
    if (s.empty()) 
    {
        return s;
    }
    s.erase(0,s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);
    return s;
}

void ConvertUtf8ToGBK(string &strUtf8)
{
	//xzy: comment
    // int len=MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, NULL,0);
    // wchar_t * wszGBK = new wchar_t[len];
    // memset(wszGBK,0,len);
    // MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, wszGBK, len); 
 
    // len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
    // char *szGBK=new char[len + 1];
    // memset(szGBK, 0, len + 1);
    // WideCharToMultiByte (CP_ACP, 0, wszGBK, -1, szGBK, len, NULL,NULL);
 
    // strUtf8 = szGBK;
    // delete[] szGBK;
    // delete[] wszGBK;
}
/*
int downloadFile(char* cnc_addr, char* machineNo, char* filePath, char* path)
{
	string md5Code;
	string tmpPath = filePath;
	ConvertUtf8ToGBK(tmpPath);

	try
	{
		md5Code = getFileMD5(tmpPath);
		md5Code = trim(md5Code);
	}
	catch (runtime_error &err)
	{
		cout << err.what() << endl;
		return -1;
	}
	string fileName = GetFileName(filePath,"\\");

#ifdef _DEBUG
	cout<<"gbk is "<<tmpPath<<endl;
	cout<<"utf8 file name is "<<fileName<<endl;
#endif

	//map<string, string> tmp = packFileDownload(machineNo, fileName, DOWNLOADURL, md5Code);
	string ipAddr = cnc_addr;
	ipAddr = ipAddr + ":8080";
	string str = "http://" + ipAddr + "/agentServer/order/jsonOrder?";

	return sendFileByPost((char*)tmpPath.c_str(), (char *)(ipAddr + DOWNLOADFILEDIR).c_str(),(char*)fileName.c_str());
}
*/
/*
int modifyPara(char* cnc_addr, char* machineNo, char* paraName, char* paraValue)
{
	map<string, string> tmp = packModifyPara(machineNo, paraName, paraValue);
	string ipAddr = cnc_addr;
	ipAddr = ipAddr + ":8080";
	string str = "http://" + ipAddr + "/agentServer/order/jsonOrder?";
	//str = urlEncode(str);
	return process(str, tmp);
}
*/
void libCleanUp()
{
	curl_global_cleanup();
}


int sendFileByPost(char* file, char *url,char* utfName)
{
	int ret = 0;
	CURL *curl;

	CURLM *multi_handle;
	int still_running;

	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr = NULL;
	struct curl_slist *headerlist = NULL;
	static const char buf[] = "Expect:";
#if 0

	/* Fill in the file upload field. This makes libcurl load data from
	the given file name when curl_easy_perform() is called. */
	curl_formadd(&formpost,
		&lastptr,
		CURLFORM_COPYNAME, "file",
		CURLFORM_FILE, file,
		CURLFORM_END);

	/* Fill in the filename field */
	curl_formadd(&formpost,
		&lastptr,
		CURLFORM_COPYNAME, "filename",
		CURLFORM_COPYCONTENTS, file,
		CURLFORM_END);

	/* Fill in the submit field too, even if this is rarely needed */
	curl_formadd(&formpost,
		&lastptr,
		CURLFORM_COPYNAME, "submit",
		CURLFORM_COPYCONTENTS, "send",
		CURLFORM_END);
#else

	curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "reqformat", CURLFORM_PTRCONTENTS, "plain", CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "file", CURLFORM_FILE, file, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "fileName", CURLFORM_PTRCONTENTS, utfName, CURLFORM_END);

#endif

	curl = curl_easy_init();
	multi_handle = curl_multi_init();

	/* initalize custom header list (stating that Expect: 100-continue is not
	wanted */
	headerlist = curl_slist_append(headerlist, buf);
	if (curl && multi_handle) {

		/* what URL that receives this POST */
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
		//curl_easy_setopt(curl, CURLOPT_PROXYPORT, 8080L);

		curl_multi_add_handle(multi_handle, curl);

		curl_multi_perform(multi_handle, &still_running);

		do {
			struct timeval timeout;
			int rc; /* select() return code */

			fd_set fdread;
			fd_set fdwrite;
			fd_set fdexcep;
			int maxfd = -1;

			long curl_timeo = -1;

			FD_ZERO(&fdread);
			FD_ZERO(&fdwrite);
			FD_ZERO(&fdexcep);

			/* set a suitable timeout to play around with */
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;

			curl_multi_timeout(multi_handle, &curl_timeo);
			if (curl_timeo >= 0) {
				timeout.tv_sec = curl_timeo / 1000;
				if (timeout.tv_sec > 1)
					timeout.tv_sec = 1;
				else
					timeout.tv_usec = (curl_timeo % 1000) * 1000;
			}

			/* get file descriptors from the transfers */
			curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

			/* In a real-world program you OF COURSE check the return code of the
			function calls.  On success, the value of maxfd is guaranteed to be
			greater or equal than -1.  We call select(maxfd + 1, ...), specially in
			case of (maxfd == -1), we call select(0, ...), which is basically equal
			to sleep. */

			rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);

			switch (rc) {
			case -1:
				/* select error */
				ret = -1;
				break;
			case 0:
			default:
				/* timeout or readable/writable sockets */
				//printf("perform!\n");
				curl_multi_perform(multi_handle, &still_running);
				//printf("running: %d!\n", still_running);
				break;
			}
		} while (still_running);

		int http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		cout << "upload response code is " << http_code << endl;


		curl_multi_cleanup(multi_handle);

		/* always cleanup */
		curl_easy_cleanup(curl);

		/* then cleanup the formpost chain */
		curl_formfree(formpost);

		/* free slist */
		curl_slist_free_all(headerlist);

		if(http_code < 200 || http_code >= 300)
		{
			//if not returned 2xx, delete this file
			cout << "upload response not ok " << endl;
			return -1;
		}
	}
	return ret;
}

size_t my_write_func(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	if (ptr && stream)
	{
		return fwrite(ptr, size, nmemb, stream);
	}
	else
		return 0;
}

int my_progress_func(char *progress_data,
	double t, /* dltotal */
	double d, /* dlnow */
	double ultotal,
	double ulnow)
{
	printf("%s %g / %g (%g %%)\n", progress_data, d, t, d*100.0 / t);
	return 0;
}

int getFile(char* url, char* destFile)
{
	CURL *curl;
	CURLcode res = CURLE_OK;
	FILE *outfile;
	char *progress_data = "* ";

	if (NULL == url || NULL == destFile)
	{
		return -1;
	}

	int len = getDownloadFileLenth(url);
	cout << "file to be download size is " << len << endl;
	if(len <= 0) //url not valid
	{
		cout << "url not valid" << endl;
	    return -1;
	}

	outfile = fopen(destFile, "ab+");
	int filelen = 0;
	if(outfile)
	{
//		filelen = filelength(fileno(outfile));
		struct stat info;
		stat(destFile, &info);
		filelen = info.st_size;
	}

	if(filelen >= len)
	{
		cout << "file already downloaded" << endl;
		return 0;
	}

	string range = to_string(filelen) + "-";
	cout << "range is " << range << endl;
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, outfile);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_write_func);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, my_progress_func);
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, progress_data);
//check if need re-download

		curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());
		//curl_easy_setopt(curl, CURLOPT_PROXYPORT, 8080L);

		res = curl_easy_perform(curl);
		if(0 == res)
		{
			int http_code = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
			cout << "httpcode is " << http_code << endl;
			if(http_code < 200 || http_code >= 300)
			{
				//if not returned 2xx, delete this file
				cout << "http response not ok, remove file " << endl;
				fclose(outfile);
				remove(destFile);
				curl_easy_cleanup(curl);
				return -1;
			}
		}

		fclose(outfile);
		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	return res;
}

static size_t save_header(void *ptr, size_t size, size_t nmemb, void *data)
{
	return (size_t)(size * nmemb);
}

int getDownloadFileLenth(const char *url)
{
	double len = 0;
	CURL *handle = curl_easy_init();

	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_HEADER, 1);    //只要求header头
	curl_easy_setopt(handle, CURLOPT_NOBODY, 1);    //不需求body
	curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, save_header);
	//curl_easy_setopt(handle, CURLOPT_PROXYPORT, 8080L);

	if (CURLE_OK != curl_easy_perform(handle))
	{
		printf("curl_easy_getinfo failed!\n");
		return -1;
	}

	if (CURLE_OK != curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &len))
	{
		return -1;
		printf("curl_easy_getinfo failed!\n");
	}

	return (int)len;
}

int waitForFile(const char *url)
{
	double totalLenth = 0;
	double currLenth = 0;

	while (1)
	{
		sleep(1);

		currLenth = getDownloadFileLenth((url));
		if (currLenth == -1)
		{
			#ifdef _DEBUG
			cout<<"file size is "<<currLenth<<endl;
			#endif
			return -1;
		}

		if (currLenth != totalLenth)
		{
			totalLenth = currLenth;
		}
		else
		{
			break;
		}
	}

	cout << "file total lenth is :" << totalLenth << endl;
	return 0;
}

void Split(const string &str, const string &delimiter, vector<string> &dest)
{
	size_t pos = 0, found = 0;
	while (found != string::npos)
	{
		found = str.find(delimiter, pos);
		dest.push_back(string(str, pos, found - pos));
		pos = found + 1;
	}
}

string GetFileName(const char *fileFullPath,char* flag)
{
	string fileName;
	vector<string> tmpVec;

	Split(fileFullPath, flag, tmpVec);
	int size = tmpVec.size();
	if (size)
	{
		fileName = tmpVec[size - 1];
	}

	return fileName;
}

static size_t process_data(void *data, size_t size, size_t nmemb, string &content)
{
    long sizes = size * nmemb;
    string temp;
    temp = string((char*)data,sizes);
    content += temp;
    return sizes;
}


int processFunc(string &str, map<string,string> &tmp)
{
	int ret = 0;
	map<string,string>::iterator m_it=tmp.begin();

	string newStr = "";
	for (;m_it!=tmp.end(); m_it++)
	{
		if(m_it!=tmp.begin())
		{
			newStr+="&";
		}
		newStr=newStr+urlEncode(m_it->first);
		newStr+="=";
		newStr+=urlEncode(m_it->second);
	}	
	str = str + newStr;

	string getResponseStr;
	auto res = curl_get_req(str, getResponseStr);
	if (res != CURLE_OK)
	{
		#ifdef _DEBUG
		cerr << "curl_easy_perform() failed: " + string(curl_easy_strerror(res)) << endl;
		#endif
		ret = -1;
	}
	else
	{
		#ifdef _DEBUG
		cout <<" response is "<<getResponseStr << endl;	
		#endif
		/*strncmp(getResponseStr.c_str(),"success",7) == 0*/
		if(getResponseStr.find("success")!=string::npos)
		{
			ret = 0;
		} 
		else 
		{
			ret = -1;
		}
	}
	return ret;
}



map<string,string>  packCheckFile(const string& md5Code,const string& fileName)
{
	map<string,string> ret;
	ret["fileName"] = fileName;
	ret["md5"] = md5Code;
	return ret;
}

/*
int checkFile(char* cnc_addr, char* machineNo, char* filePath)
{
	string md5Code;
	string tmpPath = filePath;
	ConvertUtf8ToGBK(tmpPath);

	try
	{
		md5Code = getFileMD5(tmpPath);
		md5Code = trim(md5Code);
	}
	catch (runtime_error &err)
	{
		cout << err.what() << endl;
		return -1;
	}
	string fileName = GetFileName(filePath,"\\");

#ifdef _DEBUG
	cout<<"gbk is "<<tmpPath<<endl;
	cout<<"utf8 file name is "<<fileName<<endl;
	cout<<"md5 code is "<<md5Code<<endl;
#endif

	//map<string, string> tmp = packFileDownload(machineNo, fileName, DOWNLOADURL, md5Code);
	string ipAddr = cnc_addr;
	ipAddr = ipAddr + ":8080";
	string str = "http://" + ipAddr + CHECKFILEURL + "?";

	map<string,string> mapVal = packCheckFile(md5Code,fileName);

	return processFunc(str, mapVal);
}
*/
int deleteFile(char* cnc_addr, char* machineNo, char* filePath)
{
	string md5Code;
	string tmpPath = filePath;

	string fileName = GetFileName(filePath,"\\");

	string ipAddr = cnc_addr;
	ipAddr = ipAddr + ":8080";
	string str = "http://" + ipAddr + DELETEFILEURL + "?";

	map<string,string> mapVal;
	mapVal["fileName"] = fileName;

	return processFunc(str, mapVal);
}
/*
map<string,string> packGetPara(const string& machineNo,const string& paraName)
{
	map<string,string> ret;
	string tmp = "";

	Json::Value contentData;

	contentData["paramNames"] = paraName;
	ret["orderData"]=contentData.toStyledString();
	tmp = contentData.toStyledString();

	ret["orderData"] = tmp;

	ret["cmdId"] = "cmdId";
	ret["messageType"] = "10";
	ret["orderType"] = "105";
	ret["level"] = "7";
	ret["frequence"] = "1";
	ret["switchs"] = "on";
	ret["destination"] = machineNo;
	ret["dest"] = machineNo;
	ret["source"] = "";

	return ret;
}
*/
/*
int getPara(char* cnc_addr, char* machineNo, char* paraName)
{
	map<string, string> tmp = packGetPara(machineNo, paraName);
	string ipAddr = cnc_addr;
	ipAddr = ipAddr + ":8080";
	string str = "http://" + ipAddr + "/agentServer/order/jsonOrder?";
	//str = urlEncode(str);
	int ret = process(str, tmp);
	return ret;
}
*/
