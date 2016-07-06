#!/usr/bin/env python

import argparse
import textwrap

import netCDF4
import numpy as np

#takes constructed input files, averages for a given
#x,y dimension, writes condensed input files

#Parameterizations (as of 5-18-16)
#1 - Black spruce, dry
#2 - Black spruce, wet
#3 - White spruce, dry
#4 - White spruce, wet
#5 - Deciduous, dry
#6 - Deciduous, wet
#7 - Tussock tundra
#8 - Shrub tundra
#9 - Heath tundra
#10 - Wetsedge tundra
#11 - Maritime upland forest
#12 - Maritime lowland forest
#13 - Maritime fen
#14 - Maritime alder shrubland
#15 - [Boreal shrubland] not really implemented yet

class pmtzn_cell(object):
  """A cell constructed to hold data associated with a parameterization"""
  def __init__(self, index):
    self.pmtzn = index
    self.count = 0

    self.lats = []
    self.avg_lat = 0
    self.lons = []
    self.avg_lon = 0

    self.tair_monthly = []
    self.avg_tair_monthly = []

    self.prec_monthly = []
    self.avg_prec_monthly = []

    self.nirr_monthly = []
    self.avg_nirr_monthly = []

    self.vapo_monthly = []
    self.avg_vapo_monthly = []

    self.drainage = [] 
    self.median_drain = None

    self.pct_sand = []
    self.avg_pct_sand = 0
    self.pct_silt = []
    self.avg_pct_silt = 0
    self.pct_clay = []
    self.avg_pct_clay = 0

    self.veg_type = None 
    self.soil_tex = 0
    self.fri = 0

 
def extract_pmtzn_input_data(hist_dataset, pmtzn_cells, pmtzn, row, col):
  tmp_tair_monthly = hist_dataset.variables['tair'][:360, row, col]
  pmtzn_cells[pmtzn].tair_monthly.append(tmp_tair_monthly)
  tmp_prec_monthly = hist_dataset.variables['precip'][:360, row, col]
  pmtzn_cells[pmtzn].prec_monthly.append(tmp_prec_monthly)
  tmp_nirr_monthly = hist_dataset.variables['nirr'][:360, row, col]
  pmtzn_cells[pmtzn].nirr_monthly.append(tmp_nirr_monthly)
  tmp_vapo_monthly = hist_dataset.variables['vapor_press'][:360, row, col]
  pmtzn_cells[pmtzn].vapo_monthly.append(tmp_vapo_monthly)
 
  
def avg_monthly_input(monthly_data):
  cell_count = len(monthly_data)
  months = len(monthly_data[0])
  summed_data = [0]*months 
  averaged_data = [0]*months 
  for i in range(cell_count):
    for j in range(months):
      summed_data[j] += monthly_data[i][j]

  for i in range(months):
    averaged_data[i] = summed_data[i]/cell_count

  return averaged_data


