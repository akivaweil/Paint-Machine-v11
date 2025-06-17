#ifndef CLEANING_STATE_H
#define CLEANING_STATE_H

#include "State.h"

class CleaningState : public State {
public:
    CleaningState();
    void enter() override;
    void update() override;
    void exit() override;
    const char* getName() const override;

    void setShortMode(bool mode);

private:
    bool _isCleaning;
    bool _cleaningComplete;
    bool shortMode; // Added for short cleaning cycle
    int cleaningStep; // Current step in the cleaning process
    bool atCleanPosition; // Whether we're at the cleaning position
    bool paintGunActive; // Whether the paint gun is currently active
};

#endif // CLEANING_STATE_H 