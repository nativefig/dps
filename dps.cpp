#include <cassert>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
#define ATTACK_KIND_LIST \
    X(Miss) \
    X(Dodge) \
    X(Parry) \
    X(Glance) \
    X(Block) \
    X(Crit) \
    X(Hit)

enum AttackKind {
    #define X(NAME) AK_##NAME,
    ATTACK_KIND_LIST
    #undef X
};

const char *getAttackName(AttackKind ak) {
    switch (ak) {
    #define X(NAME) case AK_##NAME: return #NAME;
    ATTACK_KIND_LIST
    #undef X
    }
    assert(0);
    return "";
}
const size_t NumAttackKinds = 0
    #define X(NAME) + 1
    ATTACK_KIND_LIST
    #undef X
    ;
////////////////////////////////////////////////////////////////////////////////

// TODO replace unsigned with size_t - should be faster?


// TODO Deep wounds is 4 ticks at 3, 6, 9, 12 for 60% of average weapon damage
// ignoring armor Doesn't refresh or benefit from e.g. mortal strike bonus damage

struct DPS;

struct Context {
    //std::random_device rd;
    std::default_random_engine random;
    std::uniform_int_distribution<unsigned> dist;

    Context() : random(std::random_device()()) { }

    unsigned rand() {
        return dist(random);
    }

    bool chance(double ch) {
        if (ch >= 1.0)
            return true;
        return unsigned(ch * UINT_MAX) > rand();
    }
};

template <EventKind EK, unsigned NumTicks, unsigned Period>
struct Tick {
    unsigned ticks = 0;

    void start(DPS &dps);
    void tick(DPS &dps);
};

bool debug = true;

// Parameters //////////////////////////////////////////////////////////////////
bool frontAttack = false;
bool dualWield = false;
unsigned enemyLevel = 60;
double armorCoefficient = 0.66;
unsigned attackPower = 100;
unsigned twoHandSpecLevel = 3;
////////////////////////////////////////////////////////////////////////////////

unsigned levelDelta = enemyLevel - 60;

struct AttackTable {
    static const size_t TableSize = NumAttackKinds - 1;

    uint64_t table[TableSize] = { 0 };

    void update(AttackKind ak, double chance) {
        assert(size_t(ak) < TableSize);
        if (chance < 0.0) {
            chance = 0.0;
        }
        assert(chance <= 1.0);
        auto oldVal = table[ak];
        auto newVal = uint64_t(chance * UINT_MAX);
        table[ak] = newVal;
        int64_t delta = newVal - oldVal;
        for (size_t i = size_t(ak) + 1; i < TableSize; ++i) {
            table[i] += delta;
        }
    }

    AttackKind roll(Context &ctx) {
        unsigned roll = ctx.rand();
        size_t i;
        for (i = 0; i < TableSize; ++i) {
            if (table[i] > roll)
                break;
        }
        return AttackKind(i);
    }
};

// TODO rename
struct DPS {
    // TODO make a typedef for the time type and define constants for not-scheduled
    double events[NumEventKinds];
    double curTime = 0.0;
    unsigned totalDamage = 0;

    unsigned rage = 0;

    Tick<EK_DeepWoundsTick, 4, 3> deepWoundsTicks;
    Tick<EK_BloodrageTick, 10, 1> bloodrageTicks;

    Context ctx;

    AttackTable whiteTable;
    AttackTable specialTable;

    DPS() {
        for (double &event : events) {
            event = DBL_MAX;
        }

        whiteTable.update(AK_Miss, 0.05 +
                        levelDelta * 0.01 +
                        (levelDelta > 2 ? 0.1 : 0.0) +
                        (dualWield ? 0.19 : 0.0));
        whiteTable.update(AK_Dodge, 0.05 + levelDelta * 0.005);
        whiteTable.update(AK_Glance, 0.1 + 0.1 * levelDelta);
        // TODO -1.8% chance to crit on lv +3 mobs
        whiteTable.update(AK_Crit, 0.05 - 0.01 * levelDelta);

        specialTable.update(AK_Miss, 0.05 +
                        levelDelta * 0.01 +
                        (levelDelta > 2 ? 0.1 : 0.0));
        specialTable.update(AK_Dodge, 0.05 + levelDelta * 0.005);
        // TODO -1.8% chance to crit on lv +3 mobs
        specialTable.update(AK_Crit, 0.05 - 0.01 * levelDelta);
    }

    void trySwordSpec() {
        // TODO
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
            trySwordSpec();
            totalDamage += 100;
            // TODO
        } else if (isWhirlwindAvailable()) {
            events[EK_WhirlwindCD] = curTime + 10;
            //trySwordSpec(); ?????????
            // TODO
        }
    }

    void gainRage(unsigned r) {
        rage += r;
        trySpecialAttack();
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

    //swingDamage *= twoHandSpecCoefficient;
    //swingDamage *= armorCoefficient;

    double totalDamage = 0.0;

    events[EK_WeaponSwing] = 0.0;
    events[EK_AngerManagement] = 0.0;
    while (curTime < 1 * 60 * 60) {
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
        case EK_WeaponSwing: {
            events[curEvent] += swingTime;

            AttackKind ak = whiteTable.roll(ctx);
            switch (ak) {
            case AK_Miss:
            case AK_Dodge:
            case AK_Parry:
                break;
            case AK_Glance:
            case AK_Block:
            case AK_Crit:
            case AK_Hit:
                totalDamage += unsigned(swingDamage);
                break;
            }
            gainRage(10);
            break;
        }
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
