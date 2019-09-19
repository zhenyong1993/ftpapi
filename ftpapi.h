#ifndef FTPAPI_H_
#define FTPAPI_H_

#include <string>
#include <vector>
#include <map>
using namespace std;

#define	ADAPTER_OK	0

#ifdef __cplusplus
extern "C"  //C++
{
#endif

	int _sendFileToCNC(void *handle,int type,const string&fileName,const string& content);
   
	int _getFileFromCNC(void *handle,int type,const     string&fileName,string& content);

	int _listDir(void *handle,const   string&dir,string& content);

	int _deleteNcFile(void *handle,const string& programName);

	void _setAddr(const string& ip, const string& user, const string& pwd);


#ifdef __cplusplus
}
#endif
 
#endif /* FTPAPI_H_ */
