#!/bin/sh

RESTART=0

# Remove all existing configuration

if uci show fastd | grep -q ".mgmt"
then
	uci delete fastd.mgmt
	RESTART=1
fi

if uci show fastd | grep -q ".aaa"
then
	uci delete fastd.aaa
	RESTART=1
fi

if uci show network | grep -q ".mgmt"
then
	uci delete network.mgmt
	RESTART=1
fi

if uci show wireless | grep -q "ssid=13pm.eu"
then
	uci delete $(uci show wireless | grep "ssid=13pm.eu" | awk -F. '{print $1 "." $2}')
fi



# And commit it

if test "$RESTART" == "1"
then
	uci commit fastd
	uci commit network
	uci commit wireless
	/etc/init.d/fastd restart
	/etc/init.d/network restart
fi

exit 0
