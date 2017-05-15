include(LibFindMacros)

find_path(x86_INCLUDE_DIR NAMES libx86.h PATHS)

find_library(x86_LIBRARY NAMES libx86)

set(x86_PROCESS_INCLUDES x86_INCLUDE_DIR)
set(x86_PROCESS_LIBS x86_LIBRARY)
libfind_process(x86)
