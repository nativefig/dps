#include <cassert>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>

#include <chrono>
#include <iostream>
#include <random>

#include "StrView.h"

////////////////////////////////////////////////////////////////////////////////
#define EVENT_LIST                                                             \
    X(MainSwing)                                                               \
    X(OffSwing)                                                                \
    X(AngerManagement)                                                         \
    X(DeepWoundsTick)                                                          \
    X(BloodrageTick)                                                           \
    X(OverpowerProcExpire)                                                     \
    X(MortalStrikeCD)                                                          \
    X(BloodthirstCD)                                                           \
    X(WhirlwindCD)                                                             \
    X(OverpowerCD)                                                             \
    X(BloodrageCD)                                                             \
    X(StanceCD)                                                                \
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
#define HIT_KIND_LIST                                                          \
    X(Miss)                                                                    \
    X(Dodge)                                                                   \
    X(Parry)                                                                   \
    X(Glance)                                                                  \
    X(Block)                                                                   \
    X(Crit)                                                                    \
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
        if (ch <= 0.0)
            return false;
        // FIXME this doesn't account for RNG::min
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
unsigned bloodthirstCost = 30;
unsigned whirlwindCost = 25;
unsigned overpowerCost = 5;
double globalCDDuration = 1.5;
double stanceCDDuration = 1.5; // TODO is this right?
double overpowerProcDuration = 5; // TODO is this right?
////////////////////////////////////////////////////////////////////////////////

// Params //////////////////////////////////////////////////////////////////////
#define PARAM_LIST                                                             \
    X(frontAttack, bool, false)                                                \
    X(dualWield, bool, false)                                                  \
    X(enemyLevel, unsigned, 63)                                                \
    X(armorMul, double, 0.66)                                                  \
                                                                               \
    /* Stats */                                                                \
                                                                               \
    X(strength, unsigned, 400)                                                 \
    X(agility, unsigned, 50)                                                   \
    X(bonusAttackPower, unsigned, 100)                                         \
    X(hitBonus, unsigned, 4)                                                   \
    X(critBonus, unsigned, 3)                                                  \
                                                                               \
    /* Weapons */                                                              \
                                                                               \
    X(mainSwingTime, double, 3.3)                                              \
    X(mainWeaponDamageMin, unsigned, 100)                                      \
    X(mainWeaponDamageMax, unsigned, 200)                                      \
                                                                               \
    X(offSwingTime, double, 3.3)                                               \
    X(offWeaponDamageMin, unsigned, 100)                                       \
    X(offWeaponDamageMax, unsigned, 200)                                       \
                                                                               \
    /* Arms talents */                                                         \
                                                                               \
    X(tacticalMasteryLevel, unsigned, 5)                                       \
    X(improvedOverpowerLevel, unsigned, 2)                                     \
    X(deepWoundsLevel, unsigned, 3)                                            \
    X(impaleLevel, unsigned, 2)                                                \
    X(twoHandSpecLevel, unsigned, 3)                                           \
    X(swordSpecLevel, unsigned, 5)                                             \
    X(axeSpecLevel, unsigned, 0)                                               \
    X(mortalStrikeLevel, unsigned, 1)                                          \
                                                                               \
    /* Fury talents */                                                         \
                                                                               \
    X(crueltyLevel, unsigned, 3)                                               \
    X(improvedBattleShoutLevel, unsigned, 0)                                   \
    X(unbridledWrathLevel, unsigned, 0)                                        \
    X(dualWieldSpecLevel, unsigned, 0)                                         \
    X(flurryLevel, unsigned, 0)                                                \
    X(improvedBerserkerRageLevel, unsigned, 0)                                 \
    X(bloodthirstLevel, unsigned, 0)                                           \

struct Params {
    #define X(NAME, TYPE, VALUE) TYPE NAME = VALUE;
    PARAM_LIST
    #undef X
};
////////////////////////////////////////////////////////////////////////////////

