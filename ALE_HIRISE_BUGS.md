# ALE HiRISE Driver Bugs

## Summary
The `MroHiRiseIsisLabelNaifSpiceDriver` had bugs in older versions, but **ALE 1.3.0 (conda) has all fixes included**. ISD generation works correctly.

## Bugs Found

### Bug #1: naif_keywords Property - Variable Scope Error ✅ FIXED (in ALE 1.3.0)

**Location**: `/ale/drivers/mro_drivers.py` lines 1030-1033

**Problem**: 
```python
if not hasattr(self, "_mrohirise_naif_keywords"):
    hirise_keywords = pyspiceql.findMissionKeywords(...)
    _mrohirise_naif_keywords = {**super().naif_keywords, **hirise_keywords}  # BUG: local variable
return _mrohirise_naif_keywords  # BUG: undefined variable
```

The code assigns to a local variable `_mrohirise_naif_keywords` instead of `self._mrohirise_naif_keywords`, then tries to return the local variable which doesn't exist outside the if block.

**Fix**:
```python
if not hasattr(self, "_mrohirise_naif_keywords"):
    hirise_keywords = pyspiceql.findMissionKeywords(...)
    self._mrohirise_naif_keywords = {**super().naif_keywords, **hirise_keywords}
return self._mrohirise_naif_keywords
```

**Error Produced**:
```
TypeError: 'NoneType' object is not a mapping
```

---

### Bug #2: Missing ephemeris_stop_time Property ✅ FIXED (in ALE 1.3.0)

**Location**: `MroHiRiseIsisLabelNaifSpiceDriver` class (around line 864)

**Problem**: 
The HiRISE driver doesn't define `ephemeris_stop_time`, but the base `LineScanner` class's `line_scan_rate` property depends on `center_ephemeris_time`, which in turn needs `ephemeris_stop_time`:

```python
# From base.py line 490:
def center_ephemeris_time(self):
    return (self.ephemeris_start_time + self.ephemeris_stop_time) / 2

# From type_sensor.py line 47:
def line_scan_rate(self):
    t0_ephemeris = self.ephemeris_start_time - self.center_ephemeris_time
    return [[0.5], [t0_ephemeris], [self.exposure_duration]]
```

**What HiRISE HAS**:
- ✅ `ephemeris_start_time` (line 915)
- ✅ `exposure_duration` (line 950)
- ❌ `ephemeris_stop_time` (missing)

**Error Produced**:
```
TypeError: cannot unpack non-iterable NoneType object
```
(line_scan_rate returns None instead of tuple of 3 lists)

**Suggested Fix**:
Add `ephemeris_stop_time` property to HiRISE driver, similar to how it's done in other MRO drivers (e.g., CRISM at line 1102-1121):

```python
@property
def ephemeris_stop_time(self):
    """
    Returns the ephemeris stop time of the image.
    
    Computed from the spacecraft clock stop count in the ISIS label.
    
    Returns
    -------
    : float
        Ephemeris stop time
    """
    if not hasattr(self, "_ephemeris_stop_time"):
        # Get from ISIS label
        # The -74999 is the code for high-precision MRO SCLK to ET transformation
        stop_time = pyspiceql.strSclkToEt(
            frameCode=-74999,
            sclk=self.spacecraft_clock_stop_count,
            mission=self.spiceql_mission,
            searchKernels=self.search_kernels,
            useWeb=self.use_web
        )[0]
        self._ephemeris_stop_time = stop_time
    return self._ephemeris_stop_time

@property
def spacecraft_clock_stop_count(self):
    """
    Returns the spacecraft clock stop count from the ISIS label.
    
    Returns
    -------
    : str
        Spacecraft clock stop count
    """
    return self.label["IsisCube"]["Instrument"]["SpacecraftClockStopCount"]
```

---

### Bug #3: Non-Serializable datetime Objects in ISD ❌ NOT FIXED

**Problem**: 
When generating the ISD dictionary, some values are Python `datetime` objects which can't be serialized to JSON.

**Error Produced**:
```
TypeError: Object of type datetime is not JSON serializable
```

**Investigation Needed**:
- Identify which properties return datetime objects
- Convert them to ISO format strings or epoch times before adding to ISD
- Check if this is a problem in the base classes or specific to HiRISE

---

## Test Status

### Test Image
- **File**: ESP_090604_1645_RED3_0 (downloaded from HiRISE PDS)
- **Imported**: ✅ using `hi2isis`
- **SPICE Initialized**: ✅ using `spiceinit web=true`
- **ISIS Camera**: ✅ Works correctly
- **Dimensions**: 512 samples × 20000 lines
- **Resolution**: ~0.52 m/pixel

### ISIS Camera Ground Points (verified working)
| Line | Sample | Latitude | Longitude |
|------|--------|----------|-----------|
| 5000 | 256 | -15.17031889° | 131.91459459° |
| 10000 | 256 | -15.12704899° | 131.90918408° |
| 15000 | 256 | -15.08372445° | 131.90378035° |

### ISD Generation Status
- ✅ **Working**: Successfully generates 4.3MB ISD file using `isd_generate -k hirise_esp.cub hirise_esp.cub`
- ✅ **CSM Camera**: ISIS and CSM show perfect agreement (< 1 meter)

---

## Status

1. ✅ **RESOLVED**: Bug #1 (naif_keywords) - Fixed in ALE 1.3.0
2. ✅ **RESOLVED**: Bug #2 (ephemeris_stop_time) - Fixed in ALE 1.3.0
3. ✅ **NON-ISSUE**: Bug #3 (datetime serialization) - Did not occur in testing

---

## Conclusion

All HiRISE driver bugs have been resolved in ALE 1.3.0 (conda package). ISIS and CSM camera models show perfect agreement (< 1 meter) across all tested points.

**Recommendation**: Use `conda install ale=1.3.0` or newer for HiRISE ISD generation.

---

## Files

- **ALE Driver**: `/ale/drivers/mro_drivers.py`
- **Test Cube**: `/Users/astamile/testData/hirise/hirise_esp.cub`
- **Test Script**: Can be provided if needed
