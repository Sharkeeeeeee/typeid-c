#define TYPEID_IMPLEMENTATION
#include <typeid.hpp>
#include <iostream>

int main() {
    std::cout << tid::TypeID::generate("conan") << "\n";
    return 0;
}
