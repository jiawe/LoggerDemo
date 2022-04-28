//logger.cpp
#define _CRT_SECURE_NO_WARNINGS
#include "logger.h"
#include <time.h>
#include <stdarg.h>
#include <string>
#include <cstdlib>
#include <cstdarg>
#include <vector>
#include <memory>
#include <iostream>
#include <exception>


#include "RC4.h"
#include "base64.h"

#ifdef __linux__
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>
#include<unistd.h>
#else
#pragma comment(lib,"Dbghelp.lib")
#endif

using std::string;
using std::vector;
using std::wstring;

string ws2s(const wstring& w_str)
{
	if (w_str.empty()) {
		return "";
		}
		unsigned len = w_str.size() * 4 + 1;
		setlocale(LC_CTYPE, "en_US.UTF-8");
		std::unique_ptr<char[]> p(new char[len]);
		wcstombs(p.get(), w_str.c_str(), len);
		std::string str(p.get());
	 return str;
}

#ifdef __linux__
	
#else
typedef struct FILEPROP{
	std::string filename;
    FILETIME filetm;
}FILEPROP;
#endif


int DeleteOldFile(const char* path)      //log文件不超过x个， 超过删除旧的
{
	char    cDir[256]  = {0};
	#ifdef __linux__
		vector<struct dirent*> fileList;	
		struct dirent* de;
		DIR*   dir = opendir(path);
		while ((de = readdir(dir)) != 0) {
				if ('.' == de->d_name[0]) {
					if (0 == de->d_name[1]) continue; // .
					if (('.' == de->d_name[1]) && (0 == de->d_name[2])) continue; // ..
				}
			//struct dirent item;
			if(strlen(de->d_name)>8)
			fileList.push_back(de);
		}
		if(fileList.size()>(LOGGER::logcount-1))
		{
			for(vector<struct dirent*>::iterator it=fileList.begin();it!=fileList.end();it++)
			{
				if(fileList.size()-(LOGGER::logcount-1)>0)
				{
					remove(((string)path+(string)((*it)->d_name)).c_str());
					fileList.erase(it);
					break;
				}
			}
		}
	#else   
		vector<FILEPROP> fileList;
		HANDLE  hFind      = NULL;
		WIN32_FIND_DATAA wfd;
		sprintf(cDir, "%s*", path);

		hFind = FindFirstFileA(cDir, &wfd);
		if (hFind == NULL || hFind == INVALID_HANDLE_VALUE)
		{
			return -1;
		}
		while (true)
		{
			if (   wfd.dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE /*FILE_ATTRIBUTE_DIRECTORY*/
				&& strcmp(wfd.cFileName, ".") != 0
				&& strcmp(wfd.cFileName, "..") != 0)
			{
				FILEPROP item;
				item.filename =(wfd.cFileName);
				item.filetm=wfd.ftCreationTime;
				if(item.filename.length()>8)
					fileList.push_back(item);
			}

			if (FindNextFileA(hFind, &wfd) == FALSE)
			{
				break;
			}
		}
		FindClose(hFind);
		
		
		if(fileList.size()>(LOGGER::logcount-1))
		{
			for(vector<FILEPROP>::iterator it=fileList.begin();it!=fileList.end();it++)
			{
				if(fileList.size()-(LOGGER::logcount-1)>0)
				{
					remove(((string)path+(*it).filename).c_str());
					fileList.erase(it);
					break;
				}
			}
		}
	#endif
    return 0;
}


bool icompare_pred(unsigned char a, unsigned char b) {
	return std::tolower(a) == std::tolower(b);
}
bool EndsWith(const std::string& str, const std::string& suffix) {
 
	if (str.size() < suffix.size()) {
		return false;
	}
 
	std::string tstr = str.substr(str.size() - suffix.size());
 
	if (tstr.length() == suffix.length()) {
		return std::equal(suffix.begin(), suffix.end(), tstr.begin(), icompare_pred);
	} else {
		return false;
	}
}

