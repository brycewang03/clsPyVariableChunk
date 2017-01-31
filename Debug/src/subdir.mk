################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/HashAlgs.cc \
../src/RollingWindow.cc \
../src/city.cc \
../src/clsNewVairableChunk.cc \
../src/dedup-table.cc \
../src/dedup-util.cc 

CPP_SRCS += \
../src/md5.cpp 

CC_DEPS += \
./src/HashAlgs.d \
./src/RollingWindow.d \
./src/city.d \
./src/clsNewVairableChunk.d \
./src/dedup-table.d \
./src/dedup-util.d 

OBJS += \
./src/HashAlgs.o \
./src/RollingWindow.o \
./src/city.o \
./src/clsNewVairableChunk.o \
./src/dedup-table.o \
./src/dedup-util.o \
./src/md5.o 

CPP_DEPS += \
./src/md5.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	clang++ -O0 -g3 -Wall -c -fmessage-length=0 -std=c++11 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	clang++ -O0 -g3 -Wall -c -fmessage-length=0 -std=c++11 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


