#include "../include/AuxPropulsor.h"

int main(int argc, char **argv){

    int grados_desfase;
    bool encendido = false;

    while(1){
        while(!encendido){}
        correct_position(grados_desfase,&encendido);
    }

    return 0;
}

void correct_position(int grades,bool *encendido){
    while(grades > 0){
        grades--;
        sleep(1);
    }
    *encendido = false;
    
}