#include "../include/mainPropulsor.h"

const char *semName = "tanque";

int main(int argc, char **argv){

    sem_t *sem_id = sem_open(semName, O_CREAT, 0600, 0);

    if (sem_id == SEM_FAILED){}

    int distance_decrease = 1;
    int distance;

    while(1){
        sem_wait(sem_id);

        sem_post(sem_id);
        distance -= distance_decrease;
        sleep(1);
    }

    return 0;
}