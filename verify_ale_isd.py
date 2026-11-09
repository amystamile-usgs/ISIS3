#!/usr/bin/env python3
"""
ALE Camera Model Verification Script

Compares CSM camera model (using ALE ISD) against ISIS camera model (using ALE SPICE).
Both use the same shape model (ellipsoid) and SPICE source (ALE) to verify camera
model implementation parity, including surface intersections.

Tests:
- Roundtrip consistency (pixel → ground → pixel) - ISIS and CSM separately
- Surface intersections (lat/lon/ground points) - CSM vs ISIS
- Spacecraft/sun distances (frame-independent) - ISD vs ISIS
- Ephemeris times - ISD vs ISIS
"""

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Dict, Tuple, List
import shutil


def generate_isd(cube_path: str) -> Dict:
    """Generate ISD from cube using ALE."""
    print(f"Generating ISD for {cube_path}...")

    try:
        import ale
        isd_str = ale.loads(cube_path)

        # ale.loads returns a JSON string, parse it
        if isinstance(isd_str, str):
            isd = json.loads(isd_str)
        else:
            isd = isd_str

        print(f"✓ ISD generated successfully")
        print(f"  Model: {isd.get('name_model', 'Unknown')}")
        print(f"  Sensor: {isd.get('name_sensor', 'Unknown')}")
        return isd
    except Exception as e:
        print(f"ERROR: Failed to generate ISD: {e}")
        sys.exit(1)


def run_spiceinit(cube_path: str):
    """Run spiceinit on the cube with ellipsoid shape model (to match CSM)."""
    print(f"\nRunning spiceinit on {cube_path}...")

    try:
        # Force ellipsoid to match CSM shape model (CSM uses ellipsoid from ISD)
        cmd = ['spiceinit', f'from={cube_path}', 'shape=ellipsoid']
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        print("✓ spiceinit completed (using ellipsoid shape model)")
        return True
    except subprocess.CalledProcessError as e:
        print(f"ERROR: spiceinit failed: {e}")
        print(f"STDERR: {e.stderr}")
        return False


def get_camera_info(cube_path: str, line: int, isd_path: str = None) -> Dict:
    """Get camera info at a specific line (ISIS or CSM)."""
    camera_type = "CSM" if isd_path else "ISIS"
    print(f"Getting {camera_type} camera info at line {line}...")

    try:
        # Use center sample, PVL format to stdout
        cmd = ['campt', f'from={cube_path}', f'line={line}', 'type=image']
        if isd_path:
            cmd.append(f'csm={isd_path}')

        result = subprocess.run(cmd, capture_output=True, text=True, check=True)

        # Parse PVL format output (handles multi-line values)
        data = {}
        lines = result.stdout.split('\n')
        i = 0
        while i < len(lines):
            line_text = lines[i].strip()

            if '=' in line_text and not line_text.startswith('#'):
                parts = line_text.split('=', 1)
                if len(parts) == 2:
                    key = parts[0].strip()
                    value = parts[1].strip()

                    # Check if value continues on next lines (doesn't end with units or complete value)
                    # For vectors like (x, y, z) that span multiple lines
                    if value.startswith('(') and not value.endswith(')'):
                        # Multi-line vector, collect continuation lines
                        i += 1
                        while i < len(lines):
                            next_line = lines[i].strip()
                            value += ' ' + next_line
                            if ')' in next_line:
                                break
                            i += 1

                    # Remove units
                    if '<' in value:
                        value = value.split('<')[0].strip()

                    data[key] = value

            i += 1

        print(f"✓ Got ISIS camera data")
        return data

    except subprocess.CalledProcessError as e:
        print(f"ERROR: campt failed: {e}")
        print(f"STDERR: {e.stderr}")
        return {}


