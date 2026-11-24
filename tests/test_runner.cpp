#include <iostream>

void RunAllMathTests();
void RunAllStateTests();
void RunAllArkTests();
void RunAllVehiclePhysicsTests();

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << "   SPLIT/SECOND: VELOCITY RECONSTRUCTION TEST SUITE         " << std::endl;
    std::cout << "============================================================" << std::endl;

    try {
        RunAllMathTests();
        RunAllStateTests();
        RunAllArkTests();
        RunAllVehiclePhysicsTests();

        std::cout << "\n============================================================" << std::endl;
        std::cout << "   ALL UNIT TESTS PASSED SUCCESSFULLY! (100% VERIFIED)      " << std::endl;
        std::cout << "============================================================" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
