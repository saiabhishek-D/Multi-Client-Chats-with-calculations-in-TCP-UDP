# Multi Client Chats with Calculations in TCP/UDP

## Lab 10: Multi Client Chatting - TCP and UDP Communication in ns-3

### Description
Simulation of multi-client chatting using TCP and UDP protocols in ns-3 with network performance measurements and server-side calculator.

### Files
- `multi-client-udp.cc` - Multi client chat using UDP protocol
- `multi-client-tcp.cc` - Multi client chat using TCP protocol
- `multi-client-metrics.cc` - Network performance measurements + calculator (client sends CALC:10+20, server returns result)

### How to Run
cd ~/ns-allinone-3.42/ns-3.42
./ns3 run scratch/multi-client-udp
./ns3 run scratch/multi-client-tcp
./ns3 run scratch/multi-client-metrics

### Network Performance Results
- Packet Loss: 0
- Avg Delay: ~2ms
- Throughput: ~0.125 Kbps (CLIENT_A), ~0.06 Kbps (CLIENT_B)

### Environment
- ns-3.42
- Ubuntu 24.04 (WSL)