namespace LOGGER
{
	CLogger::CLogger(EnumLogLevel nLogLevel, const std::string strLogPath, bool encrypt, const std::string strLogName)
		:m_nLogLevel(nLogLevel),
		m_strLogPath(strLogPath),
		m_strLogName(strLogName),
		m_isEncrypt(encrypt)
	{
		//初始化
		m_pFileStream = NULL;
		if (m_strLogPath.empty())
		{
			m_strLogPath = GetAppPathA();
		}
		if (!EndsWith(m_strLogPath,"\\")&&!EndsWith(m_strLogPath,"/"))//m_strLogPath[m_strLogPath.length()-1] != '\\'||m_strLogPath[m_strLogPath.length()-1] != '/')
		{
			#ifdef __linux__
				m_strLogPath.append("/");
			#else
				m_strLogPath.append("\\");
			#endif
			
		}
		//创建文件夹
		#ifdef __linux__

		int status =mkdir(m_strLogPath.c_str(),777);

		#else
		MakeSureDirectoryPathExists(m_strLogPath.c_str());
		#endif
		DeleteOldFile(m_strLogPath.c_str());

		//创建日志文件
		if (m_strLogName.empty())
		{
			time_t curTime;
			time(&curTime);
			tm *tm1;
			tm1=localtime(&curTime);
			std::string tmppath=m_strLogPath;
			std::string detaillog=FormatString("%02d_%02d-%02d",tm1->tm_hour, tm1->tm_mday, tm1->tm_mon + 1);
			#ifdef __linux__
			if(access(tmppath.append(detaillog).c_str(),0)!= -1)
			{
				m_nLogLevel= _EnumLogLevel::LogLevel_Info;
			}
			#else			
			if(_access(tmppath.append(detaillog).c_str(),0)!= -1)
			{
				m_nLogLevel= _EnumLogLevel::LogLevel_Info;
			}
			#endif
			//日志的名称如：201601012130.log
			m_strLogName = FormatString("%04d%02d%02d_%02d%02d%02d.log", tm1->tm_year + 1900, tm1->tm_mon + 1, tm1->tm_mday, tm1->tm_hour, tm1->tm_min, tm1->tm_sec);
		}
		m_strLogFilePath = m_strLogPath.append(m_strLogName);		
	#ifdef __linux__
		m_pFileStream=fopen64(m_strLogFilePath.c_str(), "a+");
		pthread_mutex_init(&m_cs,NULL);
	#else
		//以追加的方式打开文件流
		m_pFileStream=_fsopen(m_strLogFilePath.c_str(), "a+", 0x20);   // _SH_DENYWR     0x20    编译不通过可以改为数字
		InitializeCriticalSection(&m_cs);
	#endif
	}


	//析构函数
	CLogger::~CLogger()
	{
		//释放临界区
		#ifdef __linux__
		pthread_mutex_destroy(&m_cs);
		#else
		DeleteCriticalSection(&m_cs);
		#endif
		//关闭文件流
		if (m_pFileStream)
		{
			fclose(m_pFileStream);
			m_pFileStream = NULL;
		}
	}

	void CLogger::InsertVerInfo(const std::string moduleName)
	{
		TraceFatal("%s -- %s %s",moduleName.c_str(),(char*)__DATE__,(char*)__TIME__);
	}



	//写严重错误信息 char
	void CLogger::TraceFatal(const char *lpcszFormat, ...)
	{
		//判断当前的写日志级别
		if (EnumLogLevel::LogLevel_Fatal > m_nLogLevel)
			return;
		string strResult;
		if (NULL != lpcszFormat) {
			va_list marker;// =  {0};
			va_list arg_tmp;
			va_start(marker, lpcszFormat); // 初始化变量参数
			va_copy(arg_tmp,marker);			
			// 获取格式化字符串长度
			std::vector<char> vBuffer(vsnprintf(NULL, 0, lpcszFormat, marker) + 1); // 创建用于存储格式化字符串的字符数组
			va_end(marker);
			int nWritten = vsnprintf(vBuffer.data(), vBuffer.size(), lpcszFormat, arg_tmp);
			if (nWritten > 0) {
				strResult = vBuffer.data();
			}
			va_end(arg_tmp); // 重置变量参数
		}
		if (strResult.empty())
		{
			return;
		}
		string strLog = strFatalPrefix;
		strLog.append(GetTime()).append(strResult);

		//写日志文件
		Trace(strLog);
	}

