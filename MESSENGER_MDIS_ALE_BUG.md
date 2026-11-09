# MESSENGER MDIS Missing Taylor Distortion in ALE

## Summary
The ALE MESSENGER MDIS driver for ISIS labels (`MessengerMdisIsisLabelIsisSpiceDriver`) does not apply optical distortion, causing a ~183m (122 pixel) disagreement between ISIS and CSM camera models. ISIS applies a third-order Taylor series distortion model via `TaylorCameraDistortionMap`, but ALE uses the `NoDistortion` mixin.

## Impact
- **Severity**: High - 122 pixel disagreement at 1.5 m/pixel resolution
- **Affects**: All MESSENGER MDIS NAC and WAC images processed with ISIS labels + spiceinit
- **When**: Any workflow comparing ISIS camera model to CSM camera model

## Test Case
**Image**: EN1072174528M.cub (MDIS NAC, 1×1 pixel, Mercury)

**Test point**: Center pixel (0.5, 0.5)

| Model | Latitude | Longitude | Ground Distance from ISIS |
|-------|----------|-----------|---------------------------|
| ISIS  | 46.273580° | 248.061819° | 0 m (reference) |
| CSM   | 46.277751° | 248.060332° | 183.0 m (122 pixels) |

## Root Cause

### ISIS Implementation
ISIS uses `TaylorCameraDistortionMap` which reads 10-coefficient Taylor series distortion from SPICE kernels:

