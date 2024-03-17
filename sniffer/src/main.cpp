#include "Kikan/Engine.h"

int main(){

    Kikan::Engine::init();
    Kikan::Engine* engine = Kikan::Engine::Kikan();

    while (engine->shouldRun()) {
        engine->update();
    }

    return 0;
}