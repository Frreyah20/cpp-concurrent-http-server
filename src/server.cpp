#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

int main() {

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;

setsockopt(
    server_fd,
    SOL_SOCKET,
    SO_REUSEADDR,
    &opt,
    sizeof(opt)
);

    if (server_fd < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    sockaddr_in server_addr{};

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);
    setsockopt(...SO_REUSEADDR...)
    if (bind(server_fd,
             (sockaddr*)&server_addr,
             sizeof(server_addr)) < 0) {

        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        std::cerr << "Listen failed\n";
        close(server_fd);
        return 1;
    }
    
    std::cout << "Waiting for a client...\n";

    int client_fd = accept(server_fd, nullptr, nullptr);

    if (client_fd < 0) {
        std::cerr << "Accept failed\n";
        close(server_fd);
        return 1;
    }

    std::cout << "Client connected!\n";
    char buffer[1024];

    int bytes_received = recv(
    client_fd,
        buffer,
        sizeof(buffer) - 1,
        0
    );

    if (bytes_received < 0) {
        std::cerr << "Receive failed\n";
    }
    else {
        buffer[bytes_received] = '\0';
        std::cout << "Received: " << buffer << '\n';
        send(client_fd, buffer, bytes_received, 0);
        std::cout << "Echo sent back to client\n";
    }
    close(client_fd);
    close(server_fd);

    return 0;
}