/*#include <iostream>
#include <string>
#include <openssl/sha.h>

std::string sha256(const std::string& message) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256Context;

    SHA256_Init(&sha256Context);
    SHA256_Update(&sha256Context, message.c_str(), message.length());
    SHA256_Final(digest, &sha256Context);

    std::string hash;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        hash += static_cast<char>(digest[i]);
    }

    return hash;
}

int main() {
    std::string message = "Hello, World!";
    std::string hash = sha256(message);

    std::cout << "Message: " << message << std::endl;
    std::cout << "SHA256 Hash: " << hash << std::endl;

    return 0;
}*/
#include "ns3/core-module.h"
#include <iostream>

int main() {
    int value = 0x01020304;
    unsigned char* bytes = reinterpret_cast<unsigned char*>(&value);

    if (bytes[0] == bytes[sizeof(int) - 1]) {
        std::cout << "Little Endian" << std::endl;
    } else {
        std::cout << "Big Endian" << std::endl;
    }

    return 0;
}




