#ifndef TANK_H
#define TANK_H

#include "Sensor.h"
#include "Enums.h"
#include <string>

class Tank {
public:
    Tank(std::string name, double capacity_liters, double initial_level_liters);
    ~Tank();

    void add_liquid(double amount_liters);
    void remove_liquid(double amount_liters);
    double get_current_level_liters() const;
    double get_current_level_percentage() const;
    const std::string& get_name() const;
    LevelStatus get_level_status() const;
    const Sensor& get_level_transmitter() const;

private:
    std::string name;
    double capacity_liters;
    double current_level_liters;
    Sensor level_transmitter;
};

#endif // TANK_H
