#include<iostream>
#include "test.h"

void printAssertMessage() {

}

void printAssertMessage(const char* message) {
    std::cout << "Message: " << message << std::endl << std::endl;
}