    	//写严重错误信息  wchar
	void CLogger::TraceFatal(const wchar_t *lpcszFormat, ...)
	{
		//判断当前的写日志级别
		if (EnumLogLevel::LogLevel_Fatal > m_nLogLevel)
			return;
		wstring strResult;
		if (NULL != lpcszFormat)
		{
			va_list marker;// = {0};
			va_start(marker, lpcszFormat); //初始化变量参数
			#ifdef __linux__
			size_t nLength =vswprintf(NULL,0,lpcszFormat, marker) + 1; //获取格式化字符串长度
			std::vector<wchar_t> vBuffer(nLength, '\0'); //创建用于存储格式化字符串的字符数组
			int nWritten = vswprintf(&vBuffer[0],  nLength, lpcszFormat, marker);
			#else
			size_t nLength = _vsnwprintf(NULL,0,lpcszFormat, marker) + 1; //获取格式化字符串长度
			std::vector<wchar_t> vBuffer(nLength, '\0'); //创建用于存储格式化字符串的字符数组
			int nWritten = _vsnwprintf(&vBuffer[0],  nLength, lpcszFormat, marker);
			#endif
			if (nWritten > 0)
			{
				strResult = &vBuffer[0];
			}
			va_end(marker); //重置变量参数
		}
		if (strResult.empty())
		{
			return;
		}
		string strLog = strFatalPrefix;
		strLog.append(GetTime()).append(ws2s(strResult));

		//写日志文件
		Trace(strLog);
	}

	//写错误信息
	void CLogger::TraceError(const char *lpcszFormat, ...)
	{
		//判断当前的写日志级别
		if (EnumLogLevel::LogLevel_Error > m_nLogLevel)
			return;
		string strResult;
		if (NULL != lpcszFormat) {
			va_list marker;// =  {0};
			va_list arg_tmp;
			va_start(marker, lpcszFormat); // 初始化变量参数
			va_copy(arg_tmp,marker);			
			// 获取格式化字符串长度
			std::vector<char> vBuffer(vsnprintf(NULL, 0, lpcszFormat, marker) + 1); // 创建用于存储格式化字符串的字符数组
			va_end(marker);
			int nWritten = vsnprintf(vBuffer.data(), vBuffer.size(), lpcszFormat, arg_tmp);
			if (nWritten > 0) {
				strResult = vBuffer.data();
			}
			va_end(arg_tmp); // 重置变量参数
		}
		// if (NULL != lpcszFormat)
		// {
		// 	va_list marker;// = {0};
		// 	va_start(marker, lpcszFormat); //初始化变量参数
		// 	size_t nLength = vsnprintf(NULL,0,lpcszFormat, marker) + 1; //获取格式化字符串长度
		// 	std::vector<char> vBuffer(nLength, '\0'); //创建用于存储格式化字符串的字符数组
		// 	int nWritten = vsnprintf(&vBuffer[0],  nLength, lpcszFormat, marker);
		// 	if (nWritten > 0)
		// 	{
		// 		strResult = &vBuffer[0];
		// 	}
		// 	va_end(marker); //重置变量参数
		// }
		if (strResult.empty())
		{
			return;
		}
		string strLog = strErrorPrefix;
		strLog.append(GetTime()).append(strResult);

		//写日志文件
		Trace(strLog);
	}

    	//写错误信息     wchar
	void CLogger::TraceError(const wchar_t *lpcszFormat, ...)
	{
		//判断当前的写日志级别
		if (EnumLogLevel::LogLevel_Error > m_nLogLevel)
			return;
		wstring strResult;
		if (NULL != lpcszFormat)
		{
			va_list marker;// = {0};
			va_start(marker, lpcszFormat); //初始化变量参数
			#ifdef __linux__
			size_t nLength =vswprintf(NULL,0,lpcszFormat, marker) + 1; //获取格式化字符串长度
			std::vector<wchar_t> vBuffer(nLength, '\0'); //创建用于存储格式化字符串的字符数组
			int nWritten = vswprintf(&vBuffer[0],  nLength, lpcszFormat, marker);
			#else
			size_t nLength = _vsnwprintf(NULL,0,lpcszFormat, marker) + 1; //获取格式化字符串长度
			std::vector<wchar_t> vBuffer(nLength, '\0'); //创建用于存储格式化字符串的字符数组
			int nWritten = _vsnwprintf(&vBuffer[0],  nLength, lpcszFormat, marker);
			#endif
			if (nWritten > 0)
			{
				strResult = &vBuffer[0];
			}
			va_end(marker); //重置变量参数
		}
		if (strResult.empty())
		{
			return;
		}
		string strLog = strErrorPrefix;
		strLog.append(GetTime()).append(ws2s(strResult));

		//写日志文件
		Trace(strLog);
	}

