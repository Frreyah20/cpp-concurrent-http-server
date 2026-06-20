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
#include <atomic>


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

struct Config
{
    int port;
    int workers;
    std::string root;
};
Config config;
bool loadConfig(const std::string& filename)
{
    std::ifstream file(filename);
    if(!file.is_open())
    {
        return false;
    }
    std::string line;
    while(std::getline(file, line))
    {
        size_t pos = line.find('=');
        if(pos == std::string::npos)
        {
            continue;
        }
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos+1);
        if(key == "port")
        {
            config.port = std::stoi(value);
        }
        else if(key == "workers")
        {
            config.workers = std::stoi(value);
        }
        else if(key == "root")
        {
            config.root = value;
        }
    }
    return true;
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
class Logger{
private:
std::ofstream log_file;
std::mutex log_mutex;
    std::string getTimestamp()
    {
        auto now = std::time(NULL);
        std::tm* local_time = std::localtime(&now);
        std::ostringstream oss;
        oss << std::put_time(local_time, "%Y-%m-%d %H:%M:%S");
        return oss.str();

    }
public:
    Logger()
    {
        log_file.open("server.log", std::ios::app); 
    }

    void info(const std::string &message)
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::string log_message = "[" + getTimestamp() + "] [INFO] " + message;
        std::cout << log_message << '\n';
        log_file << log_message << '\n';
        log_file.flush();
    }
    void warning(const std::string& message)
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::string log_message = "[" + getTimestamp() + "] [WARNING] " + message;
        std::cout << log_message << '\n';
        log_file << log_message << '\n';
        log_file.flush();
    }
    void error(const std::string &message)
    {
        std::lock_guard<std::mutex> lock(log_mutex); 
        std::string log_message = "[" + getTimestamp() + "] [ERROR] " + message;
        std::cout << log_message << '\n'; 
        log_file << log_message << '\n';
        log_file.flush();
    }
};

class Router {
private:
    std::unordered_map<std::string, std::function<std::string()>> get_routes;
    std::unordered_map<std::string, std::function<std::string()>> post_routes;

public:
    std::string route(const std::string &method, const std::string &path)
    {
        if(method == "GET")
        {
            auto it = get_routes.find(path);
            if(it != get_routes.end())
            {
                return it->second();
            }
        }
        
        if(method == "POST")
        {
            auto it = post_routes.find(path);
            if(it != post_routes.end())
            {
                return it->second();
            }
        }
        return "404 Not Found";
    }
    void get(const std::string& path, std::function<std::string()> handler)
    {
        get_routes[path] = handler;
    }
    void post(const std::string &path, std::function<std::string()>handler)
    {
        post_routes[path] = handler;
    }
    bool hasRoute(const std::string &method, std::string &path)
    {
        if(method == "GET")
        {
            return get_routes.find(path) != get_routes.end();
        }
        if(method == "POST")
        {
            return post_routes.find(path) != post_routes.end();
        }
        return false;
    }
};

Router router;
Logger logger;

void handleClient(int client_fd){  
    {
        metrics.active_connections++;
    }    
    //std::cout << "Client connected!\n";
    char buffer[1024];

    int bytes_received = recv(client_fd, buffer,sizeof(buffer) - 1,0);
    auto start_time = std::chrono::steady_clock::now();

    if (bytes_received < 0) {
        std::cerr << "Receive failed\n";
    }
    else {
        buffer[bytes_received] = '\0';

        //std::cout << "\n===== REQUEST =====\n";
        //std::cout << buffer;
        //std::cout << "\n===================\n";
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
        /*
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
            std::string response = "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n" 
            "Content-Length: " + 
            std::to_string(body.size()) +
            "\r\n\r\n" +
            body;
            send(client_fd, response.c_str(), response.size(), 0);
            auto end_time = std::chrono::steady_clock::now();
            double latency_ms = std::chrono::duration<double, std::milli> (end_time - start_time).count();
            {
                std::lock_guard<std::mutex> lock(metrics_mutex);
                metrics.requests_served++;
                metrics.active_connections--;
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
            close(client_fd);
            if(metrics.requests_served > 0)
            {
                double avg_latency = metrics.total_latency_ms / metrics.requests_served;
                auto now = std::chrono::steady_clock::now();
                double uptime_seconds = std::chrono::duration<double>(now - metrics.start_time).count();
                double throughput = metrics.requests_served/uptime_seconds;
                
                //std::cout << "Average Latency: " << avg_latency << " ms\n";
                //std::cout << "Throughput: " << throughput << " req/s\n";            
            
            }
            return;
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
        logger.info(method + " " + path + " -> " + std::to_string(status_code));
        std::string response =
            status_line +
            "Content-Type: " + getMimeType(file_path) + "\r\n"+
            "Content-Length: " +
            std::to_string(body.size()) +
            "\r\n\r\n" +
            body;
            
        send(client_fd, response.c_str(), response.size(),0);
    }
    auto end_time = std::chrono::steady_clock::now();
    double latency_ms = std::chrono::duration<double, std::milli> (end_time - start_time).count();
    {
        std::lock_guard<std::mutex> lock(metrics_mutex);
        metrics.requests_served++;
        metrics.active_connections--;
        metrics.total_latency_ms+=latency_ms;
    }
        double error_rate = 100.0 * metrics.errors / metrics.requests_served;
    
    /*std::cout 
        <<"Request: "
        <<metrics.requests_served
        <<" Active: "
        <<metrics.active_connections
        <<" Errors: "
        <<metrics.errors
        <<" Error Rate: "
        <<error_rate
        <<"%\n";
    */
    if(metrics.requests_served > 0)
        {
            double avg_latency = metrics.total_latency_ms / metrics.requests_served;
            auto now = std::chrono::steady_clock::now();
            double uptime_seconds = std::chrono::duration<double>(now - metrics.start_time).count();
            double throughput = metrics.requests_served/uptime_seconds;
            //std::cout << "Average Latency: " << avg_latency << " ms\n";
            //std::cout << "Throughput: " << throughput << " req/s\n";
            
        }
    close(client_fd);

}

void workerThread()
{
    while(true)
    {
        int client_fd;
        std::unique_lock<std::mutex> lock(queue_mutex);
        queue_cv.wait(lock, []{
            return !task_queue.empty();
        });
        client_fd = task_queue.front();
        task_queue.pop();
        lock.unlock();
        std::cout << "Worker picked up a task\n";
        handleClient(client_fd);
    }
}


int main() {
    if (!loadConfig("../config.txt"))
    {
        std::cerr << "Failed to load config\n";
        return 1;
    }
    metrics.start_time = std::chrono::steady_clock::now();
    std::cout << "Port: " << config.port << '\n';
    std::cout << "Workers: " << config.workers << '\n';
    std::cout << "Root: " << config.root << '\n';

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
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
    while (true){
    std::cout << "Waiting for a client...\n";

    int client_fd = accept(server_fd, nullptr, nullptr);

    if (client_fd < 0) {
        std::cerr << "Accept failed\n";
        close(server_fd);
        return 1;
    }
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        task_queue.push(client_fd);
    }
    queue_cv.notify_one();
    }
    
    close(server_fd);

    return 0;
}