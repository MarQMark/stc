#include "Sniffer.h"

int main(){
    Sniffer sniffer;

    while (sniffer.running) {
        sniffer.update();
    }

    return 0;
}