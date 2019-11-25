#include <cassert>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdint>

#include <chrono>
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

// TODO replace most uses of unsigned with size_t - should be faster?

using RNG = std::minstd_rand;

struct Context {
    RNG rng;

    Context() : rng(std::chrono::system_clock::now().time_since_epoch().count()) { }

    uint64_t rand() {
        return rng();
    }
    bool chance(double ch) {
        if (ch >= 1.0)
            return true;
        return uint64_t(ch * RNG::max()) > rand();
    }
};

struct DPS;
template <EventKind EK, unsigned NumTicks, unsigned Period>
struct Tick {
    unsigned ticks = 0;

    void start(DPS &dps);
    void tick(DPS &dps);
};

bool verbose = false;

// Constants ///////////////////////////////////////////////////////////////////
unsigned mortalStrikeCost = 30;
unsigned whirlwindCost = 25;
////////////////////////////////////////////////////////////////////////////////

struct Params {
    bool frontAttack = false;
    bool dualWield = false;
    unsigned enemyLevel = 63;
    double armorMul = 0.66;

    // Stats
    // TODO this is error prone - everything should be using the values
    // on the DPS class, it's easy to accidentally say p.strength instead
    // of strength.
    // Maybe prefix these with "gear"?
    unsigned strength = 400;
    unsigned agility = 50;
    unsigned bonusAttackPower = 100;
    unsigned hitBonus = 4;
    unsigned critBonus = 3;

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
};

////////////////////////////////////////////////////////////////////////////////

struct AttackTable {
    static const size_t TableSize = NumHitKinds - 1;

    uint64_t table[TableSize] = { 0 };
    size_t counts[NumHitKinds] = { 0 };

    void set(HitKind hk, double chance) {
        if (chance < 0.0) {
            chance = 0.0;
        }
        assert(chance <= 1.0);

        const size_t idx = size_t(hk);
        assert(idx < TableSize);

        auto prev = idx == 0 ? RNG::min() : table[idx - 1];
        auto newVal = prev + uint64_t(chance * (RNG::max() - RNG::min()));
        auto delta = int64_t(newVal) - int64_t(table[idx]);
        table[hk] = newVal;
        for (size_t i = idx + 1; i < TableSize; ++i) {
            table[i] += delta;
        }
    }

    HitKind roll(Context &ctx) {
        uint64_t roll = ctx.rand();
        size_t i;
        for (i = 0; i < TableSize; ++i) {
            if (roll < table[i])
                break;
        }
        ++(counts[i]);
        return HitKind(i);
    }

    void print(std::ostream &out) const {
        auto printRow = [this, &out](size_t i, double chance) {
            out << "  " << getHitKindName(HitKind(i)) << ": " << chance << "\n";
        };
        size_t i;
        uint64_t prev = RNG::min();
        out << "{\n";
        for (i = 0; i < TableSize; ++i) {
            auto cur = table[i];
            printRow(i, double(cur - prev) / RNG::max());
            prev = cur;
        }
        printRow(i, double(RNG::max() - table[i - 1]) / RNG::max());
        out << "}\n";
    }
    void printStats(std::ostream &out) const {
        size_t total = 0;
        for (size_t count : counts) {
            total += count;
        }
        out << "{\n";
        for (size_t i = 0; i < NumHitKinds; ++i) {
            out << "  " << getHitKindName(HitKind(i)) << ": "
                        << (double(counts[i]) / total) << "\n";
        }
        out << "}\n";
    }
    void dump() const;
};
void AttackTable::dump() const { print(std::cerr); }

// TODO rename
struct DPS {
    const Params p;

    const unsigned levelDelta = p.enemyLevel - 60;
    const double attackMul = p.armorMul * (1.0 + 0.01 * p.twoHandSpecLevel);
    const double glanceMul = (levelDelta < 2 ? 0.95 : levelDelta == 2 ? 0.85 : 0.65) * attackMul;
    const double whiteCritMul = 2.0 * attackMul;
    const double specialCritMul = 2.0 + (p.impaleLevel * 0.1) * attackMul;

    unsigned strength = p.strength;
    unsigned agility = p.agility;
    unsigned bonusAttackPower = p.bonusAttackPower;
    unsigned hitBonus = p.hitBonus;
    unsigned critBonus = p.critBonus;

    double swingTime = p.swingTime;

    bool berserkerStance = true;

    // TODO make a typedef for the time type and define constants for not-scheduled
    double events[NumEventKinds];
    double curTime = 0.0;
    uint64_t totalDamage = 0;

    // TODO use fixed precision fraction type for this
    unsigned rage = 0;

    // Round for each tick or not?
    unsigned deepWoundsTickDamage = 0;

    Tick<EK_DeepWoundsTick, 4, 3> deepWoundsTicks;
    Tick<EK_BloodrageTick, 10, 1> bloodrageTicks;

