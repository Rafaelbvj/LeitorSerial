
#include "Graficos.h"
Graficos* Graficos::gr = 0;
Graficos& Graficos::GetInstanceGNUPlot() {
	if (gr == 0) {
		gr = new Graficos;
	}
	return *gr;
}

bool Graficos::GNUScript(string const& str) {
	if(fileExist(str)<0 || !IsGNUPlotRunning()) {
		return false;
	}
	return CmdLine("load '"+str+"'\n");
}
bool Graficos::StartGNUPlotProgram(string const& strcmd) {
	GnuFilePath.append(strcmd);

	if (!CreatePipe(&hReadPipe, &hWritePipe, &sat, 0)) {

		return false;
	}
	st.hStdError = hWritePipe;
	st.hStdOutput = hWritePipe;
	st.hStdInput = hReadPipe;
	st.dwFlags = STARTF_USESTDHANDLES;
	if (!CreateProcessA(GnuFilePath.c_str(), 0, 0, 0, TRUE, CREATE_NO_WINDOW, 0, 0, &st, &pi)) {
		return false;
	}

	return true;
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
	return true;
}


Graficos::Graficos() {
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
Graficos::~Graficos() {
	free(CurrentDirectory);
}
