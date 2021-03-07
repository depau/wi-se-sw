//
// Created by depau on 3/6/21.
//


#include <Arduino.h>

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>

int main() {
    int fdno = fileno(stdin);
    int flags = fcntl(fdno, F_GETFL);
    if (flags < 0) {
        perror("Unable to get flags stdin");
        exit(1);
    }
    if (fcntl(fdno, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("Unable to set non blocking stdin");
        exit(1);
    }
    srand(time(NULL));

    setup();
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    for (;;) {
        loop();
    }
#pragma clang diagnostic pop
}