#include "AmpProcessor.h"
#include "AmpEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

// transparent below about -2 dBFS, smooth ceiling just under 0 dBFS so hot settings can never clip the DAW
static inline float softLimit (float x)
{
    const float a = std::abs (x), t = 0.8f;
    if (a <= t) return x;
    return std::copysign (t + (1.0f - t) * std::tanh ((a - t) / (1.0f - t)), x);
}

APVTS::ParameterLayout AmpProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto knob = [&] (const char* id, const char* name, float def)
    { p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (0.f, 10.f, 0.01f), def)); };

    auto sw = [&] (const char* id, const char* name, bool def)
    { p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def)); };
    if (kIsTwoRock)
    {
        // Two-Rock Classic Reverb Signature front panel
        knob ("fet", "FET Gain", 5.0f); knob ("treble", "Treble", 5.0f); knob ("middle", "Middle", 5.0f); knob ("bass", "Bass", 5.0f);
        knob ("gain", "Gain", 5.0f); knob ("master", "Master", 5.0f); knob ("send", "Reverb Send", 4.0f); knob ("return", "Reverb Return", 3.0f);
        knob ("contour", "Contour", 5.0f); knob ("sag", "Sag", 5.0f);
        sw ("fetsw", "FET", true); sw ("bright", "Bright", false); sw ("mid", "Mid", false); sw ("deep", "Deep", false);
        sw ("bypass", "Tone Bypass", false); sw ("standby", "Standby", false);
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "eq", 1 }, "EQ", juce::StringArray { "EQ1", "EQ2" }, 1));
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "structure", 1 }, "Gain Structure", juce::StringArray { "Down", "Middle", "Up" }, 2));
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "power", 1 }, "Power", juce::StringArray { "100 W", "50 W" }, 0));
    }
    else
    {
        knob ("volume", "Volume", 4.0f); knob ("treble", "Treble", 6.0f); knob ("middle", "Middle", 5.0f); knob ("bass", "Bass", 4.0f);
        knob ("reverb", "Reverb", 2.5f); knob ("vibspeed", "Vib Speed", 5.0f); knob ("vibint", "Vib Intensity", 0.0f); knob ("sag", "Sag", 5.0f);
        sw ("bright", "Bright", false);
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "model", 1 }, "Amp", juce::StringArray { "Deluxe 22W (6V6)", "Twin 85W (6L6)" }, 1));
    }
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "input", 1 }, "Input Trim", juce::NormalisableRange<float> (-18.f, 18.f, 0.1f), 0.f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "output", 1 }, "Output", juce::NormalisableRange<float> (-24.f, 18.f, 0.1f), 0.f));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "cab", 1 }, "Cabinet", juce::StringArray { "Off", "Modelled", "Impulse Response" }, 1));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "quality", 1 }, "Quality", juce::StringArray { "2x (live)", "4x (render)" }, 0));
    return { p.begin(), p.end() };
}

AmpProcessor::AmpProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

void AmpProcessor::rebuildEngine()
{
    const int quality = (int) pv ("quality");
    const int model   = (int) pv (kIsTwoRock ? "power" : "model");
    os = quality == 1 ? os4.get() : os2.get();
    const int factor = quality == 1 ? 4 : 2;
    engine->halfPower = kIsTwoRock && model == 1;
    engine->configure (kIsTwoRock ? amp::Voicing::TwoRockCRS : (model == 0 ? amp::Voicing::Blackface22 : amp::Voicing::BlackfaceTwin));
    engine->c = cur;
    engine->prepare (hostRate * factor);
    cab.prepare (hostRate, kIsTwoRock ? 1 : 0);
    setLatencySamples ((int) std::lround (os->getLatencyInSamples()));
    appliedQuality = quality; appliedModel = model;
}

void AmpProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    hostRate = sampleRate; maxBlock = juce::jmax (samplesPerBlock, 32);
    os2 = std::make_unique<juce::dsp::Oversampling<float>> (1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    os4 = std::make_unique<juce::dsp::Oversampling<float>> (1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    os2->initProcessing ((size_t) maxBlock); os4->initProcessing ((size_t) maxBlock);
    mono.setSize (1, maxBlock);
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 1 };
    convolution.prepare (spec);
    cur = amp::Controls();
    controlsInit = false;
    rebuildEngine();
}

void AmpProcessor::loadIR (const juce::File& f)
{
    if (! f.existsAsFile()) return;
    irPath = f.getFullPathName();
    convolution.loadImpulseResponse (f, juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::yes, 0, juce::dsp::Convolution::Normalise::yes);
}

void AmpProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = buffer.getNumChannels();
    const int total = buffer.getNumSamples();

    if ((int) pv ("quality") != appliedQuality || (int) pv (kIsTwoRock ? "power" : "model") != appliedModel)
        rebuildEngine();

    // target controls (0..10 knobs -> 0..1)
    amp::Controls tgt;
    tgt.treble = pv ("treble") / 10.0; tgt.middle = pv ("middle") / 10.0; tgt.bass = pv ("bass") / 10.0; tgt.sag = pv ("sag") / 10.0;
    tgt.bright = pv ("bright") > 0.5f;
    if (kIsTwoRock)
    {
        tgt.fetGain = pv ("fet") / 10.0; tgt.gain = pv ("gain") / 10.0; tgt.master = pv ("master") / 10.0;
        tgt.send = pv ("send") / 10.0; tgt.ret = pv ("return") / 10.0; tgt.contour = pv ("contour") / 10.0;
        tgt.fetOn = pv ("fetsw") > 0.5f; tgt.midSw = pv ("mid") > 0.5f; tgt.deep = pv ("deep") > 0.5f; tgt.bypass = pv ("bypass") > 0.5f;
        tgt.eq2 = (int) pv ("eq") == 1; tgt.gainStructure = (int) pv ("structure");
    }
    else
    {
        tgt.volume = pv ("volume") / 10.0; tgt.reverb = pv ("reverb") / 10.0;
        tgt.vibSpeed = pv ("vibspeed") / 10.0; tgt.vibDepth = pv ("vibint") / 10.0; tgt.master = 1.0;
    }
    if (! controlsInit) { cur = tgt; engine->setControls (cur); controlsInit = true; }

    const double inGain  = juce::Decibels::decibelsToGain ((double) pv ("input"));
    const float  outGain = (kIsTwoRock && pv ("standby") > 0.5f) ? 0.0f : juce::Decibels::decibelsToGain (pv ("output"));
    const float  g0 = lastOutGain; lastOutGain = outGain;
    const int cabMode = (int) pv ("cab");
    const int factor = (int) os->getOversamplingFactor();

    int pos = 0;
    while (pos < total)
    {
        const int n = juce::jmin (total - pos, maxBlock);
        // mono sum in
        float* m = mono.getWritePointer (0);
        for (int i = 0; i < n; ++i)
        {
            float s = 0.f; for (int c = 0; c < numCh; ++c) s += buffer.getReadPointer (c)[pos + i];
            m[i] = s / (float) juce::jmax (1, numCh);
        }
        juce::dsp::AudioBlock<float> blk (mono); auto sub = blk.getSubBlock (0, (size_t) n);
        auto up = os->processSamplesUp (sub);
        float* u = up.getChannelPointer (0);
        const int nu = (int) up.getNumSamples();
        const int chunk = 32 * factor;
        for (int s0 = 0; s0 < nu; s0 += chunk)
        {
            // glide knobs toward target (per chunk)
            auto g = [] (double& a, double b) { a += 0.18 * (b - a); };
            g (cur.volume, tgt.volume); g (cur.treble, tgt.treble); g (cur.middle, tgt.middle); g (cur.bass, tgt.bass);
            g (cur.reverb, tgt.reverb); g (cur.master, tgt.master); g (cur.presence, tgt.presence);
            g (cur.vibSpeed, tgt.vibSpeed); g (cur.vibDepth, tgt.vibDepth); g (cur.sag, tgt.sag);
            g (cur.fetGain, tgt.fetGain); g (cur.gain, tgt.gain); g (cur.send, tgt.send); g (cur.ret, tgt.ret); g (cur.contour, tgt.contour);
            cur.bright = tgt.bright; cur.fetOn = tgt.fetOn; cur.midSw = tgt.midSw; cur.deep = tgt.deep; cur.bypass = tgt.bypass; cur.eq2 = tgt.eq2; cur.gainStructure = tgt.gainStructure;
            engine->setControls (cur);
            const int e = juce::jmin (nu, s0 + chunk);
            for (int i = s0; i < e; ++i) u[i] = (float) engine->process ((double) u[i] * inGain);
        }
        os->processSamplesDown (sub);

        // cabinet
        if (cabMode == 1) for (int i = 0; i < n; ++i) m[i] = (float) cab.process ((double) m[i]);
        else if (cabMode == 2) { juce::dsp::ProcessContextReplacing<float> ctx (sub); convolution.process (ctx); }

        for (int c = 0; c < numCh; ++c)
        {
            float* d = buffer.getWritePointer (c) + pos;
            for (int i = 0; i < n; ++i) d[i] = softLimit (m[i] * (g0 + (outGain - g0) * (float) (pos + i) / (float) total));
        }
        pos += n;
    }
}

juce::AudioProcessorEditor* AmpProcessor::createEditor() { return new AmpEditor (*this); }

void AmpProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("irPath", irPath, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void AmpProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            apvts.replaceState (vt);
            const auto p = vt.getProperty ("irPath").toString();
            if (p.isNotEmpty()) loadIR (juce::File (p));
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AmpProcessor(); }
