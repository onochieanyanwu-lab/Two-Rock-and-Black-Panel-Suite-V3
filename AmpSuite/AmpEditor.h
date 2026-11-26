#pragma once
#include "AmpProcessor.h"

// Generic editor: builds a knob / switch / menu for every parameter, so the UI always matches the parameter list.
class AmpEditor : public juce::AudioProcessorEditor
{
public:
    explicit AmpEditor (AmpProcessor& p) : juce::AudioProcessorEditor (p), proc (p)
    {
        for (auto* param : p.getParameters())
        {
            auto* r = dynamic_cast<juce::RangedAudioParameter*> (param);
            if (r == nullptr) continue;
            auto id = r->paramID;
            if (dynamic_cast<juce::AudioParameterBool*> (r))
            {
                auto* b = buttons.add (new juce::ToggleButton (r->getName (30)));
                addAndMakeVisible (b);
                bAtt.add (new juce::AudioProcessorValueTreeState::ButtonAttachment (p.apvts, id, *b));
            }
            else if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (r))
            {
                auto* box = boxes.add (new juce::ComboBox());
                box->addItemList (c->choices, 1); addAndMakeVisible (box);
                cAtt.add (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (p.apvts, id, *box));
                auto* l = labels.add (new juce::Label ({}, r->getName (30)));
                l->setJustificationType (juce::Justification::centred); l->setColour (juce::Label::textColourId, juce::Colours::lightgrey);
                addAndMakeVisible (l); boxLabels.add (l);
            }
            else
            {
                auto* s = sliders.add (new juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow));
                s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
                s->setColour (juce::Slider::rotarySliderFillColourId, kIsTwoRock ? juce::Colour (0xffc9a24b) : juce::Colour (0xffd8d8d8));
                addAndMakeVisible (s);
                sAtt.add (new juce::AudioProcessorValueTreeState::SliderAttachment (p.apvts, id, *s));
                auto* l = labels.add (new juce::Label ({}, r->getName (30)));
                l->setJustificationType (juce::Justification::centred); l->setColour (juce::Label::textColourId, juce::Colours::white);
                addAndMakeVisible (l); sliderLabels.add (l);
            }
        }
        irButton.setButtonText ("Load IR...");
        irButton.onClick = [this]
        {
            chooser = std::make_unique<juce::FileChooser> ("Choose a cabinet impulse response", juce::File(), "*.wav;*.aif;*.aiff;*.flac");
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this] (const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) { proc.loadIR (fc.getResult()); repaint(); } });
        };
        addAndMakeVisible (irButton);
        const int cols = juce::jmax (1, juce::jmin (9, sliders.size()));
        const int rows = (sliders.size() + cols - 1) / cols;
        const int W = juce::jmax (cols * 115 + 40, 800);
        const int itemsW = buttons.size() * 100 + boxes.size() * 160 + 110;
        const int rowsB = (itemsW + (W - 40) - 1) / (W - 40);
        setSize (W, 90 + rows * 130 + rowsB * 62 + 50);
    }

    void paint (juce::Graphics& g) override
    {
        const bool tr = kIsTwoRock;
        g.fillAll (tr ? juce::Colour (0xff1b1712) : juce::Colour (0xff101010));
        g.setColour (tr ? juce::Colour (0xffc9a24b) : juce::Colour (0xffe8e8e8));
        g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
        g.drawText (proc.getName(), 20, 12, getWidth() - 40, 34, juce::Justification::centredLeft);
        g.setFont (juce::FontOptions (13.0f));
        g.setColour (juce::Colours::grey);
        g.drawText ("IR: " + proc.getIRName(), getWidth() - 260, getHeight() - 30, 240, 20, juce::Justification::centredRight);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (20); area.removeFromTop (50);
        const int cols = juce::jmax (1, juce::jmin (9, sliders.size()));
        const int rows = (sliders.size() + cols - 1) / cols;
        const int cw = area.getWidth() / cols;
        for (int i = 0; i < sliders.size(); ++i)
        {
            juce::Rectangle<int> cell (area.getX() + (i % cols) * cw, area.getY() + (i / cols) * 130, cw, 130);
            sliderLabels[i]->setBounds (cell.removeFromTop (18));
            sliders[i]->setBounds (cell.reduced (6));
        }
        int x = area.getX(), y = area.getY() + rows * 130 + 10;
        auto place = [&] (int w) { if (x + w > area.getRight()) { x = area.getX(); y += 62; } };
        for (auto* b : buttons) { place (100); b->setBounds (x, y + 20, 90, 24); x += 100; }
        for (int i = 0; i < boxes.size(); ++i)
        {
            place (160); boxLabels[i]->setBounds (x, y, 150, 18); boxes[i]->setBounds (x, y + 20, 150, 26); x += 160;
        }
        place (110); irButton.setBounds (x, y + 20, 100, 26);
    }

private:
    AmpProcessor& proc;
    juce::OwnedArray<juce::Slider> sliders; juce::OwnedArray<juce::ToggleButton> buttons; juce::OwnedArray<juce::ComboBox> boxes;
    juce::OwnedArray<juce::Label> labels; juce::Array<juce::Label*> sliderLabels, boxLabels;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> sAtt;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> bAtt;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cAtt;
    juce::TextButton irButton;
    std::unique_ptr<juce::FileChooser> chooser;
};
