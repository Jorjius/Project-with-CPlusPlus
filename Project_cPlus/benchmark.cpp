
#define _WIN32_WINNT 0x0600  // Required for GetTickCount64

#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>

// Windows headers
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>  // For getaddrinfo
#pragma comment(lib, "ws2_32.lib")

// Data structures
struct RequestResult {
    long long response_time_us;
    int response_length;
    bool success;
};


CRITICAL_SECTION g_cs;
bool g_quiet = false;

void log(const char* msg) {
    if (!g_quiet) {
        EnterCriticalSection(&g_cs);
        std::cout << msg << std::endl;
        LeaveCriticalSection(&g_cs);
    }
}

std::string build_request(const char* host, const char* port, const char* path) {
    return "GET " + std::string(path) + " HTTP/1.1\r\n"
           "Host: " + std::string(host) + (std::string(port) != "80" ? ":" + std::string(port) : "") + "\r\n"
           "Connection: close\r\n"
           "User-Agent: benchmark/1.0\r\n"
           "\r\n";
}

long long manual_sum(const std::vector<long long>& vec) {
    long long s = 0;
    for (size_t i = 0; i < vec.size(); ++i) {
        s += vec[i];
    }
    return s;
}

long long my_round(double x) {
    return (long long)(x + 0.5);
}

// Forward declare ThreadData
struct ThreadData {
    const char* host;
    const char* port;
    const char* path;
    int requests_per_thread;
    std::vector<RequestResult>* results;
    CRITICAL_SECTION* cs;
};

// Worker function
DWORD WINAPI make_requests(LPVOID param) {
    ThreadData* data = (ThreadData*)param;

    // Warm-up: one untimed request
    {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock != -1) {
            struct addrinfo hints, *servinfo = 0;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            if (getaddrinfo(data->host, data->port, &hints, &servinfo) == 0) {
                connect(sock, servinfo->ai_addr, (int)servinfo->ai_addrlen);
                freeaddrinfo(servinfo);
            }
            closesocket(sock);
        }
    }

    for (int i = 0; i < data->requests_per_thread; ++i) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) continue;

        struct addrinfo hints, *servinfo = 0;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        int res = getaddrinfo(data->host, data->port, &hints, &servinfo);
        if (res != 0 || !servinfo) {
            closesocket(sock);
            continue;
        }

        res = connect(sock, servinfo->ai_addr, (int)servinfo->ai_addrlen);
        freeaddrinfo(servinfo);

        if (res == SOCKET_ERROR) {
            closesocket(sock);
            continue;
        }

        std::string request = build_request(data->host, data->port, data->path);
        DWORD start_time = GetTickCount();  // Use GetTickCount (safe for old MinGW)

        send(sock, request.c_str(), (int)request.size(), 0);

        std::vector<char> full_response;
        char buffer[4096];
        int bytes;
        while ((bytes = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
            full_response.insert(full_response.end(), buffer, buffer + bytes);
        }

        closesocket(sock);

        DWORD end_time = GetTickCount();
        long long response_time_us = (end_time - start_time) * 1000LL;  // ms → μs

        RequestResult result;
        result.response_time_us = response_time_us;
        result.response_length = (int)full_response.size();
        result.success = !full_response.empty();

        if (result.success) {
            char msg[128];
            sprintf(msg, "[SUCCESS] %d bytes received", result.response_length);
            log(msg);
        } else {
            log("[ERROR] Empty or failed response");
        }

        EnterCriticalSection(data->cs);
        data->results->push_back(result);
        LeaveCriticalSection(data->cs);
    }

    delete data;
    return 0;
}

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [OPTIONS] HOST PORT\n"
              << "Options:\n"
              << "  -p PATH       Path to request (default: /)\n"
              << "  -t N          Number of threads (default: 5)\n"
              << "  -r N          Requests per thread (default: 5)\n"
              << "  -q            Quiet mode\n"
              << "  -h            Show help\n";
}

