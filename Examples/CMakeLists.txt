# Miro examples. Each is self-contained and skips its Swift target unless
# a Swift compiler is available (on macOS that means the Ninja generator).

add_subdirectory(MiroSwiftExport)  # Swift -> C++ (Swift calls into a Bridge)
add_subdirectory(MiroSwiftServer)  # C++ -> Swift (C++ calls into Swift handlers)
