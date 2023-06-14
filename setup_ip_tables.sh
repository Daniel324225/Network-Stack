tap=$1
out=$2
cidr=$3

sudo iptables -I INPUT --source $cidr -j ACCEPT
sudo iptables -t nat -I POSTROUTING --out-interface $out -j MASQUERADE
sudo iptables -I FORWARD --in-interface $out --out-interface $tap -j ACCEPT
sudo iptables -I FORWARD --in-interface $tap --out-interface $out -j ACCEPT