def test_roundtrip(cube_path: str, line: int, sample: int, isd_path: str = None) -> Tuple[bool, float, Dict]:
    """
    Test camera roundtrip: pixel → ground → pixel.

    Args:
        cube_path: Path to cube
        line: Starting line
        sample: Starting sample
        isd_path: Optional ISD for CSM (None = ISIS camera)

    Returns:
        (passed, pixel_error, details)
    """
    camera_type = "CSM" if isd_path else "ISIS"

    try:
        # Step 1: Image to ground
        cmd = ['campt', f'from={cube_path}', f'line={line}', f'sample={sample}', 'type=image']
        if isd_path:
            cmd.append(f'csm={isd_path}')

        result = subprocess.run(cmd, capture_output=True, text=True, check=True)

        # Parse lat/lon
        lat = lon = None
        for line_text in result.stdout.split('\n'):
            if 'PlanetocentricLatitude' in line_text and '=' in line_text:
                lat = float(line_text.split('=')[1].split('<')[0].strip())
            if 'PositiveEast360Longitude' in line_text and '=' in line_text:
                lon = float(line_text.split('=')[1].split('<')[0].strip())

        if lat is None or lon is None:
            return False, float('inf'), {'error': 'Could not parse lat/lon'}

        # Step 2: Ground to image
        cmd = ['campt', f'from={cube_path}', f'lat={lat}', f'lon={lon}', 'type=ground']
        if isd_path:
            cmd.append(f'csm={isd_path}')

        result = subprocess.run(cmd, capture_output=True, text=True, check=True)

        # Parse returned line/sample (be specific - match exact keywords)
        ret_line = ret_sample = None
        for line_text in result.stdout.split('\n'):
            stripped = line_text.strip()
            if stripped.startswith('Sample ') and '=' in line_text:
                ret_sample = float(line_text.split('=')[1].split('<')[0].strip())
            if stripped.startswith('Line ') and '=' in line_text:
                ret_line = float(line_text.split('=')[1].split('<')[0].strip())

        if ret_line is None or ret_sample is None:
            return False, float('inf'), {'error': 'Could not parse returned line/sample'}

        # Calculate pixel error
        pixel_error = ((ret_line - line)**2 + (ret_sample - sample)**2)**0.5

        details = {
            'start': (line, sample),
            'ground': (lat, lon),
            'end': (ret_line, ret_sample),
            'error_pixels': pixel_error
        }

        # Pass if within 0.01 pixels (subpixel accuracy)
        passed = pixel_error < 0.01

        return passed, pixel_error, details

    except subprocess.CalledProcessError as e:
        return False, float('inf'), {'error': f'campt failed: {e}'}


def compare_surface_intersections(isis_data: Dict, csm_data: Dict) -> List[Tuple[str, float, float, float]]:
    """Compare surface intersections between ISIS and CSM cameras."""
    differences = []

    # Compare latitude
    isis_lat_str = isis_data.get('PlanetocentricLatitude', '')
    csm_lat_str = csm_data.get('PlanetocentricLatitude', '')
    if isis_lat_str and csm_lat_str:
        try:
            isis_lat = float(isis_lat_str)
            csm_lat = float(csm_lat_str)
            diff = abs(isis_lat - csm_lat)
            differences.append(('Latitude', diff, csm_lat, isis_lat))
        except ValueError as e:
            print(f"  Warning: Could not parse latitude")

    # Compare longitude
    isis_lon_str = isis_data.get('PositiveEast360Longitude', '')
    csm_lon_str = csm_data.get('PositiveEast360Longitude', '')
    if isis_lon_str and csm_lon_str:
        try:
            isis_lon = float(isis_lon_str)
            csm_lon = float(csm_lon_str)
            diff = abs(isis_lon - csm_lon)
            differences.append(('Longitude', diff, csm_lon, isis_lon))
        except ValueError as e:
            print(f"  Warning: Could not parse longitude")

    # Compare body-fixed coordinate (ground point XYZ)
    isis_bf_str = isis_data.get('BodyFixedCoordinate', '')
    csm_bf_str = csm_data.get('BodyFixedCoordinate', '')
    if isis_bf_str and csm_bf_str:
        try:
            isis_bf_cleaned = isis_bf_str.strip().strip('()')
            csm_bf_cleaned = csm_bf_str.strip().strip('()')
            isis_bf = [float(v.strip()) for v in isis_bf_cleaned.split(',') if v.strip()]
            csm_bf = [float(v.strip()) for v in csm_bf_cleaned.split(',') if v.strip()]

            if len(isis_bf) == 3 and len(csm_bf) == 3:
                # Euclidean distance between ground points
                diff = sum((a - b) ** 2 for a, b in zip(csm_bf, isis_bf)) ** 0.5
                differences.append(('GroundPoint', diff, csm_bf, isis_bf))
        except ValueError as e:
            print(f"  Warning: Could not parse BodyFixedCoordinate")

    return differences


