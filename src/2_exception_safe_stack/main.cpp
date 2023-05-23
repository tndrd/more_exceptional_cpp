#include "mystack.hpp"
#include <iostream>

int main() {
    MyStack<int> a;
    a.push(2);
    a.push(3);
    std::cout << a.top();
    a.pop();
}