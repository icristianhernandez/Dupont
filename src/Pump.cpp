#include "Pump.h"
#include "Valve.h"   // Need full definition for Valve methods
#include "Sensor.h"  // Need full definition for Sensor methods
#include <stdexcept> // For std::invalid_argument
#include <iostream>  // For temporary debugging - remove later

Pump::Pump(std::string name, Valve& sv, Valve& dv, Sensor& pt, Sensor& fs)
    : name(name),
      status(PumpStatus::OFF),
      flow_rate_lpm(0.0),
      suction_valve(sv),
      discharge_valve(dv),
      pressure_transmitter(pt),
      flow_switch(fs),
      current_pressure_psi(0.0),
      stopped_due_to_overpressure(false),
      stopped_due_to_low_flow(false) {

    if (pressure_transmitter.get_type() != SensorType::PRESSURE_TRANSMITTER) {
        throw std::invalid_argument("Pump constructor: Pressure transmitter for pump '" + name + "' is not of type PRESSURE_TRANSMITTER.");
    }
    if (flow_switch.get_type() != SensorType::FLOW_SWITCH) {
        throw std::invalid_argument("Pump constructor: Flow switch for pump '" + name + "' is not of type FLOW_SWITCH.");
    }
    // Initialize the pressure transmitter with the pump's initial pressure
    pressure_transmitter.set_pressure_psi(current_pressure_psi);
}

Pump::~Pump() {
    // Destructor implementation
}

void Pump::start() {
    // Basic start command. More complex logic in update_state handles if it actually runs.
    // If pump is stopped due to a fault that requires manual reset or specific conditions,
    // this start command might be ignored until conditions are met (handled in update_state).
    // For now, we allow setting to ON, and update_state will immediately check conditions.
    std::cout << "[" << name << "] Received START command." << std::endl;
    status = PumpStatus::ON;
    // Reset flags if we are trying to start, assuming operator wants to retry.
    // Specific recovery logic in update_state will manage these flags more granularly.
    // stopped_due_to_overpressure = false; // Let update_state manage this based on pressure
    // stopped_due_to_low_flow = false; // Let update_state manage this
}

void Pump::stop() {
    std::cout << "[" << name << "] Received STOP command." << std::endl;
    status = PumpStatus::OFF;
    flow_rate_lpm = 0.0;
    // Pressure behavior on stop is managed in update_state
}

