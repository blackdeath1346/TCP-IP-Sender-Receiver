#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <unistd.h> // for usleep
#include <arpa/inet.h> // for socket functions
#include <sys/socket.h>
#include <bitset>

#include "checksum.h" // Include your checksum header
#include "crc.h" // Include your CRC header
/*#include "helper.h" // Include your helper functions*/

using namespace std;
string inject_error(string text, int number) 
{
    if (number == 0) 
    {
        return text;
    }
    for (int i = 0; i < number; i++) 
    {
        int x = rand() % text.length();
        text[x] = (text[x] == '0') ? '1' : '0'; // Flip the bit
    }
    return text;
}
void injectBurstError(string &text, int x) {
    if (x <= 0 || text.empty()) {
        return;
    }

    // Initialize random seed
    srand(time(0));

    // Choose a random starting index within the range that allows flipping `x` bits
    int start_index = rand() % (text.length() - x + 1);

    // Flip the bits for the following `x` characters
    for (int i = 0; i < x; ++i) 
    {
        if(start_index + i==text.length())
        return;
        if (text[start_index + i] == '0') 
        {
            text[start_index + i] = '1';
        } else if (text[start_index + i] == '1') 
        {
            text[start_index + i] = '0';
        }
    }
}
void send_data(int sock, const string& data) 
{
    if (send(sock, data.c_str(), data.size(), 0) < 0) 
    {
        cerr << "Error sending data!" << endl;
    }
}

int main() 
{
    const char* HOST = "127.0.0.1"; // Use IP address for inet_pton
    const int PORT = 9999;
    const int PKT_SIZE = 16;

    // Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) 
    {
        cerr << "Error creating socket!" << endl;
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, HOST, &server_addr.sin_addr) <= 0) 
    {
        cerr << "Invalid address/ Address not supported!" << endl;
        close(sock);
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) 
    {
        cerr << "Error connecting to server!" << endl;
        close(sock);
        return -1;
    }

    // User inputs
    string enc_text;
    cout << "Enter data: ";
    getline(cin, enc_text);

    // Convert to binary
    /*string enc_text;
    for (char c : text) {
        enc_text += bitset<8>(c).to_string(); // Convert each character to 8-bit binary
    }*/

    // Padding
    int actual_len = enc_text.length();
    if (actual_len % PKT_SIZE != 0) 
    {
        enc_text += string(PKT_SIZE - (actual_len % PKT_SIZE), '0'); // Add padding
    }

    // Split into chunks
    vector<string> chunks;
    for (size_t i = 0; i < enc_text.length(); i += PKT_SIZE) 
    {
        chunks.push_back(enc_text.substr(i, PKT_SIZE)); // Split into chunks
    }
    string checksum=Checksum::generate_checksum(chunks);
   // vector<string> chunks3=
    //cout<<"checksum of chunks: "<<Checksum::generate_checksum(chunks);
    // Send checksum
    cout<<"How many bits of error do you want for checksum?";
    int num;
    cin>>num;
    send_data(sock, checksum);
    usleep(1000);
    // Inject error
    string newtext=inject_error(enc_text,num);
    cout<<"Enter size of burst error you want for checksum?";
    int num4;
    cin>>num4;
    injectBurstError(newtext,num4);
    vector<string> chunks2;
    for (size_t i = 0; i < newtext.length(); i += PKT_SIZE) 
    {
        chunks2.push_back(newtext.substr(i, PKT_SIZE)); // Split into chunks
    }
    for (const auto& chunk : chunks2) 
    {
        usleep(1000); // Sleep for 1ms
        send_data(sock, chunk);
    }
    send_data(sock, "EOF");
    string key;
    cout<<"Enter Key for crc\n";
    cin>>key;
    vector<string> chunks3= crc::encodeData(chunks,key);
    string temp;
    for(int i=0;i<chunks3.size();i++)
    {
        temp+=chunks3[i];
    }
    int size=chunks3[0].size();
    cout<<"How many bits of isolated error do you want for crc?";
    int num2;
    cin>>num2;
    string final=inject_error(temp,num2);
    cout<<"Enter size of burst error you want for crc?";
    int num3;
    cin>>num3;
    injectBurstError(final,num3);
    vector<string> chunks4;
    for (size_t i = 0; i < final.length(); i += size) 
    {
        chunks4.push_back(final.substr(i, size)); // Split into chunks
    }
    send_data(sock,key);
    for (const auto& chunk : chunks4) 
    {
        usleep(1000); // Sleep for 1ms
        send_data(sock, chunk);
    }
    send_data(sock, "EOF");

    close(sock);
    return 0;
}
