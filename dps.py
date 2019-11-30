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

def merge(params):
    result = dict()
    for p in params:
        for k, v in p.items():
            result[k] = v
    return result

def run(params):
    merged_params = merge(params)
    cmd = [ dps ]
    for k, v in merged_params.items():
        cmd.append("{}={}".format(k, v))
    if args.verbose:
        cmd.append("--verbose")
        print(" ".join(cmd))
    check_call(cmd)

def main():
    parser = argparse.ArgumentParser(description="dps runner script")
    parser.add_argument("-v", "--verbose", action="store_true")

    global args
    args = parser.parse_args()

    print("2H No talents")
    run([th, gear])
    print("2H Arms")
    run([th, gear, th_arms])
    print("2H Arms/Prot")
    run([th, gear, th_arms_prot])
    print("2H Fury")
    run([th, gear, th_fury])
    print("2H Fury/Prot")
    run([th, gear, th_fury_prot])
    print("2H Arms/Fury")
    run([th, gear, th_arms_fury])

    print("")

    print("DW No talents")
    run([dw, gear])
    print("DW Arms")
    run([dw, gear, dw_arms])
    print("DW Arms/Prot")
    run([dw, gear, dw_arms_prot])
    print("DW Fury")
    run([dw, gear, dw_fury])
    print("DW Fury/Prot")
    run([dw, gear, dw_fury_prot])
    print("DW Arms/Fury")
    run([dw, gear, dw_arms_fury])

main()
