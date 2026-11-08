#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>

using namespace std;

// Funcție pentru verificarea numerelor prime
bool is_prime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6)
        if (n % i == 0 || n % (i + 2) == 0) return false;
    return true;
}

int main() {
    const int NUM_PROCESSES = 10;
    const int INTERVAL_SIZE = 1000;

    // Vom salva file descriptorii de citire ai părintelui pentru a culege datele mai târziu
    int parent_read_fds[NUM_PROCESSES];

    for (int i = 0; i < NUM_PROCESSES; ++i) {
        int p2c[2]; // Pipe Părinte către Copil
        int c2p[2]; // Pipe Copil către Părinte

        if (pipe(p2c) == -1 || pipe(c2p) == -1) {
            cerr << "Eroare la crearea pipe-urilor!" << endl;
            return 1;
        }

        pid_t pid = fork();

        if (pid < 0) {
            cerr << "Eroare la fork!" << endl;
            return 1;
        }

        if (pid == 0) { // SUNTEM ÎN PROCESUL COPIL
            close(p2c[1]); // Închidem capătul de scriere (părinte->copil)
            close(c2p[0]); // Închidem capătul de citire (copil->părinte)

            int range[2];
            // Citim intervalul de la părinte
            read(p2c[0], range, sizeof(range));

            // Căutăm numerele prime
            for (int num = range[0]; num <= range[1]; ++num) {
                if (is_prime(num)) {
                    write(c2p[1], &num, sizeof(num)); // Trimitem numărul la părinte
                }
            }
            // Trimitem -1 ca semnal de finalizare
            int end_signal = -1;
            write(c2p[1], &end_signal, sizeof(end_signal));

            close(p2c[0]);
            close(c2p[1]);
            exit(0); // Copilul își termină execuția
        }
        else { // SUNTEM ÎN PROCESUL PĂRINTE
            close(p2c[0]); // Închidem capătul de citire (părinte->copil)
            close(c2p[1]); // Închidem capătul de scriere (copil->părinte)

            int range[2] = { i * INTERVAL_SIZE + 1, (i + 1) * INTERVAL_SIZE };
            // Trimitem intervalul către copil
            write(p2c[1], range, sizeof(range));

            close(p2c[1]); // Părintele nu mai scrie în acest pipe

            // Salvăm capătul de citire dinspre copil pentru a afișa rezultatele la final
            parent_read_fds[i] = c2p[0];
        }
    }

    // Părintele citește asincron datele de la toți copiii
    cout << "Numerele prime gasite pana la 10.000 sunt:\n";
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        int prime;
        // Citim până dăm de semnalul -1
        while (read(parent_read_fds[i], &prime, sizeof(prime)) > 0 && prime != -1) {
            cout << prime << " ";
        }
        close(parent_read_fds[i]);
        wait(NULL); // Așteptăm ca procesul copil să fie distrus curat (evităm zombie processes)
    }
    cout << endl;

    return 0;
}