struct AttackTable {
    static const size_t TableSize = NumHitKinds - 1;

    uint64_t table[TableSize] = { 0 };
    //size_t counts[NumHitKinds] = { 0 };

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

    HitKind roll(Context &ctx) const {
        uint64_t roll = ctx.rand();
        size_t i;
        for (i = 0; i < TableSize; ++i) {
            if (roll < table[i])
                break;
        }
        //++(counts[i]);
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
        (void)out;
        // TODO store these elsewhere
#if 0
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
#endif
    }
    void dump() const;
};
void AttackTable::dump() const { print(std::cerr); }

struct DPS {
    const Params p;

    const unsigned levelDelta = p.enemyLevel - 60;
    const double attackMul = p.armorMul * (1.0 + 0.01 * p.twoHandSpecLevel);
    const double glanceMul = (levelDelta < 2 ? 0.95 : levelDelta == 2 ? 0.85 : 0.65) * attackMul;
    const double whiteCritMul = 2.0 * attackMul;
    const double specialCritMul = 2.0 + (p.impaleLevel * 0.1) * attackMul;
    const double flurryBuff = 1.0 + ((p.flurryLevel == 0) ?
                                     0.0 : 0.1 + (0.05 * p.flurryLevel - 1));
    const double swordSpecChance = 0.01 * p.swordSpecLevel;
    const double unbridledWrathChance = 0.08 * p.unbridledWrathLevel;

    const double fixedCritBonus = + (0.01 * (p.crueltyLevel + p.axeSpecLevel))
                                  - (0.01 * levelDelta)
                                  - (levelDelta > 2 ? 0.018 : 0.0);

    const double battleShoutAttackPower = 193 * (1.0 + 0.05 * p.improvedBattleShoutLevel);
    const double deepWoundsTickMul = (0.2 * p.deepWoundsLevel) / 4;

    unsigned strength = p.strength;
    unsigned agility = p.agility;
    unsigned bonusAttackPower = p.bonusAttackPower;
    unsigned hitBonus = p.hitBonus;
    unsigned critBonus = p.critBonus;

    bool berserkerStance = true;

    double events[NumEventKinds];
    double curTime = 0.0;
    uint64_t totalDamage = 0;

    // TODO use fixed precision fraction type for this
    unsigned rage = 0;

    // Round for each tick or not?
    unsigned deepWoundsTickDamage = 0;
    unsigned flurryCharges = 0;

    Tick<EK_DeepWoundsTick, 4, 3> deepWoundsTicks;
    Tick<EK_BloodrageTick, 10, 1> bloodrageTicks;

    Context ctx;

    std::uniform_int_distribution<unsigned> mainWeaponDamageDist;
    std::uniform_int_distribution<unsigned> offWeaponDamageDist;

    AttackTable whiteTable;
    AttackTable specialTable;

    DPS(const Params &params) :
        p(params),
        mainWeaponDamageDist(p.mainWeaponDamageMin, p.mainWeaponDamageMax),
        offWeaponDamageDist(p.offWeaponDamageMin, p.offWeaponDamageMax) {

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

        events[EK_MainSwing] = 0.0;
        if (p.dualWield) {
            events[EK_OffSwing] = 0.0;
        }
        events[EK_AngerManagement] = 0.0;
        events[EK_BloodrageCD] = 0.0;
    }

    bool isActive(EventKind ek) const {
        return events[ek] != DBL_MAX;
    }
    void clear(EventKind ek) {
        events[ek] = DBL_MAX;
    }

    void swapStance() {
        if (verbose) {
            std::cerr << "    " << (berserkerStance ? "Battle" : "Berserker") << " stance\n";
        }
        assert(!isActive(EK_StanceCD));
        events[EK_StanceCD] = curTime + stanceCDDuration;
        berserkerStance = !berserkerStance;
        updateCritChance();
        rage = std::min(rage, 5 * p.tacticalMasteryLevel);
    }
    void trySwapStance() {
        if (!isActive(EK_StanceCD)) {
            swapStance();
        }
    }

