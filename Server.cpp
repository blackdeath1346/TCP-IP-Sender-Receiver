#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring> // For memset
#include <vector>  // For std::vector
#include "checksum.h"
#include "crc.h"
using namespace std;

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cerr << "Error opening socket" << endl;
        return 1;
    }

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(9999);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Error binding socket" << endl;
        close(sockfd);
        return 1;
    }

    listen(sockfd, 5);
    cout << "Waiting for connections..." << endl;
    //string checksum;
    while (true) {
        int newsockfd = accept(sockfd, nullptr, nullptr);
        if (newsockfd < 0) {
            cerr << "Error accepting connection" << endl;
            continue;
        }

        cout << "Connection accepted" << endl;
        char buffer0[256];
        ssize_t bytes_received0 = recv(newsockfd, buffer0, sizeof(buffer0) - 1, 0);
        buffer0[bytes_received0] = '\0'; // Null-terminate the buffer
        string checksum = buffer0;
        //cout<<checksum<<endl<<endl;
        string message;
        char buffer[256];
        ssize_t bytes_received;
        vector<string> chunks;
        int f=0;
        while ((bytes_received = recv(newsockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes_received] = '\0'; 
            for (size_t i = 0; i < bytes_received; i += 16) {
                string chunk(buffer + i, buffer + min((long)i + 16, bytes_received)); 
                chunks.push_back(chunk); 
                if (chunk.find("EOF") != string::npos) {
                    chunks.pop_back();
                    f=1;
                    break;
                }
            }
            if(f==1)
            break;
        }

        if (bytes_received < 0) {
            cerr << "Error receiving message" << endl;
        } else 
        {
            if(Checksum::check_checksum(chunks,checksum))
            cout<<"No errors detected...Message accepted(According to Checksum)"<<endl;
            else
            cout<<"Erroneous message received...Discarding(According to Checksum)"<<endl;
            /*cout << "Chunks received:" << endl;
            for (const auto& chunk : chunks) {
                cout << chunk << endl; // Print each chunk
            }*/
        }

        char buffer2[256];
        ssize_t bytes_received2 = recv(newsockfd, buffer2, sizeof(buffer2) - 1, 0);
        buffer2[bytes_received2] = '\0'; // Null-terminate the buffer
        string key = buffer2;

        vector<string> crc_chunks;
        int crc_f = 0;
        size_t chunk_size = 16 + key.length() - 1; // Data length + remainder length
        while ((bytes_received = recv(newsockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes_received] = '\0';
            for (size_t i = 0; i < bytes_received; i += chunk_size) {
                string chunk(buffer + i, buffer + min((long)(i + chunk_size), bytes_received));
                crc_chunks.push_back(chunk);
                if (chunk.find("EOF") != string::npos) {
                    crc_chunks.pop_back();
                    crc_f = 1;
                    break;
                }
            }
            if (crc_f == 1) break;
        }

        if (bytes_received < 0) {
            cerr << "Error receiving CRC message" << endl;
        } else {
            if (crc::checkRemainder(crc_chunks, key))
                cout << "No errors detected...Message accepted (According to CRC)" << endl;
            else
                cout << "Erroneous message received...Discarding (According to CRC)" << endl;
        }


        close(newsockfd); // Close the new socket after handling the connection
    }

    close(sockfd); // Close the main socket when done (this won't be reached in this loop)
    return 0;
}
