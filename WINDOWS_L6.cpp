// Fisier: WINDOWS_L6.cpp
// Compilare cu g++ (MinGW): g++ WINDOWS_L6.cpp -o WINDOWS_L6.exe
// Rulare: WINDOWS_L6.exe

#include <windows.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstdio>

using namespace std;

#define SHM_NAME "Local\\MySharedMemory"
#define SEM_NAME "Local\\MySemaphore"

struct SharedData {
    int counter;
};

int main(int argc, char* argv[]) {
    HANDLE hMapFile, hSemaphore;
    SharedData* mem;
    bool is_child = (argc > 1); // Daca are argumente, este copilul
    int proc_id = is_child ? 2 : 1;

    srand((unsigned int)time(NULL) ^ GetCurrentProcessId());

    if (!is_child) {
        // PROCES PARINTE: Creeaza obiectele
        hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedData), SHM_NAME);
        hSemaphore = CreateSemaphoreA(NULL, 1, 1, SEM_NAME);

        // Cast explicit (SharedData*) necesar in C++
        mem = (SharedData*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
        mem->counter = 0;

        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);

        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(pi));

        // Creare linie de comanda mutabila (CreateProcess necesita acest lucru in C++)
        char cmdLine[256];
        snprintf(cmdLine, sizeof(cmdLine), "\"%s\" child", argv[0]);

        CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        CloseHandle(pi.hThread);
    }
    else {
        // PROCES COPIL: Deschide obiectele create de parinte
        hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME);
        hSemaphore = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, SEM_NAME);

        // Cast explicit (SharedData*)
        mem = (SharedData*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
    }

    // BUCLA PRINCIPALA
    while (true) {
        WaitForSingleObject(hSemaphore, INFINITE); // Wait

        if (mem->counter >= 1000) {
            ReleaseSemaphore(hSemaphore, 1, NULL); // Elibereaza inainte sa iasa
            break;
        }

        int ban = (rand() % 2) + 1;
        while (ban == 2 && mem->counter < 1000) {
            mem->counter++;
            cout << "[Windows] Procesul " << proc_id << " a scris: " << mem->counter << "\n";
            ban = (rand() % 2) + 1;
        }

        ReleaseSemaphore(hSemaphore, 1, NULL); // Post
        Sleep(1); // 1 ms pauza pentru context switch
    }

    // Curatenie
    UnmapViewOfFile(mem);
    CloseHandle(hMapFile);
    CloseHandle(hSemaphore);

    if (!is_child) {
        cout << "\nProcesul Parinte a terminat.\n";
    }

    return 0;
}