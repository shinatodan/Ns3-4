#include <iostream>
#include <iomanip>
#include <openssl/dsa.h>
#include <openssl/err.h>
#include <openssl/sha.h>

void handleErrors()
{
  ERR_print_errors_fp(stderr);
  abort();
}

int main()
{
  std::string message = "Hello, OpenSSL DSA!";

  DSA* dsa = DSA_new();
  if (dsa == nullptr)
  {
    std::cerr << "Failed to create DSA key" << std::endl;
    handleErrors();
  }

  if (DSA_generate_parameters_ex(dsa, 2048, nullptr, 0, nullptr, nullptr, nullptr) != 1)
  {
    std::cerr << "Failed to generate DSA parameters" << std::endl;
    handleErrors();
  }

  // DSAパラメータの表示
  DSA_print_fp(stdout, dsa, 0);

  if (DSA_generate_key(dsa) != 1)
  {
    std::cerr << "Failed to generate DSA key pair" << std::endl;
    handleErrors();
  }

  // DSAキーの表示
  DSA_print_fp(stdout, dsa, 0);

  unsigned char digest[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(message.c_str()), message.length(), digest);

  unsigned char signature[DSA_size(dsa)];
  unsigned int signatureLength;
  if (DSA_sign(0, digest, SHA256_DIGEST_LENGTH, signature, &signatureLength, dsa) != 1)
  {
    std::cerr << "Failed to generate DSA signature" << std::endl;
    handleErrors();
  }

  if (DSA_verify(0, digest, SHA256_DIGEST_LENGTH, /*signature_bytes*/signature, signatureLength, dsa) == 1)
  {
    std::cerr << "DSA signature verification succeeded" << std::endl;
  }
  else if (DSA_verify(0, digest, SHA256_DIGEST_LENGTH, /*signature_bytes*/signature, signatureLength, dsa) != 1)
  {
    std::cerr << "DSA signature verification failed" << std::endl;
    handleErrors();
  }

  DSA_free(dsa);

  return 0;
}
