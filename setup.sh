if [ ! -e /dev/net/tap ]
then
    sudo mknod /dev/net/tap c 10 200
    sudo chmod 0666 /dev/net/tap
fi
sudo setcap cap_setpcap,cap_net_admin=ep "$(dirname $0)/TCP_IP"