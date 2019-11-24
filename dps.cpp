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

const char *getHitKindName(HitKind hk) {
    switch (hk) {
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

// Constants ///////////////////////////////////////////////////////////////////
unsigned mortalStrikeCost = 30;
unsigned whirlwindCost = 25;
////////////////////////////////////////////////////////////////////////////////

// Parameters //////////////////////////////////////////////////////////////////
bool frontAttack = false;
bool dualWield = false;
unsigned enemyLevel = 60;
double armorMul = 0.66;

// Stats
unsigned strength = 100;
unsigned bonusAttackPower = 100;
unsigned hitBonus = 4;
unsigned critBonus = 5;

// Weapons
double swingTime = 3.3;
double weaponDamageMin = 100;
double weaponDamageMax = 200;

// Talents
unsigned twoHandSpecLevel = 3;
unsigned swordSpecLevel = 5;
unsigned axeSpecLevel = 0;
unsigned impaleLevel = 2;
unsigned improvedBattleShoutLevel = 0;
unsigned crueltyLevel = 3;
////////////////////////////////////////////////////////////////////////////////

// Derived constants ///////////////////////////////////////////////////////////
unsigned attackPower = (193 * (1.0 + 0.05 * improvedBattleShoutLevel)) +
                       strength * 2 + bonusAttackPower;
unsigned levelDelta = enemyLevel - 60;

double attackMul = armorMul * (1.0 + 0.01 * twoHandSpecLevel);
double glanceMul = (levelDelta < 2 ? 0.95 : levelDelta == 2 ? 0.85 : 0.65) * attackMul;
double whiteCritMul = 2.0 * attackMul;
double specialCritMul = 2.0 + (impaleLevel * 0.1) * attackMul;
////////////////////////////////////////////////////////////////////////////////

struct AttackTable {
    static const size_t TableSize = NumHitKinds - 1;

    uint64_t table[TableSize] = { 0 };

    void update(HitKind hk, double chance) {
        assert(size_t(hk) < TableSize);
        if (chance < 0.0) {
            chance = 0.0;
        }
        assert(chance <= 1.0);
        auto oldVal = table[hk];
        auto newVal = uint64_t(chance * UINT_MAX);
        table[hk] = newVal;
        int64_t delta = newVal - oldVal;
        for (size_t i = size_t(hk) + 1; i < TableSize; ++i) {
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

    // Round for each tick?
    unsigned deepWoundsTickDamage = 0;

    Tick<EK_DeepWoundsTick, 4, 3> deepWoundsTicks;
    Tick<EK_BloodrageTick, 10, 1> bloodrageTicks;

    Context ctx;

    AttackTable whiteTable;
    AttackTable specialTable;

    DPS() {
        for (double &event : events) {
            event = DBL_MAX;
        }

        double dodgeChance = 0.05 + (levelDelta * 0.005);
        // TODO when I implement overpower I'll need to add stance bonuses
        double critChance = 0.05
                            + 0.03 // Berserker stance
                            + (0.01 * (crueltyLevel + axeSpecLevel))
                            - (0.01 * levelDelta)
                            - (levelDelta > 2 ? 0.018 : 0.0);

        whiteTable.update(HK_Miss, 0.05 +
                        levelDelta * 0.01 +
                        (levelDelta > 2 ? 0.1 : 0.0) +
                        (dualWield ? 0.19 : 0.0));
        whiteTable.update(HK_Dodge, dodgeChance);
        whiteTable.update(HK_Glance, 0.1 + 0.1 * levelDelta);
        whiteTable.update(HK_Crit, critChance);

        specialTable.update(HK_Miss, 0.05 +
                        levelDelta * 0.01 +
                        (levelDelta > 2 ? 0.1 : 0.0));
        specialTable.update(HK_Dodge, dodgeChance);
        specialTable.update(HK_Crit, critChance);
    }

    void trySwordSpec() {
        if (ctx.chance(0.01 * swordSpecLevel)) {
            weaponSwing();
        }
    }

    // Return weapon damage without any multipliers applied
    double getWeaponDamage(bool average = false) {
        unsigned base = weaponDamageMin;
        if (average) {
            base += (weaponDamageMax - weaponDamageMin) / 2;
        } else {
            base += ctx.rand(weaponDamageMax - weaponDamageMin);
        }
        return base + ((attackPower / 14) * swingTime);
    }

    bool isMortalStrikeAvailable() const {
        if (rage < mortalStrikeCost)
            return false;
        if (events[EK_MortalStrikeCD] != DBL_MAX)
            return false;
        if (events[EK_GlobalCD] != DBL_MAX)
            return false;
        return true;
    }
    bool isWhirlwindAvailable() const {
        if (rage < whirlwindCost)
            return false;
        if (events[EK_WhirlwindCD] != DBL_MAX)
            return false;
        if (events[EK_GlobalCD] != DBL_MAX)
            return false;
        return true;
    }

    void applyDeepWounds() {
        deepWoundsTicks.start(*this);
        deepWoundsTickDamage = getWeaponDamage(/*average=*/true) * (0.6 * 0.25);
    }

    void specialAttack(double bonusDamage) {
        HitKind hk = specialTable.roll(ctx);
        double mul = 0.0;
        switch (hk) {
        case HK_Miss:
        case HK_Dodge:
        case HK_Parry:
            return;
        case HK_Glance:
            assert(0);
            break;
        case HK_Crit:
            applyDeepWounds();
            mul = specialCritMul;
            break;
        case HK_Hit:
        case HK_Block:
            mul = attackMul;
            break;
        }
        double damage = (getWeaponDamage() + bonusDamage) * mul;
        totalDamage += unsigned(damage);
    }

    // TODO overpower
    void trySpecialAttack() {
        if (isMortalStrikeAvailable()) {
            events[EK_MortalStrikeCD] = curTime + 6;
            rage -= mortalStrikeCost;
            specialAttack(160.0);
            trySwordSpec();
        } else if (isWhirlwindAvailable()) {
            events[EK_WhirlwindCD] = curTime + 10;
            rage -= whirlwindCost;
            specialAttack(0.0);
            // FIXME find out about this:
            //trySwordSpec();
        }
    }

    void gainRage(unsigned r) {
        rage = std::min(rage + r, 100U);
        trySpecialAttack();
    }

    void weaponSwing() {
        events[EK_WeaponSwing] += curTime + swingTime;

        HitKind hk = whiteTable.roll(ctx);
        if (debug) {
            std::cout << "    " << getHitKindName(hk) << "\n";
        }
        double mul = 0.0;
        switch (hk) {
        case HK_Miss:
        case HK_Dodge:
        case HK_Parry:
            return;
        case HK_Glance:
            mul = glanceMul;
            break;
        case HK_Crit:
            applyDeepWounds();
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
        case EK_WeaponSwing:
            weaponSwing();
            break;
        case EK_AngerManagement:
            events[curEvent] += 3;
            gainRage(1);
            break;
        case EK_DeepWoundsTick:
            deepWoundsTicks.tick(*this);
            totalDamage += deepWoundsTickDamage;
            break;
        case EK_BloodrageTick:
            bloodrageTicks.tick(*this);
            gainRage(1);
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

// TODO
// flurry
// overpower
