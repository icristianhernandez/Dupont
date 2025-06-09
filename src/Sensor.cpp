#include "Sensor.h"
#include <stdexcept> // Required for std::runtime_error

Sensor::Sensor(std::string name, SensorType type) : name(name), type(type), flow_status(SwitchStatus::NORMAL), pressure_psi(0.0), level_liters(0.0) {
    // Initialize based on type, or leave to default values
    if (type == SensorType::FLOW_SWITCH) {
        // Specific initialization for FLOW_SWITCH if any
    } else if (type == SensorType::PRESSURE_TRANSMITTER) {
        // Specific initialization for PRESSURE_TRANSMITTER if any
    } else if (type == SensorType::LEVEL_TRANSMITTER) {
        // Specific initialization for LEVEL_TRANSMITTER if any
    }
}

Sensor::~Sensor() {
    // Destructor implementation
}

SensorType Sensor::get_type() const {
    return type;
}

std::string Sensor::get_name() const {
    return name;
}

void Sensor::set_flow_status(SwitchStatus status) {
    if (type != SensorType::FLOW_SWITCH) {
        throw std::runtime_error("Attempted to set flow status on a non-flow switch sensor: " + name);
    }
    flow_status = status;
}

SwitchStatus Sensor::get_flow_status() const {
    if (type != SensorType::FLOW_SWITCH) {
        throw std::runtime_error("Attempted to get flow status from a non-flow switch sensor: " + name);
    }
    return flow_status;
}

void Sensor::set_pressure_psi(double pressure) {
    if (type != SensorType::PRESSURE_TRANSMITTER) {
        throw std::runtime_error("Attempted to set pressure on a non-pressure transmitter sensor: " + name);
    }
    pressure_psi = pressure;
}

double Sensor::get_pressure_psi() const {
    if (type != SensorType::PRESSURE_TRANSMITTER) {
        throw std::runtime_error("Attempted to get pressure from a non-pressure transmitter sensor: " + name);
    }
    return pressure_psi;
}

void Sensor::set_level_liters(double level) {
    if (type != SensorType::LEVEL_TRANSMITTER) {
        throw std::runtime_error("Attempted to set level on a non-level transmitter sensor: " + name);
    }
    level_liters = level;
}

double Sensor::get_level_liters() const {
    if (type != SensorType::LEVEL_TRANSMITTER) {
        throw std::runtime_error("Attempted to get level from a non-level transmitter sensor: " + name);
    }
    return level_liters;
}
