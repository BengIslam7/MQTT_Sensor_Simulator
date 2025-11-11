cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr)
project(Zephyr_MQTT_Sensor_Simulator)

target_sources(app PRIVATE src/main.c)