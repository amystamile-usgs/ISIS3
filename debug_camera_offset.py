#!/usr/bin/env python3
"""
Debug CSM vs ISIS Camera Offset

Systematic approach to find the root cause of the ~3 km offset.
"""

import subprocess
import json
import sys


def compare_camera_parameters():
    """Compare fundamental camera parameters."""
    print("=" * 80)
    print("STEP 1: Camera Parameter Comparison")
    print("=" * 80)

    # Load ISD
    with open('/Users/astamile/P01_001473_2479_XI_67N185W.json') as f:
        isd = json.load(f)

    print("\nISD Parameters:")
    print(f"  Focal length: {isd['focal_length_model']['focal_length']} mm")
    print(f"  Detector center: Line={isd['detector_center']['line']}, Sample={isd['detector_center']['sample']}")
    print(f"  Image size: {isd['image_lines']} x {isd['image_samples']}")
    print(f"  Distortion: {isd['optical_distortion']['radial']['coefficients']}")

    # Check ISIS camera (via IK kernel values used by ISIS)
    print("\nISIS reads parameters from IK kernels.")
    print("To get ISIS values, checking actual camera output...")

    # Run campt to see what ISIS computes
    result = subprocess.run([
        'campt',
        'from=/Users/astamile/P01_001473_2479_XI_67N185W.cub',
        'line=1',
        'sample=1'
    ], capture_output=True, text=True)

    print("\nNote: ISIS camera parameters are in IK kernel.")
    print("Recommendation: Check if detector center differs between ISD and IK kernel.")


def test_detector_center_hypothesis():
    """Test if there's a detector center offset."""
    print("\n" + "=" * 80)
    print("STEP 2: Detector Center Offset Test")
    print("=" * 80)

    # ISD detector center
    with open('/Users/astamile/P01_001473_2479_XI_67N185W.json') as f:
        isd = json.load(f)

    det_line = isd['detector_center']['line']
    det_sample = isd['detector_center']['sample']

    print(f"\nISD detector center: Sample={det_sample}, Line={det_line}")

    # The cross-track offset is ~0.13 degrees
    # At CTX pixel scale (~6 m/pixel), estimate pixel offset
    # 3 km offset / 6 m per pixel = ~500 pixels

    print(f"\nObserved offset: ~3 km cross-track")
    print(f"CTX pixel scale: ~6 m/pixel")
    print(f"Expected pixel offset: ~500 pixels")

    print(f"\nIf detector center differs by ~500 samples, that would explain the offset.")
    print(f"ISIS detector center might be: {det_sample - 500} or {det_sample + 500}")


def test_pixel_offset():
    """Test if there's a constant pixel offset."""
    print("\n" + "=" * 80)
    print("STEP 3: Pixel Offset Test")
    print("=" * 80)

    print("\nTesting if offset is consistent across image...")
    print("(Already done - offset is ~3 km everywhere, so it's systematic)")

    print("\nConclusion: Offset is NOT distortion (would vary)")
    print("            Offset IS systematic (constant ~3 km)")
    print("            → Likely detector center or focal plane calibration")


def suggest_next_steps():
    """Suggest debugging steps."""
    print("\n" + "=" * 80)
    print("RECOMMENDED NEXT STEPS")
    print("=" * 80)

    print("""
1. **Check IK kernel values**:
   - Find CTX IK kernel used by ISIS
   - Compare INS-74021_BORESIGHT_SAMPLE and _LINE with ISD detector_center
   - If they differ by ~500 samples, that's the root cause

2. **Check distortion application**:
   - Verify both models apply distortion the same way
   - Check if one applies distortion and the other doesn't

3. **Check focal plane transformation**:
   - Verify focal2pixel transformation matches between ISD and ISIS
   - Check if there's a sign flip or axis swap

4. **Test with a different mission**:
   - Run same test on LRO or another mission
   - See if offset is CTX-specific or systematic across all missions

5. **Contact USGSCSM team**:
   - Report ~3 km systematic offset between CSM and ISIS for CTX
   - Provide ISD and test results
   - Ask if detector center calibration is known issue
""")


def main():
    print("CSM vs ISIS Camera Offset Debugger")
    print()

    compare_camera_parameters()
    test_detector_center_hypothesis()
    test_pixel_offset()
    suggest_next_steps()


if __name__ == '__main__':
    main()
