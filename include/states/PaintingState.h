#ifndef PAINTING_STATE_H
#define PAINTING_STATE_H

#include "State.h"

class PaintingState : public State {
public:
    PaintingState();
    void enter() override;
    void update() override;
    void exit() override;
    const char* getName() const override;
    
    // Method for side states to call when they complete
    void onSideCompleted();

private:
    enum PaintingSubStep {
        PS_IDLE,
        PS_START_SIDE4_PAINTING,
        PS_WAIT_FOR_SIDE4_COMPLETION,
        PS_START_SIDE3_PAINTING,
        PS_WAIT_FOR_SIDE3_COMPLETION,
        PS_START_SIDE2_PAINTING,
        PS_WAIT_FOR_SIDE2_COMPLETION,
        PS_START_SIDE1_PAINTING,
        PS_WAIT_FOR_SIDE1_COMPLETION,
        PS_PERFORM_ALL_SIDES_PAINTING,
        PS_MOVE_TO_POSITION_BEFORE_HOMING,
        PS_REQUEST_HOMING
    };
    PaintingSubStep currentStep;
};

#endif // PAINTING_STATE_H 