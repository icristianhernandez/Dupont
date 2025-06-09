#include "Valve.h" // Assuming Valve.h is accessible via include_directories
#include "assert_utils.h"
#include <iostream>

// Test default constructor and name
void testValveDefaultConstructorAndName() {
    Valve v("TestValveDefault"); // Default status is OPEN
    assertEquals(std::string("TestValveDefault"), v.get_name(), "Valve name check (default constructor).");
    assertEquals(ValveStatus::OPEN, v.get_status(), "Default valve status should be OPEN.");
    std::cout << "TestValveDefaultConstructorAndName PASSED" << std::endl;
}

// Test constructor with specific status and name
void testValveParameterizedConstructorAndName() {
    Valve v_closed("TestValveClosedParam", ValveStatus::CLOSED);
    assertEquals(std::string("TestValveClosedParam"), v_closed.get_name(), "Valve name check (closed by param).");
    assertEquals(ValveStatus::CLOSED, v_closed.get_status(), "Parameterized valve status should be CLOSED.");

    Valve v_open("TestValveOpenParam", ValveStatus::OPEN);
    assertEquals(std::string("TestValveOpenParam"), v_open.get_name(), "Valve name check (open by param).");
    assertEquals(ValveStatus::OPEN, v_open.get_status(), "Parameterized valve status should be OPEN.");
    std::cout << "TestValveParameterizedConstructorAndName PASSED" << std::endl;
}

void testValveOpenClose() {
    Valve v("TestValveOpCl");
    v.close();
    assertEquals(ValveStatus::CLOSED, v.get_status(), "Valve should be CLOSED after close().");
    v.open();
    assertEquals(ValveStatus::OPEN, v.get_status(), "Valve should be OPEN after open().");
    // Test idempotency
    v.open();
    assertEquals(ValveStatus::OPEN, v.get_status(), "Valve should remain OPEN after open() again.");
    v.close();
    assertEquals(ValveStatus::CLOSED, v.get_status(), "Valve should be CLOSED after close().");
    v.close();
    assertEquals(ValveStatus::CLOSED, v.get_status(), "Valve should remain CLOSED after close() again.");
    std::cout << "TestValveOpenClose PASSED" << std::endl;
}

// Test runner function for this file
void runValveTests() {
    std::cout << "\n--- Running Valve Tests ---" << std::endl;
    testValveDefaultConstructorAndName();
    testValveParameterizedConstructorAndName();
    testValveOpenClose();
    std::cout << "--- Valve Tests Done ---" << std::endl;
}
