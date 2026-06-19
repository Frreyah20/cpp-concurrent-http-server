#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <sstream>
#include <unordered_map>
#include <fstream>
#include <streambuf>
#include <thread>
#include <mutex> 


std::string getMimeType(const std::string& path)
{
    if (path.find(".html") != std::string::npos)
        return "text/html";

    if (path.find(".css") != std::string::npos)
        return "text/css";

    if (path.find(".js") != std::string::npos)
        return "application/javascript";

    if (path.find(".png") != std::string::npos)
        return "image/png";

    return "text/plain";
}

int request_count = 0;
std::mutex request_mutex;

void handleClient(int client_fd){
    std::cout << "Client connected!\n";
    {
        std::lock_guard<std::mutex> lock(request_mutex);
        request_count++;
        std::cout << "Request Count: " << request_count << '\n';
    }
    char buffer[1024];

    int bytes_received = recv(client_fd, buffer,sizeof(buffer) - 1,0);
    if (bytes_received < 0) {
        std::cerr << "Receive failed\n";
    }
    else {
        buffer[bytes_received] = '\0';

        std::cout << "\n===== REQUEST =====\n";
        std::cout << buffer;
        std::cout << "\n===================\n";
        std::istringstream request_stream(buffer);

        std::string method;
        std::string path;
        std::string version;

        request_stream >> method >> path >> version;
        std::unordered_map<std::string, std::string> headers;

std::string line;

std::getline(request_stream, line);

while (std::getline(request_stream, line))
{
    if (line == "\r" || line.empty())
        break;

    size_t colon = line.find(':');

    if (colon != std::string::npos)
    {
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        if (!value.empty() && value[0] == ' ')
            value.erase(0, 1);

        if (!value.empty() && value.back() == '\r')
            value.pop_back();

        headers[key] = value;
    }
}
        std::cout << "\nHeaders:\n";

        for (const auto& [key, value] : headers)
        {
            std::cout << key << " -> " << value << '\n';
        }

        std::cout << "Method: " << method << '\n';
        std::cout << "Path: " << path << '\n';
        std::cout << "Version: " << version << '\n';
        std::string body;
        std::string status_line;
        std::string file_path;

        if (method == "GET")
        {
            if (path == "/")
            {
                file_path = "../public/index.html";
            }
            else
            {
                file_path = "../public" + path;
            }

            std::ifstream file(file_path, std::ios::binary);
            if (!file.is_open())
            {
                body = "File Not Found";
                status_line = "HTTP/1.1 404 Not Found\r\n";
            }
            else
            {
                body.assign(
                    (std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>()
                );

                status_line = "HTTP/1.1 200 OK\r\n";
            }
        }
        else if (method == "POST")
        {
            body = "POST Request Received";
            status_line = "HTTP/1.1 200 OK\r\n";
        }
        else
        {
            body = "Method Not Supported";
            status_line = "HTTP/1.1 405 Method Not Allowed\r\n";
        }
        std::string response =
            status_line +
            "Content-Type: " + getMimeType(file_path) + "\r\n"+
            "Content-Length: " +
            std::to_string(body.size()) +
            "\r\n\r\n" +
            body;
        send(client_fd, response.c_str(), response.size(),0);
    }
    close(client_fd);
}

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
    if (bind(server_fd, (sockaddr*)&server_addr,sizeof(server_addr)) < 0) {

        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        std::cerr << "Listen failed\n";
        close(server_fd);
        return 1;
    }
    while (true){
    std::cout << "Waiting for a client...\n";

    int client_fd = accept(server_fd, nullptr, nullptr);

    if (client_fd < 0) {
        std::cerr << "Accept failed\n";
        close(server_fd);
        return 1;
    }
    std::thread worker(handleClient, client_fd);
    worker.detach();
    }
    
    close(server_fd);

    return 0;
}