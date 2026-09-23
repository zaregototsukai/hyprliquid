release:
	cmake -B build/Release -DCMAKE_BUILD_TYPE=Release
	cmake --build build/Release -j

debug:
	cmake -B build/Debug -DCMAKE_BUILD_TYPE=Debug
	cmake --build build/Debug -j

clean:
	rm -rf build

install:
	cmake --install build/Release --prefix /usr

uninstall:
	rm /usr/lib/libhyprliquid.so