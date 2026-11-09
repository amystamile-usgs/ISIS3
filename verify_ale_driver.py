#!/usr/bin/env python3
"""
ALE Driver Verification Script

Compares CSM camera (via ALE ISD) against ISIS camera to verify driver correctness.
Tests geometric calculations at specified image coordinates.
"""

import argparse
import json
import subprocess
import tempfile
import sys
from pathlib import Path
from typing import Dict, List, Tuple
import ale


def generate_isd(cube_path: str, output_path: str = None) -> str:
    """
    Generate ISD from a cube using ALE.

    Args:
        cube_path: Path to input ISIS cube
        output_path: Optional path to save ISD JSON

    Returns:
        Path to generated ISD file
    """
    print(f"Generating ISD for {cube_path}...")

    try:
        # Load the cube and generate ISD
        isd = ale.loads(cube_path, props={'kernels': {}})

        # Create temp file if no output path specified
        if output_path is None:
            temp_file = tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False)
            output_path = temp_file.name
            temp_file.close()

        # Write ISD to file
        with open(output_path, 'w') as f:
            json.dump(isd, f, indent=2)

        print(f"ISD written to {output_path}")
        return output_path

    except Exception as e:
        print(f"ERROR: Failed to generate ISD: {e}")
        sys.exit(1)


def run_campt(cube_path: str, sample: float, line: float, isd_path: str = None) -> Dict:
    """
    Run campt and return parsed results.

    Args:
        cube_path: Path to input cube
        sample: Sample coordinate
        line: Line coordinate
        isd_path: Optional ISD file for CSM camera

    Returns:
        Dictionary of campt output fields
    """
    # Build command
    cmd = [
        'campt',
        f'from={cube_path}',
        f'sample={sample}',
        f'line={line}',
        'format=pvl'
    ]

    if isd_path:
        cmd.append(f'csm={isd_path}')

    # Run campt
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        return parse_pvl_output(result.stdout)
    except subprocess.CalledProcessError as e:
        print(f"ERROR running campt: {e}")
        print(f"STDERR: {e.stderr}")
        return {}


def parse_pvl_output(pvl_text: str) -> Dict:
    """
    Parse campt PVL output into a dictionary.

    Args:
        pvl_text: Raw PVL output from campt

    Returns:
        Dictionary of key-value pairs
    """
    data = {}

    for line in pvl_text.split('\n'):
        line = line.strip()
        if '=' in line and not line.startswith('#'):
            # Parse key = value pairs
            parts = line.split('=', 1)
            if len(parts) == 2:
                key = parts[0].strip()
                value = parts[1].strip()

                # Remove units and clean value
                if '<' in value:
                    value = value.split('<')[0].strip()

                # Try to convert to float
                try:
                    # Handle vector values (x, y, z)
                    if value.startswith('(') and value.endswith(')'):
                        coords = value[1:-1].split(',')
                        data[key] = tuple(float(c.strip()) for c in coords)
                    else:
                        data[key] = float(value)
                except ValueError:
                    data[key] = value

    return data


def compare_values(isis_val, csm_val, tolerance: float = 1e-6) -> Tuple[float, bool]:
    """
    Compare two values and return difference and whether they match within tolerance.

    Args:
        isis_val: ISIS camera value
        csm_val: CSM camera value
        tolerance: Acceptable difference threshold

    Returns:
        Tuple of (difference, passes_tolerance)
    """
    # Handle tuple/vector values
    if isinstance(isis_val, tuple) and isinstance(csm_val, tuple):
        # Compute Euclidean distance for vectors
        diff = sum((a - b) ** 2 for a, b in zip(isis_val, csm_val)) ** 0.5
        return diff, diff < tolerance

    # Handle scalar values
    try:
        diff = abs(float(isis_val) - float(csm_val))
        return diff, diff < tolerance
    except (ValueError, TypeError):
        # Non-numeric values
        return 0, isis_val == csm_val