void Pump::update_state() {
    // --- Pre-checks for immediate stop conditions (Requirement 2) ---
    if (flow_switch.get_flow_status() == SwitchStatus::ALARM && status == PumpStatus::ON) {
        std::cout << "[" << name << "] ALARM: Low flow detected. Stopping pump." << std::endl;
        stop(); // This sets status to OFF and flow_rate_lpm to 0
        stopped_due_to_low_flow = true;
        // Pressure behavior for low flow stop is handled below in "If Pump is OFF" block
    }

    if (current_pressure_psi > 50.0 && status == PumpStatus::ON) {
        std::cout << "[" << name << "] ALARM: Overpressure detected (>50 PSI). Stopping pump." << std::endl;
        stop(); // This sets status to OFF and flow_rate_lpm to 0
        stopped_due_to_overpressure = true;
        // Pressure behavior for overpressure stop is handled below in "If Pump is OFF" block
    }

    // --- Main logic based on Pump Status ---
    if (status == PumpStatus::ON) {
        // If previously stopped for low flow, check recovery conditions (Requirement 5)
        if (stopped_due_to_low_flow) {
            if (flow_switch.get_flow_status() == SwitchStatus::NORMAL) {
                 // Assuming pressure < 20 is a permissive for restart after low flow,
                 // and discharge valve is open (otherwise it might overpressure again quickly)
                if (discharge_valve.get_status() == ValveStatus::OPEN && current_pressure_psi < 20.0) {
                    std::cout << "[" << name << "] INFO: Low flow condition cleared, pressure normal. Restarting." << std::endl;
                    stopped_due_to_low_flow = false;
                    // Status is already ON, no need to set it again unless stop() was called directly by alarm.
                    // If an alarm above called stop(), then status is OFF.
                    // This logic implies start() or direct status change is needed.
                    // For now, let's assume if we are in ON block, and low flow flag was set,
                    // and conditions are met, we clear the flag.
                } else if (discharge_valve.get_status() == ValveStatus::CLOSED) {
                    // Low flow cleared, but DV is closed. Maintain pressure, do not run.
                    // This state is tricky: pump is "ON" but can't flow.
                    // It will likely hit overpressure if DV remains closed.
                    flow_rate_lpm = 0.0; // No flow if DV is closed
                     std::cout << "[" << name << "] INFO: Low flow condition cleared, but DV closed. No flow." << std::endl;
                }
            } else {
                // Still in low flow alarm, pump should remain stopped (or stop if it was trying to run)
                 std::cout << "[" << name << "] INFO: Pump ON command, but still in low flow alarm. Will not run." << std::endl;
                flow_rate_lpm = 0.0; // No flow
                // It should have been stopped by the check at the beginning of update_state.
                // If status is still ON here, it means the stop() call in the alarm check failed or was bypassed.
                // For safety, ensure it's off.
                status = PumpStatus::OFF; // Force stop if somehow it's ON with low flow alarm
            }
        }

        // If stopped for overpressure, it cannot run until pressure drops.
        // The check at the beginning of update_state handles this by calling stop().
        // So, if status is ON here, it means it wasn't an overpressure scenario that forced a stop.
        // However, we should ensure stopped_due_to_overpressure is false if pressure is normal.
        if (current_pressure_psi <= 50.0) {
             stopped_due_to_overpressure = false; // Clear if pressure is normal
        }


        // Normal Operation (Requirement 3 & 4) - only if not stopped by a persistent fault cleared above
        if (!stopped_due_to_low_flow && !stopped_due_to_overpressure && status == PumpStatus::ON) {
            if (suction_valve.get_status() == ValveStatus::OPEN && discharge_valve.get_status() == ValveStatus::OPEN) {
                flow_rate_lpm = 100.0;
                current_pressure_psi = 33.0; // Simulate stable operating pressure
                std::cout << "[" << name << "] INFO: Running normally. SV Open, DV Open. Flow: " << flow_rate_lpm << ", Pressure: " << current_pressure_psi << std::endl;
            } else if (discharge_valve.get_status() == ValveStatus::CLOSED) {
                flow_rate_lpm = 0.0;
                current_pressure_psi += 5.0; // Pressure increases
                if (current_pressure_psi > 60.0) current_pressure_psi = 60.0; // Cap simulated increase before overpressure check
                std::cout << "[" << name << "] INFO: Running but DV Closed. Flow: " << flow_rate_lpm << ", Pressure increasing: " << current_pressure_psi << std::endl;
            } else if (suction_valve.get_status() == ValveStatus::CLOSED) {
                // Not explicitly defined, but likely leads to low flow or cavitation.
                // For now, assume it leads to low flow and will be caught by flow switch.
                flow_rate_lpm = 0.0;
                std::cout << "[" << name << "] WARNING: Suction Valve Closed while Pump ON. Expect Low Flow." << std::endl;
            }
        } else if (status == PumpStatus::ON) {
            // If status is ON but one of the fault flags is true, it means it hasn't recovered yet.
            flow_rate_lpm = 0.0; // No flow if there's an active fault condition preventing run
            std::cout << "[" << name << "] INFO: Pump ON, but fault ("
                      << (stopped_due_to_low_flow ? "LowFlow " : "")
                      << (stopped_due_to_overpressure ? "Overpressure " : "")
                      << ") still active. No flow." << std::endl;
        }

    } else { // Pump Status is OFF
        flow_rate_lpm = 0.0;

        // Requirement 4 (Overpressure recovery while OFF)
        if (stopped_due_to_overpressure) {
            if (discharge_valve.get_status() == ValveStatus::OPEN) {
                current_pressure_psi -= 10.0; // Pressure drops
                if (current_pressure_psi < 0.0) current_pressure_psi = 0.0;
                std::cout << "[" << name << "] INFO: Pump OFF (Overpressure). DV Open, pressure dropping: " << current_pressure_psi << std::endl;
            } else {
                 std::cout << "[" << name << "] INFO: Pump OFF (Overpressure). DV Closed, pressure maintained: " << current_pressure_psi << std::endl;
            }
            if (current_pressure_psi < 20.0) {
                std::cout << "[" << name << "] INFO: Overpressure condition resolved (pressure < 20 PSI)." << std::endl;
                stopped_due_to_overpressure = false; // Pump can be started again
            }
        }
        // Requirement 5 (Pressure behavior when OFF due to low flow)
        else if (stopped_due_to_low_flow) {
            if (discharge_valve.get_status() == ValveStatus::OPEN) {
                if(current_pressure_psi > 0) { // Only log if pressure actually changes
                    std::cout << "[" << name << "] INFO: Pump OFF (LowFlow). DV Open, pressure released." << std::endl;
                }
                current_pressure_psi = 0.0;
            } else { // Discharge valve is closed
                // Pressure is maintained. Do nothing to current_pressure_psi.
                std::cout << "[" << name << "] INFO: Pump OFF (LowFlow). DV Closed, pressure maintained: " << current_pressure_psi << std::endl;
            }
            // The low flow condition itself (stopped_due_to_low_flow) is reset if flow_switch becomes NORMAL
            // AND pump is commanded to start (logic within PumpStatus::ON block).
            // If flow_switch is still ALARM, this flag remains true.
            // If flow_switch is NORMAL but pump is not started, this flag also remains true until a start attempt.
            if(flow_switch.get_flow_status() == SwitchStatus::NORMAL){
                 // If pump is OFF, and the actual flow sensor is normal,
                 // it means the cause of low flow might have resolved.
                 // The flag `stopped_due_to_low_flow` will be fully reset upon a successful restart attempt.
                 std::cout << "[" << name << "] INFO: Pump OFF (LowFlow), but flow switch is now NORMAL. Ready for restart attempt." << std::endl;
            }

        } else {
            // Generic OFF state, not due to a specific latched fault
            // If discharge valve is open, pressure should ideally drop to 0 or equalize.
            // If closed, it might maintain some pressure or slowly drop.
            // For simplicity, if DV is open, pressure goes to 0.
            if (discharge_valve.get_status() == ValveStatus::OPEN) {
                if (current_pressure_psi > 0) { // Only log if pressure actually changes
                     std::cout << "[" << name << "] INFO: Pump OFF. DV Open, pressure released." << std::endl;
                }
                current_pressure_psi = 0.0;
            } else {
                 std::cout << "[" << name << "] INFO: Pump OFF. DV Closed, pressure maintained: " << current_pressure_psi << std::endl;
            }
        }
    }

    // Always update the associated pressure transmitter
    pressure_transmitter.set_pressure_psi(current_pressure_psi);
    // The flow switch is an input, so we read from it, not set it here.
    // flow_switch.set_flow_status(...) // This would be set externally or by another part of simulation
}

PumpStatus Pump::get_status() const {
    return status;
}

double Pump::get_flow_rate_lpm() const {
    return flow_rate_lpm;
}

double Pump::get_current_pressure_psi() const {
    return current_pressure_psi;
}

const std::string& Pump::get_name() const {
    return name;
}

bool Pump::is_stopped_due_to_overpressure() const {
    return stopped_due_to_overpressure;
}

bool Pump::is_stopped_due_to_low_flow() const {
    return stopped_due_to_low_flow;
}
