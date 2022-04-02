// ConsoleApplication1.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "logger.h"

#pragma comment(lib,"commonlog.lib")

using namespace LOGGER;
CLogger logger(LogLevel_Info, CLogger::GetAppPathA().append("log\\"),false);
int main()
{
    std::cout << "Hello World!\n";
	logger.TraceFatal("TraceFatal汉字如何 %d", 1);
	logger.TraceError("RraceError %s", "sun不知道");
	logger.TraceWarning("Warning");
	logger.TraceInfo("Info");
	logger.ChangeLogLevel(LOGGER::LogLevel_Error);
	logger.TraceFatal("TraceFatal %d", 2);
	logger.TraceError("RraceError %s", "sunny汉字可以");
	logger.TraceWarning("Warn汉字如何ing");
	logger.TraceInfo("Info");
	system("pause");
}

