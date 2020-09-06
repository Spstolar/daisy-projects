#include "daisy_field.h"
#include "daisysp.h" // Uncomment this if you want to use the DSP library.
#include <string>
#include <cmath> 

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
    void On()
    {
        on_ = true;
    }
    void Off()
    {
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

    private:
        bool                on_;
        daisysp::Oscillator osc_;
        float               amp_, midibase_;
};

voice v[NUM_VOICES];
static const uint8_t scale[16] = {0, 2, 4, 5, 7, 9, 11, 12,
    0, 1, 3, 0, 6, 8, 10, 0};
uint8_t base_note = scale[0];

const char* note_names[13] = {
    "C", 
    "C#", 
    "D",
    "D#",
    "E",
    "F",
    "F#",
    "G",
    "G#",
    "A",
    "A#",
    "B",
    "C"
};

const char* keyboard_note_names[16] = {
    "C", 
    "D",
    "E",
    "F",
    "G",
    "A",
    "B",
    "C",
    "C", // begin top row
    "C#",
    "D#",
    "C",
    "F#",
    "G#",
    "A#",
    "C"
};

extern FontDef Font_16x26;

float knob_vals[8];
// float cvvals[4];

void RedrawRectangles(float* rectangle_heights)
{ 
    for(int i = 0; i < NUM_VOICES; i++)
    {
        // 5-10 is a good width, start them at x1=30
        // max height is 64, start at y1=63
        hw.display.DrawRect(30 + i * 8, 63, 35 + i * 8, 63 - floor(63 * rectangle_heights[i]), true);
        // hw.display.Update();
    }
}

// This runs at a fixed rate, to prepare audio samples
void callback(float *in, float *out, size_t size)
{
    hw.ProcessAnalogControls();
    hw.UpdateDigitalControls();

    // grab the knob vals once and conveniently store them
    // in case we use them multiple times
    bool knob_change = false;
    float total_input_levels = 0.0f;

    for(int i=0; i < 8; i++)
    {
        // float current_knob_value = hw.GetKnobValue(i);
        // float knob_change_amount = std::abs(current_knob_value - knob_vals[i]);
        // if (knob_change_amount >= 0.00001)
        // {
            // knob_change = true;
        // }
        // knob_vals[i] = current_knob_value;
        knob_vals[i] = hw.GetKnobValue(i);
        total_input_levels += knob_vals[i];
    }

    // todo: if key is pressed set the base note
    // to be the pressed key (right now just scale[0])
    // float base_note = scale[0];
    for(int i = 0; i < NUM_KEYS; i++)
    {
        if (hw.KeyboardState(i))
        {
            base_note = scale[i];

            // // this was ugly, but the only way I could find to avoid
            // // getting yelled about defining note_names above
            // char* note_name = const_cast<char*>(note_names[i]);
            // // hw.display.Init();
            // hw.display.Fill(false);
            // // hw.display.Update();
            // hw.display.SetCursor(0, 0);
            // hw.display.WriteString(note_name, Font_16x26, true);
            // RedrawRectangles(knob_vals);
            // hw.display.Update();
        }
    }

    for(int i = 0; i < NUM_VOICES; i++)
    {
        v[i].set_note((12.0f * i) + 24.0f + base_note);
        if(knob_vals[i] > 0)
        {
            v[i].On();
        }

        // RedrawRectangles(knob_vals);
        // if (knob_change){
            // 5-10 is a good width, start them at x1=30
            // max height is 64, start at y1=63
            // hw.display.DrawRect(30 + i * 8, 63, 35 + i * 8, 63 - floor(63 * knob_vals[i]), true);
            // hw.display.Update();
        // }
    }

    // hw.display.Update();

    float sig;
    // Audio is interleaved stereo by default
    for (size_t i = 0; i < size; i+=2)
    {
        sig = 0.0f;
        for(int i = 0; i < NUM_VOICES; i++)
        {
            sig += v[i].Process() * knob_vals[i];
       }

        sig *= 2 / total_input_levels;
        out[i] = sig; // left
        out[i+1] = sig; // right
    }
}

int main(void)
{
    hw.Init();
    uint32_t last_led_update, led_period, now;

    led_period = 5;
    last_led_update = now = dsy_system_getnow();

    for(int i = 0; i < NUM_VOICES; i++)
    {
        v[i].Init();
        v[i].set_note((12.0f * i) + 24.0f + scale[0]);
    }

    extern FontDef Font_6x8;
    // std::string greeting = "hello";
    char* greeting = "hello";
    hw.display.WriteString(greeting, Font_6x8, true);
    hw.display.Update();

    hw.StartAudio(callback);
    hw.StartAdc();
    while(1) 
    {
        // Do Stuff InfInitely Here

        now = dsy_system_getnow();

        if (now - last_led_update > led_period)
        {
            // this was ugly, but the only way I could find to avoid
            // getting yelled about defining note_names above
            char* note_name = const_cast<char*>(note_names[base_note]);

            hw.display.Fill(false);
            hw.display.SetCursor(0, 0);
            hw.display.WriteString(note_name, Font_16x26, true);
            RedrawRectangles(knob_vals);
            hw.display.Update();
        }
    }
}
