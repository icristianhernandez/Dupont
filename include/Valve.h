#ifndef VALVE_H
#define VALVE_H

#include "Enums.h"

class Valve {
public:
    Valve(ValveStatus initial_status = ValveStatus::OPEN);
    ~Valve();
    void open();
    void close();
    ValveStatus get_status() const;

private:
    ValveStatus status;
};

#endif // VALVE_H
