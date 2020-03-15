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
	DWORD exitcode;
	FILE* script;
	string op;
	string GnuFilePath;
	char* CurrentDirectory;
	Graficos();
	int fileExist(string const&);
public:
	DWORD written;
	static Graficos& GetInstanceGNUPlot();
	bool StartGNUPlotProgram();
	bool StartGNUPlotProgram(string const&);                      //Must be called first   
	inline bool CmdLine(string const&);
	inline int SetGnuFilePath(string const&);
	inline void FinishGNUPlotProgram();
	inline bool IsGNUPlotRunning();
	bool GNUScript(string const&);

	~Graficos();


};
inline int Graficos::SetGnuFilePath(string const &pf) {
	if (pf.empty()) {
		return -1;
	}
	if (fileExist(pf)==-2) {
		return -2;
	}

	GnuFilePath = pf;
	return 0;
}

inline bool Graficos::IsGNUPlotRunning() {
	if (GetExitCodeProcess(pi.hProcess, &exitcode)) {
		if (exitcode == STILL_ACTIVE) {
			return true;
		}
	}
	return false;

}
inline int Graficos::fileExist(string const& path) {
	if (!fopen_s(&script, path.c_str(), "r")) {		//Success
		fclose(script);
		return 1;
	}
	switch (errno) {
	case EACCES:
		return -1;		//it might exists but it can't be opened
	case ENOENT:
		return -2;		//no file found
	}
	return false;
}
inline bool Graficos::CmdLine(string const& strcmd) {
	if (strcmd.empty()) {
		return false;
	}
	if (!WriteFile(hWritePipe, strcmd.c_str(), strcmd.size(), &written, 0)) {
		return false;
	}
	
	return true;
}



inline void Graficos::FinishGNUPlotProgram() {
	TerminateThread(pi.hThread, 0);
	TerminateProcess(pi.hProcess, 0);
	CloseHandle(hWritePipe);
	CloseHandle(hReadPipe);

}