def main():

  #Parameterization related values
  pmtzn_cells = []
  veg_class = []
  drain_class = []

  #Parameterizations are 1-14. Ignore pmtzn_cells[0]. TODO 
  for i in range(0,15):
    pmtzn_cells.append(pmtzn_cell(i))

  #For redistributing results for comparison
  pmtzn_map = []

  #1: black spruce
  #2: white spruce
  #3: deciduous
  #4: shrub tundra
  #5: tussock tundra
  #6: wet sedge tundra
  #7: heath tundra
  #8: maritime forest 
  with netCDF4.Dataset("vegetation.nc") as veg_dataset:
    veg_class[:] = veg_dataset.variables['veg_class'][:]

  #flat_veg_class = [item for row in veg_class for item in row]
  #print flat_veg_class

  #0 is well-drained, 1 is poorly-drained
  with netCDF4.Dataset("drainage.nc") as drain_dataset:
    drain_class[:] = drain_dataset.variables['drainage_class'][:]

  hist_dataset = netCDF4.Dataset("historic-climate.nc")

  for row in range(len(veg_class)):
    pmtzn_map_row = []
    for col in range(len(veg_class[0])):
      veg = veg_class[row][col]
      drain = drain_class[row][col]
      #veg == 0 is empty in the config files
      if veg == 1 and drain == 0: #black spruce, dry
        pmtzn_cells[1].count += 1
        pmtzn_map_row.append(1)

      elif veg == 1 and drain == 1: #black spruce, wet
        pmtzn_cells[2].count += 1
        pmtzn_map_row.append(2)

      elif veg == 2 and drain == 0: #white spruce, dry
        pmtzn_cells[3].count += 1
        pmtzn_map_row.append(3)

      elif veg == 2 and drain == 1: #white spruce, wet
        pmtzn_cells[4].count += 1
        pmtzn_map_row.append(4)

      elif veg == 3 and drain == 0: #deciduous, dry
        pmtzn_cells[5].count += 1
        pmtzn_map_row.append(5)

      elif veg == 3 and drain == 1: #deciduous, wet
        pmtzn_cells[6].count += 1
        pmtzn_map_row.append(6)

      elif veg == 4: #shrub tundra
        pmtzn_cells[8].veg_type = 4
        pmtzn_cells[8].drainage.append(drain)
        pmtzn_cells[8].count += 1
        pmtzn_map_row.append(8)
        extract_pmtzn_input_data(hist_dataset, pmtzn_cells, 8, row, col)
        pmtzn_cells[8].lats.append(hist_dataset.variables['lat'][row,col])
        pmtzn_cells[8].lons.append(hist_dataset.variables['lon'][row,col])

      elif veg == 5: #tussock tundra
        pmtzn_cells[7].veg_type = 5
        pmtzn_cells[7].drainage.append(drain)
        pmtzn_cells[7].count += 1
        pmtzn_map_row.append(7)
        extract_pmtzn_input_data(hist_dataset, pmtzn_cells, 7, row, col)
        pmtzn_cells[7].lats.append(hist_dataset.variables['lat'][row,col])
        pmtzn_cells[7].lons.append(hist_dataset.variables['lon'][row,col])

      elif veg == 6: #wet sedge tundra
        pmtzn_cells[10].count += 1
        pmtzn_map_row.append(10)

      elif veg == 7: #heath tundra
        pmtzn_cells[9].count += 1
        pmtzn_map_row.append(9)

      #elif veg == 8: #maritime forest TODO
      #Fen, alder, etc TODO

    pmtzn_map.append(pmtzn_map_row)

  print pmtzn_map

  num_pmtzns = 0

  for cell in pmtzn_cells:
    if cell.count != 0:
      cell.avg_tair_monthly = avg_monthly_input(cell.tair_monthly)
      cell.avg_prec_monthly = avg_monthly_input(cell.prec_monthly)
      cell.avg_nirr_monthly = avg_monthly_input(cell.nirr_monthly)
      cell.avg_vapo_monthly = avg_monthly_input(cell.vapo_monthly)

      cell.avg_lat = np.mean(cell.lats)
      cell.avg_lon = np.mean(cell.lons)

      cell.median_drain = np.median(cell.drainage)

      #average soil texture?
      #average FRI?

      #The number of parameterizations represented
      num_pmtzns += 1

  for cell in pmtzn_cells:
    print str(cell.pmtzn) + ", " + str(cell.count)

  #Create lists to write to output files - this looks wrong...
  output_veg_types = []
  output_drainage = []
  output_lats = []
  output_lons = []
  output_tair = []
  output_prec = []
  output_nirr = []
  output_vapo = []
  for cell in pmtzn_cells:
    if cell.count != 0:
      output_veg_types.append(cell.veg_type)
      output_drainage.append(cell.median_drain)
      output_lats.append(cell.avg_lat)
      output_lons.append(cell.avg_lon)
      output_tair.append(cell.avg_tair_monthly)
      output_prec.append(cell.avg_prec_monthly)
      output_nirr.append(cell.avg_nirr_monthly)
      output_vapo.append(cell.avg_vapo_monthly)

  #Create output structures
  #Vegetation
  veg_file = netCDF4.Dataset("avg_vegetation.nc", mode='w', format='NETCDF4')
  #Y = ncfile.createDimension('Y', 1)
  X = veg_file.createDimension('X', num_pmtzns)
  out_veg_class = veg_file.createVariable('veg_class', np.int, 'X')
  out_veg_class[:] = output_veg_types 

  #Drainage
  drain_file = netCDF4.Dataset("avg_drainage.nc", mode='w', format='NETCDF4')
  X = drain_file.createDimension('X', num_pmtzns)
  out_drain_class = drain_file.createVariable('drainage_class', np.int, 'X')
  out_drain_class[:] = output_drainage

  #Historic climate
  climate_file = netCDF4.Dataset("avg_historic-climate.nc", mode='w', format='NETCDF4')
  time_dim = climate_file.createDimension('time', 360)
  X = climate_file.createDimension('X', num_pmtzns)
  X = climate_file.createVariable('X', np.int, 'X')
  X[:] = np.arange(0, num_pmtzns)
  out_lat = climate_file.createVariable('lat', np.float32, 'X')
  out_lat[:] = output_lats 
  out_lon = climate_file.createVariable('lon', np.float32, 'X')
  out_lon[:] = output_lons
  out_tair = climate_file.createVariable('tair', np.float32, ('time', 'X'))
  out_tair[:] = output_tair
  out_prec = climate_file.createVariable('precip', np.float32, ('time', 'X'))
  out_prec[:] = output_prec
  out_nirr = climate_file.createVariable('nirr', np.float32, ('time', 'X'))
  out_nirr[:] = output_nirr
  out_vapo = climate_file.createVariable('vapor_press', np.float32, ('time', 'X'))
  out_vapo[:] = output_vapo

  #Soil texture

  #Run mask
  run_file = netCDF4.Dataset("avg_run-mask.nc", mode='w', format='NETCDF4')
  X = run_file.createDimension('X', num_pmtzns)
  out_run_mask = run_file.createVariable('run', np.int, 'X')
  out_run_mask[:] = [1]*num_pmtzns


if __name__ == '__main__':

  parser = argparse.ArgumentParser(
    formatter_class = argparse.RawDescriptionHelpFormatter,
      description=textwrap.dedent('''\
      '''),

      epilog=textwrap.dedent(''''''),
  )

  parser.add_argument('--xsize', default=10, type=int,
                      help="source window x size (default: %(default)s)")
  parser.add_argument('--ysize', default=10, type=int,
                      help="source window y size (default: %(default)s)")

  main()



