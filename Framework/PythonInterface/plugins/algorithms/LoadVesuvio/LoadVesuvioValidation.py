"""
Contains validation checks that are used in the LoadVesuvio
"""

from mantid.api import AlgorithmManager

def _validate_range_formatting(lower, upper, property_name, issues):
    """
    Validates is a range style input is in the correct form of lower-upper
    """
    upper = int(upper)
    lower = int(lower)
    if upper < lower:
        issues[property_name] = "Range must be in format lower-upper"
    return issues

#----------------------------------------------------------------------------------------

def _validate_spec_min_max(diff_mode, spectra, property_name, issues):
    """
    Validates if the spectra is with the minimum and maximum boundaries
    """
    # Only validate boundaries if in difference Mode
    if "Difference" in diff_mode:
        specMin = self._backward_spectra_list[0]
        specMax = self._forward_spectra_list[-1]
        if spectra < specMin:
            issues[property_name] = ("Lower limit for spectra is %d in difference mode" % specMin)
        if spectra > specMax:
            issues[property_name] = ("Upper limit for spectra is %d in difference mode" % specMax)

    return issues

#----------------------------------------------------------------------------------------

def _raise_error_period_scatter(run_str, back_scattering):
    """
    Checks that the input is valid for the number of periods in the data with the current scattering
    2 Period - Only Forward Scattering
    3 Period - Only Back Scattering
    6 Period - Both Forward and Back
    """
    rfi_alg = AlgorithmManager.create('RawFileInfo')
    rfi_alg.setProperty('Filename', run_str)
    rfi_alg.execute()
    nperiods = rfi_alg.getProperty('PeriodCount').value

    if nperiods == 2:
        if back_scattering:
            raise RuntimeError("2 period data can only be used for forward scattering spectra")

    if nperiods == 3:
        if not back_scattering:
            raise RuntimeError("3 period data can only be used for back scattering spectra")