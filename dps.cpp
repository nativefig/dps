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
#define HIT_KIND_LIST \
    X(Miss) \
    X(Dodge) \
    X(Parry) \
    X(Glance) \
    X(Block) \
    X(Crit) \
    X(Hit)

enum HitKind {
    #define X(NAME) HK_##NAME,
    HIT_KIND_LIST
    #undef X
};

const char *getHitKindName(HitKind ak) {
    switch (ak) {
    #define X(NAME) case HK_##NAME: return #NAME;
    HIT_KIND_LIST
    #undef X
    }
    assert(0);
    return "";
}
const size_t NumHitKinds = 0
    #define X(NAME) + 1
    HIT_KIND_LIST
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

    uint64_t rand() {
        return dist(random);
    }
    unsigned rand(unsigned x) {
        return (x * rand()) >> 32;
    }
    bool chance(double ch) {
        if (ch >= 1.0)
            return true;
        return uint64_t(ch * UINT_MAX) > rand();
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
unsigned twoHandSpecLevel = 3;
unsigned impaleLevel = 2;
double armorMul = 0.66;

unsigned strength = 100;
unsigned bonusAttackPower = 100;

double swingTime = 3.3;
double weaponDamageMin = 100;
double weaponDamageMax = 200;
////////////////////////////////////////////////////////////////////////////////

// TODO add battle shout bonus
unsigned attackPower = 20 + strength * 2 + bonusAttackPower;
unsigned levelDelta = enemyLevel - 60;

double attackMul = armorMul * (1.0 + 0.01 * twoHandSpecLevel);
double glanceMul = (levelDelta < 2 ? 0.95 : levelDelta == 2 ? 0.85 : 0.65) * attackMul;
double whiteCritMul = 2.0 * attackMul;
double specialCritMul = 2.2 * attackMul;
                   
struct AttackTable {
    static const size_t TableSize = NumHitKinds - 1;

    uint64_t table[TableSize] = { 0 };

    void update(HitKind ak, double chance) {
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

    HitKind roll(Context &ctx) {
        uint64_t roll = ctx.rand();
        size_t i;
        for (i = 0; i < TableSize; ++i) {
            if (table[i] > roll)
                break;
        }
        return HitKind(i);
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

        whiteTable.update(HK_Miss, 0.05 +
                        levelDelta * 0.01 +
                        (levelDelta > 2 ? 0.1 : 0.0) +
                        (dualWield ? 0.19 : 0.0));
        whiteTable.update(HK_Dodge, 0.05 + levelDelta * 0.005);
        whiteTable.update(HK_Glance, 0.1 + 0.1 * levelDelta);
        // TODO -1.8% chance to crit on lv +3 mobs
        whiteTable.update(HK_Crit, 0.05 - 0.01 * levelDelta);

        specialTable.update(HK_Miss, 0.05 +
                        levelDelta * 0.01 +
                        (levelDelta > 2 ? 0.1 : 0.0));
        specialTable.update(HK_Dodge, 0.05 + levelDelta * 0.005);
        // TODO -1.8% chance to crit on lv +3 mobs
        specialTable.update(HK_Crit, 0.05 - 0.01 * levelDelta);
    }

    void trySwordSpec() {
        // TODO
        // This resets the swing timer when it procs from a special attack
    }

    // Return weapon damage without any multipliers applied
    double getWeaponDamage() {
        return weaponDamageMin +
               ctx.rand(weaponDamageMax - weaponDamageMin) +
               ((attackPower / 14) * swingTime);
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

    void weaponSwing() {
        events[EK_WeaponSwing] += swingTime;

        HitKind ak = whiteTable.roll(ctx);
        if (debug) {
            std::cout << "    " << getHitKindName(ak) << "\n";
        }
        // TODO avoid multiplies at runtime by applying them doing them ahead of time
        double mul;
        switch (ak) {
        case HK_Miss:
        case HK_Dodge:
        case HK_Parry:
            return;
        case HK_Glance:
            mul = glanceMul;
            break;
        case HK_Crit:
            deepWoundsTicks.start(*this);
            mul = whiteCritMul;
            break;
        case HK_Hit:
        case HK_Block:
            mul = attackMul;
            break;
        }
        double damage = getWeaponDamage() * mul;
        totalDamage += unsigned(damage);
        gainRage(10);
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
    double weaponDamage = 100;
    double swingDamage = weaponDamage;
    // Apply attack power
    swingDamage += ((attackPower / 14) * swingTime);

    double mortalStrikeDamage = swingDamage + 160;
    (void)mortalStrikeDamage;

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
            weaponSwing();
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
        case EK_GlobalCD:
            events[curEvent] += DBL_MAX;
            trySpecialAttack();
            break;
        case EK_BloodrageCD:
            // Not on gcd
            events[curEvent] += 60;
            gainRage(10);
            bloodrageTicks.start(*this);
            break;
        }
    }
}

int main() {
    DPS dps;
    dps.run();
}
