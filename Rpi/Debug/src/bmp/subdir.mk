################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/bmp/bmpReader.cpp \
../src/bmp/endian.cpp 

OBJS += \
./src/bmp/bmpReader.o \
./src/bmp/endian.o 

CPP_DEPS += \
./src/bmp/bmpReader.d \
./src/bmp/endian.d 


# Each subdirectory must supply rules for building sources it contributes
src/bmp/%.o: ../src/bmp/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