	//写警告信息
	void CLogger::TraceWarning(const char *lpcszFormat, ...)
	{
		//判断当前的写日志级别
		if (EnumLogLevel::LogLevel_Warning > m_nLogLevel)
			return;
		string strResult;
		if (NULL != lpcszFormat) {
			va_list marker;// =  {0};
			va_list arg_tmp;
			va_start(marker, lpcszFormat); // 初始化变量参数
			va_copy(arg_tmp,marker);			
			// 获取格式化字符串长度
			std::vector<char> vBuffer(vsnprintf(NULL, 0, lpcszFormat, marker) + 1); // 创建用于存储格式化字符串的字符数组
			va_end(marker);
			int nWritten = vsnprintf(vBuffer.data(), vBuffer.size(), lpcszFormat, arg_tmp);
			if (nWritten > 0) {
				strResult = vBuffer.data();
			}
			va_end(arg_tmp); // 重置变量参数
		}
		if (strResult.empty())
		{
			return;
		}
		string strLog = strWarningPrefix;
		strLog.append(GetTime()).append(strResult);

		//写日志文件
		Trace(strLog);
	}

	//写警告信息
	void CLogger::TraceWarning(const wchar_t *lpcszFormat, ...)
	{
		//判断当前的写日志级别
		if (EnumLogLevel::LogLevel_Warning > m_nLogLevel)
			return;
		wstring strResult;
		if (NULL != lpcszFormat)
		{
			va_list marker;// = {0};
			va_start(marker, lpcszFormat); //初始化变量参数
			#ifdef __linux__
			size_t nLength =vswprintf(NULL,0,lpcszFormat, marker) + 1; //获取格式化字符串长度
			std::vector<wchar_t> vBuffer(nLength, '\0'); //创建用于存储格式化字符串的字符数组
			int nWritten = vswprintf(&vBuffer[0],  nLength, lpcszFormat, marker);
			#else
			size_t nLength = _vsnwprintf(NULL,0,lpcszFormat, marker) + 1; //获取格式化字符串长度
			std::vector<wchar_t> vBuffer(nLength, '\0'); //创建用于存储格式化字符串的字符数组
			int nWritten = _vsnwprintf(&vBuffer[0],  nLength, lpcszFormat, marker);
			#endif
			if (nWritten > 0)
			{
				strResult = &vBuffer[0];
			}
			va_end(marker); //重置变量参数
		}
		if (strResult.empty())
		{
			return;
		}
		string strLog = strWarningPrefix;
		strLog.append(GetTime()).append(ws2s(strResult));

		//写日志文件
		Trace(strLog);
	}

	//写一般信息
	void CLogger::TraceInfo(const char *lpcszFormat, ...)
	{
		//判断当前的写日志级别
		if (EnumLogLevel::LogLevel_Info > m_nLogLevel)
			return;
		string strResult;
		if (NULL != lpcszFormat) {
			va_list marker;// =  {0};
			va_list arg_tmp;
			va_start(marker, lpcszFormat); // 初始化变量参数
			va_copy(arg_tmp,marker);			
			// 获取格式化字符串长度
			std::vector<char> vBuffer(vsnprintf(NULL, 0, lpcszFormat, marker) + 1); // 创建用于存储格式化字符串的字符数组
			va_end(marker);
			int nWritten = vsnprintf(vBuffer.data(), vBuffer.size(), lpcszFormat, arg_tmp);
			if (nWritten > 0) {
				strResult = vBuffer.data();
			}
			va_end(arg_tmp); // 重置变量参数
		}
		if (strResult.empty())
		{
			return;
		}
		string strLog = strInfoPrefix;
		strLog.append(GetTime()).append(strResult);

		//写日志文件
		Trace(strLog);
	}

    	//写一般信息
	void CLogger::TraceInfo(const wchar_t *lpcszFormat, ...)
	{
		//判断当前的写日志级别
		if (EnumLogLevel::LogLevel_Info > m_nLogLevel)
			return;
		wstring strResult;
		if (NULL != lpcszFormat)
		{
			va_list marker;// =  {0};
			va_start(marker, lpcszFormat); //初始化变量参数
			#ifdef __linux__
			size_t nLength =vswprintf(NULL,0,lpcszFormat, marker) + 1; //获取格式化字符串长度
			std::vector<wchar_t> vBuffer(nLength, '\0'); //创建用于存储格式化字符串的字符数组
			int nWritten = vswprintf(&vBuffer[0],  nLength, lpcszFormat, marker);
			#else
			size_t nLength = _vsnwprintf(NULL,0,lpcszFormat, marker) + 1; //获取格式化字符串长度
			std::vector<wchar_t> vBuffer(nLength, '\0'); //创建用于存储格式化字符串的字符数组
			int nWritten = _vsnwprintf(&vBuffer[0],  nLength, lpcszFormat, marker);
			#endif
			if (nWritten > 0)
			{
				strResult = &vBuffer[0];
			}
			va_end(marker); //重置变量参数
		}
		if (strResult.empty())
		{
			return;
		}
		string strLog = strInfoPrefix;
		strLog.append(GetTime()).append(ws2s(strResult));

		//写日志文件
		Trace(strLog);
	}


