#pragma once
#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

namespace Internal
{
	// Pointers
	DWORD GetPointer(DWORD address);
	DWORD GetPointer(DWORD address, vector<DWORD> offsets);

	// Pattern Scanning
	bool bCompare(const BYTE* pData, const BYTE* bMask, const char* szMask);
	DWORD FindPattern(DWORD dwAddress, DWORD dwLen, BYTE *bMask, char * szMask);

	// Misc
	void CreateConsole();
	bool ErasePEHeader(DWORD moduleBase);

	// Hooking
	class MHook
	{
	public:
		MHook(DWORD address, DWORD hookAddress, DWORD lenght);
		MHook();
		~MHook();
		bool Hook();
		bool UnHook();

		bool hooked = false;
		DWORD jumpBack;
		void* originalFunction;
	private:
		BYTE* address;
		DWORD jumpTo;
		byte originalBytes[10];
		int lenght;
	};

	class VHook
	{
	public:
		VHook(DWORD address, DWORD hookAddress);
		VHook(void* object, int index, DWORD hookAddress);
		VHook();
		~VHook();
		bool Hook();
		bool UnHook();

		bool hooked = false;
		DWORD original;
	private:
		DWORD address;
		DWORD jumpTo;
	};
}