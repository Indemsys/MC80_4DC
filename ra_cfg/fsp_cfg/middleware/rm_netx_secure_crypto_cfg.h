/* generated configuration header file - do not edit */
#ifndef RM_NETX_SECURE_CRYPTO_CFG_H_
#define RM_NETX_SECURE_CRYPTO_CFG_H_
#ifdef __cplusplus
extern "C" {
#endif

#if (0)
#define NX_CRYPTO_STANDALONE_ENABLE
#endif
#define NETX_SECURE_CRYPTO_NX_CRYPTO_METHODS_SHA256_ALT   (0)
#define NETX_SECURE_CRYPTO_NX_CRYPTO_METHODS_AES_ALT      (1)
#define NETX_SECURE_CRYPTO_NX_CRYPTO_METHODS_TRNG_ALT     (1)
#define NETX_SECURE_CRYPTO_NX_CRYPTO_METHODS_ECC_ALT      (1)
#define NETX_SECURE_CRYPTO_NX_CRYPTO_METHODS_RSA_1024_ALT (0)
#define NETX_SECURE_CRYPTO_NX_CRYPTO_METHODS_RSA_2048_ALT (1)
#define NETX_SECURE_CRYPTO_NX_CRYPTO_METHODS_RSA_3072_ALT (1)
#define NETX_SECURE_CRYPTO_NX_CRYPTO_METHODS_RSA_4096_ALT (1)
#define NETX_SECURE_CRYPTO_HW_ACCELERATE                  (NETX_SECURE_CRYPTO_NX_CRYPTO_METHODS_SHA256_ALT | \
                                                           NETX_SECURE_CRYPTO_NX_CRYPTO_METHODS_AES_ALT | \
                                                           NETX_SECURE_CRYPTO_NX_CRYPTO_METHODS_TRNG_ALT | \
                                                           NETX_SECURE_CRYPTO_NX_CRYPTO_METHODS_ECC_ALT  | \
                                                           NETX_SECURE_CRYPTO_NX_CRYPTO_METHODS_RSA_2048_ALT | \
                                                           NETX_SECURE_CRYPTO_NX_CRYPTO_METHODS_RSA_3072_ALT | \
                                                           NETX_SECURE_CRYPTO_NX_CRYPTO_METHODS_RSA_4096_ALT)
#define NX_CRYPTO_ECDH_SCRATCH_BUFFER_SIZE                 (2464)
#define NX_CRYPTO_ECDSA_SCRATCH_BUFFER_SIZE                (3016)
#define NETX_SECURE_CRYPTO_SOFTWARE_ONLY                   (0U)
#if (1) /* Use RSA HW */
#if (1U == NETX_SECURE_CRYPTO_NX_CRYPTO_METHODS_RSA_4096_ALT)
 #define NX_CRYPTO_MAX_RSA_MODULUS_SIZE  (4096)
#elif (1U == NETX_SECURE_CRYPTO_NX_CRYPTO_METHODS_RSA_3072_ALT)
 #define NX_CRYPTO_MAX_RSA_MODULUS_SIZE  (3072)
#elif (1U == NETX_SECURE_CRYPTO_NX_CRYPTO_METHODS_RSA_2048_ALT)
 #define NX_CRYPTO_MAX_RSA_MODULUS_SIZE  (2048)
#else
 #define NX_CRYPTO_MAX_RSA_MODULUS_SIZE  (1024)
#endif
#else /* Use RSA SW */
 #define NX_CRYPTO_MAX_RSA_MODULUS_SIZE  (4)
#endif

/* Definition of the NX_CRYPTO_ECDH_MAX_KEY_SIZE is provided in both rm_netx_secure_crypto_cfg.h and nx_user.h
to support larger buffer size to hold the wrapped keys, irrespective of whether NX_CRYPTO_STANDALONE_ENABLE
is defined. */
#ifndef NX_CRYPTO_ECDH_MAX_KEY_SIZE
#define NX_CRYPTO_ECDH_MAX_KEY_SIZE      (80)
#endif

#ifdef __cplusplus
}
#endif
#endif /* RM_NETX_SECURE_CRYPTO_CFG_H_ */
