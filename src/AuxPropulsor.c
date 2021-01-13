#include "../include/AuxPropulsor.h"

const char *semName = "tanque";

int main(int argc, char **argv){

    sem_t *sem_id = sem_open(semName, O_CREAT, 0600, 0);

    if (sem_id == SEM_FAILED){}

    int grados_desfase;
    bool encendido = false;

    while(1){
        while(!encendido){}
        correct_position(grados_desfase,&encendido,sem_id);
    }

    return 0;
}

void correct_position(int grades,bool *encendido, sem_t *sem_id){
    while(grades > 0){
        sem_wait(sem_id);

        sem_post(sem_id);
        grades--;
        sleep(1);
    }
    *encendido = false;
    
}