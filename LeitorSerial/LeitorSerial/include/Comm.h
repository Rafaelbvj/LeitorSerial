#pragma once
#include <Windows.h>
#include <vector>
#include <string>
using namespace std;
typedef struct dataconf {
	int segs;				//Sampling duration
	int delaytime;			//Delay for each reading
	int prec;				//Precision
	int ganho;				//Gain
	char *localfile;		//SD file local
} DataConf;
typedef struct data {
	unsigned char signbegin[4];
	int dt;
	unsigned char signend[4];
}DataProtocol;
typedef struct commports{
	vector <string> cm;
	int nCursel{ 0 };
	string portConnected;
	DCB dcb{ 0 };
	COMMTIMEOUTS ct{ 0 };
}COMMPORTS;
int AddPortsNametoCB(COMMPORTS *,HWND);
