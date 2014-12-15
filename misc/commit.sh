#!/bin/sh

# Get private key and ID
PRIVATE_KEY=$(grep "\"privateKey\"" /etc/api/localConf.json | awk -F\" '{print $4}')
ID=$(grep "\"id\"" /etc/api/localConf.json | awk -F\" '{print $4}')

# Set fastd configuration
uci set fastd.mgmt=fastd
uci set fastd.mgmt.enabled=1
uci set fastd.mgmt.syslog_level=info
uci set fastd.mgmt.mode=tap
uci set fastd.mgmt.interface=fastd0
uci set fastd.mgmt.mtu=1426
uci set fastd.mgmt.forward=0
uci set fastd.mgmt.secure_handshakes=1
uci set fastd.mgmt.secret=$PRIVATE_KEY
uci add_list fastd.mgmt.method=xsalsa20-poly1305
uci set fastd.aaa=peer
uci set fastd.aaa.enabled=1
uci set fastd.aaa.net=mgmt
uci set fastd.aaa.key=$AAA_PUBLIC_KEY
uci add_list fastd.aaa.remote=\"$AAA_FQDN\"\ port\ 1337
uci set network.mgmt=interface
uci set network.mgmt.proto=static
uci set network.mgmt.ifname=fastd0
uci set network.mgmt.ip6addr=$IPV6_ADDR/64

# Remove default Wi-Fi network
if uci show wireless | grep -q "ssid=OpenWrt"
then
	uci delete $(uci show wireless | grep "ssid=OpenWrt" | awk -F. '{print $1 "." $2}')
fi

# Set Wi-Fi configuration
IFACE=$(uci add wireless wifi-iface)
uci set wireless.$IFACE.device=radio0
uci set wireless.$IFACE.mode=ap
uci set wireless.$IFACE.wmm=0
uci set wireless.$IFACE.encryption=wpa2+ccmp
uci set wireless.$IFACE.ssid=13pm.eu
uci set wireless.$IFACE.network=lan
uci set wireless.$IFACE.auth_server=$AAA_IPV6
uci set wireless.$IFACE.auth_secret=$AAA_SECRET
uci set wireless.$IFACE.acct_server=$AAA_IPV6
uci set wireless.$IFACE.acct_secret=$AAA_SECRET
uci set wireless.$IFACE.nasid=$ID
uci set wireless.radio0.disabled=0

# And here we go
uci commit fastd
uci commit wireless
uci commit network
/etc/init.d/fastd restart
/etc/init.d/network restart

exit 0
