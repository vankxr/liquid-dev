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
float fPhaseDelta = 0;

int32_t packet_handler(uint8_t *pubHeader, int32_t lHeaderValid, uint8_t *pubPayload, uint32_t ulPayloadSize, int32_t lPayloadValid, framesyncstats_s xStats, void *pUserdata)
{
    printf("******** callback invoked\n");

    if(!lHeaderValid)
        return 0;

    framesyncstats_print(&xStats);
    printf("    header crc          :   %s\n", lHeaderValid ?  "pass" : "FAIL");
    printf("    payload length      :   %u\n", ulPayloadSize);
    printf("    payload crc         :   %s\n", lPayloadValid ?  "pass" : "FAIL");

    return 0;
}
void signal_handler(int iSignal)
{
    DBGPRINTLN_CTX("Got signal %d, stop sampling...", iSignal);

    ubStop = 1;
}

int main(int argc, char *argv[])
{
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

    flexframesync xFrameSync = flexframesync_create(packet_handler, NULL);

    flexframesync_set_header_props(xFrameSync, &xFrameHeaderProps);

    while(!ubStop)
    {
        float fInput[2];

        uint32_t ulRead = fread(fInput, sizeof(float) * 2, 1, stdin);

        if(!ulRead)
            continue;

        float complex fInputSample = fInput[0] + _Complex_I * fInput[1];

        flexframesync_execute(xFrameSync, &fInputSample, 1);
    }

    flexframesync_destroy(xFrameSync);

/*
    nco_crcf xCoarseLO = nco_crcf_create(LIQUID_VCO);
    agc_crcf xAGC = agc_crcf_create();
    symsync_crcf xSymbolSynchronizer = symsync_crcf_create_rnyquist(LIQUID_FIRFILT_ARKAISER, 2, 7, 0.5, 16);
    eqlms_cccf xEqualizer = eqlms_cccf_create_lowpass(9, 0.45);
    nco_crcf xFineLO = nco_crcf_create(LIQUID_VCO);
    modem xModem = modem_create(LIQUID_MODEM_BPSK);
    bpacketsync xPacketSynchronizer = bpacketsync_create(0, packet_handler, NULL);

    symsync_crcf_set_output_rate(xSymbolSynchronizer, 2);

    float fBandwidth = 0.9;
    agc_crcf_set_bandwidth(xAGC, 0.02 * fBandwidth);
    symsync_crcf_set_lf_bw(xSymbolSynchronizer, 0.001 * fBandwidth);
    eqlms_cccf_set_bw(xEqualizer, 0.02 * fBandwidth);
    nco_crcf_pll_set_bandwidth(xFineLO, 0.001 * fBandwidth);

    fprintf(stdout, "setenv(\"GNUTERM\",\"X11 noraise\");x=zeros(1,64);y=zeros(1,64);");

    while(!ubStop)
    {
        float fInput[2];

        uint32_t ulRead = fread(fInput, sizeof(float) * 2, 1, stdin);

        if(!ulRead)
            continue;

        float complex fInputSample = fInput[0] + _Complex_I * fInput[1];
        float complex fOutputSample;

        agc_crcf_execute(xAGC, fInputSample, &fOutputSample);

        float complex fSymSyncOutput[8];
        uint32_t ulSymSyncOutputSize = 0;

        symsync_crcf_execute(xSymbolSynchronizer, &fOutputSample, 1, fSymSyncOutput, &ulSymSyncOutputSize);

        for(uint8_t i = 0; i < ulSymSyncOutputSize; i++)
        {
            nco_crcf_step(xFineLO);
            nco_crcf_mix_down(xFineLO, fSymSyncOutput[i], &fOutputSample);

            //float fOutput[2];
            //fOutput[0] = crealf(fOutputSample);
            //fOutput[1] = cimagf(fOutputSample);
            //fwrite(fOutput, sizeof(float) * 2, 1, stdout);

            eqlms_cccf_push(xEqualizer, fOutputSample);

            static uint32_t ulSymSyncIndex = 0;

            ulSymSyncIndex++;
            if(!(ulSymSyncIndex % 2))
                continue;

            eqlms_cccf_execute(xEqualizer, &fOutputSample);

            uint32_t ulOutputSymbol;
            modem_demodulate(xModem, fOutputSample, &ulOutputSymbol);

            nco_crcf_pll_step(xFineLO, modem_get_demodulator_phase_error(xModem));
            fPhaseDelta = modem_get_demodulator_phase_error(xModem);

            static uint16_t cnt = 0;
            fprintf(stdout, "x(%u) = %.5f;\n", (uint32_t)(cnt) + 1, crealf(fOutputSample));
            fprintf(stdout, "y(%u) = %.5f;\n", (uint32_t)(cnt++) + 1, cimagf(fOutputSample));
            //fprintf(stdout, "x(%u) = %u;\n", (uint32_t)(cnt) + 1, (uint32_t)(cnt) + 1);
            //fprintf(stdout, "y(%u) = %.5f;\n", (uint32_t)(cnt++) + 1, fPhaseDelta);
            if(!(cnt = cnt == 64 ? 0 : cnt))
                fprintf(stdout, "scatter(x,y);");

            if(ulSymSyncIndex > 400)
            {
                float complex fSample;

                modem_get_demodulator_sample(xModem, &fSample);
                eqlms_cccf_step(xEqualizer, fSample, fOutputSample);
            }

            bpacketsync_execute_sym(xPacketSynchronizer, ulOutputSymbol, modem_get_bps(xModem));
            continue;

            static uint8_t ubPrevBit = 0;
            static uint8_t ubBitCount = 0;

            for(uint8_t j = modem_get_bps(xModem); j > 0; j--)
            {
                uint8_t ubCurrentBit = (ulOutputSymbol >> (j - 1)) & 1;

                // Manchester decoding
                ubBitCount++;
                if(ubBitCount < 2)
                {
                    ubPrevBit = ubCurrentBit;

                    continue;
                }

                ubBitCount = 0;

                if(ubCurrentBit + ubPrevBit != 1)
                {
                    ubPrevBit = ubCurrentBit;
                    ubBitCount = 1;

                    continue;
                }

                //bpacketsync_execute_bit(xPacketSynchronizer, ubPrevBit && !ubCurrentBit);
            }
        }
    }

    bpacketsync_destroy(xPacketSynchronizer);
    modem_destroy(xModem);
    nco_crcf_destroy(xFineLO);
    eqlms_cccf_destroy(xEqualizer);
    symsync_crcf_destroy(xSymbolSynchronizer);
    agc_crcf_destroy(xAGC);
    nco_crcf_destroy(xCoarseLO);
    */

    return 0;
}
