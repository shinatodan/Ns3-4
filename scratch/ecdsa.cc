#include <iostream>
#include <iomanip>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/sha.h>

void handleErrors()
{
  ERR_print_errors_fp(stderr);
  abort();
}

int main()
{
  std::string message = "Hello, OpenSSL ECDSA!";

  EC_KEY* ecKey = EC_KEY_new_by_curve_name(NID_secp256k1);//ECキー生成
  if (ecKey == nullptr)
  {
    std::cerr << "Failed to create EC key" << std::endl;
    handleErrors();
  }

  if (EC_KEY_generate_key(ecKey) != 1)//公開鍵、秘密鍵ペア生成
  {
    std::cerr << "Failed to generate EC key pair" << std::endl;
    handleErrors();
  }

  // ECキーの表示
  EC_KEY_print_fp(stdout, ecKey, 0);

  unsigned char digest[SHA256_DIGEST_LENGTH];//ハッシュ値計算
  SHA256(reinterpret_cast<const unsigned char*>(message.c_str()), message.length(), digest);

  ECDSA_SIG* signature = ECDSA_do_sign(digest, SHA256_DIGEST_LENGTH, ecKey);//署名生成
  if (signature == nullptr)
  {
    std::cerr << "Failed to generate ECDSA signature" << std::endl;
    handleErrors();
  }

  if (ECDSA_do_verify(digest, SHA256_DIGEST_LENGTH, signature, ecKey) == 1)//署名検証　成功時１
  {
    std::cerr << "ECDSA signature verification succeeded" << std::endl;
  }
  else
  {
    std::cerr << "ECDSA signature verification failed" << std::endl;
    handleErrors();
  }

  ECDSA_SIG_free(signature);
  EC_KEY_free(ecKey);

  return 0;
}
