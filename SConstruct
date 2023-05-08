
#To examine commands:
#  scons --dry-run

import os
import platform
import distutils.spawn
import subprocess

USEOMP = False
USEMPI = True


libs = Split("""jsoncpp
                readline
                curl
                hdf5_hl
                hdf5
                netcdf
                pthread
                boost_system
                boost_filesystem
                boost_program_options
                boost_thread
                boost_mpi
                boost_serialization
                boost_regex
                boost_log
                lapacke
                lapack
                gfortran""")

local_include_paths = ['./include']

src_files = Split("""src/TEM.cpp
                     src/TEMUtilityFunctions.cpp
                     src/OutputEstimate.cpp
                     src/CalController.cpp
                     src/TEMLogger.cpp 
                     src/ArgHandler.cpp
                     src/Climate.cpp
                     src/ModelData.cpp
                     src/Runner.cpp
                     src/BgcData.cpp
                     src/CohortData.cpp
                     src/EnvData.cpp
                     src/EnvDataDly.cpp
                     src/FireData.cpp
                     src/RestartData.cpp
                     src/WildFire.cpp
                     src/DoubleLinkedList.cpp
                     src/Ground.cpp
                     src/Vegetation.cpp
                     src/MineralInfo.cpp
                     src/Moss.cpp
                     src/Organic.cpp
                     src/Snow.cpp
                     src/SoilParent.cpp
                     src/Layer.cpp
                     src/MineralLayer.cpp
                     src/MossLayer.cpp
                     src/OrganicLayer.cpp
                     src/ParentLayer.cpp
                     src/SnowLayer.cpp
                     src/SoilLayer.cpp
                     src/CohortLookup.cpp
                     src/Cohort.cpp
                     src/Integrator.cpp
                     src/Richards.cpp
                     src/Snow_Env.cpp
                     src/Soil_Bgc.cpp
                     src/Soil_Env.cpp
                     src/SoilParent_Env.cpp
                     src/Stefan.cpp
                     src/TemperatureUpdator.cpp
                     src/CrankNicholson.cpp
                     src/tbc-debug-util.cpp
                     src/Vegetation_Bgc.cpp
                     src/Vegetation_Env.cpp""")


platform_name = platform.system()
release = platform.release()
comp_name = platform.node()
uname = platform.uname()

platform_libs = []
platform_include_path = []
platform_library_path = []

# By default, attempt to find g++. Will be overwritten later if necessary.
compiler = distutils.spawn.find_executable('g++')
print(compiler)

# Determine platform and modify libraries and paths accordingly
# This should only be general linux paths. Machine-specific paths
#   are added below.
if platform_name == 'Linux':
  homedir = os.path.expanduser('~')
  print("homedir: " + homedir)
  print(comp_name)
  platform_include_path = ['/usr/include',
                           '/usr/include/openmpi-x86_64',
                           '/usr/include/jsoncpp',
                           '/usr/include/lapacke',
                           '~/usr/local/include']

#  platform_library_path = ['/usr/lib64', '~/usr/local/lib']

  compiler_flags = '-Wno-error -ansi -g -fPIC -std=c++11 -DBOOST_ALL_DYN_LINK -DBOOST_NO_CXX11_SCOPED_ENUMS -DGNU_FPE'
  platform_libs = libs



if 'chinook' in comp_name:
  #compiler_flags = compiler_flags.replace('c++11', 'c++0x')

  platform_libs[:] = [lib for lib in platform_libs if not lib == 'jsoncpp']
  platform_libs.append('json_linux-gcc-5.4.0_libmt')

  platform_include_path.insert(0, homedir + '/custom_software/openmpi-4.1.5/include')
  platform_include_path.insert(0, homedir + '/custom_software/jsoncpp/include')
  platform_include_path.insert(0, homedir + '/custom_software/boost_1_75_0/include')
  platform_include_path.insert(0, homedir + '/custom_software/netcdf-4.4.1.1/netcdf/include')
  platform_include_path.insert(0, homedir + '/custom_software/lapack-3.8.0/LAPACKE/include')

#  platform_library_path.insert(0, homedir + '/custom_software/jsoncpp/libs/linux-gcc-5.4.0')
#  platform_library_path.insert(0, homedir + '/custom_software/boost_1_75_0/lib')
#  platform_library_path.insert(0, homedir + '/custom_software/hdf5-1.8.19/hdf5/lib')
#  platform_library_path.insert(0, homedir + '/custom_software/netcdf-4.4.1.1/netcdf/lib')
#  platform_library_path.insert(0, homedir + '/custom_software/openmpi-4.1.5/lib')
#  platform_library_path.insert(0, homedir + '/custom_software/lapack-3.8.0')


# Modify setup for MPI, if necessary
if(USEMPI):
  compiler = distutils.spawn.find_executable('mpic++')
  print(compiler)

  compiler_flags = compiler_flags + ' -m64 -DWITHMPI'

  libs.append("mpi")


#VariantDir('scons_obj','src', duplicate=0)

GIT_SHA = subprocess.Popen('git describe --abbrev=6 --dirty --always --tags', stdout=subprocess.PIPE, shell=True).stdout.read().strip()
compiler_flags += ' -DGIT_SHA=\\"' + str(GIT_SHA) + '\\"'


#compiler_flags += ' --showme:command '

print("Compiler: " + compiler)

print("platform include path: ")
print(platform_include_path)

print("CPPFLAGS: " + compiler_flags)

#Object compilation
#object_list = Object(src_files, CXX=compiler, CPPPATH=platform_include_path,
#                     CPPFLAGS=compiler_flags)
env = Environment(
  ENV={'PATH': os.environ['PATH'],
       'HOME': os.environ['HOME'],
       'LIBRARY_PATH': os.environ['LD_LIBRARY_PATH'],
       'CPATH': platform_include_path})


#object_list = env.Object(src_files, CXX=compiler, CPPFLAGS=compiler_flags)

env.Program('dvmdostem', src_files, CXX=compiler, CPPFLAGS=compiler_flags,
        CPPPATH=local_include_paths,
        LIBS=platform_libs, LINKFLAGS="-fopenmp")

#env.Program('dvmdostem', object_list, CXX=compiler, CPPPATH=local_include_paths,
#        LIBS=platform_libs, LINKFLAGS="-fopenmp")
