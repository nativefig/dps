#!/usr/bin/env python3

import argparse
import os
from subprocess import check_call

dps_dir = os.path.dirname(os.path.abspath(__file__))
dps = os.path.join(dps_dir, "dps")

#enemyLevel=63
#armorMul=0.66

th = {
    "dualWield" : 0,
    "mainSwingTime" : 3.3,
    "mainWeaponDamageMin" : 142,
    "mainWeaponDamageMax" : 214,
}

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
    "strength" : 223,
    "agility" : 172,
    "bonusAttackPower" : 140,
    "hitBonus" : 4,
    "critBonus" : 1,
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
    "bloodthirstLevel" : 1,
}

th_fury_prot = {
    "crueltyLevel" : 5,
    "improvedBattleShoutLevel" : 5,
    "unbridledWrathLevel" : 5,
    "flurryLevel" : 5,
    "improvedBerserkerRageLevel" : 2,
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
    "improvedBerserkerRageLevel" : 0,
    "bloodthirstLevel" : 1,
}

dw_fury_prot = {
    "crueltyLevel" : 5,
    "unbridledWrathLevel" : 5,
    "improvedBattleShoutLevel" : 5,
    "dualWieldSpecLevel" : 5,
    "flurryLevel" : 5,
    "improvedBerserkerRageLevel" : 2,
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

def run_params(params):
    cmd = [ dps ]
    for k, v in params.items():
        cmd.append("{}={}".format(k, v))
    if args.verbose:
        cmd.append("--verbose")
        print(" ".join(cmd))
    check_call(cmd)

def do_run(run):
    print("Run: " + run.name)
    cmd = [ dps ]
    for k, v in run.params.items():
        cmd.append("{}={}".format(k, v))
    if args.verbose:
        cmd.append("--verbose")
        print(" ".join(cmd))
    if args.log:
        cmd.append("--log={}.text".format(run.name))
    check_call(cmd)
    print("")

def do_run_set(run):
    print("Run: " + run.name)
    for i in range(5):
        run_params(add(run.params, {"hitBonus":i}))

def main():
    parser = argparse.ArgumentParser(description="dps runner script")
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument("--log", action="store_true")
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
        Run("dw-arms", merge(dw, gear, dw_arms)),
        #Run("dw-arms-prot", merge(dw, gear, dw_arms_prot)),
        Run("dw-fury", merge(dw, gear, dw_fury)),
        #Run("dw-fury-prot", merge(dw, gear, dw_fury_prot)),
        #Run("dw-arms-fury", merge(dw, gear, dw_arms_fury)),
    ]

    for run in runs:
        do_run_set(run)

main()
