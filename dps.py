#!/usr/bin/env python3

import argparse
import os
import sys
import subprocess

dps_dir = os.path.dirname(os.path.abspath(__file__))
dps = os.path.join(dps_dir, "dps")

warblade = {
    "dualWield" : 0,
    "mainSwingTime" : 3.3,
    "mainWeaponDamageMin" : 142 + 1,
    "mainWeaponDamageMax" : 214 + 22,
}

demonshear = {
    "dualWield" : 0,
    "mainSwingTime" : 3.8,
    "mainWeaponDamageMin" : 163,
    "mainWeaponDamageMax" : 246,
}

th = warblade

dw = {
    "dualWield" : 1,
    "mainSwingTime" : 2.3,
    "mainWeaponDamageMin" : 63,
    "mainWeaponDamageMax" : 118,

    "offSwingTime" : 1.8,
    "offWeaponDamageMin" : 57,
    "offWeaponDamageMax" : 87,
}

gear = {
    "strength" : 237,
    "agility" : 172,
    "bonusAttackPower" : 100,
    "hitBonus" : 4,
    "critBonus" : 4,
}

th_arms = {
    "tacticalMasteryLevel" : 5,
    "angerManagementLevel" : 1,
    "improvedOverpowerLevel" : 2,
    "deepWoundsLevel" : 3,
    "impaleLevel" : 2,
    "twoHandSpecLevel" : 5,
    "swordSpecLevel" : 5,
    "axeSpecLevel" : 0,
    "mortalStrikeLevel" : 1,
    "crueltyLevel" : 5,
    "unbridledWrathLevel" : 5,
    "improvedBattleShoutLevel" : 5,
}

th_arms_prot = {
    "tacticalMasteryLevel" : 5,
    "angerManagementLevel" : 1,
    "improvedOverpowerLevel" : 2,
    "deepWoundsLevel" : 3,
    "impaleLevel" : 2,
    "twoHandSpecLevel" : 1,
    "swordSpecLevel" : 5,
    "axeSpecLevel" : 0,
    "mortalStrikeLevel" : 1,
    "crueltyLevel" : 3,
}

th_fury = {
    "tacticalMasteryLevel" : 5,
    "angerManagementLevel" : 1,
    "improvedOverpowerLevel" : 2,
    "deepWoundsLevel" : 3,
    "impaleLevel" : 2,
    "twoHandSpecLevel" : 2, # Or imp serker rage?
    "crueltyLevel" : 5,
    "unbridledWrathLevel" : 5,
    "improvedBattleShoutLevel" : 5,
    "flurryLevel" : 5,
    "improvedBerserkerRageLevel" : 0,
    "deathWishLevel" : 1,
    "bloodthirstLevel" : 1,
}

th_fury_prot = {
    "crueltyLevel" : 5,
    "improvedBattleShoutLevel" : 5,
    "unbridledWrathLevel" : 5,
    "flurryLevel" : 5,
    "improvedBerserkerRageLevel" : 2,
    "deathWishLevel" : 1,
    "bloodthirstLevel" : 1,
}

th_arms_fury = {
    "tacticalMasteryLevel" : 5,
    "angerManagementLevel" : 1,
    "improvedOverpowerLevel" : 2,
    "deepWoundsLevel" : 3,
    "impaleLevel" : 2,
    "twoHandSpecLevel" : 5,
    "crueltyLevel" : 5,
    "unbridledWrathLevel" : 5,
    "improvedBattleShoutLevel" : 5,
}

dw_arms = {
    "tacticalMasteryLevel" : 5,
    "angerManagementLevel" : 1,
    "improvedOverpowerLevel" : 2,
    "deepWoundsLevel" : 3,
    "impaleLevel" : 2,
    "swordSpecLevel" : 5,
    "axeSpecLevel" : 0,
    "mortalStrikeLevel" : 1,
    "crueltyLevel" : 5,
    "unbridledWrathLevel" : 5,
    "improvedBattleShoutLevel" : 5,
    "dualWieldSpecLevel" : 5,
}

dw_arms_prot = {
    "tacticalMasteryLevel" : 5,
    "angerManagementLevel" : 1,
    "improvedOverpowerLevel" : 2,
    "deepWoundsLevel" : 3,
    "impaleLevel" : 2,
    "swordSpecLevel" : 5,
    "axeSpecLevel" : 0,
    "crueltyLevel" : 5,
    "unbridledWrathLevel" : 4,
}

