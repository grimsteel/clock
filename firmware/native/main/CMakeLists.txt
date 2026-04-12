idf_component_register(SRCS "clock.cpp" "wifi.cpp" "mqtt.cpp" "servo.cpp"
                    INCLUDE_DIRS ".")
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++26 "-Wno-missing-field-initializers")
