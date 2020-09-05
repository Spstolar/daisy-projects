#include "daisy_field.h"
#include "daisysp.h" // Uncomment this if you want to use the DSP library.

#define NUM_VOICES 8
#define NUM_KEYS 16

using namespace daisy;
using namespace daisysp;

// Declare a local daisy_field for hardware access
DaisyField hw;

struct voice
{
    void Init()
    {
        osc_.Init(DSY_AUDIO_SAMPLE_RATE);
        amp_ = 0.0f;
        osc_.SetAmp(0.7f);
        osc_.SetWaveform(daisysp::Oscillator::WAVE_SIN);
        on_ = false;
    }
    float Process()
    {
        float sig;
        amp_ += 0.0025f * ((on_ ? 1.0f : 0.0f) - amp_);
        sig = osc_.Process() * amp_;
        return sig;
    }
    void set_note(float nn) { osc_.SetFreq(daisysp::mtof(nn)); }

    daisysp::Oscillator osc_;
    float               amp_, midibase_;
    bool                on_;
};

voice v[NUM_VOICES];
float scale[16] = {0.f, 2.f, 4.f, 5.f, 7.f, 9.f, 11.f, 12.f,
    0.f, 1.f, 3.f, 0.f, 6.f, 8.f, 10.f, 0.f};
float base_note = scale[0];

float knob_vals[8];
// float cvvals[4];

// This runs at a fixed rate, to prepare audio samples
void callback(float *in, float *out, size_t size)
{
    hw.ProcessAnalogControls();
    hw.UpdateDigitalControls();

    // grab the knob vals once and conveniently store them
    // in case we use them multiple times
    for(int i=0; i < 8; i++)
    {
        knob_vals[i] = hw.GetKnobValue(i);
    }

    // todo: if key is pressed set the base note
    // to be the pressed key (right now just scale[0])
    // float base_note = scale[0];
    for(int i = 0; i < NUM_KEYS; i++)
    {
        if (hw.KeyboardState(i))
        {
            base_note = scale[i];
        }
    }

    for(int i = 0; i < NUM_VOICES; i++)
    {
        v[i].set_note((12.0f * i) + 24.0f + base_note);
        if(knob_vals[i] > 0)
        {
            v[i].on_ = true;
        }
    }

    float sig, send;
    // Audio is interleaved stereo by default
    for (size_t i = 0; i < size; i+=2)
    {
        sig = 0.0f;
        for(int i = 0; i < NUM_VOICES; i++)
        {
            sig += v[i].Process() * knob_vals[i];
        }

        out[i] = sig; // left
        out[i+1] = sig; // right
    }
}

int main(void)
{
    hw.Init();
    for(int i = 0; i < NUM_VOICES; i++)
    {
        v[i].Init();
        v[i].set_note((12.0f * i) + 24.0f + scale[0]);
    }

    hw.StartAudio(callback);
    hw.StartAdc();
    while(1) 
    {
        // Do Stuff InfInitely Here
    }
}