def compare_ground_points(isd: Dict, isis_data: Dict, line_idx: int) -> List[Tuple[str, float, float, float]]:
    """Compare ground points (lat/lon) - frame independent."""
    differences = []

    # Note: We can't directly compare spacecraft/sun positions because they're in different
    # reference frames (ISD uses J2000, ISIS uses body-fixed). Instead, compare derived
    # quantities like ground coordinates which should match.

    # Compare planetocentric latitude
    isis_lat_str = isis_data.get('PlanetocentricLatitude', '')
    if isis_lat_str:
        try:
            isis_lat = float(isis_lat_str)
            # For now, we don't have ground point in ISD to compare
            # This would require running CSM model's imageToGround
        except ValueError:
            pass

    # Compare spacecraft altitude/distance as a frame-independent check
    if 'instrument_position' in isd:
        isd_pos = isd['instrument_position']
        if 'positions' in isd_pos and len(isd_pos['positions']) > line_idx:
            isd_sc_pos = isd_pos['positions'][line_idx]

            # Calculate distance from Mars center (frame-independent)
            # Note: ISD positions are in km, not meters
            isd_dist_km = sum(x**2 for x in isd_sc_pos) ** 0.5

            # ISIS reports TargetCenterDistance
            isis_dist_str = isis_data.get('TargetCenterDistance', '')
            if isis_dist_str:
                try:
                    isis_dist_km = float(isis_dist_str)
                    diff = abs(isd_dist_km - isis_dist_km)
                    differences.append(('TargetCenterDistance', diff, isd_dist_km, isis_dist_km))
                except ValueError as e:
                    print(f"  Warning: Could not parse TargetCenterDistance: '{isis_dist_str}'")

    # Compare sun distance (frame-independent)
    if 'sun_position' in isd:
        isd_sun = isd['sun_position']
        if 'positions' in isd_sun and len(isd_sun['positions']) > 0:
            # Use first sun position (doesn't change much over short observation)
            # Note: ISD positions are in km, not meters
            isd_sun_pos = isd_sun['positions'][0]
            isd_sun_dist_km = sum(x**2 for x in isd_sun_pos) ** 0.5

            # ISIS reports SolarDistance in AU
            isis_sun_dist_str = isis_data.get('SolarDistance', '')
            if isis_sun_dist_str:
                try:
                    isis_sun_dist_au = float(isis_sun_dist_str)
                    isis_sun_dist_km = isis_sun_dist_au * 149597870.7  # AU to km

                    diff = abs(isd_sun_dist_km - isis_sun_dist_km)
                    differences.append(('SolarDistance', diff, isd_sun_dist_km, isis_sun_dist_km))
                except ValueError as e:
                    print(f"  Warning: Could not parse SolarDistance: '{isis_sun_dist_str}'")

    return differences


def compare_times(isd: Dict, isis_data: Dict, line_idx: int) -> List[Tuple[str, float, float, float]]:
    """Compare ephemeris times."""
    differences = []

    if 'instrument_position' in isd and 'ephemeris_times' in isd['instrument_position']:
        et_list = isd['instrument_position']['ephemeris_times']
        if len(et_list) > line_idx:
            isd_et = et_list[line_idx]

            isis_et_str = isis_data.get('EphemerisTime', '')
            if isis_et_str:
                isis_et = float(isis_et_str)
                diff = abs(isd_et - isis_et)
                differences.append(('EphemerisTime', diff, isd_et, isis_et))

    return differences


