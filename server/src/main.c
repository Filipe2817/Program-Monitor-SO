#include <unistd.h>

int main(int argc, char *argv[]) {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-result"
    write(STDOUT_FILENO, "This is the server!\n", 20);
    #pragma GCC diagnostic pop

    return 0;
}