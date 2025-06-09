#include "Mixer.h"
#include "Tank.h"  // Required for Tank methods if any are called internally, and for type completeness
#include "Valve.h" // Required for Valve methods if any are called internally, and for type completeness
#include <iostream> // For temporary debugging - remove later

Mixer::Mixer(std::string name, Tank& tank_ref, Valve& valve_ref)
    : name(name),
      mixing_tank(tank_ref),
      drain_valve(valve_ref),
      motor_on(false),
      target_mixing_time_seconds(0.0), // Default, should be set before use
      current_mixing_duration_seconds(0.0) {
    std::cout << "[" << name << "] Mixer created." << std::endl;
}

Mixer::~Mixer() {
    // Destructor implementation
}

void Mixer::start_motor() {
    if (target_mixing_time_seconds <= 0) {
        std::cout << "[" << name << "] WARNING: Target mixing time not set or invalid. Motor not started." << std::endl;
        return;
    }
    std::cout << "[" << name << "] Motor started. Target mixing time: " << target_mixing_time_seconds << "s." << std::endl;
    motor_on = true;
    current_mixing_duration_seconds = 0.0; // Reset duration for a fresh mix
}

void Mixer::stop_motor() {
    if (motor_on) { // Only log if it was on
        std::cout << "[" << name << "] Motor stopped. Mixing duration: " << current_mixing_duration_seconds << "s." << std::endl;
    }
    motor_on = false;
    // current_mixing_duration_seconds is not reset here, so it holds the last completed duration
}

void Mixer::update_state(double time_delta_seconds) {
    if (motor_on) {
        current_mixing_duration_seconds += time_delta_seconds;
        // std::cout << "[" << name << "] Mixing... Current duration: " << current_mixing_duration_seconds << "s." << std::endl; // Too noisy for every tick
        if (current_mixing_duration_seconds >= target_mixing_time_seconds) {
            std::cout << "[" << name << "] Target mixing time reached. Stopping motor." << std::endl;
            stop_motor();
        }
    }
}

bool Mixer::is_motor_on() const {
    return motor_on;
}

double Mixer::get_current_mixing_duration() const {
    return current_mixing_duration_seconds;
}

void Mixer::set_target_mixing_time(double seconds) {
    if (seconds > 0) {
        target_mixing_time_seconds = seconds;
        std::cout << "[" << name << "] Target mixing time set to: " << seconds << "s." << std::endl;
    } else {
        std::cout << "[" << name << "] WARNING: Invalid target mixing time: " << seconds << "s. Not set." << std::endl;
    }
}

Tank& Mixer::get_tank() {
    return mixing_tank;
}

Valve& Mixer::get_drain_valve() {
    return drain_valve;
}

const std::string& Mixer::get_name() const {
    return name;
}