    void addDamage(double damage) {
        if (verbose) { std::cerr << "    " << damage << " damage\n"; }
        totalDamage += uint64_t(damage);
    }

    // TODO add speed enchant as a param
    double getMainSwingTime() const {
        return p.mainSwingTime / flurryBuff;
    }
    double getOffSwingTime() const {
        return p.offSwingTime / flurryBuff;
    }

    double getCritChance() const {
        return + 0.05
               + (berserkerStance ? 0.03 : 0.0)
               + ((0.01 / 20) * agility)
               + fixedCritBonus;
    }

    // TODO how can we make sure that things like this stay in sync? E.g.
    // any time agility is updated, this method must be called.
    void updateCritChance() {
        double ch = getCritChance();
        whiteTable.set(HK_Crit, ch);
        specialTable.set(HK_Crit, ch);
    }
    double getAttackPower() const {
        return strength * 2 + battleShoutAttackPower + bonusAttackPower;
    }

    void applyDeepWounds() {
        if (p.deepWoundsLevel == 0)
            return;
        deepWoundsTicks.start(*this);
        deepWoundsTickDamage = getWeaponDamage(true, /*average=*/true) *
                               deepWoundsTickMul;
    }
    void applyFlurry() {
        if (p.flurryLevel)
            return;
        flurryCharges = 3;
        // TODO Decide if this should update pending swings?
    }
    void applySwordSpec() {
        if (ctx.chance(swordSpecChance)) {
            if (verbose) { std::cerr << "    Sword spec!\n"; }
            weaponSwing();
        }
    }
    void applyUnbridledWrath() {
        if (ctx.chance(unbridledWrathChance)) {
            gainRage(1);
        }
    }

    // Return weapon damage without any multipliers applied
    // TODO How does deep wounds work with offhand crits?
    double getWeaponDamage(bool main = true, bool average = false) {
        auto min = main ? p.mainWeaponDamageMin : p.offWeaponDamageMin;
        auto max = main ? p.mainWeaponDamageMax : p.offWeaponDamageMax;
        auto &dist = main ? mainWeaponDamageDist : offWeaponDamageDist;
        // TODO Should this be using base swing time or modified swing time? Surely base.
        auto swingTime = main ? p.mainSwingTime : p.offSwingTime;
        double base;
        if (average) {
            base = min + double(max - min) / 2;
        } else {
            base = dist(ctx.rng);
        }
        return base + ((getAttackPower() / 14) * swingTime);
    }

    void gainRage(unsigned r) {
        rage = std::min(rage + r, 100U);
        if (verbose) { std::cerr << "    +" << r << " rage, " << rage << " total\n"; }
        // FIXME doing this here could have unexpected side-effects.
        // Might be better to do this after each iteration instead, since we
        // can never do more than one special attack in a row.
        trySpecialAttack();
    }

    bool isMortalStrikeAvailable() const {
        if (!p.mortalStrikeLevel)
            return false;
        if (rage < mortalStrikeCost)
            return false;
        if (isActive(EK_MortalStrikeCD))
            return false;
        if (isActive(EK_GlobalCD))
            return false;
        return true;
    }
    bool isBloodthirstAvailable() const {
        if (!p.bloodthirstLevel)
            return false;
        if (rage < bloodthirstCost)
            return false;
        if (isActive(EK_BloodthirstCD))
            return false;
        if (isActive(EK_GlobalCD))
            return false;
        return true;
    }
    bool isWhirlwindAvailable() const {
        if (rage < whirlwindCost)
            return false;
        if (isActive(EK_WhirlwindCD))
            return false;
        if (isActive(EK_GlobalCD))
            return false;
        return true;
    }
    // Note: doesn't account for stance
    bool isOverpowerAvailable() const {
        if (rage < overpowerCost)
            return false;
        if (isActive(EK_OverpowerCD))
            return false;
        if (isActive(EK_GlobalCD))
            return false;
        return isActive(EK_OverpowerProcExpire);
    }

