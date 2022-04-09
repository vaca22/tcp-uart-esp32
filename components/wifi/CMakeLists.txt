idf_component_register(SRCS "wifi.c" 
                    INCLUDE_DIRS  "."
                    REQUIRES console nvs_flash bt
                    REQUIRED_IDF_TARGETS esp32)
