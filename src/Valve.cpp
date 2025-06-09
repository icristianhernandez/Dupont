#include "Valve.h"

Valve::Valve(ValveStatus initial_status) : status(initial_status) {
    // Constructor implementation
}

Valve::~Valve() {
    // Destructor implementation
}

void Valve::open() {
    status = ValveStatus::OPEN;
}

void Valve::close() {
    status = ValveStatus::CLOSED;
}

ValveStatus Valve::get_status() const {
    return status;
}
