# Options
option(OSSIA_SANITIZE "Sanitize build" OFF)
option(OSSIA_TIDY "Use clang-tidy" OFF)
option(OSSIA_STATIC "Make a static build" OFF)
option(OSSIA_COVERAGE "Run code coverage" OFF)
option(OSSIA_EXAMPLES "Build examples" OFF)
option(OSSIA_TESTING "Build tests" OFF)
option(OSSIA_CI "Continuous integration run" OFF)
option(OSSIA_FRAMEWORK "Build an OS X framework" OFF)
option(OSSIA_JAVA "Build JNI bindings" OFF)
option(OSSIA_DISABLE_COTIRE "Disable cotire" OFF)
option(OSSIA_NO_SONAME "Set NO_SONAME property" OFF)
option(OSSIA_LTO "Link-time optimizations. Fails on Windows." OFF)
option(OSSIA_PD "Build PureData externals" ON)
option(OSSIA_OSX_FAT_LIBRARIES "Build 32 and 64 bit fat libraries on OS X" OFF)
option(OSSIA_OSX_RETROCOMPATIBILITY "Build for older OS X versions" OFF)

if(OSSIA_OSX_RETROCOMPATIBILITY)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)
endif()

if(OSSIA_OSX_FAT_LIBRARIES)
    set(CMAKE_OSX_ARCHITECTURES "i386;x86_64")
endif()

# System detection
include(ProcessorCount)
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-Wmisleading-indentation" SUPPORTS_MISLEADING_INDENT_FLAG)
check_cxx_compiler_flag("-Wl,-z,defs" WL_ZDEFS_SUPPORTED)
check_cxx_compiler_flag("-fuse-ld=gold" GOLD_LINKER_SUPPORTED)

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES ".*arm.*")
    set(OSSIA_ARCHITECTURE arm)
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES ".*aarch64.*")
    set(OSSIA_ARCHITECTURE arm)
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES ".*64.*")
    set(OSSIA_ARCHITECTURE amd64)
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES ".*86.*")
    set(OSSIA_ARCHITECTURE x86)
else()
    message("Could not determine target architecture")
    return()
endif()

# Common setup
set(CMAKE_POSITION_INDEPENDENT_CODE 1)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 14)

# So that make install after make all_unity does not rebuild everything :
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY True)

if(CMAKE_SYSTEM_NAME MATCHES Emscripten)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${PROJECT_SOURCE_DIR}/CMake;${PROJECT_SOURCE_DIR}/CMake/cmake-modules;")

# We disable debug infos on OS X on travis because it takes up too much space
if(OSSIA_CI)
  if(APPLE)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g0")
  endif()
endif()

if(OSSIA_STATIC)
  set(BUILD_SHARED_LIBS OFF)
  set(OSSIA_FRAMEWORK OFF)
else()
  set(BUILD_SHARED_LIBS ON)
endif()

if(OSSIA_COVERAGE)
  include(CodeCoverage)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_COVERAGE}")
endif()

# Compiler & linker flags
if(MSVC)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /PDBCompress /OPT:REF /OPT:ICF")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /PDBCompress /OPT:REF /OPT:ICF")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /PDBCompress /OPT:REF /OPT:ICF")

    set(OSSIA_COMPILE_OPTIONS
        "/wd4068" # pragma mark -
        "/wd4250" # inherits via dominance
        "/wd4251" # DLL stuff
        "/wd4275" # DLL stuff
        "/wd4244" # return : conversion from foo to bar, possible loss of data
        "/wd4800" # conversion from int to bool, performance warning
        "/wd4804" # unsafe mix of const bool <= const int
        "/wd4805" # unsafe mix of const bool == const int
        "/wd4996" # SCL_SECURE_NO_WARNINGS
        "/wd4503" # decorated name length exceeded
        "/wd4305" # argument : truncation from double to float
        "/MP"
        "/bigobj"
        ${OSSIA_LINK_OPTIONS}
    )
else()
    set(OSSIA_LINK_OPTIONS
        -ffunction-sections
        -fdata-sections
        -gdwarf-4
        )

    if(CMAKE_COMPILER_IS_GNUCXX)
        set(OSSIA_LINK_OPTIONS ${OSSIA_LINK_OPTIONS}
            -Wl,--gc-sections
            -Wl,--gdb-index
            -fvar-tracking-assignments
            -Wl,--compress-debug-sections=zlib
            -gsplit-dwarf
            -Wa,--compress-debug-sections
            -Wl,--dynamic-list-cpp-new
            -Wl,--dynamic-list-cpp-typeinfo
            -Wl,-Bsymbolic-functions
            )
    endif()

    if(OSSIA_CI)
        if(CMAKE_COMPILER_IS_GNUCXX)
            set(OSSIA_LINK_OPTIONS ${OSSIA_LINK_OPTIONS} -s)
        endif()
    endif()

    set(OSSIA_COMPILE_OPTIONS
        -std=c++14
        -Wall
        -Wextra
        -Wno-unused-parameter
        -Wno-unknown-pragmas
        -Wno-missing-braces
        -Wnon-virtual-dtor
        -pedantic
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -pipe
        -Werror=return-type
        -Werror=trigraphs
        -Wmissing-field-initializers
        ${OSSIA_LINK_OPTIONS}
    )

    if(OSSIA_CI)
        if(NOT CMAKE_COMPILER_IS_GNUCXX)
            set(OSSIA_LINK_OPTIONS ${OSSIA_LINK_OPTIONS} -Wl,-S)
        endif()
    endif()

    if("${SUPPORTS_MISLEADING_INDENT_FLAG}")
        set(OSSIA_COMPILE_OPTIONS ${OSSIA_COMPILE_OPTIONS} -Wmisleading-indentation)
    endif()
endif()