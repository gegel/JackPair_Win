#ifndef SCALARMULT__
#define SCALARMULT__


#define crypto_scalarmult_curve25519 scalarmult
#define elligator2_isrt r2p



//Diffie-Hellmann
void scalarmult(unsigned char *q, const unsigned char *n, const unsigned char *p);
//public key from private key
void scalarmultbase(unsigned char *p, const unsigned char *n);
//public key from id
void r2p(unsigned char *p, const unsigned char *n);


#endif