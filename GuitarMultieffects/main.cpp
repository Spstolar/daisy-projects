#include "daisy_field.h"
#include "daisysp.h" // Uncomment this if you want to use the DSP library.
#include <string>
#include <stdio.h>
#include <vector>

using namespace daisy;
using namespace daisysp;
using std::vector; 

// Declare a local daisy_field for hardware access
DaisyField hw;

// vtime - reverb time, infinite tail when set to 1.0
// vfreq - low pass freq
// vsend - how much of the dry value to send
Parameter vtime, vfreq, vsend;
bool bypass;
ReverbSc verb;

static Decimator decim;

// Set max delay time to 0.5 of sample rate.
// 0.75 was apparently too long
#define MAX_DELAY static_cast<size_t>(48000 * 0.5f)
DelayLine<float, MAX_DELAY> delay;

char* dist_name = "Dist";
char* reverb_name = "Verb";
char* delay_name = "Dlay";
char* effect_names[3] = {dist_name, delay_name, reverb_name};
bool dist_active = true;
bool reverb_active = true;
bool delay_active = true;
bool effect_states[3] = {dist_active, reverb_active, delay_active};

float kvals[8];

// This runs at a fixed rate, to prepare audio samples
void callback(float *in, float *out, size_t size)
{
    float dryl, dryr, wetl, wetr, sendl, sendr;
    float del_out, sig_out, feedback;
    float decimated_out;
    
    hw.ProcessAnalogControls();
    hw.UpdateDigitalControls();

    verb.SetFeedback(vtime.Process());
    verb.SetLpFreq(vfreq.Process());
    vsend.Process();
    if(hw.GetSwitch(DaisyField::SW_1)->RisingEdge())
    {
        bypass = !bypass;
    }
    for (int e = 0; e < 3; e++){
        // check for key
        // if rising flip effect state
        if (hw.KeyboardRisingEdge(e))
        {
            effect_states[e] = !effect_states[e];
        }
    }
    for(int i = 0; i < 8; i++)
    {
        kvals[i] = hw.GetKnobValue(i);
        // if(i < 4)
        // {
        //     cvvals[i] = hw.GetCvValue(i);
        // }
    }

    // if(hw.GetSwitch(DaisyField::SW_2)->RisingEdge())
    // {
    //     bypass = true;
    // }
    // Audio is interleaved stereo by default
    
    // likely a better away to do this
    dist_active = effect_states[0];
    delay_active = effect_states[1];
    reverb_active = effect_states[2];

    for (size_t i = 0; i < size; i+=2)
    {
        dryl = 10.0f * in[i];
        dryr = 10.0f * in[i+1];

        if (delay_active)
        {
            del_out = delay.Read();
        }
        else
        {
            del_out = dryl;
        }
        

        if (dist_active)
        {
            decimated_out = decim.Process(dryl);
            sig_out = 0.25f * del_out + decimated_out;
        }
        else
        {
            sig_out = 0.25f * del_out + dryl;
        }
        
        if (delay_active)
        {
            feedback = (del_out * 0.5f) + sig_out;
            delay.Write(feedback);
        }

        // sendl = dryl * vsend.Value();
        // sendr = dryr * vsend.Value();
        // sendl = decimated_out * vsend.Value();
        // sendr = decimated_out * vsend.Value();
        sendl = sig_out * vsend.Value();
        sendr = sig_out * vsend.Value();

        verb.Process(sendl, sendr, &wetl, &wetr);
        if (bypass)
        {
            out[i] = in[i]; // left
            out[i+1] = in[i+1]; // right
        }
        else
        {
            if (reverb_active)
            {
                // out[i] = dryl + wetl;
                // out[i+1] = dryr + wetr;
                out[i] = sig_out + wetl;
                out[i+1] = sig_out + wetr;
                
            }
            else
            {
                out[i] = sig_out;
                out[i+1] = sig_out;
            }
        }
    }
}

extern FontDef Font_16x26;
extern FontDef Font_6x8;


