#include <iostream>
#include <windows.h>
#include <string>

using namespace std;

bool is_prime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6)
        if (n % i == 0 || n % (i + 2) == 0) return false;
    return true;
}

int main(int argc, char* argv[]) {
    // --- RAMURA PROCESULUI COPIL ---
    if (argc == 4 && string(argv[1]) == "--child") {
        // Preluam handle-urile din argumentele liniei de comanda
        HANDLE hReadFromParent = (HANDLE)stoull(argv[2]);
        HANDLE hWriteToParent = (HANDLE)stoull(argv[3]);

        int range[2];
        DWORD bytesRead, bytesWritten;

        // Citim intervalul prin pipe
        ReadFile(hReadFromParent, range, sizeof(range), &bytesRead, NULL);

        for (int i = range[0]; i <= range[1]; ++i) {
            if (is_prime(i)) {
                WriteFile(hWriteToParent, &i, sizeof(i), &bytesWritten, NULL);
            }
        }

        // Semnal de stop
        int end_signal = -1;
        WriteFile(hWriteToParent, &end_signal, sizeof(end_signal), &bytesWritten, NULL);

        CloseHandle(hReadFromParent);
        CloseHandle(hWriteToParent);
        return 0;
    }

    // --- RAMURA PROCESULUI PaRINTE ---
    const int NUM_PROCESSES = 10;
    const int INTERVAL_SIZE = 1000;

    HANDLE parentReadHandles[NUM_PROCESSES];
    HANDLE childProcesses[NUM_PROCESSES];

    // Setam atributele de securitate pentru ca handle-urile pipe-urilor sa fie mostenite de copii
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    for (int i = 0; i < NUM_PROCESSES; ++i) {
        HANDLE p2c_Read, p2c_Write;
        HANDLE c2p_Read, c2p_Write;

        CreatePipe(&p2c_Read, &p2c_Write, &sa, 0);
        CreatePipe(&c2p_Read, &c2p_Write, &sa, 0);

        // Ne asiguram ca partile folosite DOAR de parinte NU sunt mostenite
        SetHandleInformation(p2c_Write, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(c2p_Read, HANDLE_FLAG_INHERIT, 0);

        parentReadHandles[i] = c2p_Read;

        // Construim comanda pentru a rula acelasi executabil sub forma de copil
        string cmd = string(argv[0]) + " --child " +
            to_string((unsigned long long)p2c_Read) + " " +
            to_string((unsigned long long)c2p_Write);

        STARTUPINFO si; ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
        PROCESS_INFORMATION pi; ZeroMemory(&pi, sizeof(pi));

        // Conversie cmd (std::string) in std::wstring pentru CreateProcessW
        std::wstring wcmd(cmd.begin(), cmd.end());

        // CreateProcessW asteapta LPWSTR (wchar_t*)
        CreateProcessW(NULL, &wcmd[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
        childProcesses[i] = pi.hProcess;
        CloseHandle(pi.hThread); // Nu avem nevoie de handle-ul thread-ului principal

        // Parintele trimite intervalul 
        int range[2] = { i * INTERVAL_SIZE + 1, (i + 1) * INTERVAL_SIZE };
        DWORD bytesWritten;
        WriteFile(p2c_Write, range, sizeof(range), &bytesWritten, NULL);

        // Parintele inchide capetele sale pentru care nu are treaba sau a terminat cu ele
        CloseHandle(p2c_Read);
        CloseHandle(c2p_Write);
        CloseHandle(p2c_Write);
    }

    cout << "Numerele prime gasite pana la 10.000 (Windows) sunt:\n";
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        int prime;
        DWORD bytesRead;
        while (ReadFile(parentReadHandles[i], &prime, sizeof(prime), &bytesRead, NULL) && bytesRead > 0 && prime != -1) {
            cout << prime << " ";
        }
        CloseHandle(parentReadHandles[i]);

        // Asteptam sa se inchida copilul complet
        WaitForSingleObject(childProcesses[i], INFINITE);
        CloseHandle(childProcesses[i]);
    }
    cout << endl;

    return 0;
}