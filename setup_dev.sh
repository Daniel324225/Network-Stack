tap=$1
local=$2
cidr=$3

if [ $# -ge 1 ]; then
    sudo ip link set dev $tap up
fi
if [ $# -ge 2 ]; then
    sudo ip address add dev $tap local $local
fi
if [ $# -ge 3 ]; then
    sudo ip route add dev $tap $cidr
fi
