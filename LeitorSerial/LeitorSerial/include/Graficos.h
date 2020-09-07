#pragma once
#include <Windows.h>
#include <string>

class Graficos {
private:
	static Graficos* gr;
	PROCESS_INFORMATION pi;
	STARTUPINFOA st;
	SECURITY_ATTRIBUTES sat;
	HANDLE hWritePipe, hReadPipe;
	DWORD written;
	BOOL startgnu;
	FILE* script;
	std::string op;
	std::string GnuFilePath;
	char* CurrentDirectory;
	int fileExist(std::string&);
public:
	Graficos();
	bool StartGNUPlotProgram();
	bool StartGNUPlotProgram(std::string&);                      //Must be called first   
	int CmdLine(std::string);
	int SetGnuFilePath(std::string);
	std::string GetGnuFilePath() const;
	void FinishGNUPlotProgram();
	bool IsGNUPlotRunning();
	int GNUScript(std::string&);
	~Graficos();

};
