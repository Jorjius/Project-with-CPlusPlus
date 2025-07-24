
#include <bits/stdc++.h>


#include <iostream>
#include <chrono>
#include <thread>
//#include <vector>
//#include <cstring>
#include <mutex>
#include <algorithm>
#include <cstdlib>

using namespace std;

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")  
    #define close closesocket
#else
    #include <sys/socket.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

struct RequestResult {
    chrono::microseconds response_time;
    int response_length;
    bool success;
};

mutex print_mutex;

string build_request(const char* host, const char* port_str, const char* path) {
    return "GET " + std::string(path) + " HTTP/1.1\r\n"
           "Host: " + std::string(host) + (std::string(port_str) != "80" ? ":" + std::string(port_str) : "") + "\r\n"
           "Connection: close\r\n"
           "User-Agent: benchmark/1.0\r\n"
           "\r\n";
}

void log(const string& msg, bool quiet) {
    if (!quiet) {
        lock_guard<mutex> lock(print_mutex);
        cout << msg << :endl;
    }
}

RequestResult make_request(const char *host, const char *port, const char *path, bool quiet) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    // if (sock == false) {
    //     cout<<"socket not working"<<endl;
    // }
    if (sock == -1) {
        //     cout<<"socket not working"<<endl;
        log("[ERROR] socket creation failed", quiet);
        return {chrono::microseconds(0), 0, false};
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *servinfo = nullptr;
    int res = getaddrinfo(host, port, &hints, &servinfo);
    if (res != 0) {
        log(string("[ERROR] getaddrinfo: ") + to_string(res), quiet);
        close(sock);
        return {chrono::microseconds(0), 0, false};
    }

    res = connect(sock, servinfo->ai_addr, (int)servinfo->ai_addrlen);
    // if (res == false) {
    //    cout<<"connect failed"<<endl;
    // }
    // else{

    // }

    if (res == -1) {
        log("[ERROR] connect failed", quiet);
        freeaddrinfo(servinfo);
        close(sock);
        return {chrono::microseconds(0), 0, false};
    }

    freeaddrinfo(servinfo);

    string request = build_request(host, port, path);
    auto start_time = std::chrono::high_resolution_clock::now();

    if (send(sock, request.c_str(), (int)request.size(), 0) == -1) {
        log("[ERROR] send failed", quiet);
        close(sock);
        return {chrono::microseconds(0), 0, false};
    }

   // vector<string> full_response;
   vector<char> full_response;
    char buffer[4096];
    int bytes_received;

    while ((bytes_received = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        full_response.insert(full_response.end(), buffer, buffer + bytes_received);
    }

    close(sock);

    auto end_time = chrono::high_resolution_clock::now();
    auto response_time = chrono::duration_cast<chrono::microseconds>(end_time - start_time);

    if (full_response.empty()) {
        //cout<<"empty working"<<endl;
        log("[ERROR] Empty response", quiet);
        return {response_time, 0, false};
    }

    log("[SUCCESS] Request completed: " + std::to_string(full_response.size()) + " bytes", quiet);
    return {response_time, (int)full_response.size(), true};
}

void run_benchmark(const char* host, const char* port, const char* path,int num_threads,int requests_per_thread,bool warmup,bool quiet) {
    vector<RequestResult> results;
    mutex results_mutex;

    auto worker = [&](int thread_id) {
        if (warmup) {
            make_request(host, port, path, true);
        }

        for (int i = 0; i < requests_per_thread; ++i) {
            RequestResult result = make_request(host, port, path, quiet);
            {
                lock_guard<mutex> lock(results_mutex);
                results.push_back(result);
            }
        }
    };

    vector<thread> threads;
   // int count_treads =0;
    auto start_total = chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i) {
       // threads.push_back(worker, i);
    // count_treads++;
        threads.emplace_back(worker, i);
    }

   // int verify_thrd = threads.nums();

    for (auto& t : threads) {t.join();}

    auto end_total = chrono::high_resolution_clock::now();
    //double total_time= end_total - start_time;

      auto total_duration = chrono::duration_cast<chrono::microseconds>(end_total - start_time);

    //vector<double> times;
    vector<int64_t> times;
    int total_bytes = 0;
    int success_count = 0;

    for (const auto& r : results) {
        if (r.success) {
            times.push_back(r.response_time.count());
            total_bytes += r.response_length;
            ++success_count;
        }
    }

    if (times.empty()) {
        cout << " All requests failed!" << endl;
        return;
    }

    sort(times.begin(), times.end());
    auto min_time = times.front();
    auto max_time = times.back();
    auto avg_time = accumulate(times.begin(), times.end(), 0LL) / times.size();
    auto avg_length = total_bytes / success_count;
    int idx_90 = (90 * (int)times.size()) / 100;
    auto p90 = times[idx_90];
    double rps = (double)success_count / (total_duration.count() / 1e6);

    // Print results
    cout << "\n Benchmark Results\n";
    cout << "========================\n";
    cout << "Host: " << host << ":" << port << "\n";
    cout << "Path: " << path << "\n";
    cout << "Threads: " << num_threads << "\n";
    cout << "Requests per thread: " << requests_per_thread << "\n";
    cout << "Total requests: " << num_threads * requests_per_thread << "\n";
    cout << "Successful: " << success_count << "\n";
    cout << "Failed: " << (int)results.size() - success_count << "\n";
    cout << "Total time: " << total_duration.count() / 1000.0 << " ms\n";
    cout << "Requests/sec: " << round(rps) << "\n";
    cout << "Avg response time: " << avg_time << " μs (" << avg_time / 1000.0 << " ms)\n";
    cout << "Min response time: " << min_time << " μs\n";
    cout << "Max response time: " << max_time << " μs\n";
    cout << "90th percentile: " << p90 << " μs\n";
    cout << "Avg response size: " << avg_length << " bytes\n";
}

// void print_usage(const char* prog) {
// }

void print_usage(const char* prog) {
    cout << "Usage: " << prog << " [OPTIONS] HOST PORT\n"
              << "Options:\n"
             << "  -p PATH           Path to request (default: /)\n"
             << "  -t N              Number of threads (default: 10)\n"
             << "  -r N              Requests per thread (default: 10)\n"
             << "  -w                Warm-up phase (do one request before timing)\n"
             << "  -q                Quiet mode (no per-request logs)\n"
}

// void chk_link_valid(string &link){

// }

int main(int argc, char *argv[]) {
    const char* host = nullptr;
    const char* port = nullptr;
    const char* path = "/";
    int num_threads = 10;
    int requests_per_thread = 10;
    bool warmup = false;
    bool quiet = false;

  
   // cout<<argc.nums();

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-p" && i + 1 < argc) {
            path = argv[++i];
        } else if (arg == "-t" && i + 1 < argc) {
            num_threads = atoi(argv[++i]);
        } else if (arg == "-r" && i + 1 < argc) {
            requests_per_thread = atoi(argv[++i]);
        } else if (arg == "-w") {
            warmup = true;
        } else if (arg == "-q") {
            quiet = true;
        } else if (arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (!host) {
            host = argv[i];
        } else if (!port) {
            port = argv[i];
        }
    }

    if (!host || !port) {
        cerr << " Error: HOST and PORT are required.\n";
        print_usage(argv[0]);
        return 1;
    }

    if (num_threads <= 0 || requests_per_thread <= 0) {
        cerr << " Threads and requests must be > 0.\n";
        return 1;
    }

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        cerr << " WSAStartup failed!" << endl;
        return 1;
    }
#endif

    log(" Starting benchmark...", quiet);
    run_benchmark(host, port, path, num_threads, requests_per_thread, warmup, quiet);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}