	//获取系统当前时间
	string CLogger::GetTime()
	{
		time_t curTime;
		time(&curTime);
		tm *tm1;
		tm1=localtime(&curTime);
		//2016-01-01 21:30:00
		string strTime = FormatString("%04d-%02d-%02d %02d:%02d:%02d ", tm1->tm_year + 1900, tm1->tm_mon + 1, tm1->tm_mday, tm1->tm_hour, tm1->tm_min, tm1->tm_sec);

		return strTime;
	}

	//改变写日志级别
	void CLogger::ChangeLogLevel(EnumLogLevel nLevel)
	{
		m_nLogLevel = nLevel;
	}

	//写文件操作
	void CLogger::Trace(const string &strLog)
	{
		try
		{

			//进入临界区
			#ifdef __linux__
				pthread_mutex_lock(&m_cs);
			#else
				EnterCriticalSection(&m_cs);
			#endif
			//若文件流没有打开，则重新打开
			if (NULL == m_pFileStream)
			{
				m_pFileStream=fopen(m_strLogFilePath.c_str(), "a+");
				if (!m_pFileStream)
				{
					return;
				}
			}
			if(m_isEncrypt){
				unsigned char key[18] = { 0x01, 0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x66,0x88,0x99 };
				RC4_KEY rc4_key;
				RC4_key(&rc4_key, key, sizeof(key));
				unsigned char* encodeRc4LogOut=new unsigned char[strLog.size()];
				RC4(&rc4_key, (unsigned char*)strLog.c_str(), strLog.size(),encodeRc4LogOut);
				unsigned char* encodeLog=new unsigned char[BASE64_ENCODE_OUT_SIZE(strLog.size())];
				base64_encode(encodeRc4LogOut,strLog.size(),encodeLog);
				fprintf(m_pFileStream, "%s\n", encodeLog);
				delete [] encodeRc4LogOut;
				delete [] encodeLog;
			}
			else
			{
				fprintf(m_pFileStream, "%s\n", strLog.c_str());
				fflush(m_pFileStream);
			}
			//离开临界区
			#ifdef __linux__
				pthread_mutex_unlock(&m_cs);
			#else
				LeaveCriticalSection(&m_cs);
			#endif

		}
		//若发生异常，则先离开临界区，防止死锁
		catch (...)
		{
			#ifdef __linux__
				pthread_mutex_unlock(&m_cs);
			#else
				LeaveCriticalSection(&m_cs);
			#endif
		}
	}

	string CLogger::GetAppPathA()
	{
		#ifdef __linux__
		char path[500]={0};
		getcwd(path,500);
		string str(path);
		str+="/";

		#else
		char szFilePath[MAX_PATH] = { 0 }, szDrive[MAX_PATH] = { 0 }, szDir[MAX_PATH] = { 0 }, szFileName[MAX_PATH] = { 0 }, szExt[MAX_PATH] = { 0 };
		GetModuleFileNameA(NULL, szFilePath, sizeof(szFilePath));
		_splitpath(szFilePath, szDrive, szDir, szFileName, szExt);

		string str(szDrive);
		str.append(szDir);
		#endif
		return str;
	}

	string CLogger::FormatString(const char *lpcszFormat, ...) {
		string strResult;
		if (NULL != lpcszFormat) {
			va_list marker;// =  {0};
			va_list arg_tmp;
			va_start(marker, lpcszFormat); // 初始化变量参数
			va_copy(arg_tmp,marker);			
			// 获取格式化字符串长度
			std::vector<char> vBuffer(vsnprintf(NULL, 0, lpcszFormat, marker) + 1); // 创建用于存储格式化字符串的字符数组
			va_end(marker);
			int nWritten = vsnprintf(vBuffer.data(), vBuffer.size(), lpcszFormat, arg_tmp);
			if (nWritten > 0) {
				strResult = vBuffer.data();
			}
			va_end(arg_tmp); // 重置变量参数
		}
		return strResult;
	}
}
