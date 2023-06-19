tap=$1
out=$2
cidr=$3

if [ $# -ge 3 ]; then
    if ! sudo iptables -C INPUT --source $cidr -j ACCEPT 1>/dev/null 2>&1; then
        sudo iptables -I INPUT --source $cidr -j ACCEPT
    fi
fi

if [ $# -ge 2 ]; then
	if ! sudo iptables -t nat -C POSTROUTING --out-interface $out -j MASQUERADE 1>/dev/null 2>&1; then
		sudo iptables -t nat -I POSTROUTING --out-interface $out -j MASQUERADE
	fi

	if ! sudo iptables -C FORWARD --in-interface $out --out-interface $tap -j ACCEPT 1>/dev/null 2>&1; then
		sudo iptables -I FORWARD --in-interface $out --out-interface $tap -j ACCEPT
	fi

	if ! sudo iptables -C FORWARD --in-interface $tap --out-interface $out -j ACCEPT 1>/dev/null 2>&1; then
		sudo iptables -I FORWARD --in-interface $tap --out-interface $out -j ACCEPT
	fi

fi
