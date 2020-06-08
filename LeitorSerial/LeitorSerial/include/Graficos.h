#pragma once
#include <Windows.h>
#include <string>
using namespace std;

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
	string op;
	string GnuFilePath;
	char* CurrentDirectory;
	int fileExist(string );
public:
	Graficos();
	bool StartGNUPlotProgram();
	bool StartGNUPlotProgram(string);                      //Must be called first   
	int CmdLine(string);
	int SetGnuFilePath(string);
	void FinishGNUPlotProgram();
	bool IsGNUPlotRunning();
	int GNUScript(string);
	~Graficos();

};
