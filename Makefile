NO_SYSTEMD ?= OFF

release:
	cmake -B build/Release -DCMAKE_BUILD_TYPE=Release -DNO_SYSTEMD=$(NO_SYSTEMD)
	cmake --build build/Release -j

debug:
	cmake -B build/Debug -DCMAKE_BUILD_TYPE=Debug -DNO_SYSTEMD=$(NO_SYSTEMD)
	cmake --build build/Debug -j

clean:
	rm -rf build

install:
	cmake --install build/Release --prefix /usr

uninstall:
	rm /usr/lib/libhyprliquid.so