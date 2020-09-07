
#include "Graficos.h"

int Graficos::GNUScript(std::string &str) {
	if(fileExist(str)<0 || !IsGNUPlotRunning()) {
		return false;
	}
	std::string res = "load '" + str + "'\n";
	return CmdLine(res);
}
std::string Graficos::GetGnuFilePath() const {
	return GnuFilePath;
}
bool Graficos::StartGNUPlotProgram(std::string &strcmd) {
	GnuFilePath.append(strcmd);
	return StartGNUPlotProgram();
}
bool Graficos::StartGNUPlotProgram() {
	if (!CreatePipe(&hReadPipe, &hWritePipe, &sat, 0)) {
		return false;
	}
	st.hStdError = hWritePipe;
	st.hStdOutput = hWritePipe;
	st.hStdInput = hReadPipe;
	st.dwFlags = STARTF_USESTDHANDLES;
	if (!CreateProcessA(GnuFilePath.c_str(), 0, 0, 0, TRUE, CREATE_NO_WINDOW, 0, CurrentDirectory, &st, &pi)) {
		return false;
	}
	startgnu = TRUE;
	return true;
}


Graficos::Graficos() :startgnu{ FALSE } {
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&st, sizeof(STARTUPINFO));
	ZeroMemory(&sat, sizeof(SECURITY_ATTRIBUTES));
	sat.bInheritHandle = TRUE;
	sat.lpSecurityDescriptor = NULL;
	sat.nLength = sizeof(SECURITY_ATTRIBUTES);
	CurrentDirectory  = NULL;
	_dupenv_s(&CurrentDirectory, NULL, "PROGRAMFILES");
	if (CurrentDirectory) {
		GnuFilePath.append(CurrentDirectory);
		GnuFilePath.append("\\gnuplot\\bin\\gnuplot.exe");	//Default installation path
	}
	free(CurrentDirectory);
	CurrentDirectory = (char*)malloc(sizeof(char) * MAX_PATH);
	GetCurrentDirectoryA(MAX_PATH, CurrentDirectory);
}

int Graficos::SetGnuFilePath(std::string pf) {
	if (pf.empty()) {
		return -1;
	}
	if (fileExist(pf) != 1) {
		return -2;
	}

	GnuFilePath = pf;
	return 1;
}

bool Graficos::IsGNUPlotRunning() {
	return startgnu;
}
int Graficos::fileExist(std::string &path) {
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
	return 0;
}
int Graficos::CmdLine(std::string strcmd) {
	if (strcmd.empty()) {
		return 0;
	}
	if (!WriteFile(hWritePipe, strcmd.c_str(), strcmd.size(), &written, 0)) {
		return 0;
	}

	return written;
}

void Graficos::FinishGNUPlotProgram() {
	if (startgnu) {
		TerminateThread(pi.hThread, 0);
		TerminateProcess(pi.hProcess, 0);
		CloseHandle(hWritePipe);
		CloseHandle(hReadPipe);
	}
	startgnu = FALSE;
}
Graficos::~Graficos() {
	free(CurrentDirectory);
}
