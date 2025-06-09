#include <iostream>

// Forward declare test functions from other test files
void runValveTests();
void runTankTests();
// Add more declarations as new test files are added (e.g., void runSensorTests();)

int main() {
    std::cout << "Starting All Unit Tests..." << std::endl;
    try {
        runValveTests();
        runTankTests();
        // Call other test runners here
        // e.g., runSensorTests();

        std::cout << "\nAll tests completed successfully!" << std::endl;
        return 0; // Success
    } catch (const std::exception& e) {
        std::cerr << "\nTEST RUN FAILED due to an exception: " << e.what() << std::endl;
        return 1; // Failure
    } catch (...) {
        std::cerr << "\nTEST RUN FAILED due to an unknown exception." << std::endl;
        return 1; // Failure
    }
}
