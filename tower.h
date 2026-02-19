#ifndef TOWER_H
#define TOWER_H

#include <stack>
#include <string>

class Tower {
public:
    std::stack<int> disks;
    std::string name;

    Tower() : name("") {}
    Tower(std::string n) : name(n) {}

    void push(int diskSize) {
        disks.push(diskSize);
    }

    int pop() {
        if (disks.empty()) return -1;
        int t = disks.top();
        disks.pop();
        return t;
    }

    int top() {
        if (disks.empty()) return -1;
        return disks.top();
    }

    bool isEmpty() {
        return disks.empty();
    }

    int size() {
        return (int)disks.size();
    }
};

#endif // TOWER_H
