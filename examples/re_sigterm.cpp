#include <signal.h>

int main(){
    raise(SIGTERM);
    return 0;
}
