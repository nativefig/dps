#include <cassert>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <chrono>
#include <memory>
#include <sstream>
#include <random>
#include <utility>

#include "StrView.h"

////////////////////////////////////////////////////////////////////////////////
#define EVENT_LIST                                                             \
    X(MainSwing)                                                               \
    X(OffSwing)                                                                \
    X(AngerManagement)                                                         \
    X(DeepWoundsTick)                                                          \
    X(BloodrageTick)                                                           \
    X(OverpowerProcExpire)                                                     \
    X(DeathWishExpire)                                                         \
    X(MortalStrikeCD)                                                          \
    X(BloodthirstCD)                                                           \
    X(WhirlwindCD)                                                             \
    X(OverpowerCD)                                                             \
    X(BloodrageCD)                                                             \
    X(BerserkerRageCD)                                                         \
    X(DeathWishCD)                                                             \
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

////////////////////////////////////////////////////////////////////////////////
#define DAMAGE_SOURCE_LIST \
    X(MainSwing) \
    X(OffSwing) \
    X(SwordSpec) \
    X(Bloodthirst) \
    X(MortalStrike) \
    X(DeepWounds) \
    X(Whirlwind) \
    X(Overpower)

enum DamageSource {
    #define X(NAME) DS_##NAME,
    DAMAGE_SOURCE_LIST
    #undef X
};
const char *getDamageSourceName(DamageSource ds) {
    switch (ds) {
    #define X(NAME) case DS_##NAME: return #NAME;
    DAMAGE_SOURCE_LIST
    #undef X
    }
    assert(0);
    return "";
}
const size_t NumDamageSources = 0
    #define X(NAME) + 1
    DAMAGE_SOURCE_LIST
    #undef X
    ;
////////////////////////////////////////////////////////////////////////////////

FILE *logFile = nullptr;
#if 0
void log(const char *format, ...) {
    if (logFile) {
        va_list va;
        va_start(va, format);
        vfprintf(logFile, format, va);
        va_end(va);
    }
}
#else
#define log(...)                           \
    do {                                   \
        if (logFile) {                     \
            fprintf(logFile, __VA_ARGS__); \
        }                                  \
    } while(0)
#endif

// TODO replace most uses of unsigned with size_t - should be faster?

using RNG = std::minstd_rand;

struct Context {
    RNG rng;

    Context(unsigned seed) : rng(seed) { }

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
unsigned deathWishCost = 10;
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
    X(armorMul, double, 0.80)                                                  \
                                                                               \
    /* Stats */                                                                \
                                                                               \
    X(strength, unsigned, 0)                                                   \
    X(agility, unsigned, 0)                                                    \
    X(bonusAttackPower, unsigned, 0)                                           \
    X(hitBonus, unsigned, 0)                                                   \
    X(critBonus, unsigned, 0)                                                  \
    X(hasteBonus, unsigned, 0)                                                 \
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
    X(tacticalMasteryLevel, unsigned, 0)                                       \
    X(angerManagementLevel, unsigned, 0)                                       \
    X(improvedOverpowerLevel, unsigned, 0)                                     \
    X(deepWoundsLevel, unsigned, 0)                                            \
    X(impaleLevel, unsigned, 0)                                                \
    X(twoHandSpecLevel, unsigned, 0)                                           \
    X(swordSpecLevel, unsigned, 0)                                             \
    X(axeSpecLevel, unsigned, 0)                                               \
    X(mortalStrikeLevel, unsigned, 0)                                          \
                                                                               \
    /* Fury talents */                                                         \
                                                                               \
    X(crueltyLevel, unsigned, 0)                                               \
    X(unbridledWrathLevel, unsigned, 0)                                        \
    X(improvedBattleShoutLevel, unsigned, 0)                                   \
    X(dualWieldSpecLevel, unsigned, 0)                                         \
    X(deathWishLevel, unsigned, 0)                                             \
    X(flurryLevel, unsigned, 0)                                                \
    X(improvedBerserkerRageLevel, unsigned, 0)                                 \
    X(bloodthirstLevel, unsigned, 0)                                           \

void printVal(FILE *file, double val) {
    fprintf(file, "%.2f", val);
}
void printVal(FILE *file, unsigned val) {
    fprintf(file, "%u", val);
}
void printVal(FILE *file, bool val) {
    fprintf(file, "%u", unsigned(val));
}

struct Params {
    #define X(NAME, TYPE, VALUE) TYPE NAME = VALUE;
    PARAM_LIST
    #undef X

