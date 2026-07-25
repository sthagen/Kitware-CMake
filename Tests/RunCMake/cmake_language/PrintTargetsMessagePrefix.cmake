add_library(zz_mp_lib INTERFACE)

# Default banner opens the message.
cmake_language(PRINT_TARGETS REGEX "^zz_mp_")

# The internal __MESSAGE_PREFIX keyword overrides the default banner.
cmake_language(PRINT_TARGETS REGEX "^zz_mp_" __MESSAGE_PREFIX "CUSTOM_BANNER\n")
