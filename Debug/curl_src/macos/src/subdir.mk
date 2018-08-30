################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/Users/AlexandruG/Proiecte/curl-7.61.0/src/macos/src/curl_GUSIConfig.cpp \
/Users/AlexandruG/Proiecte/curl-7.61.0/src/macos/src/macos_main.cpp 

OBJS += \
./curl_src/macos/src/curl_GUSIConfig.o \
./curl_src/macos/src/macos_main.o 

CPP_DEPS += \
./curl_src/macos/src/curl_GUSIConfig.d \
./curl_src/macos/src/macos_main.d 


# Each subdirectory must supply rules for building sources it contributes
curl_src/macos/src/curl_GUSIConfig.o: /Users/AlexandruG/Proiecte/curl-7.61.0/src/macos/src/curl_GUSIConfig.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/local/include/curl/ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

curl_src/macos/src/macos_main.o: /Users/AlexandruG/Proiecte/curl-7.61.0/src/macos/src/macos_main.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/local/include/curl/ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


