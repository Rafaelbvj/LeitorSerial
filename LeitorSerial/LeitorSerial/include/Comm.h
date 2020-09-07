#pragma once
#include <Windows.h>
#include <CommCtrl.h>
#include <vector>
#include <string>
#include <xlsxwriter.h>
#include "ErrorList.h"
enum TypeFormat {
		FLOAT_32BITS,
		INT_32BITS
	};
using namespace std;
typedef struct dataconf {
	unsigned int msegs;					//Tempo de amostragem
	unsigned int prec;					//Precisão
	unsigned int ganho;					//Ganho
} DataConf;
typedef struct data {
	unsigned char signbegin[4];			//3 bytes for padding memory
	int dt;								//Digital signal from H7X11
	unsigned int mtime;					//Time for each step on arduino's loop		
	unsigned char signend[4];			//3 bytes for padding memory
}DataProtocol;
typedef struct commports{
	vector <wstring> cm;
	int nCursel{ 0 };
	wstring portname;
	DCB dcb{ 0 };
	COMMCONFIG cc;
	COMMTIMEOUTS ct{ 0 };
}COMMPORTS;
typedef struct filedata {
	int mtime;
	int dt;
}FileData;
typedef struct fileheader {
	char ID[3];
	char type;
	size_t nblocks;
}FileHeader;
int SaveFile(string, HWND&, int);
int PutDataInLV(string,HWND&);
int ExportFile(string, HWND&,int, int);
bool AddPortsNametoCB(COMMPORTS&,HWND&);
