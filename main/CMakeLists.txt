# main — Layer 4: the application.
#
# ESP-IDF requires the component that defines `app_main` to be named `main`,
# so this layer keeps that name even though it is the application layer.
#
# It is the only layer allowed to depend on all three wrapper layers.

idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES core rtos drivers)
