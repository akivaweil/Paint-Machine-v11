#ifndef ERROR_STATE_H
#define ERROR_STATE_H

#include "State.h"

class ErrorState : public State {
public:
    ErrorState();
    void enter() override;
    void update() override;
    void exit() override;
    const char* getName() const override;
};

#endif // ERROR_STATE_H 