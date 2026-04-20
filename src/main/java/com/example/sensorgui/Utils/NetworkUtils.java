package com.example.sensorgui.Utils;


import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.Enumeration;

public class NetworkUtils {

    public static String getLocalIp() {
        try {
            Enumeration<NetworkInterface> interfaces = NetworkInterface.getNetworkInterfaces();

            while (interfaces.hasMoreElements()) {
                NetworkInterface iface = interfaces.nextElement();

                if (iface.isLoopback() || !iface.isUp())
                    continue;

                Enumeration<InetAddress> addresses = iface.getInetAddresses();

                while (addresses.hasMoreElements()) {
                    InetAddress addr = addresses.nextElement();

                    if (!addr.isLoopbackAddress()
                            && addr.getHostAddress().contains(".")       // Only IPv4
                            && !addr.getHostAddress().startsWith("169.254")) { // ignore APIPA
                        return addr.getHostAddress();
                    }
                }
            }
        } catch (Exception ignored) {}

        return null; // nothing found
    }
}
