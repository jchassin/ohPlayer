#include <openssl/ssl.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>

// 1. Wipe out the modern macros so we can define the actual raw functions
#undef SSL_library_init
#undef SSL_load_error_strings
#undef OPENSSL_add_all_algorithms_noconf
#undef CRYPTO_cleanup_all_ex_data
#undef ERR_free_strings
#undef ENGINE_cleanup
#undef EVP_cleanup

// 2. Export the exact raw C symbols that libohPipeline.a is hunting for
extern "C" {

    int SSL_library_init(void) {
        return OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
    }

    void SSL_load_error_strings(void) {
        OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS, NULL);
    }

    void OPENSSL_add_all_algorithms_noconf(void) {
        OPENSSL_init_crypto(OPENSSL_INIT_ADD_ALL_CIPHERS | OPENSSL_INIT_ADD_ALL_DIGESTS, NULL);
    }

    void CRYPTO_cleanup_all_ex_data(void) {
        // Automatically handled in OpenSSL 3
    }

    void ERR_free_strings(void) {
        // Automatically handled in OpenSSL 3
    }

    void ENGINE_cleanup(void) {
        // Automatically handled in OpenSSL 3
    }

    void EVP_cleanup(void) {
        // Automatically handled in OpenSSL 3
    }
}
