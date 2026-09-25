board := "esp32:esp32:esp32"
port := "/dev/ttyUSB0"
baud := "115200"
build_dir := "build"

default: build

build:
    arduino-cli compile --fqbn {{board}} --build-path {{build_dir}} .

flash: build
    arduino-cli upload --fqbn {{board}} --port {{port}} --input-dir {{build_dir}}

monitor:
    picocom -b {{baud}} {{port}}

clean:
    rm -rf {{build_dir}}
