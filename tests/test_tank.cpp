#include "Tank.h" // Assuming Tank.h is accessible via include_directories
#include "Sensor.h" // Tank owns a Sensor
#include "Enums.h"
#include "assert_utils.h"
#include <iostream>
#include <string> // For std::to_string in messages
#include <stdexcept> // For std::invalid_argument in constructor test

const double TOLERANCE = 1e-6; // Tolerance for double comparisons

void testTankConstructorValid() {
    Tank t("TestTankValid", 100.0, 25.0);
    assertEquals(std::string("TestTankValid"), t.get_name(), "Tank name check (valid constructor).");
    assertEquals(100.0, t.get_capacity_liters(), TOLERANCE, "Tank capacity check (valid constructor).");
    assertEquals(25.0, t.get_current_level_liters(), TOLERANCE, "Tank initial level check (valid constructor).");
    assertEquals(25.0, t.get_current_level_percentage(), TOLERANCE, "Tank initial percentage check (valid constructor).");
    assertEquals(LevelStatus::NORMAL_LEVEL, t.get_level_status(), "Tank initial level status check (valid constructor).");
    // Check owned sensor
    assertEquals(SensorType::LEVEL_TRANSMITTER, t.get_level_transmitter().get_type(), "Tank sensor type check.");
    assertEquals(25.0, t.get_level_transmitter().get_level_liters(), TOLERANCE, "Tank sensor initial level check.");
    std::cout << "TestTankConstructorValid PASSED" << std::endl;
}

void testTankConstructorInvalidCapacity() {
    bool exception_thrown = false;
    try {
        Tank t("TestTankInvalidCap", 0.0, 0.0); // Zero capacity
    } catch (const std::invalid_argument& e) {
        exception_thrown = true;
        std::string expected_msg = "Tank capacity must be positive";
        // Check if the error message contains the expected text
        assertTrue(std::string(e.what()).find(expected_msg) != std::string::npos,
                   "Invalid capacity exception message should contain '" + expected_msg + "'. Got: " + e.what());
    }
    assertTrue(exception_thrown, "Tank constructor should throw std::invalid_argument for zero capacity.");

    exception_thrown = false;
    try {
        Tank t("TestTankInvalidCapNeg", -100.0, 0.0); // Negative capacity
    } catch (const std::invalid_argument& e) {
        exception_thrown = true;
    }
    assertTrue(exception_thrown, "Tank constructor should throw std::invalid_argument for negative capacity.");
    std::cout << "TestTankConstructorInvalidCapacity PASSED" << std::endl;
}

void testTankConstructorLevelClamping() {
    Tank t_over("TestTankOver", 100.0, 120.0); // Initial level exceeds capacity
    assertEquals(100.0, t_over.get_current_level_liters(), TOLERANCE, "Tank initial level clamped to capacity.");
    assertEquals(100.0, t_over.get_level_transmitter().get_level_liters(), TOLERANCE, "Sensor level clamped to capacity.");

    Tank t_under("TestTankUnder", 100.0, -20.0); // Initial level is negative
    assertEquals(0.0, t_under.get_current_level_liters(), TOLERANCE, "Tank initial level clamped to zero.");
    assertEquals(0.0, t_under.get_level_transmitter().get_level_liters(), TOLERANCE, "Sensor level clamped to zero.");
    std::cout << "TestTankConstructorLevelClamping PASSED" << std::endl;
}

void testTankAddLiquid() {
    Tank t("AddLiquidTank", 100.0, 10.0);
    t.add_liquid(20.0);
    assertEquals(30.0, t.get_current_level_liters(), TOLERANCE, "Tank level after adding liquid.");
    assertEquals(30.0, t.get_level_transmitter().get_level_liters(), TOLERANCE, "Sensor level after adding liquid.");

    t.add_liquid(80.0); // Attempt to overfill
    assertEquals(100.0, t.get_current_level_liters(), TOLERANCE, "Tank level after attempt to overfill.");
    assertEquals(100.0, t.get_level_transmitter().get_level_liters(), TOLERANCE, "Sensor level after attempt to overfill.");

    t.add_liquid(-5.0); // Attempt to add negative
    assertEquals(100.0, t.get_current_level_liters(), TOLERANCE, "Tank level after adding negative liquid (should not change).");
    std::cout << "TestTankAddLiquid PASSED" << std::endl;
}

void testTankRemoveLiquid() {
    Tank t("RemoveLiquidTank", 100.0, 50.0);
    t.remove_liquid(20.0);
    assertEquals(30.0, t.get_current_level_liters(), TOLERANCE, "Tank level after removing liquid.");
    assertEquals(30.0, t.get_level_transmitter().get_level_liters(), TOLERANCE, "Sensor level after removing liquid.");

    t.remove_liquid(40.0); // Attempt to over-empty
    assertEquals(0.0, t.get_current_level_liters(), TOLERANCE, "Tank level after attempt to over-empty.");
    assertEquals(0.0, t.get_level_transmitter().get_level_liters(), TOLERANCE, "Sensor level after attempt to over-empty.");
    assertEquals(LevelStatus::EMPTY, t.get_level_status(), "Tank status should be EMPTY.");

    t.remove_liquid(-5.0); // Attempt to remove negative
    assertEquals(0.0, t.get_current_level_liters(), TOLERANCE, "Tank level after removing negative liquid (should not change).");
    std::cout << "TestTankRemoveLiquid PASSED" << std::endl;
}

void testTankLevelStatus() {
    Tank t("LevelStatusTank", 100.0, 0.0);
    assertEquals(LevelStatus::EMPTY, t.get_level_status(), "Level status EMPTY at 0L (0%).");

    t.add_liquid(2.0); // 2%
    assertEquals(LevelStatus::LOW, t.get_level_status(), "Level status LOW at 2L (2%).");

    t.add_liquid(2.9); // 4.9% (2.0 + 2.9 = 4.9L)
    assertEquals(LevelStatus::LOW, t.get_level_status(), "Level status LOW at 4.9L (4.9%).");

    t.add_liquid(0.1); // 5.0% (4.9 + 0.1 = 5.0L)
    assertEquals(LevelStatus::NORMAL_LEVEL, t.get_level_status(), "Level status NORMAL at 5L (5%).");

    t.add_liquid(90.0); // 95.0% (5.0 + 90.0 = 95.0L)
    assertEquals(LevelStatus::NORMAL_LEVEL, t.get_level_status(), "Level status NORMAL at 95L (95%).");

    // Test case for capacity 0, though constructor should prevent it.
    // If it were possible, get_current_level_percentage might divide by zero.
    // Tank tZeroCap("ZeroCapTank", 0.0, 0.0); // This throws, so can't test status directly.
    // assertEquals(LevelStatus::EMPTY, tZeroCap.get_level_status(), "Level status EMPTY for zero capacity tank.");

    std::cout << "TestTankLevelStatus PASSED" << std::endl;
}

void runTankTests() {
    std::cout << "\n--- Running Tank Tests ---" << std::endl;
    testTankConstructorValid();
    testTankConstructorInvalidCapacity();
    testTankConstructorLevelClamping();
    testTankAddLiquid();
    testTankRemoveLiquid();
    testTankLevelStatus();
    std::cout << "--- Tank Tests Done ---" << std::endl;
}
