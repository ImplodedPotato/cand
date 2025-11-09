set -ex

arg=$1
args="${@:1}"

if [[ "$arg" == 'web' ]]; then
    make PLATFORM=PLATFORM_WEB ${args:3}
    cd ./build/Wasm
    python3 -m http.server
else
    make $args
    ./build/Darwin/cand
fi
