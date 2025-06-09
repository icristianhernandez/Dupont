#include "Tank.h"
#include <algorithm> // For std::min and std::max
#include <stdexcept> // For std::invalid_argument

Tank::Tank(std::string name, double capacity_liters, double initial_level_liters)
    : name(name),
      capacity_liters(capacity_liters > 0 ? capacity_liters : throw std::invalid_argument("Tank capacity must be positive")),
      current_level_liters(0.0), // Initialize to 0 before clamping
      level_transmitter(name + "_LevelTransmitter", SensorType::LEVEL_TRANSMITTER) {

    // Clamp initial_level_liters to be within [0, capacity_liters]
    current_level_liters = std::max(0.0, std::min(initial_level_liters, this->capacity_liters));
    level_transmitter.set_level_liters(current_level_liters);
}

Tank::~Tank() {
    // Destructor implementation
}

void Tank::add_liquid(double amount_liters) {
    if (amount_liters < 0) {
        // Or throw std::invalid_argument("Cannot add a negative amount of liquid.");
        return;
    }
    current_level_liters = std::min(capacity_liters, current_level_liters + amount_liters);
    level_transmitter.set_level_liters(current_level_liters);
}

void Tank::remove_liquid(double amount_liters) {
    if (amount_liters < 0) {
        // Or throw std::invalid_argument("Cannot remove a negative amount of liquid.");
        return;
    }
    current_level_liters = std::max(0.0, current_level_liters - amount_liters);
    level_transmitter.set_level_liters(current_level_liters);
}

double Tank::get_current_level_liters() const {
    return current_level_liters;
}

double Tank::get_current_level_percentage() const {
    if (capacity_liters == 0) { // Should be prevented by constructor
        return 0.0;
    }
    return (current_level_liters / capacity_liters) * 100.0;
}

const std::string& Tank::get_name() const {
    return name;
}

LevelStatus Tank::get_level_status() const {
    if (current_level_liters == 0.0) {
        return LevelStatus::EMPTY;
    }
    // Using 5.0% as the threshold for LOW
    // Ensure capacity_liters is not zero to prevent division by zero,
    // though constructor should prevent capacity_liters <= 0.
    if (capacity_liters > 0 && (current_level_liters / capacity_liters) < 0.05) {
        return LevelStatus::LOW;
    }
    // Add other statuses like HIGH if needed, e.g. > 95%
    // if (capacity_liters > 0 && (current_level_liters / capacity_liters) > 0.95) {
    //     return LevelStatus::HIGH;
    // }
    return LevelStatus::NORMAL_LEVEL;
}

const Sensor& Tank::get_level_transmitter() const {
    return level_transmitter;
}
