#!/usr/bin/env python3

# ai generated - not checked for code quality! for testing purposes only

import socket
import struct
import sys


def main():
    print("Snooping socket traffic (requires sudo/root)...")
    try:
        # Capture all ethernet frames (ETH_P_ALL = 0x0003)
        s = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.ntohs(3))
    except PermissionError:
        print("Error: Permission denied. Please run with sudo.", file=sys.stderr)
        sys.exit(1)

    print(
        "Listening for Chatty UDP (1337) and dynamic TCP traffic. Press Ctrl+C to stop."
    )
    active_tcp_ports = set()

    try:
        while True:
            raw_data, addr = s.recvfrom(65535)
            # Ethernet header is 14 bytes
            if len(raw_data) < 34:
                continue

            eth_hdr = struct.unpack("!6s6sH", raw_data[:14])
            eth_type = eth_hdr[2]

            if eth_type != 0x0800:  # IPv4 only
                continue

            # IP header
            ip_hdr = struct.unpack("!BBHHHBBH4s4s", raw_data[14:34])
            ip_proto = ip_hdr[6]
            src_ip = socket.inet_ntoa(ip_hdr[8])
            dst_ip = socket.inet_ntoa(ip_hdr[9])

            ip_hdr_len = (ip_hdr[0] & 0x0F) * 4
            if len(raw_data) < 14 + ip_hdr_len:
                continue

            ip_payload = raw_data[14 + ip_hdr_len :]

            if ip_proto == 17:  # UDP
                if len(ip_payload) < 8:
                    continue
                udp_hdr = struct.unpack("!HHHH", ip_payload[:8])
                src_port, dst_port, udp_len, udp_chk = udp_hdr

                if src_port == 1337 or dst_port == 1337:
                    payload = ip_payload[8:udp_len]
                    try:
                        text = payload.decode("utf-8", errors="ignore").strip()
                        if text:
                            print(
                                f"[UDP] {src_ip}:{src_port} -> {dst_ip}:{dst_port} | {text}"
                            )

                            # Parse Hello/Discover to dynamically track TCP ports
                            # Format: HELLO chattyp2p 2 <port> <id>
                            parts = text.split()
                            if len(parts) >= 4 and parts[0] in ("HELLO", "DISCOVER"):
                                try:
                                    port = int(parts[3])
                                    if port > 0:
                                        active_tcp_ports.add(port)
                                except ValueError:
                                    pass
                    except Exception:
                        pass

            elif ip_proto == 6:  # TCP
                if len(ip_payload) < 20:
                    continue
                tcp_hdr = struct.unpack("!HHIIBBHHH", ip_payload[:20])
                src_port, dst_port = tcp_hdr[0], tcp_hdr[1]

                tcp_hdr_len = ((tcp_hdr[4] >> 4) & 0x0F) * 4
                if len(ip_payload) < tcp_hdr_len:
                    continue

                payload = ip_payload[tcp_hdr_len:]

                if payload:
                    try:
                        text = payload.decode("utf-8", errors="ignore").strip()
                        # Display if it belongs to one of our tracked dynamic TCP ports
                        if text and (
                            src_port in active_tcp_ports or dst_port in active_tcp_ports
                        ):
                            print(
                                f"[TCP] {src_ip}:{src_port} -> {dst_ip}:{dst_port} | {text}"
                            )
                    except Exception:
                        pass
    except KeyboardInterrupt:
        print("\nSnooping stopped.")


if __name__ == "__main__":
    main()
