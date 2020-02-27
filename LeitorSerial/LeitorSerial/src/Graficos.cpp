
#include "Graficos.h"
Graficos* Graficos::gr = 0;
Graficos& Graficos::GetInstanceGNUPlot() {
	if (gr == 0) {
		gr = new Graficos;
	}
	return *gr;
}
bool Graficos::LoadGNUScript(const char* path)
{
	fopen_s(&script, path, "rb");
	if (script == NULL) {
		return false;
	}
	fseek(script, 0, SEEK_END);
	scriptcode.size = ftell(script);
	if (scriptcode.size < 1) {			//No data or error
		return false;
	}
	scriptcode.data = (char*)malloc(sizeof(char) * scriptcode.size);
	rewind(script);
	do {
		fread(scriptcode.data, sizeof(char), scriptcode.size, script);
	} while (!feof(script)&&!ferror(script));
	fclose(script);
	return true;
}
bool Graficos::ExecuteGNUScript(bool keeponmem) {
	if(scriptcode.data == NULL || !IsGNUPlotRunning()) {
		return false;
	}
	if (!WriteFile(hWritePipe, scriptcode.data, scriptcode.size, &written, 0)) {
		return false;
	}
	if (!keeponmem) {
		free(scriptcode.data);
	}
	return true;
	
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
	ZeroMemory(&scriptcode, sizeof(FileRead));
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