    // TODO work out how rage refund works for miss/dodge/parry
    template <class AttackCallback>
    void specialAttack(unsigned cost, const AttackTable &table,
                       AttackCallback &&attack) {
        assert(rage >= cost);
        rage -= cost;
        events[EK_GlobalCD] = curTime + globalCDDuration;
        HitKind hk = table.roll(ctx);
        if (verbose) { std::cerr << "    " << getHitKindName(hk) << "\n"; }
        double mul = 0.0;
        switch (hk) {
        case HK_Miss:
        case HK_Parry:
            return;
        case HK_Dodge:
            events[EK_OverpowerProcExpire] = curTime + overpowerProcDuration;
            return;
        case HK_Glance:
            assert(0);
            break;
        case HK_Crit:
            applyDeepWounds();
            applyFlurry();
            mul = specialCritMul;
            break;
        case HK_Hit:
        case HK_Block:
            mul = attackMul;
            break;
        }
        attack(mul);
        applyUnbridledWrath();
    }

    void trySpecialAttack() {
        if (isActive(EK_GlobalCD))
            return;

        // FIXME GCD gets checked 3 times here - optimise?
        if (isMortalStrikeAvailable()) {
            if (verbose) { std::cerr << "    Mortal Strike\n"; }
            events[EK_MortalStrikeCD] = curTime + 6;
            specialAttack(mortalStrikeCost, specialTable,
                          [this](double mul) {
                addDamage((getWeaponDamage() + 160) * mul);
            });
            applySwordSpec();
        } else if (isBloodthirstAvailable()) {
            if (verbose) { std::cerr << "    Bloodthirst\n"; }
            events[EK_BloodthirstCD] = curTime + 6;
            specialAttack(bloodthirstCost, specialTable,
                          [this](double mul) {
                addDamage(getAttackPower() * mul);
            });
        } else if (isWhirlwindAvailable() && rage > 50) {
            // TODO only available in serker stance
            if (verbose) { std::cerr << "    Whirlwind\n"; }
            events[EK_WhirlwindCD] = curTime + 10;
            specialAttack(whirlwindCost, specialTable,
                          [this](double mul) {
                addDamage(getWeaponDamage() * mul);
            });
            // FIXME find out about this:
            //applySwordSpec();
        } else if (isOverpowerAvailable()) {
            // TODO use whirlwind if possible before stance swap
            if (berserkerStance) {
                trySwapStance();
            }
            if (!berserkerStance) {
                if (verbose) { std::cerr << "    Overpower\n"; }
                events[EK_OverpowerCD] = curTime + 5;
                clear(EK_OverpowerProcExpire);
                AttackTable table = specialTable;
                table.set(HK_Dodge, 0);
                table.set(HK_Parry, 0);
                if (p.improvedOverpowerLevel) {
                    table.set(HK_Crit, getCritChance() + 0.25 * p.improvedOverpowerLevel);
                }
                specialAttack(overpowerCost, table,
                              [this](double mul) {
                    addDamage((getWeaponDamage() + 35) * mul);
                });
                applySwordSpec();
                trySwapStance();
            }
        }
    }

    double getWeaponSwingRage(double damage) {
        return damage / 30.7;
    }

