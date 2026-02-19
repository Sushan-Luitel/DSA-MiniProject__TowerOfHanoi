#ifndef MOVE_H
#define MOVE_H

#include <string>

struct Move {
    std::string from;
    std::string to;
    int diskSize;

    Move() : from(""), to(""), diskSize(0) {}
    Move(std::string f, std::string t, int d) : from(f), to(t), diskSize(d) {}
};

#endif // MOVE_H
