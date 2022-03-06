#pragma once

#include <math.h>

#include <AL/al.h>
#include <AL/alc.h>

struct Audio {
    ALbyte audio_buffer[4410];
    ALCdevice * device = 0;
    float last_volume = 0;
    float big_lp = 0;
    float amax = 1e-10;
    Audio() {
        device = alcCaptureOpenDevice(NULL, 44100, AL_FORMAT_MONO16, 1024);
        if (device) {
            alcCaptureStart(device);
        } else {
            printf("Error alcCaptureOpenDevice(): %s\n", alGetString(alGetError()));
            device = 0;
        }
    }


    float get_rms() {
        if (device) {
            ALCint nsamples;
            alcGetIntegerv(device, ALC_CAPTURE_SAMPLES, 1, &nsamples);
            if ((size_t)nsamples > sizeof(audio_buffer) / 2) {
                nsamples = sizeof(audio_buffer) / 2;
            }
            if (nsamples > 1024) {
                alcCaptureSamples(device, (ALCvoid *)audio_buffer, nsamples);
                int16_t * buf = (int16_t *)audio_buffer;
                float low = 0;
                //float rms = 0;
                float lp = 0.99;
                float vp = 0;
                for (int i = 0; i < nsamples; i++) {
                    float v = (float)buf[i] / (UINT16_MAX+1);
                    //rms += v * v;
                    vp = vp * lp + v * (1 - lp);
                    low += vp * vp;
                }
                last_volume = sqrt(low) / nsamples;
            }
            big_lp = 0.2 * big_lp + 0.8*last_volume;
            amax = amax * 0.999;
            if (big_lp > amax) amax = big_lp;
        }
        float res = powf(big_lp / amax, 4);
        return res;
    }

    ~Audio() {
        if (device) {
            alcCaptureStop(device);
            alcCaptureCloseDevice(device);
            device = 0;
        }
    }
};
