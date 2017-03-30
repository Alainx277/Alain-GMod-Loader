#include "Internal.h"

DWORD Internal::GetPointer(DWORD address)
{
	return *(DWORD*)address;
}

DWORD Internal::GetPointer(DWORD address, vector<DWORD> offsets)
{
	for each (DWORD offset in offsets)
	{
		address = *(DWORD*)(address + offset);
	}
	return address;
}

bool Internal::bCompare(const BYTE * pData, const BYTE * bMask, const char * szMask)
{
	for (; *szMask; ++szMask, ++pData, ++bMask)
		if (*szMask == 'x' && *pData != *bMask)
			return 0;
	return (*szMask) == NULL;
}

DWORD Internal::FindPattern(DWORD dwAddress, DWORD dwLen, BYTE * bMask, char * szMask)
{
	for (DWORD i = 0; i<dwLen; i++)
		if (bCompare((BYTE*)(dwAddress + i), bMask, szMask))
			return (DWORD)(dwAddress + i);
	return 0;
}

void Internal::CreateConsole()
{
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONIN$", "r", stdin);
}

bool Internal::ErasePEHeader(DWORD GetModuleBase)
{
	// Get headers
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)GetModuleBase;
	PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader + (DWORD)pDosHeader->e_lfanew);

	// Check for valid signature
	if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
		return false;

	// Check if size exists
	if (pNTHeader->FileHeader.SizeOfOptionalHeader)
	{
		DWORD Protect;
		// Get header size
		WORD Size = pNTHeader->FileHeader.SizeOfOptionalHeader;
		// Unprotect header
		VirtualProtect((void*)GetModuleBase, Size, PAGE_EXECUTE_READWRITE, &Protect);
		// Zero (delete) header
		RtlZeroMemory((void*)GetModuleBase, Size);
		// Reprotect header
		VirtualProtect((void*)GetModuleBase, Size, Protect, &Protect);
		return true;
	}
	return false;
}

Internal::MHook::MHook(DWORD address, DWORD hookAddress, DWORD lenght)
{
	this->address = (BYTE*)address;
	this->jumpTo = hookAddress;
	this->lenght = lenght;
	this->jumpBack = address + lenght;
}

Internal::MHook::MHook()
{
}

Internal::MHook::~MHook()
{
}

bool Internal::MHook::Hook()
{
	// Check if already hooked
	if (this->hooked)
	{
		return false;
	}

	// Check if hook lenght between 5 and 10
	if (this->lenght < 5 || this->lenght > 10)
	{
		return false;
	}

	// Unprotect hook section
	DWORD oldProtect;
	VirtualProtect((LPVOID)this->address, this->lenght, PAGE_EXECUTE_READWRITE, &oldProtect);
	// Copy bytes
	memcpy(this->originalBytes, (void*)address, this->lenght);
	// Calculate jump distance
	DWORD relativeAddress = this->jumpTo - (DWORD)this->address - 5;
	// jmp instruction 0xE9
	*address = 0xE9;
	// Jump address
	*(DWORD*)(address + 1) = relativeAddress;
	// Overwrite the rest of the bytes with NOPs
	for (DWORD x = 0x5; x < this->lenght; x++)
		*(this->address + x) = 0x90;
	// Restore protection
	VirtualProtect((LPVOID)this->address, this->lenght, oldProtect, &oldProtect);
	// Allocate memory for original function
	LPVOID allocated = VirtualAlloc(NULL, this->lenght + 5, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	// Write original bytes to memory
	memcpy(allocated, this->originalBytes, this->lenght);
	// Write jump back
	*(BYTE*)((DWORD)allocated + lenght) = 0xE9;
	// Write address
	*(DWORD*)((DWORD)allocated + lenght + 1) = this->jumpBack;
	// save function location
	this->originalFunction = allocated;
	// set hooked bool
	this->hooked = true;
	// return success
	return true;
}

bool Internal::MHook::UnHook()
{
	// Check if not hooked
	if (!this->hooked)
	{
		return false;
	}

	// Unprotect hook section
	DWORD oldProtect;
	VirtualProtect((LPVOID)this->address, this->lenght, PAGE_EXECUTE_READWRITE, &oldProtect);
	// Copy bytes
	memcpy((void*)address, this->originalBytes, this->lenght);
	// Restore protection
	VirtualProtect((LPVOID)this->address, this->lenght, oldProtect, &oldProtect);

	// Unhooked
	this->hooked = false;
	return true;
}

Internal::VHook::VHook(DWORD address, DWORD hookAddress)
{
	this->address = address;
	this->jumpTo = hookAddress;
}

Internal::VHook::VHook(void * object, int index, DWORD hookAddress)
{
	DWORD* vTable = *(DWORD**)object;
	this->address = vTable[index];
	this->jumpTo = hookAddress;
}

Internal::VHook::VHook()
{
}

Internal::VHook::~VHook()
{
	UnHook();
}

bool Internal::VHook::Hook()
{
	// Check if already hooked
	if (this->hooked)
	{
		return false;
	}

	// Unprotect hook section
	DWORD oldProtect;
	VirtualProtect((LPVOID)this->address, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
	// Copy pointer
	this->original = *(DWORD*)this->address;
	// Overwrite pointer
	*(DWORD*)this->address = this->jumpTo;
	// Restore protection
	VirtualProtect((LPVOID)this->address, 5, oldProtect, &oldProtect);

	// set hooked bool
	this->hooked = true;
	// return success
	return true;
}

bool Internal::VHook::UnHook()
{
	// Check if already hooked
	if (!this->hooked)
	{
		return false;
	}

	// Unprotect hook section
	DWORD oldProtect;
	VirtualProtect((LPVOID)this->address, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
	// write original pointer
	*(DWORD*)this->address = this->original;
	// Restore protection
	VirtualProtect((LPVOID)this->address, 5, oldProtect, &oldProtect);
	// set hooked bool
	this->hooked = false;
	// return success
	return true;
}
