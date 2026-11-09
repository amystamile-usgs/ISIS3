#!/usr/bin/env python3
"""
Compare camera parameters between ISIS and CSM to find the actual mismatch
"""

import subprocess
import json
import sys

cube = "/Users/astamile/P01_001473_2479_XI_67N185W.cub"
isd_file = "/Users/astamile/P01_001473_2479_XI_67N185W.json"

print("=" * 80)
print("PARAMETER COMPARISON: ISIS vs CSM")
print("=" * 80)

# Load ISD
with open(isd_file) as f:
    isd = json.load(f)

print("\n1. ISD PARAMETERS (what CSM uses):")
print(f"   detector_center.sample: {isd['detector_center']['sample']}")
print(f"   detector_center.line: {isd['detector_center']['line']}")
print(f"   starting_detector_sample: {isd['starting_detector_sample']}")
print(f"   starting_detector_line: {isd['starting_detector_line']}")
print(f"   focal_length: {isd['focal_length_model']['focal_length']}")
print(f"   image_samples: {isd['image_samples']}")

# Check IK kernel (what ISIS uses)
print("\n2. IK KERNEL PARAMETERS (what ISIS uses):")
ik_kernel = subprocess.run([
    'getkey', f'from={cube}', 'grp=Kernels', 'keyword=InstrumentAddendum'
], capture_output=True, text=True)

if ik_kernel.returncode == 0:
    ik_path = ik_kernel.stdout.strip().replace('$mro', '/Volumes/isis_data/mro')
    print(f"   IK kernel: {ik_path}")

    result = subprocess.run(['grep', '-E', 'BORESIGHT|FOCAL_LENGTH', ik_path],
                          capture_output=True, text=True)
    for line in result.stdout.strip().split('\n'):
        if 'BORESIGHT' in line or 'FOCAL_LENGTH' in line:
            print(f"   {line.strip()}")

# Get what ISIS actually computes
print("\n3. ISIS CAMERA COMPUTATION (sample=2040, line=2048):")
result = subprocess.run([
    'campt', f'from={cube}', 'sample=2040', 'line=2048'
], capture_output=True, text=True)

for line in result.stdout.split('\n'):
    if any(key in line for key in ['Sample', 'Line', 'DetectorSample', 'FocalPlane']):
        print(f"   {line.strip()}")

# Get what CSM computes
print("\n4. CSM CAMERA COMPUTATION (sample=2040, line=2048):")
result = subprocess.run([
    'campt', f'from={cube}', 'sample=2040', 'line=2048', f'csm={isd_file}'
], capture_output=True, text=True)

for line in result.stdout.split('\n'):
    if any(key in line for key in ['Sample', 'Line', 'DetectorSample', 'FocalPlane']):
        print(f"   {line.strip()}")

print("\n" + "=" * 80)
print("KEY QUESTION:")
print("=" * 80)
print("Does ISIS apply starting_detector_sample differently than CSM?")
print("Or is the ISD starting_detector_sample value wrong?")
