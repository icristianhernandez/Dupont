#ifndef PUMP_H
#define PUMP_H

#include "Enums.h"
#include <string>

// Forward declarations
class Valve;
class Sensor;

class Pump {
public:
    Pump(std::string name, Valve& sv, Valve& dv, Sensor& pt, Sensor& fs);
    ~Pump();

    void start();
    void stop();
    void update_state();

    PumpStatus get_status() const;
    double get_flow_rate_lpm() const;
    double get_current_pressure_psi() const;
    const std::string& get_name() const;
    bool is_stopped_due_to_overpressure() const;
    bool is_stopped_due_to_low_flow() const;

private:
    std::string name;
    PumpStatus status;
    double flow_rate_lpm;
    Valve& suction_valve;
    Valve& discharge_valve;
    Sensor& pressure_transmitter;
    Sensor& flow_switch;
    double current_pressure_psi;
    bool stopped_due_to_overpressure;
    bool stopped_due_to_low_flow;
};

#endif // PUMP_H