void CreateEffectBox(int box_num, char* effect_name, bool active)
{
    int size = 30;
    int top_left_x = 5 + box_num * (size + 10);
    int top_left_y = 10;
    int bottom_right_x = top_left_x + size;
    int bottom_right_y = top_left_y + size;
    hw.display.DrawRect(top_left_x, top_left_y, bottom_right_x, bottom_right_y, true);
    if (active)
    {
        for (uint8_t i = 0; i < size; i++)
        {
            hw.display.DrawLine(top_left_x + i,
                top_left_y,
                top_left_x + i,
                bottom_right_y,
                true);
        }
    }
    hw.display.SetCursor(top_left_x + 3, top_left_y + 10);
    hw.display.WriteString(effect_name, Font_6x8, !active);
}



void WriteToScreen()
{
    if (bypass)
    {
        char* bypass_state = "Bypassed";
        hw.display.SetCursor(0, 20);
        hw.display.WriteString(bypass_state, Font_16x26, true);
    }
    else
    {
        for (int i=0; i < 3; i++)
        {
            // char* bypass_state = "Dist";
            CreateEffectBox(i, effect_names[i], effect_states[i]);
        }
        // hw.display.SetCursor(0, 20);
        // hw.display.WriteString(bypass_state, Font_16x26, true);
    }
}

void UpdateLeds(float *knob_vals) 
{
	// knob_vals is exactly 8 members
    size_t knob_leds[] = {
        DaisyField::LED_KNOB_1,
        DaisyField::LED_KNOB_2,
        DaisyField::LED_KNOB_3,
        DaisyField::LED_KNOB_4,
        DaisyField::LED_KNOB_5,
        DaisyField::LED_KNOB_6,
        DaisyField::LED_KNOB_7,
        DaisyField::LED_KNOB_8,
    };

    size_t keyboard_leds[] = {
        DaisyField::LED_KEY_A1,
        DaisyField::LED_KEY_A2,
        DaisyField::LED_KEY_A3,
        DaisyField::LED_KEY_A4,
        DaisyField::LED_KEY_A5,
        DaisyField::LED_KEY_A6,
        DaisyField::LED_KEY_A7,
        DaisyField::LED_KEY_A8,
        DaisyField::LED_KEY_B1,
        DaisyField::LED_KEY_B2,
        DaisyField::LED_KEY_B3,
        DaisyField::LED_KEY_B4,
        DaisyField::LED_KEY_B5,
        DaisyField::LED_KEY_B6,
        DaisyField::LED_KEY_B7,
        DaisyField::LED_KEY_B8,
    };

    for(size_t i = 0; i < 8; i++) {
        hw.led_driver_.SetLed(knob_leds[i], knob_vals[i]);
	}
    for(size_t i = 0; i < 3; i++) {
        if (effect_states[i])
        {
            hw.led_driver_.SetLed(keyboard_leds[i], 1.f);
        }
        else
        {
            hw.led_driver_.SetLed(keyboard_leds[i], 0.f);
        }
	}
    hw.led_driver_.SwapBuffersAndTransmit();
}

int main(void)
{
    float sample_rate;

    hw.Init();
    sample_rate = hw.SampleRate();

    decim.Init();
    decim.SetDownsampleFactor(0.4f);
    decim.SetBitsToCrush(8);

    // delay
    delay.Init();
    delay.SetDelay(sample_rate * 0.75f);

    // reverb
    vtime.Init(*hw.GetKnob(0), 0.6f, 0.999f, Parameter::LOGARITHMIC);
    vfreq.Init(*hw.GetKnob(1), 500.0f, 20000.0f, Parameter::LOGARITHMIC);
    vsend.Init(*hw.GetKnob(2), 0.0f, 1.0f, Parameter::LINEAR);
    verb.Init(sample_rate);

    hw.StartAdc();
    hw.StartAudio(callback);
    while(1) 
    {
        // Do Stuff InfInitely Here
        dsy_system_delay(10);
        UpdateLeds(kvals);

        // Write Active/Bypassed
        hw.display.Fill(false);
        WriteToScreen();
        hw.display.Update();
    }
}
