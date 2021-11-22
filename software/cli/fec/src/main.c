#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <liquid/liquid.h>
#include "debug_macros.h"
struct fec_s {
    // common
    fec_scheme scheme;
    //unsigned int dec_msg_len;
    //unsigned int enc_msg_len;
    float rate;

    // lengths: convolutional, Reed-Solomon
    unsigned int num_dec_bytes;
    unsigned int num_enc_bytes;

    // convolutional : internal memory structure
    unsigned char * enc_bits;
    void * vp;      // decoder object
    int * poly;     // polynomial
    unsigned int R; // primitive rate, inverted (e.g. R=3 for 1/3)
    unsigned int K; // constraint length
    unsigned int P; // puncturing rate (e.g. p=3 for 3/4)
    int * puncturing_matrix;

    // viterbi decoder function pointers
    void*(*create_viterbi)(int);
    //void (*set_viterbi_polynomial)(int*);
    int  (*init_viterbi)(void*,int);
    int  (*update_viterbi_blk)(void*,unsigned char*,int);
    int  (*chainback_viterbi)(void*,unsigned char*,unsigned int,unsigned int);
    void (*delete_viterbi)(void*);

    // Reed-Solomon
    int symsize;    // symbol size (bits per symbol)
    int genpoly;    // generator polynomial
    int fcs;        //
    int prim;       //
    int nroots;     // number of roots in the polynomial
    //int ntrials;    //
    unsigned int rspad; // number of implicit padded symbols
    int nn;         // 2^symsize - 1
    int kk;         // nn - nroots
    void * rs;      // Reed-Solomon internal object

    // Reed-Solomon decoder
    unsigned int num_blocks;    // number of blocks: ceil(dec_msg_len / nn)
    unsigned int dec_block_len; // number of decoded bytes per block: 
    unsigned int enc_block_len; // number of encoded bytes per block: 
    unsigned int res_block_len; // residual bytes in last block
    unsigned int pad;           // padding for each block
    unsigned char * tblock;     // decoder input sequence [size: 1 x n]
    int * errlocs;              // error locations [size: 1 x n]
    int * derrlocs;             // decoded error locations [size: 1 x n]
    int erasures;               // number of erasures

    // encode function pointer
    int (*encode_func)(fec _q,
                       unsigned int _dec_msg_len,
                       unsigned char * _msg_dec,
                       unsigned char * _msg_enc);

    // decode function pointer
    int (*decode_func)(fec _q,
                       unsigned int _dec_msg_len,
                       unsigned char * _msg_enc,
                       unsigned char * _msg_dec);

    // decode function pointer (soft decision)
    int (*decode_soft_func)(fec _q,
                            unsigned int _dec_msg_len,
                            unsigned char * _msg_enc,
                            unsigned char * _msg_dec);
};

int main(int argc, char *argv[])
{
    srand(time(NULL));

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    unsigned int n = 64;                    // decoded message length (bytes)
    fec_scheme fs = LIQUID_FEC_CONV_V615;   // error-correcting scheme

    // compute encoded message length
    unsigned int k = fec_get_enc_msg_length(fs, n);

    DBGPRINTLN_CTX("n %u, k %u", n, k);

    // allocate memory for data arrays
    unsigned char msg_org[n];       // original message
    unsigned char msg_enc[k];       // encoded message
    unsigned char msg_dec[n];       // decoded message

    // create fec objects
    fec encoder = fec_create(fs,NULL);
    fec decoder = fec_create(fs,NULL);

    DBGPRINTLN_CTX("pa %x, pb %x", encoder->poly[0], encoder->poly[1]);

    {
        for(unsigned int i = 0; i < n; i++)
            msg_org[i] = rand() % 256;

        // encode message
        fec_encode(encoder, n, msg_org, msg_enc);

        // decode message
        fec_decode(decoder, n, msg_enc, msg_dec);

        for(unsigned int i = 0; i < n; i++)
            if(msg_org[i] != msg_dec[i])
                DBGPRINTLN_CTX("FEC Error at %u", i);
    }

    // clean up objects
    fec_destroy(encoder);
    fec_destroy(decoder);

    return 0;
}