dw_fury = {
    "tacticalMasteryLevel" : 5,
    "angerManagementLevel" : 1,
    "deepWoundsLevel" : 3,
    "impaleLevel" : 2,
    "crueltyLevel" : 5,
    "unbridledWrathLevel" : 5,
    "improvedBattleShoutLevel" : 5,
    "dualWieldSpecLevel" : 5,
    "flurryLevel" : 5,
    "improvedBerserkerRageLevel" : 2,
    "deathWishLevel" : 1,
    "bloodthirstLevel" : 1,
}

dw_fury_prot = {
    "crueltyLevel" : 5,
    "unbridledWrathLevel" : 5,
    "improvedBattleShoutLevel" : 5,
    "dualWieldSpecLevel" : 5,
    "flurryLevel" : 5,
    "improvedBerserkerRageLevel" : 2,
    "deathWishLevel" : 1,
    "bloodthirstLevel" : 1,
}

dw_arms_fury = {
    "tacticalMasteryLevel" : 5,
    "angerManagementLevel" : 1,
    "deepWoundsLevel" : 3,
    "impaleLevel" : 2,
    "improvedOverpowerLevel" : 2,
    "crueltyLevel" : 5,
    "unbridledWrathLevel" : 5,
    "improvedBattleShoutLevel" : 5,
    "dualWieldSpecLevel" : 5,
    "flurryLevel" : 5,
    "improvedBerserkerRageLevel" : 0,
}

class Run:
    def __init__(self, name, params):
        self.name = name
        self.params = params

def merge(*params_list):
    result = dict()
    for p in params_list:
        for k, v in p.items():
            result[k] = v
    return result

def add(*params_list):
    result = dict()
    for p in params_list:
        for k, v in p.items():
            if k in result:
                result[k] += v
            else:
                result[k] = v
    return result

def fatal(msg):
    sys.stderr.write(msg + "\n")
    sys.exit(1)

def run_capture(cmd):
    sys.stdout.flush()
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    out_text, _ = proc.communicate()
    exit_code = proc.returncode
    if exit_code:
        fatal("Command exited with status {0}".format(exit_code))
    return out_text.decode("utf-8").strip()

def run_params(params, log=None):
    cmd = [ args.bin ]
    cmd.append("--duration={}".format(args.duration))
    if args.verbose:
        cmd.append("--verbose")
    if log:
        cmd.append("--log={}".format(log))

    for k, v in params.items():
        cmd.append("{}={}".format(k, v))
    return run_capture(cmd)

def quick_run(run):
    print("Run: " + run.name)

    log_file = None
    if args.log:
        log_file = "{}.txt".format(run.name)

    dps = run_params(run.params, log=log_file)
    print(dps)

def full_run(run):
    print("Run: " + run.name)

    log_file = None
    if args.log:
        log_file = "{}.txt".format(run.name)

    base  = run_params(run.params, log=log_file)
    rows = [
        ["x", "0"],
        ["hit"],
        ["crit"],
        ["*10 str"],
    ]
    for row in rows[1:]:
        row.append(base)
    for i in range(1,20):
        rows[0].append(str(i))
        rows[1].append(run_params(add(run.params, {"hitBonus" : i})))
        rows[2].append(run_params(add(run.params, {"critBonus" : i})))
        rows[3].append(run_params(add(run.params, {"strength" : i * 10})))

    with open("{}.csv".format(run.name), "w") as f:
        for row in rows:
            f.write(",".join(row) + "\n")

def main():
    parser = argparse.ArgumentParser(description="dps runner script")
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument("-l", "--log", action="store_true")
    parser.add_argument("-q", "--quick", action="store_true")
    parser.add_argument("--duration", default="100")
    parser.add_argument("--bin", default=dps)
    #parser.add_argument("modes", nargs="+")

    global args
    args = parser.parse_args()

    runs = [
        #Run("2h-no-talents", merge(th, gear)),
        Run("2h-arms", merge(th, gear, th_arms)),
        Run("2h-arms-prot", merge(th, gear, th_arms_prot)),
        Run("2h-fury", merge(th, gear, th_fury)),
        #Run("2h-fury-prot", merge(th, gear, th_fury_prot)),
        #Run("2h-arms-fury", merge(th, gear, th_arms_fury)),

        #Run("dw-no-talents", merge(dw, gear)),
        #Run("dw-arms", merge(dw, gear, dw_arms)),
        #Run("dw-arms-prot", merge(dw, gear, dw_arms_prot)),
        Run("dw-fury", merge(dw, gear, dw_fury)),
        #Run("dw-fury-prot", merge(dw, gear, dw_fury_prot)),
        #Run("dw-arms-fury", merge(dw, gear, dw_arms_fury)),
    ]

    if args.quick:
        for run in runs:
            quick_run(run)
    else:
        for run in runs:
            full_run(run)

main()
