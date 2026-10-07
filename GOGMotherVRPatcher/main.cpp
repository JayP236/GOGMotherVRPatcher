#include "utilities.h"
#include <iostream>
#include <fstream>
#include <cstdio>

using namespace std;

LPCWSTR lpVersion = L"1.0.0";

int main() {
    wstring message = L"GOGMotherVRPatcher ";
    message += lpVersion;

	FilePatcher::Patch patch;
	if (FilePatcher::loadPatchEmbedded("MOTHER_TO_GOG_PATCH", patch))
	{
		char buffer[MAX_PATH];
		DWORD length = GetCurrentDirectoryA(MAX_PATH, buffer);

		const string inputFilePath = string(buffer) + "\\dxgi.dll";
		const string bakFilePath = string(buffer) + "\\dxgi.dll.bak";

		// verify crc of dxgi.dll
        unsigned int dxgiCrc = FilePatcher::calculateCrc32(inputFilePath);
        if (dxgiCrc == 2938639801)
        {
            cout << "MotherVR v0.8.1 detected." << endl;
        }
        else
        {
            if (dxgiCrc == 0)
            {
                MessageBoxW((HWND)0x0, L"'MotherVR dxgi.dll not found!\n\nMake sure this executable is placed in the GOG Alien: Isolation folder and MotherVR is installed.\n\n", message.c_str(), MB_ICONERROR | MB_OK);
                return 1;
            }
            cout << "Unsupported MotherVR version detected, crc: " << dxgiCrc << endl;
            MessageBoxW((HWND)0x0, L"Unsupported MotherVR version detected!\n\nOnly v0.8.1 of MotherVR is supported.\n\n", message.c_str(), MB_ICONERROR | MB_OK);
            return 1;
        }

        // check if .bak file already exists, if so delete it
        ifstream bakFile(bakFilePath.c_str());
        if (bakFile.good()) {
            bakFile.close();
            cout << "dxgi.dll.bak already exists. Removing...\n";
            if (remove(bakFilePath.c_str()) == 0)
            {
                cout << "Old backup file dxgi.dll.bak successfully removed.\n";
            }
            else
            {
                cout << "Error removing file" << endl;
                MessageBoxW((HWND)0x0, L"Error removing dxgi.dll.bak.\n\n", message.c_str(), MB_ICONERROR | MB_OK);
                return 1;
            }
        }
        else {
            cout << "dxgi.dll.bak does not already exist.\n";
        }

		// rename to .bak
        if (rename(inputFilePath.c_str(), bakFilePath.c_str()) == 0) {
            cout << "Successfully renamed dxgi.dll to dxgi.dll.bak.\n";
        }
        else {
            cout << "Error renaming file" << endl;
            MessageBoxW((HWND)0x0, L"Error renaming dxgi.dll to dxgi.dll.bak.\n\n", message.c_str(), MB_ICONERROR | MB_OK);
            return 1;
        }

		// apply patch
        if (FilePatcher::applyPatch(bakFilePath, inputFilePath, patch))
        {
            cout << "Successfully patched file dxgi.dll.\n" << endl;
            MessageBoxW((HWND)0x0, L"Successfully patched file dxgi.dll.\n\n", message.c_str(), MB_ICONINFORMATION | MB_OK);
        }
        else
        {
            cout << "Error patching dxgi.dll.\n" << endl;
            MessageBoxW((HWND)0x0, L"Error patching dxgi.dll.\n\n", message.c_str(), MB_ICONERROR | MB_OK);
            return 1;
        }
	}

	return 0;
}