    Context ctx;

    std::uniform_int_distribution<unsigned> weaponDamageDist;

    AttackTable whiteTable;
    AttackTable specialTable;

    DPS(const Params &params) :
        p(params),
        weaponDamageDist(p.weaponDamageMin, p.weaponDamageMax) {

        for (double &event : events) {
            event = DBL_MAX;
        }

        double dodgeChance = 0.05 + (levelDelta * 0.005);
        double specialMissChance = 0.05
                                   + levelDelta * 0.01
                                   + (levelDelta > 2 ? 0.01 : 0.0);

        whiteTable.set(HK_Miss, specialMissChance + (p.dualWield ? 0.19 : 0.0));
        whiteTable.set(HK_Dodge, dodgeChance);
        whiteTable.set(HK_Glance, 0.1 + 0.1 * levelDelta);

        specialTable.set(HK_Miss, specialMissChance);
        specialTable.set(HK_Dodge, dodgeChance);

        updateCritChance();

        events[EK_WeaponSwing] = 0.0;
        events[EK_AngerManagement] = 0.0;
        events[EK_BloodrageCD] = 0.0;
    }

    void addDamage(double damage) {
        if (verbose) { std::cerr << "    " << damage << " damage\n"; }
        totalDamage += uint64_t(damage);
    }

    // TODO how can we make sure that things like this stay in sync? E.g.
    // any time agility is updated, this method must be called.
    void updateCritChance() {
        double ch = 0.05
                    + (berserkerStance ? 0.03 : 0.0)
                    + ((0.01 / 14) * agility) // FIXME 14 here is wrong
                    + (0.01 * (p.crueltyLevel + p.axeSpecLevel))
                    - (0.01 * levelDelta)
                    - (levelDelta > 2 ? 0.018 : 0.0);

        whiteTable.set(HK_Crit, ch);
        specialTable.set(HK_Crit, ch);
    }
    double getAttackPower() const {
        return strength * 2
               + (193 * (1.0 + 0.05 * p.improvedBattleShoutLevel))
               + bonusAttackPower;
    }

    void trySwordSpec() {
        if (ctx.chance(0.01 * p.swordSpecLevel)) {
            if (verbose) { std::cerr << "    Sword spec!\n"; }
            weaponSwing();
        }
    }

    // Return weapon damage without any multipliers applied
    double getWeaponDamage(bool average = false) {
        double base;
        if (average) {
            base = p.weaponDamageMin +
                    double(p.weaponDamageMax - p.weaponDamageMin) / 2;
        } else {
            base = weaponDamageDist(ctx.rng);
        }
        // TODO Should this be using base swing time or modified swing time? Surely base.
        return base + ((getAttackPower() / 14) * p.swingTime);
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
        if (verbose) { std::cerr << "    " << getHitKindName(hk) << "\n"; }
        addDamage((getWeaponDamage() + bonusDamage) * mul);
    }

    // TODO overpower
    void trySpecialAttack() {
        if (isMortalStrikeAvailable()) {
            if (verbose) { std::cerr << "    Mortal Strike\n"; }
            events[EK_MortalStrikeCD] = curTime + 6;
            rage -= mortalStrikeCost;
            specialAttack(160.0);
            trySwordSpec();
        } else if (isWhirlwindAvailable() && rage > 60) {
            if (verbose) { std::cerr << "    Whirlwind\n"; }
            events[EK_WhirlwindCD] = curTime + 10;
            rage -= whirlwindCost;
            specialAttack(0.0);
            // FIXME find out about this:
            //trySwordSpec();
        }
    }

    void gainRage(unsigned r) {
        rage = std::min(rage + r, 100U);
        if (verbose) { std::cerr << "    +" << r << " rage, " << rage << " total\n"; }
        trySpecialAttack();
    }

    double getWeaponSwingRage(double damage) {
        return damage / 30.7;
    }

    void weaponSwing() {
        events[EK_WeaponSwing] = curTime + swingTime;

        HitKind hk = whiteTable.roll(ctx);
        if (verbose) { std::cerr << "    " << getHitKindName(hk) << "\n"; }
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
        addDamage(damage);
        gainRage(getWeaponSwingRage(damage));
    }
    void run(double duration);
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

void DPS::run(double duration) {
    double endTime = curTime + duration;
    while (curTime < endTime) {
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

        if (verbose) {
            std::cerr << curTime << " " << getEventName(curEvent) << "\n";
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
            addDamage(deepWoundsTickDamage);
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
    Params params;
    DPS dps(params);
    double duration = 100 * 60 * 60;
    dps.run(duration);
    std::cout << "DPS: " << dps.totalDamage / duration << "\n";
    if (verbose) {
        dps.whiteTable.print(std::cerr);
        dps.whiteTable.printStats(std::cerr);
    }
}

// TODO
// flurry
// overpower
// bloodthirst
