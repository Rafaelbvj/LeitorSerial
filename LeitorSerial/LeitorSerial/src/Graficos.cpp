
#include "Graficos.h"
Graficos* Graficos::gr = 0;
Graficos& Graficos::GetInstanceGNUPlot() {
	if (gr == 0) {
		gr = new Graficos;
	}
	return *gr;
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
	if (!CreateProcessA(GnuFilePath.c_str(),0,0,0,TRUE,NORMAL_PRIORITY_CLASS|CREATE_NO_WINDOW,0,0,&st,&pi)) {
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
	if (!CreateProcessA(GnuFilePath.c_str(), 0,0, 0, TRUE, NORMAL_PRIORITY_CLASS|CREATE_NO_WINDOW  , 0, 0, &st, &pi)) {
		return false;
	}
	return true;
}

 
int Graficos::SetScriptFile(string const& path) {
	char* f = NULL;
	long size;
	if (fileExist(path)) {
		fopen_s(&script, path.c_str(), "rb");
		if (script == NULL){
			return -2;
		}
		fseek(script, 0, SEEK_END);
		size = ftell(script);
		rewind(script);
		
		f = (char*)malloc(sizeof(char) * size);
		if (f) {
			if (fread(f, sizeof(char), size, script) == size) {
				scriptcode = f;
				fclose(script);
				free(f);
				return size;
			}
		}
	}
	free(f);
	return -1;

}

Graficos::Graficos() {
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&st, sizeof(STARTUPINFO));
	ZeroMemory(&sat, sizeof(SECURITY_ATTRIBUTES));
	sat.bInheritHandle = TRUE;
	sat.lpSecurityDescriptor = NULL;
	sat.nLength = sizeof(SECURITY_ATTRIBUTES);
	char* pf = NULL;

	_dupenv_s(&pf, NULL, "PROGRAMFILES(X86)");
 
	if (pf) {
		GnuFilePath.append(pf);
		GnuFilePath.append("\\gnuplot\\bin\\gnuplot.exe");
	}
	free(pf);
}
Graficos::~Graficos() {
	
}
