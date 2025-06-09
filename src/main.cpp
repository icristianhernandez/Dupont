#include "System.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>        // For std::this_thread::sleep_for
#include <chrono>        // For std::chrono::milliseconds
#include <iomanip>       // For std::fixed and std::setprecision

// Simulation parameters
const double TIME_DELTA_SECONDS = 0.5; // Simulate 0.5s time step
const int SCREEN_REFRESH_MILLISECONDS = 500; // Refresh screen every 0.5s
const int TOTAL_SIMULATION_STEPS = 600; // Run for 600 steps (300 seconds or 5 minutes)
const std::string INPUT_FILEPATH = "input.txt";

// Function to clear console (OS-dependent)
void clear_console() {
#if defined(_WIN32) || defined(_WIN64)
    std::system("cls");
#else
    // ANSI escape code for POSIX systems (Linux, macOS)
    std::cout << "\033[2J\033[H"; // Clears screen and moves cursor to home
#endif
}


int main() {
    System paint_mixing_system;

    // Initial load from file
    paint_mixing_system.load_commands_from_file(INPUT_FILEPATH);

    std::cout << "\n--- Initial System Status (after loading " << INPUT_FILEPATH << ") ---\n";
    std::cout << paint_mixing_system.get_system_status_report();

    const auto& initial_logs = paint_mixing_system.get_logs();
    if (!initial_logs.empty()) {
        std::cout << "--- Initialization Logs ---\n";
        for(const auto& log : initial_logs) {
             std::cout << "INIT_LOG: " << log << std::endl;
        }
    }
    paint_mixing_system.clear_logs();

    std::cout << "\nStarting simulation loop for " << TOTAL_SIMULATION_STEPS << " steps ("
              << TOTAL_SIMULATION_STEPS * TIME_DELTA_SECONDS << " simulated seconds)." << std::endl;
    std::cout << "Screen refresh interval: " << SCREEN_REFRESH_MILLISECONDS << "ms. Press Ctrl+C to exit early." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    int idle_cycles_count = 0;
    const int IDLE_CYCLES_TO_STOP = 10; // Stop after 10 idle cycles (5 seconds) if no batch

    for (int i = 0; i < TOTAL_SIMULATION_STEPS; ++i) {
        paint_mixing_system.update(TIME_DELTA_SECONDS);

        // Optional: Clear console for a cleaner display. Can be disorienting for quick changes.
        // if (i > 0) clear_console();
        // Using a separator is often better for tracking changes.
        if (i > 0) {
             std::cout << "\n===============================================================================\n";
        }

        std::cout << "Time: " << std::fixed << std::setprecision(1) << ((i + 1) * TIME_DELTA_SECONDS) << "s"
                  << " (Step " << i + 1 << "/" << TOTAL_SIMULATION_STEPS << ")" << std::endl;
        std::cout << paint_mixing_system.get_system_status_report();

        const auto& current_logs = paint_mixing_system.get_logs();
        if (!current_logs.empty()) {
            std::cout << "--- Recent Events ---\n";
            for(const auto& log : current_logs) {
                 std::cout << "EVENT: " << log << std::endl;
            }
        }
        paint_mixing_system.clear_logs();

        if (paint_mixing_system.get_current_process_state() == ProcessState::ERROR_STATE) {
            std::cout << "\nSYSTEM HALTED DUE TO ERROR. See last message in report." << std::endl;
            break;
        }

        // Check for graceful exit if system is idle and no new commands are pending
        if (paint_mixing_system.get_current_process_state() == ProcessState::IDLE &&
            !paint_mixing_system.is_batch_in_progress() &&
            paint_mixing_system.get_start_command() == OnOffStatus::OFF_COMMAND) { // Ensure no pending start
            idle_cycles_count++;
            if (idle_cycles_count >= IDLE_CYCLES_TO_STOP) {
                std::cout << "\nSystem has been idle for " << IDLE_CYCLES_TO_STOP * TIME_DELTA_SECONDS
                          << " seconds. Ending simulation." << std::endl;
                break;
            }
        } else {
            idle_cycles_count = 0; // Reset if not idle or batch active/pending
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(SCREEN_REFRESH_MILLISECONDS));
    }

    std::cout << "\nSimulation finished." << std::endl;
    std::cout << "\n--- Final System Status ---\n";
    std::cout << paint_mixing_system.get_system_status_report();
    // Print any final logs
    const auto& final_logs = paint_mixing_system.get_logs();
    if (!final_logs.empty()) {
        std::cout << "--- Final Logs ---\n";
        for(const auto& log : final_logs) {
             std::cout << "FINAL_LOG: " << log << std::endl;
        }
    }

    return 0;
}