def verify_ale_driver(cube_path: str, test_lines: List[int] = None, tolerance: float = 1.0):
    """
    Verify ALE driver by comparing ISD against ISIS camera.

    Args:
        cube_path: Path to input ISIS cube
        test_lines: List of line numbers to test
        tolerance: Acceptable difference in km for positions
    """
    print("=" * 80)
    print("ALE Camera Model Verification")
    print("=" * 80)
    print(f"Cube: {cube_path}")
    print(f"Position Tolerance: {tolerance} km")
    print(f"Lat/Lon Tolerance: 0.0001° (~10m)")
    print(f"Ground Point Tolerance: 10m")
    print("Shape Model: Ellipsoid (both ISIS and CSM)")

    # Copy cube to temp location to avoid modifying original
    with tempfile.TemporaryDirectory() as tmpdir:
        temp_cube = Path(tmpdir) / Path(cube_path).name
        shutil.copy(cube_path, temp_cube)
        print(f"\nWorking with temporary copy: {temp_cube}")

        # Generate ISD
        isd = generate_isd(str(temp_cube))

        # Save ISD to temp directory for CSM campt
        isd_file = Path(tmpdir) / "isd.json"
        with open(isd_file, 'w') as f:
            json.dump(isd, f, indent=2)

        # Run spiceinit
        if not run_spiceinit(str(temp_cube)):
            print("\n✗ Verification FAILED - could not initialize SPICE")
            return False

        # Determine test lines
        if test_lines is None:
            # Test beginning, middle, end
            num_lines = isd.get('image_lines', 1000)
            test_lines = [1, num_lines // 2, num_lines]

        print(f"\nTesting at lines: {test_lines}")

        # First, test roundtrip consistency for each camera model
        print("\n" + "=" * 80)
        print("ROUNDTRIP TESTS (Camera → Ground → Camera)")
        print("=" * 80)

        sample = isd.get('image_samples', 4080) // 2  # Center sample

        roundtrip_results = {'ISIS': [], 'CSM': []}

        for test_line in test_lines[:2]:  # Test first two lines for roundtrip
            print(f"\nTesting roundtrip at line {test_line}, sample {sample}")
            print("─" * 60)

            # Test ISIS roundtrip
            isis_passed, isis_error, isis_details = test_roundtrip(str(temp_cube), test_line, sample)
            roundtrip_results['ISIS'].append((test_line, isis_passed, isis_error))
            status = "✓ PASS" if isis_passed else "✗ FAIL"
            print(f"ISIS roundtrip:  {isis_error:>8.4f} pixels  {status}")
            if not isis_passed and isis_error < float('inf'):
                print(f"  Start: Line={isis_details['start'][0]}, Sample={isis_details['start'][1]}")
                print(f"  End:   Line={isis_details['end'][0]:.4f}, Sample={isis_details['end'][1]:.4f}")

            # Test CSM roundtrip
            csm_passed, csm_error, csm_details = test_roundtrip(str(temp_cube), test_line, sample, str(isd_file))
            roundtrip_results['CSM'].append((test_line, csm_passed, csm_error))
            status = "✓ PASS" if csm_passed else "✗ FAIL"
            print(f"CSM roundtrip:   {csm_error:>8.4f} pixels  {status}")
            if not csm_passed and csm_error < float('inf'):
                print(f"  Start: Line={csm_details['start'][0]}, Sample={csm_details['start'][1]}")
                print(f"  End:   Line={csm_details['end'][0]:.4f}, Sample={csm_details['end'][1]:.4f}")

        # Roundtrip summary
        isis_roundtrip_passed = all(passed for _, passed, _ in roundtrip_results['ISIS'])
        csm_roundtrip_passed = all(passed for _, passed, _ in roundtrip_results['CSM'])

        print("\n" + "─" * 60)
        print("Roundtrip Summary:")
        print(f"  ISIS: {'✓ All passed' if isis_roundtrip_passed else '✗ Some failed'}")
        print(f"  CSM:  {'✓ All passed' if csm_roundtrip_passed else '✗ Some failed'}")

        print("\n" + "=" * 80)
        print("CAMERA MODEL COMPARISON (CSM vs ISIS)")
        print("=" * 80)

        # Compare at each line
        all_passed = True
        results = []

        for line in test_lines:
            print(f"\n{'─' * 80}")
            print(f"Testing line {line}")
            print('─' * 80)

            # Get ISIS camera data
            isis_data = get_camera_info(str(temp_cube), line)
            if not isis_data:
                print(f"✗ Failed to get ISIS data for line {line}")
                all_passed = False
                continue

            # Get CSM camera data (using ISD)
            csm_data = get_camera_info(str(temp_cube), line, str(isd_file))
            if not csm_data:
                print(f"✗ Failed to get CSM data for line {line}")
                all_passed = False
                continue

            # Calculate line index (0-based) for ISD arrays
            line_idx = line - 1

            # Compare surface intersections (ISIS vs CSM)
            surface_diffs = compare_surface_intersections(isis_data, csm_data)

            # Compare frame-independent quantities (ISD vs ISIS)
            pos_diffs = compare_ground_points(isd, isis_data, line_idx)

            # Compare times (ISD vs ISIS)
            time_diffs = compare_times(isd, isis_data, line_idx)

            # Combine all differences
            all_diffs = surface_diffs + pos_diffs + time_diffs

            if not all_diffs:
                print("⚠ No comparable data found")
                continue

            print(f"\n{'Field':<30} {'Difference':<15} {'Pass':<10}")
            print('─' * 65)

            line_passed = True
            for field, diff, isd_val, isis_val in all_diffs:
                # Set tolerance based on field type
                if 'Time' in field:
                    field_tol = 1e-3  # 1 millisecond for times
                elif 'SolarDistance' in field:
                    field_tol = 5000  # 5000 km for sun distance (~0.003 AU, accounts for interpolation differences)
                elif 'Latitude' in field or 'Longitude' in field:
                    field_tol = 0.0001  # 0.0001 degrees (~10 meters, expect sub-pixel alignment)
                elif 'GroundPoint' in field:
                    field_tol = 0.010  # 10 meters (0.010 km) for 3D ground point distance
                else:
                    field_tol = tolerance  # Use default tolerance for other fields

                passed = diff < field_tol

                status = "✓ PASS" if passed else "✗ FAIL"
                print(f"{field:<30} {diff:>14.6e} {status}")

                if not passed:
                    line_passed = False
                    # Label depends on what's being compared
                    if field in ['Latitude', 'Longitude', 'GroundPoint']:
                        print(f"  CSM:  {isd_val}")
                        print(f"  ISIS: {isis_val}")
                    else:
                        print(f"  ISD:  {isd_val}")
                        print(f"  ISIS: {isis_val}")

            results.append({
                'line': line,
                'passed': line_passed,
                'differences': all_diffs
            })

            if not line_passed:
                all_passed = False

        # Summary
        print("\n" + "=" * 80)
        print("SUMMARY")
        print("=" * 80)

        print("\nRoundtrip Tests (Internal Consistency):")
        print(f"  ISIS: {'✓ Passed' if isis_roundtrip_passed else '✗ Failed'} - Camera is {'internally consistent' if isis_roundtrip_passed else 'NOT internally consistent'}")
        print(f"  CSM:  {'✓ Passed' if csm_roundtrip_passed else '✗ Failed'} - Camera is {'internally consistent' if csm_roundtrip_passed else 'NOT internally consistent'}")

        passed_count = sum(1 for r in results if r['passed'])
        print(f"\nCamera Model Comparison (CSM vs ISIS):")
        print(f"  Lines tested: {len(results)}")
        print(f"  Lines passed: {passed_count}/{len(results)}")

        if not all_passed:
            print("\nFailed lines:")
            for result in results:
                if not result['passed']:
                    print(f"  Line {result['line']}")
                    for field, diff, _, _ in result['differences']:
                        print(f"    {field}: {diff:.6e}")

        overall_passed = all_passed and isis_roundtrip_passed and csm_roundtrip_passed

        if overall_passed:
            print("\n✓ ALL TESTS PASSED")
        else:
            print("\n✗ SOME TESTS FAILED")
            if not isis_roundtrip_passed or not csm_roundtrip_passed:
                print("  → Roundtrip tests failed - camera model(s) not internally consistent")
            if not all_passed:
                print("  → Comparison tests failed - CSM and ISIS models differ")

        return overall_passed


def main():
    parser = argparse.ArgumentParser(
        description='Verify ALE ISD by comparing against ISIS camera',
        epilog='''
Examples:
  # Test at default lines (start, middle, end)
  %(prog)s cube.cub

  # Test at specific lines
  %(prog)s cube.cub --lines 1 100 500 1000

  # Adjust position tolerance (km)
  %(prog)s cube.cub --tolerance 0.1
        ''',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument('cube', help='Path to ISIS cube')
    parser.add_argument(
        '--lines',
        nargs='+',
        type=int,
        help='Specific line numbers to test (default: start, middle, end)'
    )
    parser.add_argument(
        '--tolerance',
        type=float,
        default=1.0,
        help='Position difference tolerance in km (default: 1.0)'
    )

    args = parser.parse_args()

    # Check cube exists
    if not Path(args.cube).exists():
        print(f"ERROR: Cube not found: {args.cube}")
        sys.exit(1)

    # Run verification
    success = verify_ale_driver(args.cube, args.lines, args.tolerance)
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
