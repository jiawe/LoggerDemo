// ConsoleApplication1.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "logger.h"

#pragma comment(lib,"commonlog.lib")

using namespace LOGGER;
//CLogger logger(LogLevel_Info, CLogger::GetAppPathA().append("log\\"),false);
int main()
{
	std::cout << "begin!\n";
	LOGGER::CLogger::getInstance().TraceInfo("Hello");
	LOGGER::CLogger::getInstance().TraceInfo("test %d",1586);
	CLogger logger(LogLevel_Info, CLogger::GetAppPathA().append("log_main_test/"),false);
	logger.InsertVerInfo();
	logger.TraceFatal("TraceFatal汉字如何 %d", 1);
	logger.TraceError("RraceError %s %d", "sun不知道",66);
	logger.TraceWarning("Warning %d %s",8,"好吧");
	logger.TraceInfo("Info");
	logger.ChangeLogLevel(LOGGER::LogLevel_Error);
	logger.TraceFatal("TraceFatal %d", 2);
	logger.TraceError("RraceError %s", "sun2汉字如何");
	logger.TraceWarning("Warn汉字如何ing");
	logger.TraceInfo("Info");
	getchar();
}

