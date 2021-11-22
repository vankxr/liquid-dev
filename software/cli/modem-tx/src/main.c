#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <liquid/liquid.h>
#include "debug_macros.h"

volatile uint8_t ubStop = 0;

void manchester_encode(uint8_t *pubInput, uint32_t ulInputSize, uint8_t *pubOutput)
{
    for(uint32_t i = 0; i < ulInputSize; i++)
    {
        pubOutput[i * 2 + 0] = 0x00;
        pubOutput[i * 2 + 1] = 0x00;

        for(uint8_t j = 8; j > 0; j--)
        {
            if((pubInput[i] >> (j - 1)) & 1)
                pubOutput[i * 2 + (j < 5)] |= 0b01 << (((j - 1) * 2) % 8);
            else
                pubOutput[i * 2 + (j < 5)] |= 0b10 << (((j - 1) * 2) % 8);
        }
    }
}
int32_t manchester_weight(uint8_t *pubInput, uint32_t ulInputSize)
{
    int32_t lWeight = 0;

    for(uint32_t i = 0; i < ulInputSize; i++)
        for(uint8_t j = 8; j > 0; j -= 2)
            if(((pubInput[i] >> (j - 1)) ^ (pubInput[i] >> (j - 2))) & 1)
                lWeight++;
            else
                lWeight--;

    return lWeight;
}
void manchester_decode(uint8_t *pubInput, uint32_t ulInputSize, uint8_t *pubOutput)
{
    uint8_t *pubInputShifted = (uint8_t *)malloc(ulInputSize);

    for(uint32_t i = 0; i < ulInputSize; i++)
        pubInputShifted[i] = pubInput[i] << 1 | ((i == (ulInputSize - 1) ? pubInput[0] : pubInput[i + 1]) >> 7);

    int32_t lWeight = manchester_weight(pubInput, ulInputSize);
    int32_t lWeightShifted = manchester_weight(pubInputShifted, ulInputSize);

    uint8_t *pubCorrect = NULL;
    uint32_t ulCorrectSize = 0;

    if(lWeight >= lWeightShifted)
        pubCorrect = pubInput;
    else
        pubCorrect = pubInputShifted;

    for(uint32_t i = 0; i < ulInputSize; i += 2)
    {
        pubOutput[i >> 1] = 0x00;

        for(uint8_t j = 0; j < 8; j += 2)
        {
            pubOutput[i >> 1] >>= 1;
            pubOutput[i >> 1] |= ((pubCorrect[i + 1] >> j) & 1) << 3;
            pubOutput[i >> 1] |= ((pubCorrect[i] >> j) & 1) << 7;
        }
    }

    if(pubCorrect == pubInputShifted)
        pubOutput[ulInputSize - 1] &= 0xFC;
}
uint8_t differential_encode(uint8_t *pubInput, uint32_t ulInputSize, uint8_t *pubOutput, uint8_t ubPrev)
{
    for(uint32_t i = 0; i < ulInputSize; i++)
    {
        for(uint8_t j = 8; j > 0; j--)
        {
            ubPrev ^= (pubInput[i] >> (j - 1)) & 1;
            pubInput[i] = (pubInput[i] & ~(1 << (j - 1))) | (ubPrev << (j - 1));
        }
    }

    return ubPrev;
}

void signal_handler(int iSignal)
{
    DBGPRINTLN_CTX("Got signal %d, stop sampling...", iSignal);

    ubStop = 1;
}

