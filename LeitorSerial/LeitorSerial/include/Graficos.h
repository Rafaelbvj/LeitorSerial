#pragma once
#include <Windows.h>
#include <string>

using namespace std;

class Graficos {
private:
    static Graficos *gr;
    PROCESS_INFORMATION pi;
    STARTUPINFOA st;
    SECURITY_ATTRIBUTES sat;
    HANDLE hWritePipe, hReadPipe;
    DWORD exitcode;
    FILE* script;
    string GnuFilePath;
    string scriptcode;
    Graficos();
    bool fileExist(string const&);
public:
    DWORD written;
    static Graficos& GetInstanceGNUPlot();
    bool StartGNUPlotProgram();
    bool StartGNUPlotProgram(string const&);                      //Must be called first
    int SetScriptFile(string const&);   
    inline int CmdLine(string const&);
    inline int SetGnuFilePath(string const&);
    inline void FinishGNUPlotProgram();
    inline bool IsGNUPlotRunning();
 
    ~Graficos();
    
    
};
inline int Graficos::SetGnuFilePath(string const& pf) {
    if (pf.empty()) {
        return -1;
    }
    if (!fileExist(pf)) {
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
inline bool Graficos::fileExist(string const& path) {
    if (!fopen_s(&script, path.c_str(), "r")) {		//Success
        fclose(script);
        return true;
    }
    return false;
}
inline int Graficos::CmdLine(string const& strcmd) {
    if (strcmd.empty()) {
        return -1;
    }
    if (!WriteFile(hWritePipe, strcmd.c_str(), strcmd.size(), &written, 0)) {
        return -2;
    }
    return 0;
}
 
 
inline void Graficos::FinishGNUPlotProgram() {
    TerminateThread(pi.hThread, 0);
    TerminateProcess(pi.hProcess,  0);
    CloseHandle(hWritePipe);
    CloseHandle(hReadPipe);
 
}