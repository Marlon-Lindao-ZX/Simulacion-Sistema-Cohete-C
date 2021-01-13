#include "../include/TankSystem.h"

const char *semName = "tanque";
sem_t *sem_id = NULL;

int main(int argc, char **argv){

    sem_id = sem_open(semName, O_CREAT, 0600, 0);

    if (sem_id == SEM_FAILED){}

    float level_tank = TANK_LEVEL;

    float controller = TANK_LEVEL * 0.1;

    while(1){
        sem_wait(sem_id);

        sem_post(sem_id);
        if(level_tank < controller) printf("Envia mensaje al control");
        sleep(1);
    }

    return 0;
}