    void print(FILE *file) const {
        fprintf(file, "Params:\n");
        #define X(NAME, TYPE, VALUE) \
        fprintf(file, "    " #NAME "="); \
        printVal(file, NAME); \
        fprintf(file, "\n");
        PARAM_LIST
        #undef X
    }
};
////////////////////////////////////////////////////////////////////////////////

struct AttackTable {
    static const size_t TableSize = NumHitKinds - 1;

    uint64_t table[TableSize] = { 0 };
    mutable size_t counts[NumHitKinds] = { 0 };

    void set(HitKind hk, double chance) {
        if (chance < 0.0) {
            chance = 0.0;
        }

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
        ++(counts[i]);
        return HitKind(i);
    }

    void print(FILE *file) const {
        (void)file;
#if 0
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
#endif
    }
    void printStats(FILE *file) const {
        size_t total = 0;
        for (size_t count : counts) {
            total += count;
        }
        fprintf(file, "{\n");
        for (size_t i = 0; i < NumHitKinds; ++i) {
            fprintf(file, "    %s: %.2f%%\n",
                    getHitKindName(HitKind(i)),
                    (double(counts[i] * 100) / total));
        }
        fprintf(file, "}\n");
    }
    void dump() const;
};
void AttackTable::dump() const { print(stderr); }

struct DPS {
    const Params p;

    const unsigned levelDelta = p.enemyLevel - 60;
    const double attackMul = p.armorMul * (1.0 + 0.01 * p.twoHandSpecLevel);
    const double glanceMul = (levelDelta < 2 ? 0.95 : levelDelta == 2 ? 0.85 : 0.65) * attackMul;
    const double whiteCritMul = 2.0 * attackMul;
    const double specialCritMul = (2.0 + (p.impaleLevel * 0.1)) * attackMul;

    const double flurryBuff = 1.0 + ((p.flurryLevel == 0) ?
                                     0.0 : 0.1 + (0.05 * (p.flurryLevel - 1)));
    const double swordSpecChance = 0.01 * p.swordSpecLevel;
    const double unbridledWrathChance = 0.08 * p.unbridledWrathLevel;

    const double fixedCritBonus = + (0.01 * (p.crueltyLevel + p.axeSpecLevel))
                                  - (0.01 * levelDelta)
                                  - (levelDelta > 2 ? 0.018 : 0.0);

    const double battleShoutAttackPower = 193 * (1.0 + 0.05 * p.improvedBattleShoutLevel);
    const double deepWoundsTickMul = (0.2 * p.deepWoundsLevel) / 4;

    const double hitBonus = p.hitBonus * 0.01;
    const double critBonus = p.critBonus * 0.01;

    const unsigned stanceSwapMaxRage = 5 * p.tacticalMasteryLevel;

    unsigned strength = p.strength;
    unsigned agility = p.agility;
    unsigned bonusAttackPower = p.bonusAttackPower;

    bool berserkerStance = true;

    double events[NumEventKinds];
    double curTime = 0.0;

    // TODO use fixed precision fraction type for this
    unsigned rage = 0;

    double deepWoundsTickDamage = 0;
    unsigned flurryCharges = 0;

    Tick<EK_DeepWoundsTick, 4, 3> deepWoundsTicks;
    Tick<EK_BloodrageTick, 10, 1> bloodrageTicks;

    Context ctx;

