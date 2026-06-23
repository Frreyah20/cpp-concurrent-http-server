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
#include <queue>
#include <vector>
#include <condition_variable>
#include <functional>
#include <ctime>
#include <iomanip>
#include <chrono>
#include <csignal>
#include <atomic>
#include <algorithm>
#include <cctype>
#include <cerrno>
#include "logger/logger.h"
#include "router/router.h"
#include "config/config.h"
#include "threadpool/thread_pool.h"



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

std::queue<int> task_queue;
std::mutex queue_mutex;
std::condition_variable queue_cv;
std::atomic<bool> running(true);
int server_fd;


bool sendRequest(int fd, const std::string& response)
{
    ssize_t sent = send(fd, response.c_str(), response.size(), 0);
    return sent >= 0;
}

struct Metrics
{
    std::atomic<int> requests_served{0};
    std::atomic<int> active_connections{0};
    std::atomic<int> errors{0};
    double total_latency_ms = 0.0;
    std::chrono::steady_clock::time_point start_time;
    
}; 
std::mutex metrics_mutex;
Metrics metrics;
const size_t MAX_HEADER_SIZE = 8192;
const size_t MAX_HEADER_COUNT = 100;

Router router;
Logger logger;

void handleClient(int client_fd){  

    {
        std::lock_guard<std::mutex> lock(metrics_mutex);
        metrics.active_connections++;
    }    
    //std::cout << "Client connected!\n";
    struct timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    while(true)
    {
    std::string request;

while(request.find("\r\n\r\n") == std::string::npos)
{
    char buffer[4096];
    int bytes_received = recv(client_fd,buffer,sizeof(buffer), 0);
    if(bytes_received <= 0)
    {
        if(bytes_received < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            std::string response = 
            "HTTP/1.1 408 Request Timeout\r\n"
            "Connection: close\r\n"
            "Content-Length: 0\r\n\r\n";
            if(!sendRequest(client_fd, response))
            {
                break;
            }
        }
        request.clear();
        break;
    }
    request.append(buffer, bytes_received);
    if(request.size() > MAX_HEADER_SIZE)
    {
        std::string response = "HTTP/1.1 431 Request Header Fields Too Large\r\n"
        "Connection: close\r\n"
        "Content-Length: 0\r\n\r\n";
        if(!sendRequest(client_fd, response))
        {
            break;
        }
        close(client_fd);
        return;
    }
}

if(request.empty())
{
    break;
}

auto start_time = std::chrono::steady_clock::now();

std::istringstream request_stream(request);

    std::string method;
    std::string path;
    std::string version;

    request_stream >> method >> path >> version;
    if(path.find("..") != std::string::npos)
    {
        std::string response = 
        "HTTP/1.1 403 Forbidden\r\n"
        "Connection: close\r\n"
        "Content-Length: 0\r\n\r\n";
        if(!sendRequest(client_fd, response))
            {
                break;
            }
        break;
    }
    if(path.find('\\') != std::string::npos)
    {
        std::string response = 
        "HTTP/1.1 403 Forbidden\r\n"
        "Connection: close\r\n"
        "Content-Length: 0\r\n\r\n";
        if(!sendRequest(client_fd, response))
            {
                break;
            }
        break;
    }
    if(method.empty() || path.empty() || version.empty())
    {
        std::string response = 
        "HTTP/1.1 400 Bad Request\r\n"
        "Connection: close\r\n"
        "Content-Length: 0\r\n\r\n";
        if(!sendRequest(client_fd, response))
            {
                break;
            }
        break;
    }
    if(version != "HTTP/1.0" && version != "HTTP/1.1")
    {
        std::string response = 
        "HTTP/1.1 400 Bad Request\r\n"
        "Connection: close\r\n"
        "Content-Length: 0\r\n\r\n";
       if(!sendRequest(client_fd, response))
            {
                break;
            }
        break;
    }
    if(method != "GET" && method != "POST")
    {
        std::string response = 
        "HTTP/1.1 405 Method Not Allowed\r\n"
        "Connection: close\r\n"
        "Content-Length: 0\r\n\r\n";
        if(!sendRequest(client_fd, response))
            {
                break;
            }
        break;
    }

    std::unordered_map<std::string, std::string> headers;
    std::string line;
    std::getline(request_stream, line);
    size_t header_count = 0;
    while (std::getline(request_stream, line))
    {
        if (line == "\r" || line.empty())
        {
            break;
        }
        header_count++;
        if(header_count > MAX_HEADER_COUNT)
        {
            std::string response = 
            "HTTP/1.1 431 Request Header Fields Too Large\r\n"
            "Connection: close\r\n"
            "Content-Length: 0\r\n\r\n";
            if(!sendRequest(client_fd, response))
            {
                break;
            }
            break;
        }
        size_t colon = line.find(':');
        if (colon == std::string::npos)
        {
            std::string response = 
            "HTTP/1.1 400 Bad Request\r\n"
            "Connection: close\r\n"
            "Content-Length: 0\r\n\r\n";
            if(!sendRequest(client_fd, response))
            {
                break;
            }
            break;
        }
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        if (!value.empty() && value[0] == ' ')
        {
            value.erase(0, 1);
        }
        if (!value.empty() && value.back() == '\r')
        {   
            value.pop_back();
        }
        headers[key] = value;
    }
    bool keep_alive = true;
    if(version == "HTTP/1.0")
    {
        keep_alive = false;
    }
    if(headers.count("Connection"))
    {
        std::string connection = headers["Connection"];
        std::transform(connection.begin(), connection.end(), connection.begin(), ::tolower);
        if(connection == "close")
        {
            keep_alive = false;
        }
        if(connection == "keep-alive")
        {
            keep_alive = true;
        }
    }
    

        /*
        std::cout << "Keep-Alive: " << keep_alive << '\n';
        for (const auto& [key, value] : headers)
        {
            std::cout << key << " -> " << value << '\n';
        }
        */

        //std::cout << "Method: " << method << '\n';
        //std::cout << "Path: " << path << '\n';
        //std::cout << "Version: " << version << '\n';
    logger.info(method + " " + path);
    if(router.hasRoute(method, path))
    {
        std::string body = router.route(method, path);
        std::string connection_header = keep_alive ? 
        "Connection: keep-alive\r\n":
        "Connection: close\r\n";
        std::string response = std::string("HTTP/1.1 200 OK\r\n") + 
        "Content-Type: text/plain\r\n" +  
        connection_header +
        "Content-Length: " + 
        std::to_string(body.size()) +
        "\r\n\r\n" +
        body;
        if(!sendRequest(client_fd, response))
            {
                break;
            }
        auto end_time = std::chrono::steady_clock::now();
        double latency_ms = std::chrono::duration<double, std::milli> (end_time - start_time).count();
        {
            std::lock_guard<std::mutex> lock(metrics_mutex);
            metrics.requests_served++;
            metrics.total_latency_ms += latency_ms;
        }
        double error_rate = 100.0 * metrics.errors / metrics.requests_served;
            /*std::cout << "Requests: "
            <<metrics.requests_served
            <<" Active: "
            <<metrics.active_connections
            <<" Error: "
            <<metrics.errors
            <<" Error Rate: "
            <<error_rate
            <<"%\n";
            */
            //close(client_fd);
        if(metrics.requests_served > 0)
        {
            double avg_latency = metrics.total_latency_ms / metrics.requests_served;
            auto now = std::chrono::steady_clock::now();
            double uptime_seconds = std::chrono::duration<double>(now - metrics.start_time).count();
            double throughput = metrics.requests_served/uptime_seconds;
                //std::cout << "Average Latency: " << avg_latency << " ms\n";
                //std::cout << "Throughput: " << throughput << " req/s\n";            
            
        }
        if(!keep_alive)
        {
            break;
        }
        continue;
    }
        std::string body;
        std::string status_line;
        std::string file_path;
        int status_code = 200;

        if (method == "GET")
        {
            if (path == "/")
            {
                file_path = config.root + "/index.html";
            }
            else
            {
                file_path = config.root + path;
            }

            std::ifstream file(file_path, std::ios::binary);
            if (!file.is_open())
            {
                body = "File Not Found";
                status_line = "HTTP/1.1 404 Not Found\r\n";
                status_code = 404;
                {
                    metrics.errors++;
                } 
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
            status_code = 405;
            {
                std::lock_guard<std::mutex> lock(metrics_mutex);
                metrics.errors++;
            }
        }
        std::string connection_header = keep_alive ? 
            "Connection: keep-alive\r\n" : 
            "Connection: close\r\n";
        std::string response = status_line +
            "Content-Type: "+ getMimeType(file_path) + "\r\n" +
            connection_header +
            "Content-Length: " +
            std::to_string(body.size()) + 
            "\r\n\r\n" + 
            body;
        if(!sendRequest(client_fd, response))
            {
                break;
            } 
        auto end_time = std::chrono::steady_clock::now();
        double latency_ms = std::chrono::duration<double, std::milli> (end_time - start_time).count();
        {
            std::lock_guard<std::mutex> lock(metrics_mutex);
            metrics.requests_served++;
            metrics.total_latency_ms += latency_ms;
        }
        if(!keep_alive)
        {
            break;
        }
   }
   {
    std::lock_guard<std::mutex> lock(metrics_mutex);
    metrics.active_connections--;
   }
   close(client_fd);
}


void signalHandler(int signal)
{
    std::cout << "\nShutdown Signal received..\n";
    running = false;
    close(server_fd);
    queue_cv.notify_all();
}

int main() {
    signal(SIGINT, signalHandler);
    if (!loadConfig("../config.txt"))
    {
        std::cerr << "Failed to load config\n";
        return 1;
    }
    metrics.start_time = std::chrono::steady_clock::now();
    std::cout << "Port: " << config.port << '\n';
    std::cout << "Workers: " << config.workers << '\n';
    std::cout << "Root: " << config.root << '\n';

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (server_fd < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    sockaddr_in server_addr{};

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(config.port);
    if (bind(server_fd, (sockaddr*)&server_addr,sizeof(server_addr)) < 0) {

        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 1024) < 0) {
        std::cerr << "Listen failed\n";
        close(server_fd);
        return 1;
    }

    router.get("/", [](){return "Home Page";});
    router.get("/about", [](){return "About Page";});
    router.post("/api/data", [](){return "Data Received";});
    router.get("/contact", [](){return "Contact Page";});

    std::cout<<router.route("GET", "/") << '\n';
    std::cout<<router.route("GET", "/about") << '\n';
    std::cout<<router.route("POST", "/api/data") << '\n';

    const int NUM_WORKERS = config.workers;
    std::vector<std::thread> workers;
    for(int i = 0; i < NUM_WORKERS; i++)
    {
        workers.emplace_back(workerThread);
    }
    logger.info("Server started");
    logger.warning("Test warning");
    logger.error("Test error");
    while (running){
    std::cout << "Waiting for a client...\n";

    int client_fd = accept(server_fd, nullptr, nullptr);

    if (client_fd < 0) {
        if(!running)
            break;
        std::cerr << "Accept failed\n";
        continue;
    }
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        task_queue.push(client_fd);
    }
    queue_cv.notify_one();
    }
    queue_cv.notify_all();
    //std::cout << "Joining worker thread...\n";
    for(auto &worker : workers)
    {
        worker.join();
        //std::cout << "Worker joined\n";
    }
    //std::cout << "Shutdown complete.\n";
    
    close(server_fd);

    return 0;
}