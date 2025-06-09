#ifndef SENSOR_H
#define SENSOR_H

#include "Enums.h"
#include <string>
#include <stdexcept> // Required for std::runtime_error

class Sensor {
public:
    Sensor(std::string name, SensorType type);
    ~Sensor();

    SensorType get_type() const;
    std::string get_name() const;

    void set_flow_status(SwitchStatus status);
    SwitchStatus get_flow_status() const;

    void set_pressure_psi(double pressure);
    double get_pressure_psi() const;

    void set_level_liters(double level);
    double get_level_liters() const;

private:
    std::string name;
    SensorType type;
    SwitchStatus flow_status;
    double pressure_psi;
    double level_liters;
};

#endif // SENSOR_H
