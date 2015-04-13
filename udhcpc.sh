#!/bin/sh

RESOLV_CONF="/etc/resolv.conf"

[ -n "$1" ] || { echo "Error: should be called from udhcpc"; exit 1; }

[ -n "$broadcast" ] && BROADCAST="broadcast $broadcast"
[ -n "$subnet" ] && NETMASK="netmask $subnet"

case "$1" in
    deconfig)
        ifconfig $interface 0.0.0.0
        if [ "Xwlan0" == "X$interface" ]; then
            wpa_supplicant -B -i $interface -c /tmp/wpa.conf
        fi
        
        ;;

    renew|bound)
        ifconfig $interface $ip $NETMASK $BROADCAST

        if [ -n "$router" ]; then
            while route del default gw 0.0.0.0 dev $interface
            do
                :
            done

            metric=0
            for i in $router
            do
                route add default gw $i dev $interface metric $((metric++))
            done
        fi

        if [ -n "$dns" ]
        then
            echo -n > $RESOLV_CONF
            [ -n "$domain" ] && echo domain $domain >> $RESOLV_CONF
            for i in $dns
            do
                echo adding dns $i
                echo nameserver $i >> $RESOLV_CONF
            done
        fi
        ;;
esac

exit 0
