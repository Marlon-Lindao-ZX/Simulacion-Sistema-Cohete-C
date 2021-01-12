#include "../include/TankSystem.h"

int main(int argc, char **argv){

    float level_tank = TANK_LEVEL;

    float controller = TANK_LEVEL * 0.1;

    while(1){
        if(level_tank < controller) printf("Envia mensaje al control");
        sleep(1);
    }

    return 0;
}