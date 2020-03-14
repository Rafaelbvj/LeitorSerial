#pragma once
#include <Windows.h>
#include <vector>
#include <string>
using namespace std;
typedef struct dataconf {
	unsigned int msegs;					//Sampling duration
	unsigned int prec;					//Precision
	unsigned int ganho;					//Gain
	char localfile[100];		//SD file local
} DataConf;
typedef struct data {
	unsigned char signbegin[4];	//3 bytes for padding memory
	int dt;						//Digital signal from H7X11
	unsigned int mtime;			//Time for each step on arduino's loop		
	unsigned char signend[4];	//3 bytes for padding memory
}DataProtocol;
typedef struct commports{
	vector <string> cm;
	int nCursel{ 0 };
	string portConnected;
	DCB dcb{ 0 };
	COMMTIMEOUTS ct{ 0 };
}COMMPORTS;
int AddPortsNametoCB(COMMPORTS *,HWND);
