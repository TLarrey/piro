################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/FakeSensor.cpp \
../src/Motor.cpp \
../src/Rpi_main.cpp \
../src/Sensor.cpp \
../src/debug.cpp \
../src/dstar.cpp \
../src/generic.cpp \
../src/map.cpp \
../src/servo.cpp 

OBJS += \
./src/FakeSensor.o \
./src/Motor.o \
./src/Rpi_main.o \
./src/Sensor.o \
./src/debug.o \
./src/dstar.o \
./src/generic.o \
./src/map.o \
./src/servo.o 

CPP_DEPS += \
./src/FakeSensor.d \
./src/Motor.d \
./src/Rpi_main.d \
./src/Sensor.d \
./src/debug.d \
./src/dstar.d \
./src/generic.d \
./src/map.d \
./src/servo.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