int main(int argc, char *argv[])
{
    /* Manchester test
    uint8_t x[10] = {0xDA, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xBC};
    uint8_t x_man[20];
    uint8_t x_man_s[20];
    uint8_t x_dec[10];
    uint8_t x_dec_s[10];

    DBGPRINTLN_CTX("x - [");
    for(int i = 0; i < sizeof(x); i++)
        DBGPRINT("%02X ", x[i]);
    DBGPRINTLN("]");

    manchester_encode(x, 10, x_man);

    DBGPRINTLN_CTX("x_man - [");
    for(int i = 0; i < sizeof(x_man); i++)
        DBGPRINT("%02X ", x_man[i]);
    DBGPRINTLN("]");

    for(uint32_t i = 0; i < 20; i++)
        x_man_s[i] = x_man[i] << 1 | ((i == (20 - 1) ? x_man[0] : x_man[i + 1]) >> 6);

    DBGPRINTLN_CTX("x_man_s - [");
    for(int i = 0; i < sizeof(x_man_s); i++)
        DBGPRINT("%02X ", x_man_s[i]);
    DBGPRINTLN("]");

    manchester_decode(x_man, 20, x_dec);

    DBGPRINTLN_CTX("x_dec - [");
    for(int i = 0; i < sizeof(x_dec); i++)
        DBGPRINT("%02X ", x_dec[i]);
    DBGPRINTLN("]");

    manchester_decode(x_man_s, 20, x_dec);

    for(uint32_t i = 0; i < 10; i++)
        x_dec_s[i] = x_dec[i] >> 1 | (((i == 0 ? 0 : x_dec[i - 1]) & 1) << 7);

    DBGPRINTLN_CTX("x_dec_s - [");
    for(int i = 0; i < sizeof(x_dec_s); i++)
        DBGPRINT("%02X ", x_dec_s[i]);
    DBGPRINTLN("]");
    */
    srand(time(NULL));

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    DBGPRINTLN_CTX("Set SIGINT handler...");
    signal(SIGINT, signal_handler);

    flexframegenprops_s xFrameHeaderProps = {
        .check = LIQUID_CRC_32,
        .fec0 = LIQUID_FEC_SECDED7264,
        .fec1 = LIQUID_FEC_HAMMING84,
        .mod_scheme = LIQUID_MODEM_BPSK,
    };
    flexframegenprops_s xFramePayloadProps = {
        .check = LIQUID_CRC_32,
        .fec0 = LIQUID_FEC_CONV_V29P34,
        .fec1 = LIQUID_FEC_RS_M8,
        .mod_scheme = LIQUID_MODEM_BPSK,
    };

    flexframegen xFrameGen = flexframegen_create(NULL);

    flexframegen_set_header_props(xFrameGen, &xFrameHeaderProps);
    flexframegen_setprops(xFrameGen, &xFramePayloadProps);

    DBGPRINTLN_CTX("Coded payload length: %u bytes", fec_get_enc_msg_length(LIQUID_FEC_CONV_V29P34, fec_get_enc_msg_length(LIQUID_FEC_RS_M8, 219 + 4)));

    uint8_t *pubPayload = (uint8_t *)malloc(223 - 4);
    uint8_t pubHeader[14];

    while(!ubStop)
    {
        for(uint32_t i = 0; i < sizeof(pubHeader); i++)
            pubHeader[i] = rand() % 256;

        for(uint32_t i = 0; i < 223 - 4; i++)
            pubPayload[i] = rand() % 256;

        flexframegen_assemble(xFrameGen, pubHeader, pubPayload, 223 - 4);

        int32_t iFrameComplete = 0;

        while(!iFrameComplete)
        {
            float complex fSample;

            iFrameComplete = flexframegen_write_samples(xFrameGen, &fSample, 1);

            float fOutput[2];
            fOutput[0] = crealf(fSample);
            fOutput[1] = cimagf(fSample);
            fwrite(fOutput, sizeof(float) * 2, 1, stdout);
        }
    }

/*
    bpacketgen xPacketGen = bpacketgen_create(0, 223 - 4, LIQUID_CRC_32, LIQUID_FEC_NONE, LIQUID_FEC_RS_M8);
    modem xModem = modem_create(LIQUID_MODEM_BPSK);
    firinterp_crcf xShapingFilter = firinterp_crcf_create_prototype(LIQUID_FIRFILT_ARKAISER, 2, 7, 0.35, 0);

    uint32_t ulPacketSize = bpacketgen_get_packet_len(xPacketGen);
    uint32_t ulModemBps = modem_get_bps(xModem);
    DBGPRINTLN_CTX("Packet size: %u bytes", ulPacketSize);
    DBGPRINTLN_CTX("Modem bits per symbol: %u", ulModemBps);

    uint32_t ulSymbolBufferSize = ceil((float)ulPacketSize * 8.0 / ulModemBps);

    uint8_t *pubPayload = malloc(223 - 4);
    uint8_t *pubPacket = malloc(ulPacketSize);
    uint8_t *pubPacketManchester = malloc(ulPacketSize * 2);
    uint8_t *pubSymbols = malloc(ulSymbolBufferSize);

    while(!ubStop)
    {
        for(uint32_t i = 0; i < 223 - 4; i++)
            pubPayload[i] = rand() % 256;

        bpacketgen_encode(xPacketGen, pubPayload, pubPacket);

        //manchester_encode(pubPacket, ulPacketSize, pubPacketManchester);

        uint32_t ulValidSymbols = 0;
        liquid_repack_bytes(pubPacket, 8, ulPacketSize, pubSymbols, ulModemBps, ulSymbolBufferSize, &ulValidSymbols);

        for(uint32_t i = 0; i < ulSymbolBufferSize; i++)
        {
            float complex fSample;
            float complex fInterpSamples[2];
            float fOutput[4];

            modem_modulate(xModem, pubSymbols[i], &fSample);
            firinterp_crcf_execute(xShapingFilter, fSample, fInterpSamples);

            fOutput[0] = crealf(fInterpSamples[0]);
            fOutput[1] = cimagf(fInterpSamples[0]);
            fOutput[2] = crealf(fInterpSamples[1]);
            fOutput[3] = cimagf(fInterpSamples[1]);

            fwrite(fOutput, sizeof(float) * 2, 2, stdout);
        }
    }

    free(pubSymbols);
    free(pubPacket);
    free(pubPayload);

    modem_destroy(xModem);
    firinterp_crcf_destroy(xShapingFilter);
    bpacketgen_destroy(xPacketGen);
*/

    free(pubPayload);

    flexframegen_destroy(xFrameGen);

    return 0;
}
