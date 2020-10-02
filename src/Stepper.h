#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace TeensyStep
{
    class Stepper
    {
        static constexpr int32_t vMaxMax = 300000;   // largest speed possible (steps/s)
        static constexpr uint32_t aMax = 500000;     // speed up to 500kHz within 1 s (steps/s^2)
        static constexpr uint32_t vMaxDefault = 800; // should work with every motor (1 rev/sec in 1/4-step mode)
        static constexpr uint32_t vPullInOutDefault = 100;
        static constexpr uint32_t aDefault = 2500; // reasonably low (~0.5s for reaching the default speed)

     public:
        Stepper(const int StepPin, const int DirPin, const char* name = "");

        virtual Stepper& setMaxSpeed(int32_t speed);                          // steps/s
        Stepper& setPullInSpeed(int32_t speed);                               // steps/s
        Stepper& setPullInOutSpeed(int32_t pullInpeed, int32_t pullOutSpeed); // steps/s
        virtual Stepper& setAcceleration(uint32_t _a);                        // steps/s^2

        Stepper& setStepPinPolarity(int p);  // HIGH -> positive pulses, LOW -> negative pulses
        Stepper& setInverseRotation(bool b); // Change polarity of the dir pulse

        virtual void setTargetAbs(int32_t pos);   // Set target position absolute
        virtual void setTargetRel(int32_t delta); // Set target position relative to current position
        bool addTargetAbs(int32_t pos, int32_t speed = 0, int32_t pullIn = 0, int32_t pullOut = 0);
        bool addTargetRel(int32_t delta, int32_t speed = 0, int32_t pullIn = 0, int32_t pullOut = 0);
        bool nextTarget();
        void repeatTargets();
        void removeTargets();

        inline int32_t getPosition() const { return current; }
        inline void setPosition(int32_t pos) { current = pos; }
        int32_t dir;

     protected:
        // Internal representation of a target
        class Target
        {
        public:
            Target(int32_t target, int32_t speed = 0, int32_t vPullIn = -1, int32_t vPullOut = -1, bool abs = false) :
                target(target), speed(speed), vPullIn(vPullIn), vPullOut(vPullOut), abs(abs) {}
            int32_t target;
            int32_t speed;
            int32_t vPullIn;
            int32_t vPullOut;
            bool abs = false;
        };

        void loadTarget(const Target& t);

        inline void doStep();
        inline void clearStepPin() const;

        inline void setDir(int d);
        inline void toggleDir();

        // positions
        volatile int32_t current;
        volatile int32_t currentSpeed;
        volatile int32_t target;
        std::vector<Target*> targets;
        unsigned t_index = 0;

        int32_t A, B; // Bresenham paramters
        int32_t vMax;
        int32_t vPullIn, vPullOut;
        uint32_t a;

        // compare functions
        static bool cmpDelta(const Stepper* a, const Stepper* b) { return a->A > b->A; }
        static bool cmpAcc(const Stepper* a, const Stepper* b) { return a->a < b->a; }
        static bool cmpVmin(const Stepper* a, const Stepper* b) { return std::abs(a->vMax) < std::abs(b->vMax); }
        static bool cmpVmax(const Stepper* a, const Stepper* b) { return std::abs(a->vMax) > std::abs(b->vMax); }

        // Pin & Dir registers
#if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)
        volatile uint32_t* stepPinActiveReg;
        volatile uint32_t* stepPinInactiveReg;
        volatile uint32_t* dirPinCwReg;
        volatile uint32_t* dirPinCcwReg;
#else
        volatile uint8_t polarity;
        volatile uint8_t reverse;
#endif
        const int stepPin, dirPin;
        std::string name;

        // Friends
        template <typename a, typename t>
        friend class StepControlBase;

        template <typename a, typename t>
        friend class RotateControlBase;

        template <typename t>
        friend class MotorControlBase;
    };

    // Inline implementation -----------------------------------------
#if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)
    void Stepper::doStep()
    {
        *stepPinActiveReg = 1;
        current += dir;
    }

    void Stepper::clearStepPin() const
    {
        *stepPinInactiveReg = 1;
    }

    void Stepper::setDir(int d)
    {
        dir = d;
        dir == 1 ? *dirPinCwReg = 1 : *dirPinCcwReg = 1;
    }
#else
    void Stepper::doStep()
    {
        digitalWrite(stepPin, polarity);
        current += dir;
    }

    void Stepper::clearStepPin() const
    {
        digitalWrite(stepPin, !polarity);
    }

    void Stepper::setDir(int d)
    {
        dir = d;
        digitalWrite(dirPin, dir == 1 ? reverse : !reverse);
    }
#endif

    void Stepper::toggleDir()
    {
        setDir(-dir);
    }
}
