#!/usr/bin/env python3
"""
Verify CSM image<->detector transformations are consistent.
"""

import json

# Load ISD
with open('/Users/astamile/P01_001473_2479_XI_67N185W.json') as f:
    isd = json.load(f)

# Extract parameters
starting_detector_sample = isd['starting_detector_sample']
detector_sample_origin = isd['detector_center']['sample']
detector_sample_summing = 1.0  # CTX doesn't sum

print("CSM Transformation Analysis")
print("=" * 80)
print(f"starting_detector_sample: {starting_detector_sample}")
print(f"detector_sample_origin: {detector_sample_origin}")
print(f"detector_sample_summing: {detector_sample_summing}")
print()

# ============================================================================
# Ground-to-Image direction (UsgsAstroLsSensorModel.cpp line 871):
#   imageSample = (detectorSample + detectorSampleOrigin - startingDetectorSample) / summing
# ============================================================================

print("Ground-to-Image (line 871):")
print("  imageSample = (detectorSample + detectorSampleOrigin - startingDetectorSample) / summing")
print()

# Test: if detectorSample = 0, what is imageSample?
det_sample = 0.0
img_sample_g2i = (det_sample + detector_sample_origin - starting_detector_sample) / detector_sample_summing
print(f"  If detectorSample = {det_sample}")
print(f"  Then imageSample = ({det_sample} + {detector_sample_origin} - {starting_detector_sample}) / {detector_sample_summing}")
print(f"                   = {img_sample_g2i}")
print()

# ============================================================================
# Image-to-Ground direction (Utilities.cpp line 135):
#   detSample = sample * sampleSumming + startingSample
# ============================================================================

print("Image-to-Ground (Utilities.cpp line 135):")
print("  detSample = sample * sampleSumming + startingSample")
print()

# Test: if imageSample = img_sample_g2i (from above), what is detectorSample?
img_sample = img_sample_g2i
det_sample_i2g = img_sample * detector_sample_summing + starting_detector_sample
print(f"  If imageSample = {img_sample}")
print(f"  Then detSample = {img_sample} * {detector_sample_summing} + {starting_detector_sample}")
print(f"                 = {det_sample_i2g}")
print()

# ============================================================================
# CHECK: Do the transformations invert each other?
# ============================================================================

print("=" * 80)
print("ROUNDTRIP TEST:")
print(f"  Starting detectorSample: {det_sample}")
print(f"  After ground-to-image:   imageSample = {img_sample_g2i}")
print(f"  After image-to-ground:   detectorSample = {det_sample_i2g}")
print()

if abs(det_sample - det_sample_i2g) < 0.01:
    print("✓ Transformations are consistent (roundtrip works)")
else:
    print(f"✗ MISMATCH! Detector sample changed by {det_sample_i2g - det_sample}")
    print()
    print("THE BUG:")
    print("  Ground-to-image uses: imageSample = (detectorSample + origin - starting) / summing")
    print("  Image-to-ground uses: detectorSample = imageSample * summing + starting")
    print()
    print("  The inverse should be: detectorSample = imageSample * summing - origin + starting")
    print("  But it's missing the '- origin' term!")
    print()
    print(f"  Expected offset: {detector_sample_origin} pixels")
    print(f"                 = {detector_sample_origin} pixels * ~6 m/pixel")
    print(f"                 ≈ {detector_sample_origin * 0.006} km")
    print()
    print("  ^^^ This matches the observed ~3 km offset! ^^^")

print()
print("=" * 80)
print("CONCLUSION:")
print("The bug is in Utilities.cpp line 135:")
print("  Current:  detSample = sample * sampleSumming + startingSample")
print("  Should be: detSample = sample * sampleSumming - sampleOrigin + startingSample")