    void weaponSwing(bool main = true) {
        HitKind hk = whiteTable.roll(ctx);
        if (verbose) { std::cerr << "    " << getHitKindName(hk) << "\n"; }
        double mul = 0.0;
        switch (hk) {
        case HK_Miss:
        case HK_Parry:
            return;
        case HK_Dodge:
            events[EK_OverpowerProcExpire] = curTime + overpowerProcDuration;
            trySpecialAttack();
            return;
        case HK_Glance:
            mul = glanceMul;
            break;
        case HK_Crit:
            applyDeepWounds();
            applyFlurry();
            mul = whiteCritMul;
            break;
        case HK_Hit:
        case HK_Block:
            mul = attackMul;
            break;
        }
        applySwordSpec();
        applyUnbridledWrath();

        // Set next swing time after (possibly) applying flurry
        events[EK_MainSwing] = curTime + (main ? getMainSwingTime() : getOffSwingTime());

        double damage = getWeaponDamage(main) * mul;
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
        dps.clear(EK);
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
        case EK_MainSwing:
            if (flurryCharges) {
                --flurryCharges;
            }
            weaponSwing(true);
            break;
        case EK_OffSwing:
            if (flurryCharges) {
                --flurryCharges;
            }
            weaponSwing(false);
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
        case EK_BloodthirstCD:
        case EK_WhirlwindCD:
        case EK_OverpowerCD:
        case EK_GlobalCD:
            clear(curEvent);
            trySpecialAttack();
            break;
        case EK_BloodrageCD:
            // Not on gcd
            events[curEvent] += 60;
            gainRage(10);
            bloodrageTicks.start(*this);
            break;
        case EK_OverpowerProcExpire:
            clear(curEvent);
            if (!berserkerStance) {
                trySwapStance();
            }
            break;
        case EK_StanceCD:
            clear(curEvent);
            if (berserkerStance) {
                trySpecialAttack();
            } else if (!isActive(EK_OverpowerProcExpire)) {
                swapStance();
            }
            break;
        }
    }
}

void parseParam(StrView str, double &out, StrView param) {
    char *end = nullptr;
    double tmp = std::strtod(str.data(), &end);
    if (end != str.end()) {
        std::cerr << "Invalid value '" << str << "' for param '"
                  << param << "'. Expected decimal.";
        exit(1);
    }
    out = tmp;
}
void parseParam(StrView str, unsigned &out, StrView param) {
    char *end = nullptr;
    unsigned long tmp = strtoul(str.data(), &end, 10);
    if (end != str.end() || tmp != unsigned(tmp)) {
        std::cerr << "Invalid value '" << str << "' for param '"
                  << param << "'. Expected unsigned integer.\n";
        exit(1);
    }
    out = unsigned(tmp);
}
void parseParam(StrView str, bool &out, StrView param) {
    if (str == "1") {
        out = true;
    } else if (str == "0") {
        out = false;
    } else {
        std::cerr << "Invalid value '" << str << "' for param '"
                  << param << "'. Expected 0 or 1.\n";
        exit(1);
    }
}

void parseParamArg(Params &params, StrView str) {
    auto eq = str.find('=');
    if (eq == StrView::npos) {
        std::cerr << "Invalid argument '" << str << "'\n";
        exit(1);
    }
    StrView name = str.substr(0, eq);
    StrView valStr = str.substr(eq + 1);

    #define X(NAME, TYPE, VALUE)               \
    if (name == #NAME) {                       \
        parseParam(valStr, params.NAME, name); \
    } else
    PARAM_LIST
    #undef X
    {
        std::cerr << "Invalid param name '" << name << "'\n";
        exit(1);
    }
}

int main(int argc, char **argv) {
    Params params;
    for (int i = 1; i < argc; ++i) {
        StrView arg = argv[i];
        if (arg.startswith("-")) {
            if (arg == "--verbose") {
                verbose = true;
            } else {
                std::cerr << "Invalid argument '" << arg << "'\n";
                exit(1);
            }
        } else {
            parseParamArg(params, argv[i]);
        }
    }

    DPS dps(params);
    double duration = 100 * 60 * 60; // TODO make this an option
    dps.run(duration);
    std::cout << "DPS: " << dps.totalDamage / duration << "\n";
    if (verbose) {
        dps.whiteTable.print(std::cerr);
        dps.whiteTable.printStats(std::cerr);
    }
}

// TODO options: rng seed, duration
// TODO print out seed on every run to help reproduce bugs
