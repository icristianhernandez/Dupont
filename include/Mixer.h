#ifndef MIXER_H
#define MIXER_H

#include "Enums.h" // For any relevant enums, though not directly used by Mixer itself yet
#include <string>

// Forward declarations
class Tank;
class Valve;

class Mixer {
public:
    Mixer(std::string name, Tank& tank_ref, Valve& valve_ref);
    ~Mixer();

    void start_motor();
    void stop_motor();
    void update_state(double time_delta_seconds);

    bool is_motor_on() const;
    double get_current_mixing_duration() const;
    void set_target_mixing_time(double seconds);
    Tank& get_tank(); // Allows access to the tank's properties
    Valve& get_drain_valve(); // Allows control of the drain valve

    const std::string& get_name() const;

private:
    std::string name;
    Tank& mixing_tank;
    Valve& drain_valve;
    bool motor_on;
    double target_mixing_time_seconds;
    double current_mixing_duration_seconds;
};

#endif // MIXER_H
