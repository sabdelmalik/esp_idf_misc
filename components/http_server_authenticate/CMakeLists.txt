idf_component_register( SRCS  "src/http_digest_authentication.c" 
                              "src/http_basic_authentication.c" 
                              "src/http_authentication_common.c"
                              "src/base64_encoding.c"
                        INCLUDE_DIRS "." "./include"
                        REQUIRES esp_http_server mbedtls)
