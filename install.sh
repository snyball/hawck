SCRIPT=$(realpath "./build-scripts/setup_user.sh")

function show_error() {
    if which zenity; then
        zenity --error --ellipsize --text="$1"
    else
        echo "$1"
    fi
}

## Generate documentation
doxygen

## Run the setup script
msg=$("$SCRIPT" "$(whoami)" "$(realpath .)")
if [ $? -ne 0 ]; then
    show_error "$msg"
    exit $?
fi

## Set up user directories

LOCAL_SHARE="~/.local/share"
if ! [ -d "$LOCAL_SHARE" ]; then
    mkdir -p "$LOCAL_SHARE"
fi

mkdir -p "$LOCAL_SHARE/hawck/scripts"
mkdir -p "$LOCAL_SHARE/hawck/scripts-enabled"
ln -s /usr/share/hawck/LLib "$LOCAL_SHARE/hawck/scripts/LLib"
ln -s /usr/share/hawck/keymaps "$LOCAL_SHARE/hawck/scripts/keymaps"
ln -s /usr/share/hawck/LLib/init.lua "$LOCAL_SHARE/hawck/scripts/init.lua"