int main(int argc, char* argv[]) {
    const char* host = 0;
    const char* port = 0;
    const char* path = "/";
    int num_threads = 5;
    int requests_per_thread = 5;
    g_quiet = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-p") == 0 && i+1 < argc) {
            path = argv[++i];
        } else if (strcmp(argv[i], "-t") == 0 && i+1 < argc) {
            num_threads = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-r") == 0 && i+1 < argc) {
            requests_per_thread = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-q") == 0) {
            g_quiet = true;
        } else if (strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (!host) {
            host = argv[i];
        } else if (!port) {
            port = argv[i];
        }
    }

    if (!host || !port) {
        std::cerr << "❌ HOST and PORT required.\n";
        print_usage(argv[0]);
        return 1;
    }

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "❌ WSAStartup failed.\n";
        return 1;
    }

    InitializeCriticalSection(&g_cs);
    log(" Starting benchmark...");

    std::vector<RequestResult> results;
    std::vector<HANDLE> threads;
    DWORD start_total = GetTickCount();

    for (int i = 0; i < num_threads; ++i) {
        ThreadData* data = new ThreadData;
        data->host = host;
        data->port = port;
        data->path = path;
        data->requests_per_thread = requests_per_thread;
        data->results = &results;
        data->cs = &g_cs;

        HANDLE h = CreateThread(0, 0, make_requests, data, 0, 0);
        if (h) threads.push_back(h);
    }

    WaitForMultipleObjects((DWORD)threads.size(), &threads[0], TRUE, INFINITE);

    for (size_t i = 0; i < threads.size(); ++i) {
        CloseHandle(threads[i]);
    }

    DWORD end_total = GetTickCount();
    long long total_duration_us = (end_total - start_total) * 1000LL;

    std::vector<long long> times;
    long long total_bytes = 0;
    int success_count = 0;

    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i].success) {
            times.push_back(results[i].response_time_us);
            total_bytes += results[i].response_length;
            ++success_count;
        }
    }

    if (times.empty()) {
        std::cout << "❌ All requests failed!\n";
        DeleteCriticalSection(&g_cs);
        WSACleanup();
        return 1;
    }

    // Simple sort (bubble)
    for (size_t i = 0; i < times.size(); ++i) {
        for (size_t j = i+1; j < times.size(); ++j) {
            if (times[i] > times[j]) {
                long long t = times[i]; times[i] = times[j]; times[j] = t;
            }
        }
    }

    long long min_time = times[0];
    long long max_time = times[times.size()-1];
    long long avg_time = manual_sum(times) / (long long)times.size();
    long long avg_length = total_bytes / success_count;
    size_t p90_idx = (90 * times.size()) / 100;
    long long p90 = times[p90_idx];
    double rps = (double)success_count / (total_duration_us / 1e6);

    std::cout << "\n Benchmark Results\n";
    std::cout << "========================\n";
    std::cout << "Host: " << host << ":" << port << "\n";
    std::cout << "Path: " << path << "\n";
    std::cout << "Threads: " << num_threads << "\n";
    std::cout << "Requests per thread: " << requests_per_thread << "\n";
    std::cout << "Total requests: " << num_threads * requests_per_thread << "\n";
    std::cout << "Successful: " << success_count << "\n";
    std::cout << "Failed: " << (int)results.size() - success_count << "\n";
    std::cout << "Total time: " << total_duration_us / 1000.0 << " ms\n";
    std::cout << "Requests/sec: " << my_round(rps) << "\n";
    std::cout << "Avg response time: " << avg_time << " μs (" << avg_time / 1000.0 << " ms)\n";
    std::cout << "Min response time: " << min_time << " μs\n";
    std::cout << "Max response time: " << max_time << " μs\n";
    std::cout << "90th percentile: " << p90 << " μs\n";
    std::cout << "Avg response size: " << avg_length << " bytes\n";

    DeleteCriticalSection(&g_cs);
    WSACleanup();
    return 0;
}