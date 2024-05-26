#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <utility>
#include "Constants.h"
#include "LookAndFeel.h"
#include "Components/DraggableNumber.h"
#include "PluginEditor.h"

class OversampleSettings : public Component {
public:
    std::function<void(int)> onChange = [](int) {};
    
    explicit OversampleSettings(int currentSelection)
    {
        one.setConnectedEdges(Button::ConnectedOnRight);
        two.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
        four.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
        eight.setConnectedEdges(Button::ConnectedOnLeft);

        auto buttons = Array<TextButton*> { &one, &two, &four, &eight };

        int i = 0;
        for (auto* button : buttons) {
            button->setRadioGroupId(hash("oversampling_selector"));
            button->setClickingTogglesState(true);
            button->onClick = [this, i]() {
                onChange(i);
            };

            button->setColour(TextButton::textColourOffId, findColour(PlugDataColour::popupMenuTextColourId));
            button->setColour(TextButton::textColourOnId, findColour(PlugDataColour::popupMenuActiveTextColourId));
            button->setColour(TextButton::buttonColourId, findColour(PlugDataColour::popupMenuBackgroundColourId).contrasting(0.04f));
            button->setColour(TextButton::buttonOnColourId, findColour(PlugDataColour::popupMenuBackgroundColourId).contrasting(0.075f));
            button->setColour(ComboBox::outlineColourId, Colours::transparentBlack);

            addAndMakeVisible(button);
            i++;
        }

        buttons[currentSelection]->setToggleState(true, dontSendNotification);

        setSize(180, 50);
    }

private:
    void resized() override
    {
        auto b = getLocalBounds().reduced(4, 4);
        auto buttonWidth = b.getWidth() / 4;

        one.setBounds(b.removeFromLeft(buttonWidth));
        two.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
        four.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
        eight.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
    }

    TextButton one = TextButton("1x");
    TextButton two = TextButton("2x");
    TextButton four = TextButton("4x");
    TextButton eight = TextButton("8x");
};

class AudioOutputSettings : public Component {
    
    class LimiterEnableButton : public Component
        , public SettableTooltipClient {
            
            String icon;
            String text;
            bool state;
            bool buttonHover = false;
            
    public:
        std::function<void(bool)> onClick;

            LimiterEnableButton(AudioOutputSettings* parent, String const& iconText, String text, bool initState)
            : icon(iconText)
            , text(text)
            , state(initState)
        {
        }

        void paint(Graphics& g) override
        {
            auto iconColour = state ? findColour(PlugDataColour::toolbarActiveColourId) : findColour(PlugDataColour::toolbarTextColourId);
            auto textColour = findColour(PlugDataColour::toolbarTextColourId);

            if (isMouseOver()) {
                iconColour = iconColour.contrasting(0.3f);
                textColour = textColour.contrasting(0.3f);
            }

            Fonts::drawIcon(g, icon, Rectangle<int>(0, 0, 30, getHeight()), iconColour, 14);
            Fonts::drawText(g, text, Rectangle<int>(30, 0, getWidth(), getHeight()), textColour, 14);
        }


        void mouseEnter(MouseEvent const& e) override
        {
            buttonHover = true;
            repaint();
        }

        void mouseExit(MouseEvent const& e) override
        {
            buttonHover = false;
            repaint();
        }

        void mouseDown(MouseEvent const& e) override
        {
            state = !state;
            onClick(state);
            repaint();
        }
    };
    
public:
    AudioOutputSettings(PluginProcessor* pd) : oversampleSettings(SettingsFile::getInstance()->getValueTree().getProperty("Oversampling"))
    {
        enableLimiterButton = std::make_unique<LimiterEnableButton>(this, Icons::Protection, "Enable limiter", SettingsFile::getInstance()->getProperty<int>("protected"));
        enableLimiterButton->onClick = [pd](bool state){
            pd->setProtectedMode(state);
            SettingsFile::getInstance()->setProperty("protected", state);
        };
        addAndMakeVisible(*enableLimiterButton);
        addAndMakeVisible(oversampleSettings);
        oversampleSettings.onChange = [pd](int value){
            pd->setOversampling(value);
        };
        
        setSize(160, 125);
    }
    
    ~AudioOutputSettings()
    {
        isShowing = false;
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds().reduced(4.0f).withTrimmedTop(24);
    
        enableLimiterButton->setBounds(bounds.removeFromTop(32));
        
        bounds.removeFromTop(32);
        oversampleSettings.setBounds(bounds.removeFromTop(28));
    }
    
    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::popupMenuTextColourId));
        g.setFont(Fonts::getBoldFont().withHeight(15));
        g.drawText("Limiter", 0, 0, getWidth(), 24, Justification::centred);
        
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(4, 24, getWidth() - 8, 24);
        
        
        g.setColour(findColour(PlugDataColour::popupMenuTextColourId));
        g.setFont(Fonts::getBoldFont().withHeight(15));
        g.drawText("Oversampling", 0, 56, getWidth(), 24, Justification::centred);
        
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(4, 84, getWidth() - 8, 84);
    }
    
    static void show(PluginEditor* editor, Rectangle<int> bounds)
    {
        if (isShowing)
            return;

        isShowing = true;

        auto audioOutputSettings = std::make_unique<AudioOutputSettings>(editor->pd);
        editor->showCalloutBox(std::move(audioOutputSettings), bounds);
    }
    
private:
    static inline bool isShowing = false;
    
    std::unique_ptr<LimiterEnableButton> enableLimiterButton;
    OversampleSettings oversampleSettings;

};