**Source**: [isis/src/messenger/objs/MdisCamera/TaylorCameraDistortionMap.cpp:63-70](../isis/src/messenger/objs/MdisCamera/TaylorCameraDistortionMap.cpp#L63-L70)

```cpp
void TaylorCameraDistortionMap::SetDistortion(const int naifIkCode) {
  QString odtxkey = "INS" + toString(naifIkCode) + "_OD_T_X";
  QString odtykey = "INS" + toString(naifIkCode) + "_OD_T_Y";
  for(int i = 0; i < 10; ++i) {
    p_odtx.push_back(p_camera->getDouble(odtxkey, i));
    p_odty.push_back(p_camera->getDouble(odtykey, i));
  }
}
```

**Distortion model**: Third-order Taylor series expansion
```
distorted_x = a0 + a1*x + a2*y + a3*x² + a4*xy + a5*y² + a6*x³ + a7*x²y + a8*xy² + a9*y³
distorted_y = b0 + b1*x + b2*y + b3*x² + b4*xy + b5*y² + b6*x³ + b7*x²y + b8*xy² + b9*y³
```

**Test cube coefficients**:
```
INS-236820_OD_T_X = (0.0, 1.0018542696238, 0.0, 0.0, -5.09444047494111e-04, 
                     0.0, 1.00401047146886e-05, 0.0, 1.00401047146886e-05, 0.0)

INS-236820_OD_T_Y = (0.0, 0.0, 1.0, 9.06001059499675e-04, 0.0, 
                     3.57484262662076e-04, 0.0, 1.00401047146886e-05, 0.0, 1.00401047146886e-05)
```

### ALE Implementation
**Current**: Uses `NoDistortion` mixin which returns zero coefficients

**Source**: [ale/drivers/mess_drivers.py:19](../ale/drivers/mess_drivers.py#L19)
```python
class MessengerMdisIsisLabelIsisSpiceDriver(Framer, IsisLabel, IsisSpice, NoDistortion, Driver):
    # ... no distortion handling
```

**Generated ISD**:
```json
{
  "optical_distortion": {
    "radial": {
      "coefficients": [0.0, 0.0, 0.0]
    }
  }
}
```

### PDS3 Driver Status
The `MessengerMdisPds3NaifSpiceDriver` at line 75 has a `usgscsm_distortion_model` method (lines 225-241) that returns transverse distortion:

```python
@property
def usgscsm_distortion_model(self):
    return {
        "transverse": {
            "x" : self.odtx,
            "y" : self.odty
        }
    }
```

However, this driver:
1. Still inherits from `NoDistortion` (the local method overrides it)
2. References `self.odtx` and `self.odty` which are defined in `NaifSpice` mixin (ale/base/data_naif.py:223-260)
3. Works for PDS3 labels but not tested

The IsisLabel driver doesn't have this method at all.

## USGSCSM Support
USGSCSM already implements the `TRANSVERSE` distortion type which matches ISIS TaylorCameraDistortionMap.

**Source**: [usgscsm/include/usgscsm/Distortion.h:10](../usgscsm/include/usgscsm/Distortion.h#L10)
```cpp
enum DistortionType { RADIAL, TRANSVERSE, KAGUYALISM, DAWNFC, LROLROCNAC, CAHVOR,
                     LUNARORBITER, RADTAN, KPLOSHADOWCAM, CASSIS };
```

The TRANSVERSE type implements the same Taylor series model that ISIS uses.

## Fix Required

### Option 1: Add properties to IsisLabel driver (Recommended)
Add `odtx`, `odty`, and `usgscsm_distortion_model` to `MessengerMdisIsisLabelIsisSpiceDriver`:

```python
class MessengerMdisIsisLabelIsisSpiceDriver(Framer, IsisLabel, IsisSpice, NoDistortion, Driver):
    # ... existing code ...
    
    @property
    def odtx(self):
        """
        Returns the x coefficient for the Taylor series optical distortion model.
        Reads from INS<ikid>_OD_T_X in the ISIS cube's NaifKeywords table.
        
        Returns
        -------
        : list
          10 Taylor series x coefficients
        """
        if not hasattr(self, "_odtx"):
            odtx = self.naif_keywords['INS{}_OD_T_X'.format(self.ikid)]
            if not isinstance(odtx, list):
                odtx = [odtx]
            self._odtx = odtx
        return self._odtx
    
    @property
    def odty(self):
        """
        Returns the y coefficient for the Taylor series optical distortion model.
        Reads from INS<ikid>_OD_T_Y in the ISIS cube's NaifKeywords table.
        
        Returns
        -------
        : list
          10 Taylor series y coefficients
        """
        if not hasattr(self, "_odty"):
            odty = self.naif_keywords['INS{}_OD_T_Y'.format(self.ikid)]
            if not isinstance(odty, list):
                odty = [odty]
            self._odty = odty
        return self._odty
    
    @property
    def usgscsm_distortion_model(self):
        """
        Returns the USGSCSM transverse distortion model for MESSENGER MDIS.
        ISIS uses TaylorCameraDistortionMap with third-order Taylor series expansion,
        which matches USGSCSM's TRANSVERSE distortion type.
        
        Returns
        -------
        : dict
          USGSCSM transverse distortion specification
        """
        return {
            "transverse": {
                "x": self.odtx,
                "y": self.odty
            }
        }
```

### Option 2: Fix PDS3 driver
The PDS3 driver already has `usgscsm_distortion_model` but needs the `odtx`/`odty` properties added (same as Option 1).

## Similar Fixes in ALE History
This is the same type of bug that was fixed for other missions:

- **PR #740**: NEAR MSI and Clementine NIR - added RadialDistortion
- **PR #739**: Cassini ISS - distortion fixes
- **PR #736**: Fix single-coefficient distortion odtk

## Testing After Fix
1. Apply fix to ALE driver
2. Regenerate ISD: `isd_generate EN1072174528M.cub -k EN1072174528M.cub`
3. Reinitialize CSM: `csminit from=EN1072174528M.cub`
4. Test ground points: `campt from=EN1072174528M.cub sample=0.5 line=0.5 type=ground`
5. Expected: ISIS and CSM should agree to < 1 pixel

## References
- ALE repo: https://github.com/DOI-USGS/ale
- USGSCSM repo: https://github.com/DOI-USGS/usgscsm
- Similar fix PR #740: https://github.com/DOI-USGS/ale/pull/740

## Files to Modify
- `ale/drivers/mess_drivers.py` - Add distortion handling to line 19 (IsisLabel driver)
- Update tests in `tests/pytests/test_messenger_drivers.py`
- Add entry to `CHANGELOG.md`
