#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

#define EVENT_LIST \
    X(WeaponSwing) \
    X(AngerManagement) \
    X(DeepWoundsTick) \
    X(BloodrageTick) \
    X(MortalStrikeCD) \
    X(WhirlwindCD) \
    X(BloodrageCD) \
    X(GlobalCD)

enum EventKind {
    #define X(NAME) EK_##NAME,
    EVENT_LIST
    #undef X
};

const char *getEventName(EventKind ek) {
    switch (ek) {
    #define X(NAME) case EK_##NAME: return #NAME;
    EVENT_LIST
    #undef X
    }
    assert(0);
    return "";
}

const size_t NumEventKinds = 0
    #define X(NAME) + 1
    EVENT_LIST
    #undef X
    ;

double armorCoefficient = 0.66;
// 14 attack power = 1 dps
unsigned attackPower = 100;
double twoHandSpecCoefficient = 1.03;

// TODO Deep wounds is 4 ticks at 3, 6, 9, 12 for 60% of average weapon damage
// ignoring armor Doesn't refresh or benefit from e.g. mortal strike bonus damage

struct DPS;

template <EventKind EK, unsigned NumTicks, unsigned Period>
struct Tick {
    unsigned ticks = 0;

    void start(DPS &dps);
    void tick(DPS &dps);
};

// TODO rename
struct DPS {
    // TODO make a typedef for the time type and define constants for not-scheduled
    double events[NumEventKinds];
    double curTime = 0.0;
    unsigned totalDamage = 0;

    unsigned rage = 0;

    bool debug = true;

    Tick<EK_DeepWoundsTick, 4, 3> deepWoundsTicks;
    Tick<EK_BloodrageTick, 10, 1> bloodrageTicks;

    DPS() {
        for (double &event : events) {
            event = DBL_MAX;
        }
    }

    bool isMortalStrikeAvailable() const {
        if (rage < 30)
            return false;
        if (events[EK_MortalStrikeCD] != DBL_MAX)
            return false;
        if (events[EK_GlobalCD] != DBL_MAX)
            return false;
        return true;
    }
    bool isWhirlwindAvailable() const {
        if (rage < 25)
            return false;
        if (events[EK_WhirlwindCD] != DBL_MAX)
            return false;
        if (events[EK_GlobalCD] != DBL_MAX)
            return false;
        return true;
    }

    void trySpecialAttack() {
        if (isMortalStrikeAvailable()) {
            events[EK_MortalStrikeCD] = curTime + 6;
            // TODO
        } else if (isWhirlwindAvailable()) {
            events[EK_WhirlwindCD] = curTime + 10;
            // TODO
        }
    }

    void gainRage(unsigned r) {
        rage += r;
    }

    void run();
};

// TODO should this reset the tick time if it's already active?
template <EventKind EK, unsigned NumTicks, unsigned Period>
void Tick<EK, NumTicks, Period>::start(DPS &dps) {
    ticks = NumTicks;
    dps.events[EK] = dps.curTime + Period;
}

template <EventKind EK, unsigned NumTicks, unsigned Period>
void Tick<EK, NumTicks, Period>::tick(DPS &dps) {
    assert(ticks);
    --ticks;
    if (ticks) {
        dps.events[EK] += Period;
    } else {
        dps.events[EK] = DBL_MAX;
    }
}

void DPS::run() {
    double swingTime = 3.3;
    double weaponDamage = 100;
    double swingDamage = weaponDamage;
    // Apply attack power
    swingDamage += ((attackPower / 14) * swingTime);

    double mortalStrikeDamage = swingDamage + 160;
    (void)mortalStrikeDamage;

    swingDamage *= twoHandSpecCoefficient;
    //swingDamage *= armorCoefficient;

    double totalDamage = 0.0;

    events[EK_WeaponSwing] = 0.0;
    events[EK_AngerManagement] = 0.0;
    while (curTime < 10 * 60) {
        EventKind curEvent;
        {
            size_t lowIndex = 0;
            double lowTime = events[0];
            for (size_t i = 1; i < NumEventKinds; ++i) {
                if (events[i] < lowTime) {
                    lowTime = events[i];
                    lowIndex = i;
                }
            }
            curEvent = EventKind(lowIndex);
            assert(lowTime >= curTime);
            curTime = lowTime;
        }

        if (debug) {
            std::cout << curTime << " " << getEventName(curEvent) << "\n";
        }

        switch (curEvent) {
        case EK_WeaponSwing:
            totalDamage += swingDamage;
            events[curEvent] += swingTime;
            gainRage(10);
            break;
        case EK_AngerManagement:
            events[curEvent] += 3;
            gainRage(1);
            break;
        case EK_DeepWoundsTick:
            deepWoundsTicks.tick(*this);
            gainRage(1);
            break;
        case EK_BloodrageTick:
            bloodrageTicks.tick(*this);
            break;
        case EK_MortalStrikeCD:
        case EK_WhirlwindCD:
            trySpecialAttack();
            break;
        case EK_BloodrageCD:
            // Not on gcd
            events[curEvent] += 60;
            gainRage(10);
            bloodrageTicks.start(*this);
            break;
        case EK_GlobalCD:
            trySpecialAttack();
            break;
        }
    }
}

int main() {
    DPS dps;
    dps.run();
}
