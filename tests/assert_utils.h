#ifndef ASSERT_UTILS_H
#define ASSERT_UTILS_H

#include <iostream>
#include <string>
#include <stdexcept> // Required for std::runtime_error
#include <sstream>   // Required for std::ostringstream

// Helper to convert values to string for messages
template<typename T>
std::string toString(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

inline void assertTrue(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "ASSERTION FAILED: " << message << std::endl;
        throw std::runtime_error("Assertion failed: " + message);
    } else {
        // std::cout << "ASSERTION PASSED: " << message << std::endl; // Too verbose for every pass
    }
}

template<typename T>
void assertEquals(const T& expected, const T& actual, const std::string& message) {
    if (expected != actual) {
        std::cerr << "ASSERTION FAILED: " << message
                  << ". Expected: " << toString(expected) << ", Actual: " << toString(actual) << std::endl;
        throw std::runtime_error("Assertion failed: " + message +
                                 ". Expected: " + toString(expected) + ", Actual: " + toString(actual));
    } else {
        // std::cout << "ASSERTION PASSED: " << message << std::endl; // Too verbose for every pass
    }
}

// Specialization for double with tolerance
inline void assertEquals(double expected, double actual, double tolerance, const std::string& message) {
    if (std::abs(expected - actual) > tolerance) {
        std::cerr << "ASSERTION FAILED (double with tolerance): " << message
                  << ". Expected: " << toString(expected) << ", Actual: " << toString(actual)
                  << ", Tolerance: " << toString(tolerance) << std::endl;
        throw std::runtime_error("Assertion failed (double with tolerance): " + message +
                                 ". Expected: " + toString(expected) + ", Actual: " + toString(actual) +
                                 ", Tolerance: " + toString(tolerance));
    } else {
        // std::cout << "ASSERTION PASSED: " << message << std::endl;
    }
}


#endif // ASSERT_UTILS_H
