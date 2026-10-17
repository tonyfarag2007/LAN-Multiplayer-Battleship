#include <SFML/Network.hpp>
#include <iostream>

void startHosting(sf::TcpListener& listener) {
    listener.listen(53000);
    listener.setBlocking(false);

    std::optional<sf::IpAddress> myIp = sf::IpAddress::getLocalAddress();
    if (myIp.has_value()) {
        std::cout << "Hosting on: " << myIp->toString() << std::endl;
    }
}

sf::Socket::Status tryAccept(sf::TcpListener& listener, sf::TcpSocket& socket) {
    sf::Socket::Status status = listener.accept(socket); // returns immediately now
    if (status == sf::Socket::Status::Done) {
        socket.setBlocking(false); // switch the game socket to non-blocking once connected
        std::cout << "Client connected!" << std::endl;
    }
    return status; // Done, NotReady, or Error, caller decides what to do
}

bool startJoining(sf::TcpSocket& socket, const std::string& hostIp) {
    std::optional<sf::IpAddress> ip = sf::IpAddress::resolve(hostIp);
    if (!ip.has_value()) {
        std::cout << "Invalid IP" << std::endl;
        return false;
    }
    socket.setBlocking(false); // non-blocking BEFORE connect, not after
    socket.connect(ip.value(), 53000); // returns immediately, connection proceeds in background
    return true;
}
sf::Socket::Status connectToHost(sf::TcpSocket& socket, const sf::IpAddress& ip) {
    socket.setBlocking(true);
    sf::Socket::Status s = socket.connect(ip, 53000, sf::seconds(5));
    std::cout << "connect() returned " << static_cast<int>(s) << std::endl;
    if (s == sf::Socket::Status::Done) socket.setBlocking(false);
    else socket.disconnect();
    return s;
}

sf::Socket::Status tryConnect(sf::TcpSocket& socket) {
    // Calling connect() again on an in-progress non-blocking socket checks its status
    return socket.getRemoteAddress().has_value() ? sf::Socket::Status::Done : sf::Socket::Status::NotReady;
}