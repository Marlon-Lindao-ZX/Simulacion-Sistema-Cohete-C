#include "../include/mainPropulsor.h"

int main(int argc, char **argv){

    int distance_decrease = 1;
    int distance;

    while(1){
        distance -= distance_decrease;
        sleep(1);
    }

    return 0;
}