def verify_driver(cube_path: str, test_points: List[Tuple[float, float]],
                 tolerance: float = 1e-6, isd_path: str = None, generate_isd_flag: bool = True):
    """
    Verify ALE driver by comparing CSM and ISIS camera results.

    Args:
        cube_path: Path to input cube
        test_points: List of (sample, line) coordinates to test
        tolerance: Acceptable difference for comparisons
        isd_path: Optional pre-generated ISD path
        generate_isd_flag: Whether to generate ISD if not provided
    """
    print("=" * 80)
    print("ALE Driver Verification")
    print("=" * 80)
    print(f"Cube: {cube_path}")
    print(f"Test points: {len(test_points)}")
    print(f"Tolerance: {tolerance}")
    print()

    # Generate ISD if not provided and flag is True
    if isd_path is None:
        if generate_isd_flag:
            isd_path = generate_isd(cube_path)
            cleanup_isd = True
        else:
            print("ERROR: No ISD provided and ISD generation disabled")
            print("Use --isd to provide an ISD file, or enable --generate-isd")
            sys.exit(1)
    else:
        print(f"Using provided ISD: {isd_path}")
        cleanup_isd = False

    # Key fields to compare
    compare_fields = [
        'PlanetocentricLatitude',
        'PositiveEast360Longitude',
        'BodyFixedCoordinate',
        'SpacecraftPosition',
        'LocalRadius',
        'SlantDistance',
        'SampleResolution',
        'LineResolution',
        'Phase',
        'Incidence',
        'Emission',
        'EphemerisTime',
        'SubSpacecraftLatitude',
        'SubSpacecraftLongitude',
        'SubSolarLatitude',
        'SubSolarLongitude'
    ]

    # Track results
    all_passed = True
    point_results = []

    for sample, line in test_points:
        print(f"\nTesting point: Sample={sample}, Line={line}")
        print("-" * 80)

        # Run campt with ISIS camera
        print("Running campt with ISIS camera...")
        isis_data = run_campt(cube_path, sample, line)

        # Run campt with CSM camera (ISD)
        print("Running campt with CSM camera (ISD)...")
        csm_data = run_campt(cube_path, sample, line, isd_path)

        if not isis_data or not csm_data:
            print("ERROR: Failed to get camera data")
            all_passed = False
            continue

        # Compare fields
        point_passed = True
        failures = []

        print("\nComparison Results:")
        print(f"{'Field':<35} {'ISIS':<20} {'CSM':<20} {'Difference':<15} {'Status'}")
        print("-" * 110)

        for field in compare_fields:
            if field in isis_data and field in csm_data:
                diff, passed = compare_values(isis_data[field], csm_data[field], tolerance)
                status = "✓ PASS" if passed else "✗ FAIL"

                if not passed:
                    point_passed = False
                    failures.append(field)

                # Format values for display
                isis_str = str(isis_data[field])[:18]
                csm_str = str(csm_data[field])[:18]
                diff_str = f"{diff:.6e}"

                print(f"{field:<35} {isis_str:<20} {csm_str:<20} {diff_str:<15} {status}")
            elif field in isis_data:
                print(f"{field:<35} {'Present':<20} {'MISSING':<20} {'-':<15} ✗ FAIL")
                point_passed = False
                failures.append(f"{field} (missing in CSM)")
            elif field in csm_data:
                print(f"{field:<35} {'MISSING':<20} {'Present':<20} {'-':<15} ✗ FAIL")
                point_passed = False
                failures.append(f"{field} (missing in ISIS)")

        point_results.append({
            'sample': sample,
            'line': line,
            'passed': point_passed,
            'failures': failures
        })

        if not point_passed:
            all_passed = False

    # Summary
    print("\n" + "=" * 80)
    print("SUMMARY")
    print("=" * 80)

    passed_count = sum(1 for r in point_results if r['passed'])
    print(f"Points tested: {len(test_points)}")
    print(f"Points passed: {passed_count}/{len(test_points)}")

    if not all_passed:
        print("\nFailed points:")
        for result in point_results:
            if not result['passed']:
                print(f"  Sample={result['sample']}, Line={result['line']}")
                for failure in result['failures']:
                    print(f"    - {failure}")

    # Cleanup temp ISD if we created it
    if cleanup_isd:
        Path(isd_path).unlink()

    print("\n" + ("✓ ALL TESTS PASSED" if all_passed else "✗ SOME TESTS FAILED"))
    return all_passed


def main():
    parser = argparse.ArgumentParser(
        description='Verify ALE driver by comparing CSM and ISIS camera models',
        epilog='''
Examples:
  # With pre-generated ISD
  %(prog)s cube.cub --isd cube.json --points 100,200 500,600

  # Generate ISD automatically (requires ALE)
  %(prog)s cube.cub --generate-isd --points center

  # Test multiple points
  %(prog)s cube.cub --isd cube.json --points 10,10 100,100 500,500
        ''',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument('cube', help='Path to input ISIS cube (must have SPICE via spiceinit)')
    parser.add_argument(
        '--points',
        nargs='+',
        default=['center'],
        help='Test points as "sample,line" or "center" (default: center)'
    )
    parser.add_argument(
        '--tolerance',
        type=float,
        default=1e-6,
        help='Acceptable difference tolerance (default: 1e-6)'
    )
    parser.add_argument(
        '--isd',
        help='Path to pre-generated ISD JSON file'
    )
    parser.add_argument(
        '--generate-isd',
        action='store_true',
        help='Generate ISD using ALE (requires ale to be installed)'
    )

    args = parser.parse_args()

    # Parse test points
    test_points = []
    for point_str in args.points:
        if point_str.lower() == 'center':
            # We'll determine center from cube metadata
            # For now, use a placeholder - could enhance to read cube size
            print("Note: 'center' will use image center (requires cube dimensions)")
            # TODO: Read cube dimensions and compute center
            test_points.append((512, 512))  # Placeholder
        else:
            try:
                sample, line = map(float, point_str.split(','))
                test_points.append((sample, line))
            except ValueError:
                print(f"ERROR: Invalid point format '{point_str}'. Use 'sample,line' or 'center'")
                sys.exit(1)

    # Run verification
    success = verify_driver(args.cube, test_points, args.tolerance, args.isd, args.generate_isd)
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
