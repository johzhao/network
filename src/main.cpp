#include "loguru/loguru.cpp"

int main(int argc, char *argv[]) {
    loguru::init(argc, argv);

    loguru::shutdown();
    return 0;
}
