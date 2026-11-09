#!/usr/bin/env python3
"""
Test detector offset fix across multiple missions
"""

import json
import sys
sys.path.insert(0, '/opt/usgs/anaconda3/envs/isis/lib/python3.10/site-packages')
import ale

# Test cubes available
test_cubes = [
    ("CTX (affected)", "/Users/astamile/P01_001473_2479_XI_67N185W.cub"),
    ("CTX #2", "/Users/astamile/testData/P03_002289_1969_XN_16N126W.cub"),
    ("LROC NAC", "/Users/astamile/isisTestData/isis/src/lro/unitTestData/M1153718003LE.reduced.cub"),
]

print("=" * 80)
print("MISSION DETECTOR OFFSET SURVEY")
print("=" * 80)

affected = []
not_affected = []

for name, cube in test_cubes:
    print(f"\n{name}:")
    print(f"  {cube.split('/')[-1]}")

    try:
        isd_str = ale.loads(cube, props={'kernels': {}})
        isd = json.loads(isd_str) if isinstance(isd_str, str) else isd_str

        starting_sample = isd.get('starting_detector_sample', 0)
        starting_line = isd.get('starting_detector_line', 0)

        print(f"  starting_detector_sample: {starting_sample}")
        print(f"  starting_detector_line: {starting_line}")

        if starting_sample != 0 or starting_line != 0:
            print(f"  ✓ AFFECTED - should see fix")
            affected.append((name, cube))
        else:
            print(f"  ✗ Not affected")
            not_affected.append((name, cube))

    except Exception as e:
        print(f"  ERROR: {e}")

print("\n" + "=" * 80)
print("SUMMARY")
print("=" * 80)
print(f"\nAffected missions (need testing): {len(affected)}")
for name, _ in affected:
    print(f"  - {name}")

print(f"\nNot affected: {len(not_affected)}")
for name, _ in not_affected:
    print(f"  - {name}")

if affected:
    print("\n" + "=" * 80)
    print("NEXT: Test affected missions with verify_ale_isd.py")
    print("=" * 80)
    for name, cube in affected:
        print(f"\npython3 verify_ale_isd.py --cube {cube}")