    std::uniform_int_distribution<unsigned> mainWeaponDamageDist;
    std::uniform_int_distribution<unsigned> offWeaponDamageDist;

    AttackTable whiteTable;
    AttackTable specialTable;
    AttackTable overpowerTable;

    struct DamageStat {
        unsigned long damage = 0;
        unsigned count = 0;
    };
    DamageStat damageStats[NumDamageSources];

    unsigned wastedRageSpillOver = 0;
    unsigned wastedRageStanceSwap = 0;
    unsigned spentRage = 0;

    DPS(const Params &params, unsigned seed) :
        p(params), ctx(seed),
        mainWeaponDamageDist(p.mainWeaponDamageMin, p.mainWeaponDamageMax),
        offWeaponDamageDist(p.offWeaponDamageMin, p.offWeaponDamageMax) {

        for (double &event : events) {
            event = DBL_MAX;
        }

        double dodgeChance = 0.05 + (levelDelta * 0.005);
        double specialMissChance = 0.05
                                   + levelDelta * 0.01
                                   + (levelDelta > 2 ? 0.01 : 0.0)
                                   - hitBonus;

        whiteTable.set(HK_Miss, specialMissChance + (p.dualWield ? 0.19 : 0.0));
        whiteTable.set(HK_Dodge, dodgeChance);
        whiteTable.set(HK_Glance, 0.1 + 0.1 * levelDelta);

        specialTable.set(HK_Miss, specialMissChance);
        specialTable.set(HK_Dodge, dodgeChance);

        overpowerTable.set(HK_Miss, specialMissChance);

        updateCritChance();

        events[EK_MainSwing] = 0.0;
        if (p.dualWield) {
            events[EK_OffSwing] = 0.0;
        }
        if (p.angerManagementLevel) {
            events[EK_AngerManagement] = 0.0;
        }
        events[EK_BloodrageCD] = 0.0;
    }

    unsigned long getTotalDamage() const {
        unsigned long result = 0;
        for (const DamageStat &d : damageStats) {
            result += d.damage;
        }
        return result;
    }

    bool isActive(EventKind ek) const {
        return events[ek] != DBL_MAX;
    }
    void clear(EventKind ek) {
        events[ek] = DBL_MAX;
    }

    void swapStance() {
        log("    %s stance\n", (berserkerStance ? "Battle" : "Berserker"));
        assert(!isActive(EK_StanceCD));
        events[EK_StanceCD] = curTime + stanceCDDuration;
        berserkerStance = !berserkerStance;
        updateCritChance();
        if (rage > stanceSwapMaxRage) {
            auto waste = rage - stanceSwapMaxRage;
            wastedRageStanceSwap += waste;
            log("    stance swap wasted %u rage\n", waste);
            rage = stanceSwapMaxRage;
        }
    }
    void trySwapStance() {
        if (!isActive(EK_StanceCD)) {
            swapStance();
        }
    }

    void addDamage(DamageSource source, double damage) {
        log("    %.2f damage\n", damage);
        damageStats[source].damage += (unsigned long)damage;
        damageStats[source].count += 1;
    }

    // TODO add speed enchant as a param
    double getMainSwingTime() const {
        return p.mainSwingTime / ((flurryCharges ? flurryBuff : 1.0) * (1.0 + 0.01 * p.hasteBonus));
    }
    double getOffSwingTime() const {
        return p.offSwingTime / ((flurryCharges ? flurryBuff : 1.0) * (1.0 + 0.01 * p.hasteBonus));
    }

    double getCritChance() const {
        return + 0.05
               + (berserkerStance ? 0.03 : 0.0)
               + ((0.01 / 20) * agility)
               + fixedCritBonus
               + critBonus;
    }

