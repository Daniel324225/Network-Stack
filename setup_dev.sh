tap=$1
local=$2
cidr=$3

sudo ip link set dev $tap up
sudo ip route add dev $tap $cidr
sudo ip address add dev $tap local $cidr
