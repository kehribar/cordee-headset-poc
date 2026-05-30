# -----------------------------------------------------------------------------
# 
#
# -----------------------------------------------------------------------------

all:
	rm -rf build && mkdir build && \
	cd build && cmake .. -DPICO_SDK_PATH=~/GitRepo/pico-sdk && \
	cp ../scripts/iterate_osx.sh . && \
	cp ../scripts/iterate_linux.sh . 

clean:
	rm -rf build