    // TODO how can we make sure that things like this stay in sync? E.g.
    // any time agility is updated, this method must be called.
    void updateCritChance() {
        double ch = getCritChance();
        whiteTable.set(HK_Crit, ch);
        specialTable.set(HK_Crit, ch);
        overpowerTable.set(HK_Crit, ch + 0.25 * p.improvedOverpowerLevel);
    }
    double getAttackPower() const {
        return strength * 2 + battleShoutAttackPower + bonusAttackPower;
    }

    void applyDeepWounds() {
        if (p.deepWoundsLevel == 0)
            return;
        deepWoundsTicks.start(*this);
        deepWoundsTickDamage = getWeaponDamage(true, /*average=*/true) *
                               deepWoundsTickMul *
                               (isActive(EK_DeathWishExpire) ? 1.2 : 1.0);
    }
    void applyFlurry() {
        if (!p.flurryLevel)
            return;
        flurryCharges = 3;
        // TODO Decide if this should update pending swings?
    }
    // TODO can this proc off misses?
    void applySwordSpec() {
        if (ctx.chance(swordSpecChance)) {
            log("    Sword spec!\n");
            weaponSwing(DS_SwordSpec);
        }
    }
    void applyUnbridledWrath() {
        if (ctx.chance(unbridledWrathChance)) {
            log("    Unbridled wrath\n");
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
        rage += r;
        if (rage > 100) {
            auto waste = rage - 100;
            wastedRageSpillOver += waste;
            log("    +%u rage, 100 total, %u wasted\n", r, waste);
            rage = 100;
        } else {
            log("    +%u rage, %u total\n", r, rage);
        }
    }
    void spendRage(unsigned r) {
        assert(rage >= r);
        spentRage += r;
        rage -= r;
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
    bool isBerserkerRageAvailable() const {
        if (!p.improvedBerserkerRageLevel)
            return false;
        if (!berserkerStance)
            return false;
        if (isActive(EK_GlobalCD))
            return false;
        if (isActive(EK_BerserkerRageCD))
            return false;
        return true;
    }
    bool isDeathWishAvailable() const {
        if (!p.deathWishLevel)
            return false;
        if (rage < deathWishCost)
            return false;
        if (isActive(EK_GlobalCD))
            return false;
        if (isActive(EK_DeathWishCD))
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
        if (!berserkerStance)
            return false;
        if (isActive(EK_WhirlwindCD))
            return false;
        if (isActive(EK_GlobalCD))
            return false;
        if (rage >= 70)
            return true;
        if (rage < whirlwindCost)
            return false;
        if (isActive(EK_OverpowerProcExpire) && (rage > stanceSwapMaxRage + 10))
            return true;
        return false;
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

    void triggerGlobalCD() {
        assert(!isActive(EK_GlobalCD));
        events[EK_GlobalCD] = curTime + globalCDDuration;
    }

    // TODO work out how rage refund works for miss/dodge/parry
    template <class AttackCallback>
    void specialAttack(DamageSource ds, unsigned cost,
                       const AttackTable &table,
                       AttackCallback &&attack) {
        spendRage(cost);
        triggerGlobalCD();
        HitKind hk = table.roll(ctx);
        log("    %s\n", getHitKindName(hk));
        double mul = 0.0;
        bool success = true;
        switch (hk) {
        case HK_Dodge:
            events[EK_OverpowerProcExpire] = curTime + overpowerProcDuration;
            // FALL THROUGH
        case HK_Miss:
        case HK_Parry:
            success = false;
            break;
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
        if (success) {
            mul *= isActive(EK_DeathWishExpire) ? 1.2 : 1.0;
            addDamage(ds, attack() * mul);
        }
        applyUnbridledWrath();
    }

    void trySpecialAttack() {
        if (isActive(EK_GlobalCD))
            return;

        if (isBerserkerRageAvailable()) {
            log("    Berserker Rage\n");
            gainRage(5 * p.improvedBerserkerRageLevel);
            events[EK_BerserkerRageCD] = curTime + 30;
            triggerGlobalCD();
        } else if (isDeathWishAvailable()) {
            log("    Death Wish\n");
            spendRage(deathWishCost);
            events[EK_DeathWishExpire] = curTime + 30;
            events[EK_DeathWishCD] = curTime + 180;
            triggerGlobalCD();
        } else if (isMortalStrikeAvailable()) {
            log("    Mortal Strike\n");
            events[EK_MortalStrikeCD] = curTime + 6;
            specialAttack(DS_MortalStrike, mortalStrikeCost, specialTable,
                          [this]() {
                return getWeaponDamage() + 160;
            });
            applySwordSpec();
        } else if (isBloodthirstAvailable()) {
            log("    Bloodthirst\n");
            events[EK_BloodthirstCD] = curTime + 6;
            specialAttack(DS_Bloodthirst, bloodthirstCost, specialTable,
                          [this]() {
                return getAttackPower() * 0.45;
            });
        } else if (isWhirlwindAvailable()) {
            log("    Whirlwind\n");
            events[EK_WhirlwindCD] = curTime + 10;
            specialAttack(DS_Whirlwind, whirlwindCost, specialTable,
                          [this]() {
                return getWeaponDamage();
            });
            // FIXME find out about this:
            //applySwordSpec();
        } else if (isOverpowerAvailable()) {
            if (berserkerStance) {
                trySwapStance();
            }
            if (!berserkerStance && isOverpowerAvailable()) {
                log("    Overpower\n");
                events[EK_OverpowerCD] = curTime + 5;
                clear(EK_OverpowerProcExpire);
                specialAttack(DS_Overpower, overpowerCost, overpowerTable,
                              [this]() {
                    return getWeaponDamage() + 35;
                });
                applySwordSpec();
                trySwapStance();
            }
        }
    }

    double getWeaponSwingRage(double damage) {
        return damage / 30.7;
    }

    void weaponSwing(DamageSource ds) {
        HitKind hk = whiteTable.roll(ctx);
        log("    %s\n", getHitKindName(hk));
        double mul = 0.0;
        bool success = true;
        switch (hk) {
        case HK_Dodge:
            events[EK_OverpowerProcExpire] = curTime + overpowerProcDuration;
            // FALL THROUGH
        case HK_Miss:
        case HK_Parry:
            success = false;
            break;
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
        if (ds == DS_OffSwing) {
            mul *= 0.5 * (1.0 + 0.05 * p.dualWieldSpecLevel);
        }
        mul *= isActive(EK_DeathWishExpire) ? 1.2 : 1.0;

        // Set next swing time after (possibly) applying flurry
        {
            auto ek = (ds == DS_OffSwing) ? EK_OffSwing : EK_MainSwing;
            auto swingTime = (ds == DS_OffSwing) ? getOffSwingTime() : getMainSwingTime();
            events[ek] = curTime + swingTime;
        }

        if (success) {
            double damage = getWeaponDamage(ds != DS_OffSwing) * mul;
            addDamage(ds, damage);
            // TODO does sword spec generate rage?
            gainRage(getWeaponSwingRage(damage));
        }

        // TODO can sword spec trigger sword spec?
        applySwordSpec();
        applyUnbridledWrath();
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

        log("%.4f %s\n", curTime, getEventName(curEvent));

        switch (curEvent) {
        case EK_MainSwing:
            if (flurryCharges) {
                --flurryCharges;
            }
            weaponSwing(DS_MainSwing);
            break;
        case EK_OffSwing:
            if (flurryCharges) {
                --flurryCharges;
            }
            weaponSwing(DS_OffSwing);
            break;
        case EK_AngerManagement:
            events[curEvent] += 3;
            gainRage(1);
            break;
        case EK_DeepWoundsTick:
            deepWoundsTicks.tick(*this);
            addDamage(DS_DeepWounds, deepWoundsTickDamage);
            break;
        case EK_BloodrageTick:
            bloodrageTicks.tick(*this);
            gainRage(1);
            break;
        case EK_MortalStrikeCD:
        case EK_BloodthirstCD:
        case EK_DeathWishCD:
        case EK_WhirlwindCD:
        case EK_OverpowerCD:
        case EK_BerserkerRageCD:
        case EK_GlobalCD:
            clear(curEvent);
            break;
        case EK_BloodrageCD:
            // Not on gcd
            events[curEvent] += 60;
            gainRage(10);
            bloodrageTicks.start(*this);
            break;
        case EK_DeathWishExpire:
            clear(curEvent);
            break;
        case EK_OverpowerProcExpire:
            clear(curEvent);
            if (!berserkerStance) {
                trySwapStance();
            }
            break;
        case EK_StanceCD:
            clear(curEvent);
            if (!berserkerStance && !isActive(EK_OverpowerProcExpire)) {
                swapStance();
            }
            break;
        }

        trySpecialAttack();
    }
}

bool parseVal(StrView str, double &out) {
    char *end = nullptr;
    double tmp = std::strtod(str.data(), &end);
    if (end != str.end())
        return false;
    out = tmp;
    return true;
}
bool parseVal(StrView str, unsigned &out) {
    char *end = nullptr;
    unsigned long tmp = strtoul(str.data(), &end, 10);
    if (end != str.end() || tmp != unsigned(tmp))
        return false;
    out = unsigned(tmp);
    return true;
}
bool parseVal(StrView str, bool &out) {
    if (str == "1") {
        out = true;
    } else if (str == "0") {
        out = false;
    } else {
        return false;
    }
    return true;
}
bool parseVal(StrView str, StrView &out) {
    out = str;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

struct ErrorImpl {
    std::stringstream message;
    bool fatal = false;

    ErrorImpl(bool fatal) : fatal(fatal) { }

    ~ErrorImpl() {
        std::string str = message.str();
        fprintf(stderr, "%s", str.c_str());
        if (fatal) {
            exit(1);
        }
    }
};
using Error = std::unique_ptr<ErrorImpl>;

Error error() {
    return Error(new ErrorImpl(false));
}
Error fatal() {
    return Error(new ErrorImpl(true));
}

////////////////////////////////////////////////////////////////////////////////

template <class T>
const Error &operator<<(const Error &err, T &&arg) {
    err->message << std::forward<T>(arg);
    return err;
}

void parseParam(StrView str, unsigned &out, StrView param) {
    if (!parseVal(str, out)) {
        fatal() << "Invalid value '" << str << "' for param '"
                << param << "'. Expected unsigned integer.\n";
    }
}
void parseParam(StrView str, double &out, StrView param) {
    if (!parseVal(str, out)) {
        fatal() << "Invalid value '" << str << "' for param '"
                << param << "'. Expected decimal.\n";
    }
}
void parseParam(StrView str, bool &out, StrView param) {
    if (!parseVal(str, out)) {
        fatal() << "Invalid value '" << str << "' for param '"
                << param << "'. Expected 0 or 1.\n";
    }
}

void parseParamArg(Params &params, StrView str) {
    auto eq = str.find('=');
    if (eq == StrView::npos) {
        fatal() << "Invalid argument '" << str << "'\n";
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
        fatal() << "Invalid param name '" << name << "'\n";
    }
}

struct ArgParser {
    const char *const *argv;
    size_t numArgs;
    size_t idx = 0;

    ArgParser(const char *const *argv, size_t numArgs) :
        argv(argv), numArgs(numArgs) { }

    bool finished() const { return idx >= numArgs; }

    StrView peek() const {
        assert(!finished());
        return argv[idx];
    }

    StrView consume() {
        assert(!finished());
        StrView result = peek();
        ++idx;
        return result;
    }

    template <class T>
    bool consume(char shortName, StrView longName, T &val) {
        assert(!finished());
        assert(!longName.empty());
        StrView arg = peek();
        StrView valStr;
        bool sep = false;

        if (arg.startswith("--") && arg.substr(2).startswith(longName)) {
            if (arg.size() == longName.size() + 2) {
                sep = true;
            } else if (arg[longName.size() + 2] == '=') {
                valStr = arg.substr(longName.size() + 3);
            } else {
                return false;
            }
        } else if (shortName && arg.size() == 2 &&
                   arg[0] == '-' && arg[1] == shortName) {
            sep = true;
        } else {
            return false;
        }
        if (sep) {
            ++idx;
            if (finished()) {
                fatal() << "Missing value for argument " << arg << "\n";
            }
            valStr = argv[idx];
        }
        if (!parseVal(valStr, val)) {
            fatal() << "Invalid value for argument "
                    << arg << ": '" << valStr << "'\n";
        }
        ++idx;
        return true;
    }
    template <class T>
    bool consume(StrView longName, T &val) {
        return consume<T>(0, longName, val);
    }

    bool consume(char shortName, StrView longName) {
        assert(!finished());
        assert(!longName.empty());
        StrView arg = peek();
        if (arg.startswith("--") && arg.substr(2) == longName) {
            // Good
        } else if (shortName && arg.size() == 2 &&
                   arg[0] == '-' && arg[1] == shortName) {
            // Good
        } else {
            return false;
        }
        ++idx;
        return true;
    }
    bool consume(StrView longName) {
        return consume(0, longName);
    }
};

enum ResultKind {
    RK_dps,
};
void emitResult(ResultKind rk, const DPS &dps) {
    switch (rk) {
    case RK_dps:
        printf("%.2f\n", dps.getTotalDamage() / dps.curTime);
        return;
    }
    assert(0);
}

int main(int argc, char **argv) {
    Params params;
    unsigned durationHours = 100;

    bool haveSeed = false;
    unsigned seed = 0;

    bool haveLog = false;
    StrView logFilename;

    ResultKind resultKind = RK_dps;

    ArgParser argParser(argv + 1, argc - 1);
    while (!argParser.finished()) {
        if (argParser.consume('v', "verbose")) {
            verbose = true;
        } else if (argParser.consume("duration", durationHours)) {
            // Pass
        } else if (argParser.consume("seed", seed)) {
            haveSeed = true;
        } else if (argParser.consume("log", logFilename)) {
            haveLog = true;
        } else if (argParser.peek().startswith("-")) {
            fatal() << "Invalid argument '" << argParser.peek() << "'\n";
        } else {
            parseParamArg(params, argParser.consume());
        }
    }

    if (!haveSeed) {
        seed = unsigned(std::chrono::system_clock::now().time_since_epoch().count());
    }

    if (haveLog) {
        const char *str = logFilename.data();
        assert(str[logFilename.size()] == '\0');
        logFile = ::fopen(str, "w");
    } else if (verbose) {
        logFile = stderr;
    }

    log("Seed: %u\n", seed);
    if (logFile) {
        params.print(logFile);
    }

    DPS dps(params, seed);
    dps.run(durationHours * 60 * 60);

    auto totalDamage = dps.getTotalDamage();
    log("Damage: %lu\n", totalDamage);
    for (unsigned i = 0; i < NumDamageSources; ++i) {
        DamageSource ds = DamageSource(i);
        const DPS::DamageStat &stat = dps.damageStats[i];
        log("    %s: %u events, %lu damage, %.2f%%\n",
            getDamageSourceName(ds), stat.count, stat.damage,
            (double(stat.damage * 100) / totalDamage));
    }

    log("Total wasted rage due to spill-over: %u\n", dps.wastedRageSpillOver);
    log("Total wasted rage due to stance swap: %u\n", dps.wastedRageStanceSwap);
    log("Total spent rage: %u\n", dps.spentRage);

    if (logFile) {
        log("White hit table ");
        dps.whiteTable.printStats(logFile);
        log("Special hit table ");
        dps.specialTable.printStats(logFile);
        log("Overpower hit table ");
        dps.overpowerTable.printStats(logFile);
    }

    emitResult(resultKind, dps);
}
