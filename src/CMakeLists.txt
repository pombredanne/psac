cmake_minimum_required(VERSION 2.6)

# project settings
project(psac-main)

#################
#  shared libs  #
#################

#####################
#  main executable  #
#####################

# our main executable for suffix array construction
add_executable(psac psac.cpp)
target_link_libraries(psac ${EXTRA_LIBS} rt)

add_executable(gsac gsac.cpp)
target_link_libraries(gsac ${EXTRA_LIBS} rt)
target_link_libraries(gsac divsufsort)
target_link_libraries(gsac divsufsort64)

################
#  benchmarks  #
################

# benchmarks of our different internal methods
add_executable(benchmark_sac benchmark.cpp)
target_link_libraries(benchmark_sac ${EXTRA_LIBS} rt)

# benchmark with different values of `k` for initial k-mer sorting
add_executable(benchmark_k benchmark_k.cpp)
target_link_libraries(benchmark_k ${EXTRA_LIBS} rt)

# benchmark ANSV
add_executable(benchmark-ansv benchmark_ansv.cpp)
target_link_libraries(benchmark-ansv ${EXTRA_LIBS} rt)

################
# tools/utils  #
################
add_executable(print64 print64.cpp)

################
#  divsufsort  #
################

# divsufsort executable (supporting 32 and 64 bits)
add_executable(dss dss.cpp)
target_link_libraries(dss ${EXTRA_LIBS})
target_link_libraries(dss divsufsort)
target_link_libraries(dss divsufsort64)

# compare our algorithm against divsufsort and check correctness
add_executable(psac-vs-dss psac_vs_dss.cpp)
target_link_libraries(psac-vs-dss ${EXTRA_LIBS} rt)
target_link_libraries(psac-vs-dss divsufsort)
target_link_libraries(psac-vs-dss divsufsort64)

