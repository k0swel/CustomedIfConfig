#include <sys/types.h>
#include <ifaddrs.h>
#include <cstdio>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <string>
#include <cstring>
#include <vector>
#include <cmath>
#include <linux/if_link.h>

#define MAC_LENGTH_IN_SYMBOL 19
#define IP_LENGTH_IN_SYMBOL 16


class network_interface {
public:
// геттеры
    const uint8_t get_index() const { return this->index; };
    const std::string& get_interface_name() const { return this->interface_name; } ;
    const std::string get_mac_address() const{ return this->mac_address; } ;
    const std::string& get_ip_address() const { return this->ip_address; } ;
    const uint32_t get_rx_packets() const {return this->rx_packets;} ;
    const uint32_t get_rx_bytes() const {return this->rx_bytes;} ;
    const uint32_t get_tx_packets() const { return this->tx_packets;};
    const uint32_t get_tx_bytes() const { return this->tx_bytes;};

// сетеры
    void set_index(const uint8_t& index) { this->index = index;}
    void set_interface_name(const std::string& interface_name) {this->interface_name = std::move(interface_name);}
    void set_mac_address(const std::string& mac_address) { this->mac_address = std::move(mac_address);}
    void set_ip_address(const std::string& ip_address) {this->ip_address = std::move(ip_address);}
    const uint32_t set_rx_packets(const uint32_t& rx_packets)  {return this->rx_packets;} ;
    const uint32_t set_rx_bytes(const uint32_t& rx_bytes)  {return this->rx_bytes;} ;
    const uint32_t set_tx_packets(const uint32_t& tx_packets)  { return this->tx_packets;};
    const uint32_t set_tx_bytes(const uint32_t& tx_bytes)  { return this->tx_bytes;};

private:
    uint8_t index;
    std::string interface_name;
    std::string mac_address;
    std::string ip_address = "-";
    uint32_t tx_packets, rx_packets, tx_bytes, rx_bytes;
};

std::vector<network_interface> all_interfaces;

const std::string get_mac_address(const unsigned char* mac_address_parametr, const size_t& size) {
    char mac_address[MAC_LENGTH_IN_SYMBOL] {'\0'};
    for (int i = 0, j = 0; j < size; i += 3, j +=1) {
        if (j != size - 1)
            sprintf(&mac_address[i], "%02X:", mac_address_parametr[j]);
        else 
            sprintf(&mac_address[i], "%02X", mac_address_parametr[j]);
    }
    return std::string(mac_address);
}

const std::string get_ip4_address(const in_addr& address) {
    char ip_address[IP_LENGTH_IN_SYMBOL] {'\0'};
    if (inet_ntop(AF_INET, &address, &ip_address[0], IP_LENGTH_IN_SYMBOL) == nullptr) {
        perror("get_ip4_address: ");
        return "-";
    }
    return ip_address;
}


void beautiful_output_mac_ip(const std::vector<network_interface>& interfaces) {
    printf("========================================================================================================\n");
    printf("||         index        ||     interface_name     ||       mac_address      ||        ip_address      ||\n");
    printf("========================================================================================================\n");
    for (const network_interface& interface: interfaces) {
        uint8_t sequence_space_if_name = 24 - strlen(interface.get_interface_name().c_str());
        uint8_t sequence_space_if_mac = 24 - strlen(interface.get_mac_address().c_str());
        uint8_t sequence_space_if_ipv4 = 24 - strlen(interface.get_ip_address().c_str());
printf("||%12i%10c||%*c%s%*c||%*c%s%*c||%*c%s%*c||\n", 
       interface.get_index(), 
       ' ',  int(ceil(sequence_space_if_name / (double)2)), ' ', interface.get_interface_name().c_str(), sequence_space_if_name / 2, ' ',
        int(ceil(sequence_space_if_mac / (double)2)), ' ', interface.get_mac_address().c_str(), sequence_space_if_mac / 2, ' ',
        int(ceil(sequence_space_if_ipv4 / (double)2)), ' ', interface.get_ip_address().c_str(), sequence_space_if_ipv4 / 2, ' '
    );
    
}
printf("========================================================================================================\n");


}


int main() {
    ifaddrs* network_interfaces, *first_interface; // связанная сырая структура, которая хранит указатели на каждую структуру интерфейса
    if (getifaddrs(&network_interfaces) == -1) {
        perror("main() getifaddrs: ");
    }
    first_interface = network_interfaces;
    //  вытаскиваем информацию канального уровня
    while (network_interfaces != nullptr) {
        if (network_interfaces->ifa_addr->sa_family == AF_PACKET) {
            sockaddr_ll* device_info = reinterpret_cast<sockaddr_ll*>(network_interfaces->ifa_addr);
            rtnl_link_stats* statistics = reinterpret_cast<rtnl_link_stats*>(network_interfaces->ifa_data);
            const char* device_name = network_interfaces->ifa_name;
            const std::string& mac_address = get_mac_address(device_info->sll_addr, device_info->sll_halen);
            const uint8_t index = device_info->sll_ifindex;
            
            network_interface interface;
            interface.set_index(index);
            interface.set_interface_name(device_name);
            interface.set_mac_address(mac_address);
            interface.set_rx_packets(statistics->tx_packets);
            interface.set_rx_bytes(statistics->rx_bytes);
            interface.set_tx_packets(statistics->tx_packets);
            interface.set_tx_bytes(statistics->tx_bytes);
            all_interfaces.push_back(interface);
        }
        network_interfaces = network_interfaces->ifa_next;
    }
    network_interfaces = first_interface;

    //  вытаскиваем информацию сетевого уровня (ipv4)
    while (network_interfaces != nullptr) {
        if (network_interfaces->ifa_addr->sa_family == AF_INET) {
            sockaddr_in* device_info = reinterpret_cast<sockaddr_in*>(network_interfaces->ifa_addr);
            const char* device_name = network_interfaces->ifa_name;
            const std::string& ip_address = get_ip4_address(device_info->sin_addr);


            for (network_interface& interface: all_interfaces) {
                if (strcmp(interface.get_interface_name().c_str(), device_name) == 0) {
                    interface.set_ip_address(ip_address);
                    break;
                }
            }
        }
        network_interfaces = network_interfaces->ifa_next;
    }
    network_interfaces = first_interface;

    freeifaddrs(network_interfaces);
    beautiful_output_mac_ip(all_interfaces);
}