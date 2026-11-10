#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <ctime>

using namespace std;

// Structura care va sta in memoria partajata
struct SharedData {
    sem_t semafor;
    int counter;
};

int main() {
    // 1. Creare memorie partajata cu cast explicit pentru C++
    SharedData* mem = (SharedData*)mmap(NULL, sizeof(SharedData),
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (mem == MAP_FAILED) {
        cerr << "Eroare la alocarea memoriei partajate (mmap)\n";
        exit(EXIT_FAILURE);
    }

    // 2. Initializare semafor in memoria partajata
    if (sem_init(&mem->semafor, 1, 1) == -1) {
        cerr << "Eroare la initializarea semaforului\n";
        exit(EXIT_FAILURE);
    }

    mem->counter = 0;

    // 3. Creare proces copil
    pid_t pid = fork();

    if (pid < 0) {
        cerr << "Eroare la fork\n";
        exit(EXIT_FAILURE);
    }

    // Initializare generator numere aleatoare
    srand(time(NULL) ^ (getpid() << 16));

    int id_proces = (pid == 0) ? 2 : 1; // 1 = Parinte, 2 = Copil

    // 4. Bucla principala
    while (true) {
        sem_wait(&mem->semafor); // Intram in regiunea critica

        if (mem->counter >= 1000) {
            sem_post(&mem->semafor); // Eliberam inainte sa iesim
            break;
        }

        int ban = (rand() % 2) + 1; // Genereaza 1 sau 2

        while (ban == 2 && mem->counter < 1000) {
            mem->counter++;
            cout << "[Ubuntu] Procesul " << id_proces << " a scris: " << mem->counter << "\n";
            ban = (rand() % 2) + 1;
        }

        sem_post(&mem->semafor); // Iesim din regiunea critica
        usleep(100); // Pauza scurta pentru context switch
    }

    // 5. Curatenie
    if (pid != 0) {
        wait(NULL); // Parintele asteapta copilul
        sem_destroy(&mem->semafor);
        munmap(mem, sizeof(SharedData));
        cout << "\nProcesul Parinte (1) s-a incheiat si a eliberat resursele.\n";
    }

    return 0;
}