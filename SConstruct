
#To examine commands:
#  scons --dry-run

import os
import platform
import distutils.spawn
import subprocess

USEOMP = True
USEMPI = False

libs = Split("""jsoncpp
                readline
                netcdf_c++
                netcdf
                pthread
                boost_system
                boost_filesystem
                boost_program_options
                boost_thread
                boost_log""")

local_include_paths = Split("""./src
                               ./include
                               ./src/atmosphere
                               ./src/data
                               ./src/disturb
                               ./src/ecodomain
                               ./src/ecodomain/horizon
                               ./src/ecodomain/layer
                               ./src/inc
                               ./src/input
                               ./src/lookup
                               ./src/runmodule
                               ./src/snowsoil
                               ./src/util
                               ./src/vegetation""")

                                                            
src_files = Split("""src/TEM.cpp
                     src/TEMUtilityFunctions.cpp  
                     src/CalController.cpp
                     src/TEMLogger.cpp 
                     src/ArgHandler.cpp
                     src/Climate.cpp
                     src/ModelData.cpp
                     src/Runner.cpp
                     src/data/BgcData.cpp
                     src/data/CohortData.cpp
                     src/data/EnvData.cpp
                     src/data/EnvDataDly.cpp
                     src/data/FirData.cpp
                     src/data/RestartData.cpp
                     src/disturb/WildFire.cpp
                     src/ecodomain/DoubleLinkedList.cpp
                     src/ecodomain/Ground.cpp
                     src/ecodomain/Vegetation.cpp
                     src/ecodomain/horizon/MineralInfo.cpp
                     src/ecodomain/horizon/Moss.cpp
                     src/ecodomain/horizon/Organic.cpp
                     src/ecodomain/horizon/Snow.cpp
                     src/ecodomain/horizon/SoilParent.cpp
                     src/ecodomain/layer/Layer.cpp
                     src/ecodomain/layer/MineralLayer.cpp
                     src/ecodomain/layer/MossLayer.cpp
                     src/ecodomain/layer/OrganicLayer.cpp
                     src/ecodomain/layer/ParentLayer.cpp
                     src/ecodomain/layer/SnowLayer.cpp
                     src/ecodomain/layer/SoilLayer.cpp
                     src/lookup/CohortLookup.cpp
                     src/runmodule/Cohort.cpp
                     src/runmodule/Integrator.cpp
                     src/snowsoil/Richards.cpp
                     src/snowsoil/Snow_Env.cpp
                     src/snowsoil/Soil_Bgc.cpp
                     src/snowsoil/Soil_Env.cpp
                     src/snowsoil/SoilParent_Env.cpp
                     src/snowsoil/Stefan.cpp
                     src/snowsoil/TemperatureUpdator.cpp
                     src/util/CrankNicholson.cpp
                     src/util/tbc-debug-util.cpp
                     src/vegetation/Vegetation_Bgc.cpp
                     src/vegetation/Vegetation_Env.cpp""")


platform_name = platform.system()
release = platform.release()
comp_name = platform.node()
uname = platform.uname()

platform_libs = []
platform_include_path = []
platform_library_path = []

# By default, attempt to find g++. Will be overwritten later if necessary.
compiler = distutils.spawn.find_executable('g++')

# Determine platform and modify libraries and paths accordingly
if platform_name == 'Linux':
  platform_include_path = ['/usr/include',
                           '/usr/include/openmpi-x86_64',
                           '/usr/include/jsoncpp',
                           '~/usr/local/include']

  platform_library_path = ['/usr/lib64', '~/usr/local/lib']

  compiler_flags = '-Werror -ansi -g -fPIC -DBOOST_ALL_DYN_LINK -DGNU_FPE'
  platform_libs = libs


elif platform_name == 'Darwin':

  # On OSX, using Homebrew, alternate g++ versions are installed so as not
  # to interfere with the system g++, so here, we have to set the compiler
  # to the specific version of g++ that we need.
  compiler = distutils.spawn.find_executable('g++-4.8')

  platform_include_path = ['/usr/local/include']
  platform_library_path = ['/usr/local/lib']

  compiler_flags = '-Werror -fpermissive -ansi -g -fPIC -DBOOST_ALL_DYN_LINK -DBSD_FPE'

  # This is not really a Darwin-specific thing so much as the fact that
  # for Tobey, when he installed boost, he inadvertantly specified that
  # the multi-threaded libs be named with the -mt suffix.
  for lib in libs:
    if lib.startswith('boost'):
      platform_libs.append(lib + '-mt')
    else:
      platform_libs.append(lib)

  # statically link jsoncpp
  # apparently the shared library version of jsoncpp has some bugs.
  # See the note at the top of the SConstruct file:
  # https://github.com/jacobsa/jsoncpp/blob/master/SConstruct
  platform_libs[:] = [lib for lib in platform_libs if not lib == 'jsoncpp']
  platform_libs.append(File('/usr/local/lib/libjsoncpp.a'))

  # no profiler at this time
  platform_libs[:] = [lib for lib in platform_libs if not lib == 'profiler']


if comp_name == 'aeshna':
  platform_include_path.append('/home/tobey/usr/local/include')
  platform_library_path.append('/home/tobey/usr/local/lib')

if comp_name == 'atlas.snap.uaf.edu':
  platform_libs[:] = [lib for lib in platform_libs if not lib == 'jsoncpp']
  platform_libs.append('json_linux-gcc-4.4.7_libmt')

  platform_include_path.insert(0, '/home/UA/tcarman2/boost_1_55_0/')
  platform_include_path.insert(0, '/home/UA/rarutter/include')

  platform_library_path.insert(0, '/home/UA/rarutter/lib')
  platform_library_path.insert(0, '/home/UA/tcarman2/boost_1_55_0/stage/lib')


if(USEOMP):
  #append build flag for openmp
  compiler_flags = compiler_flags + ' -fopenmp'

# Modify setup for MPI, if necessary
if(USEMPI):
  compiler = distutils.spawn.find_executable('mpic++')

  # append src/parallel-code stuff to src_files and include_paths and libs
  src_files.append(Split("""src/parallel-code/Master.cpp
                            src/parallel-code/Slave.cpp
                         """))
  local_include_paths.append('src/parallel-code')

  compiler_flags = compiler_flags + ' -m64 -DWITHMPI'

  libs.append(Split("""mpi_cxx
                       mpi"""))


#VariantDir('scons_obj','src', duplicate=0)

print "Compiler: " + compiler

GIT_SHA = subprocess.Popen('git describe --abbrev=6 --dirty --always --tags', stdout=subprocess.PIPE, shell=True).stdout.read().strip()
compiler_flags += ' -DGIT_SHA=\\"' + GIT_SHA + '\\"'

#Object compilation
object_list = Object(src_files, CXX=compiler, CPPPATH=platform_include_path,
                     CPPFLAGS=compiler_flags)

#remove paths from the object file names - unused for now
#object_file_list = [os.path.basename(str(object)) for object in object_list]

Program('dvmdostem', object_list, CXX=compiler, CPPPATH=local_include_paths,
        LIBS=platform_libs, LIBPATH=platform_library_path, 
        LINKFLAGS="-fopenmp")
#Library()
