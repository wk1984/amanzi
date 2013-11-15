import matplotlib.pyplot as plt
import numpy
import prettytable 
import os, re
import utils_dispersion_aligned_point_2d

#
# Amanzi Input File:
#
input_filename = "amanzi_dispersion_aligned_point_2d.xml"
    
#
# Overwrite:  True|False
#
# If Amanzi or AT123D-AT results exist, force them to be recomputed, and hence, overwritten
#
overwrite=False

[obs_slices, subtests, analytic] = utils_dispersion_aligned_point_2d.SetupTests()
[obs_xml, obs_data, obs_scatter] = utils_dispersion_aligned_point_2d.AmanziResults(input_filename,subtests,obs_slices,overwrite)
analytic_soln = utils_dispersion_aligned_point_2d.AnalyticSolutions(analytic,overwrite)

#
# One of three plots 'centerline', 'x=0.0', and 'x=424.0'
# 
slice='centerline'

# Plot the data:
fig1 = plt.figure()
axes1 = fig1.add_axes([0.15,0.15,0.80,0.80])
axes1.set_yscale('log')
axes1.set_xlim(obs_slices[slice]['domain'][0],obs_slices[slice]['domain'][1])

# Plot centerline (y=0) 
utils_dispersion_aligned_point_2d.PlotObservations(obs_scatter,slice,subtests,axes1)
utils_dispersion_aligned_point_2d.PlotAnalyticSoln(analytic_soln,analytic,slice,obs_slices,axes1)

#
axes1.legend(loc='lower right')
axes1.set_xlabel('Position along the Plume Centerline, x[m]',fontsize=14)
axes1.set_ylabel('Concentration [kg/m$^3$]',fontsize=14)
axes1.text(150,0.003,'Concentration along y=0, at t=1440 days.',fontsize=14)

plt.show()
    
