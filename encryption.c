#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>

#include <arpa/inet.h> /* For htonl() */

struct encryption_key {
    unsigned char *ek;
    int eklen;
    unsigned char iv[EVP_MAX_IV_LENGTH];
    char *ciphertext;
    int cipertext_len;
};

int do_evp_seal(FILE *rsa_pkey_file, char *in, char *out,
                struct encryption_key *ek)
{
    int retval = 0;
    RSA *rsa_pkey = NULL;
    EVP_PKEY *pkey = EVP_PKEY_new();
    EVP_CIPHER_CTX ctx;
    unsigned char buffer[4096];
    unsigned char buffer_out[4096 + EVP_MAX_IV_LENGTH];
    size_t len;
    int len_out;
    int cipher_len;
    //unsigned char *ek = NULL;
    //int eklen;
    uint32_t eklen_n;
    
    //char *in = "hello world";
    //char *out;
    len = strlen(in);
    
    if (!PEM_read_RSA_PUBKEY(rsa_pkey_file, &rsa_pkey, NULL, NULL))
    {
        fprintf(stderr, "Error loading RSA Public Key File.\n");
        ERR_print_errors_fp(stderr);
        retval = 2;
        goto out;
    }
    
    if (!EVP_PKEY_assign_RSA(pkey, rsa_pkey))
    {
        fprintf(stderr, "EVP_PKEY_assign_RSA: failed.\n");
        retval = 3;
        goto out;
    }
    
    EVP_CIPHER_CTX_init(&ctx);
    ek->ek = malloc(EVP_PKEY_size(pkey));
    
    if (!EVP_SealInit(&ctx, EVP_aes_128_cbc(), &ek->ek, &ek->eklen, ek->iv, &pkey, 1))
    {
        fprintf(stderr, "EVP_SealInit: failed.\n");
        retval = 3;
        goto out_free;
    }
    
    
    /* Now we process the input file and write the encrypted data to the
     * output file. */
    
    if (!EVP_SealUpdate(&ctx, out, &len_out, in, len))
    {
        fprintf(stderr, "EVP_SealUpdate: failed.\n");
        retval = 3;
        goto out_free;
    }
    
    cipher_len = len_out;
    
    
    if (!EVP_SealFinal(&ctx, out+len_out, &len_out))
    {
        fprintf(stderr, "EVP_SealFinal: failed.\n");
        retval = 3;
        goto out_free;
    }
    
    cipher_len += len_out;
    
    ek->ciphertext = out;
    ek->cipertext_len = cipher_len;
    
    printf(out);
    printf("\n");
    
    out_free:
        EVP_PKEY_free(pkey);
        //free(ek);
    
    out:
        return cipher_len;
}


int do_evp_unseal(FILE *rsa_pkey_file, char *plaintext, struct encryption_key *key)
{
    int retval = 0;
    RSA *rsa_pkey = NULL;
    EVP_PKEY *pkey = EVP_PKEY_new();
    EVP_CIPHER_CTX ctx;
    
    int cipertext_len = key->cipertext_len;
    char *ciphertext = key->ciphertext;
    
    int len_out;
    int plaintext_len;
    
    
    if (!PEM_read_RSAPrivateKey(rsa_pkey_file, &rsa_pkey, NULL, NULL))
    {
        fprintf(stderr, "Error loading RSA Private Key File.\n");
        ERR_print_errors_fp(stderr);
        retval = 2;
        goto out;
    }
    
    if (!EVP_PKEY_assign_RSA(pkey, rsa_pkey))
    {
        fprintf(stderr, "EVP_PKEY_assign_RSA: failed.\n");
        retval = 3;
        goto out;
    }
    
    EVP_CIPHER_CTX_init(&ctx);
    
    /* First need to fetch the encrypted key length, encrypted key and IV */
    if (!EVP_OpenInit(&ctx, EVP_aes_128_cbc(), key->ek, key->eklen, key->iv, pkey))
    {
        fprintf(stderr, "EVP_OpenInit: failed.\n");
        retval = 3;
        goto out_free;
    }
    
    if (!EVP_OpenUpdate(&ctx, plaintext, &len_out, ciphertext, cipertext_len))
    {
        fprintf(stderr, "EVP_OpenUpdate: failed.\n");
        retval = 3;
        goto out_free;
    }
    
    plaintext_len = len_out;
    
    
    
    
    if (!EVP_OpenFinal(&ctx, plaintext+plaintext_len, &len_out))
    {
        fprintf(stderr, "EVP_SealFinal: failed.\n");
        retval = 3;
        goto out_free;
    }
    
    plaintext_len += len_out;
    
    out_free:
        EVP_PKEY_free(pkey);
    
    out:
        return plaintext_len;
}


int main(int argc, char *argv[])
{
    FILE *rsa_pkey_file;
    int rv;
    char *in = "encryption worksss dsfsdfs\n";
    char ciphertext[1000];
    struct encryption_key key;
    
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <PEM RSA Public Key File>\n", argv[0]);
        exit(1);
    }
    
    rsa_pkey_file = fopen(argv[1], "rb");
    if (!rsa_pkey_file)
    {
        perror(argv[1]);
        fprintf(stderr, "Error loading PEM RSA Public Key File.\n");
        exit(2);
    }
    
    rv = do_evp_seal(rsa_pkey_file, in, ciphertext, &key);
    
    printf("%s\n----\n", ciphertext);
    printf("%s\n----\n", key.iv);
    
    fclose(rsa_pkey_file);
    
    FILE *secret_key_file;
    char plaintext[100];
    int drv;
    
    secret_key_file = fopen("private.pem", "rb");
    
    drv = do_evp_unseal(secret_key_file, plaintext, &key);
    
    printf("%s\n size: %d \n", plaintext, drv);
    
    
    
    return rv;
}
