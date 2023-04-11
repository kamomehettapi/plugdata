/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

// Eyedropper will create a snapshot of the top level component,
// to allow the user to pick colours from anywhere in the app
class Eyedropper : public Timer, public MouseListener
{
    class EyedropperDisplayComponnent : public Component
    {
        Colour colour;
    public:
        
        std::function<void()> onClick = [](){};
        
        EyedropperDisplayComponnent()
        {
            setVisible(true);
            setAlwaysOnTop(true);
            setInterceptsMouseClicks(true, true);
            setSize(50, 50);
            setMouseCursor(MouseCursor::CrosshairCursor);
        }
        
        void show()
        {
            addToDesktop(ComponentPeer::windowIsTemporary);
        }
        
        void hide()
        {
            removeFromDesktop();
        }

        void mouseDown(const MouseEvent& e) override
        {
            onClick();
        }

        void setColour(Colour& c)
        {
            colour = c;
            repaint();
        }
        
        void paint(Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat().withTrimmedTop(20).withTrimmedLeft(20).reduced(8);

            Path shadowPath;
            shadowPath.addEllipse(bounds.reduced(2));
            StackShadow::renderDropShadow(g, shadowPath, Colours::black.withAlpha(0.85f), 7, {0, 2}, 0);

            g.setColour (colour);
            g.fillEllipse (bounds);
            
            g.setColour (Colour::greyLevel (0.9f));
            g.drawEllipse (bounds, 2.0f);
        }
    };
    
public:
    
    class EyedropperButton : public TextButton
    {
      void paint(Graphics& g)
      {
          TextButton::paint(g);
          Fonts::drawIcon(g, Icons::Eyedropper, getLocalBounds().reduced(2), findColour(TextButton::textColourOffId));
      }
    };
    
    Eyedropper()
    {
        colourDisplayer.onClick = [this](){
            
            hideEyedropper();
        };
    }
    
    ~Eyedropper()
    {
        if(topLevel)
        {
            topLevel->removeMouseListener(this);
        }
    }
    
    void showEyedropper(Component* topLevelComponent, std::function<void(Colour)> cb)
    {
        callback = cb;
        colourDisplayer.show();
        topLevel = topLevelComponent;
        topLevel->addMouseListener(this, true);
        
        timerCount = 0;
        timerCallback();
        startTimerHz(60);
    }
    
    void hideEyedropper()
    {
        callback(currentColour);
        callback = [](Colour){};
        colourDisplayer.hide();
        stopTimer();
        topLevel->removeMouseListener(this);
        topLevel = nullptr;
    }
    
private:
    
    void setColour(Colour colour)
    {
        colourDisplayer.setColour(colour);
        currentColour = colour;
    }
    
    void timerCallback() override
    {
        timerCount--;
        if(timerCount <= 0) {
            componentImage = topLevel->createComponentSnapshot(topLevel->getLocalBounds(), false, 1.0f);
            timerCount = 20;
        }
        
        auto position = topLevel->getMouseXYRelative();
        
        colourDisplayer.setTopLeftPosition(topLevel->localPointToGlobal(position).translated(-20, -20));
        setColour(componentImage.getPixelAt(position.x, position.y));
    }
    
    std::function<void(Colour)> callback;
    int timerCount = 0;
    Component* topLevel = nullptr;
    
    EyedropperDisplayComponnent colourDisplayer;
    Image componentImage;
    Colour currentColour;
};

class ColourPicker  : public Component
{
    struct ColourComponentSlider  : public Slider
    {
        ColourComponentSlider (const String& name)  : Slider (name)
        {
            setTextBoxStyle(TextBoxLeft, false, 35, 20);
            setRange (0.0, 255.0, 1.0);
        }
        
        String getTextFromValue (double value) override
        {
            return String(static_cast<int>(value));
        }
        
        double getValueFromText (const String& text) override
        {
            return text.getIntValue();
        }
    };
    
public:
    
    static void show(Component* topLevelComponent, bool onlySendCallbackOnClose, Colour currentColour, Rectangle<int> bounds, std::function<void(Colour)> callback)
    {
        if (isShowing)
            return;
        
        isShowing = true;
        
        std::unique_ptr<ColourPicker> colourSelector = std::make_unique<ColourPicker>(topLevelComponent, onlySendCallbackOnClose, callback);
        
        colourSelector->setCurrentColour(currentColour);
        CallOutBox::launchAsynchronously(std::move(colourSelector), bounds, nullptr);
    }
    
    ColourPicker (Component* topLevelComponent, bool noLiveChangeCallback, std::function<void(Colour)> cb)
    : colour (Colours::white)
    , edgeGap (2)
    , callback(cb)
    , onlyCallBackOnClose(noLiveChangeCallback)
    , colourSpace(*this, h, s, v)
    , brightnessSelector(*this, v)
    {
        updateHSV();
        
        addAndMakeVisible (sliders[0]);
        addAndMakeVisible (sliders[1]);
        addAndMakeVisible (sliders[2]);
        
        for (auto* slider : sliders) {
            slider->onValueChange = [this] { changeColour(); };
            slider->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
            slider->setColour(Slider::textBoxBackgroundColourId, findColour(PlugDataColour::popupMenuBackgroundColourId));
            slider->setColour(Slider::textBoxTextColourId, findColour(PlugDataColour::popupMenuTextColourId));
        }
        
        addAndMakeVisible (colourSpace);
        addAndMakeVisible (brightnessSelector);
        
        showRgb.setColour(TextButton::buttonOnColourId, findColour(PlugDataColour::toolbarHoverColourId));
        showHex.setColour(TextButton::buttonOnColourId, findColour(PlugDataColour::toolbarHoverColourId));
        showRgb.setColour(TextButton::textColourOnId, findColour(TextButton::textColourOffId));
        showHex.setColour(TextButton::textColourOnId, findColour(TextButton::textColourOffId));
        
        showRgb.setRadioGroupId(8888);
        showHex.setRadioGroupId(8888);
        
        showRgb.setClickingTogglesState(true);
        showHex.setClickingTogglesState(true);

        
        addAndMakeVisible(showRgb);
        addAndMakeVisible(showHex);
        addAndMakeVisible(showEyedropper);
        
        hexEditor.setColour(Label::outlineWhenEditingColourId, Colours::transparentBlack);
        hexEditor.setJustificationType(Justification::centred);
        hexEditor.setEditable(true);
        hexEditor.onEditorShow = [this](){
            if(auto* editor = hexEditor.getCurrentTextEditor())
            {
                editor->setInputRestrictions(6, "ABCDEFabcdef0123456789");
                editor->setJustification(Justification::centred);
            }
        };
        hexEditor.onTextChange = [this](){ changeColour(); };
        
        addChildComponent(hexEditor);
        
        showRgb.onClick = [this](){
            setMode(false);
        };
        
        showHex.onClick = [this](){
            setMode(true);
        };
        
        showEyedropper.onClick = [this, topLevelComponent](){
            eyedropper.showEyedropper(topLevelComponent, [this](Colour pickedColour){
                setCurrentColour(pickedColour);
            });
        };
        
        showRgb.setToggleState(true, dontSendNotification);
        
        showRgb.setConnectedEdges(Button::ConnectedOnLeft);
        showHex.setConnectedEdges(Button::ConnectedOnRight);
        
        update (dontSendNotification);
        
        setMode(false);
    }
    
    
    ~ColourPicker() override
    {
        if (onlyCallBackOnClose)
            callback(getCurrentColour());
        
        isShowing = false;
    }
    
    void setMode(bool hex) {
        for(auto& slider : sliders)
        {
            slider->setVisible(!hex);
        }
        
        hexEditor.setVisible(hex);
        update(dontSendNotification);
        repaint();
        
        if(hex)
        {
            setSize(200, 256);
        }
        else {
            setSize(200, 300);
        }
    }
    
    
    Colour getCurrentColour() const
    {
        return colour.withAlpha((uint8) 0xff);
    }
    
    void setCurrentColour (Colour c, NotificationType notification = dontSendNotification)
    {
        if (c != colour)
        {
            colour = c.withAlpha ((uint8) 0xff);
            
            updateHSV();
            update (notification);
        }
    }
private:
    
    
    void setBrightness (float newV)
    {
        newV = jlimit (0.0f, 1.0f, newV);
        
        if (v != newV)
        {
            v = newV;
            colour = Colour (h, s, v, colour.getFloatAlpha());
            update (dontSendNotification);
        }
    }
    
    std::pair<float, float> getHS()
    {
        return {h, s};
    }
    
    void setHS (float newH, float newS)
    {
        newH = jlimit (0.0f, 1.0f, newH);
        newS = jlimit (0.0f, 1.0f, newS);
        
        if (h != newH || s != newS)
        {
            h = newH;
            s = newS;
            
            colour = Colour (h, s, v, colour.getFloatAlpha());
            update (sendNotification);
        }
    }
    
    void updateHSV()
    {
        colour.getHSB (h, s, v);
    }
    
    void update (NotificationType notification)
    {
        if (sliders[0] != nullptr)
        {
            sliders[0]->setValue ((int) colour.getRed(),   notification);
            sliders[1]->setValue ((int) colour.getGreen(), notification);
            sliders[2]->setValue ((int) colour.getBlue(),  notification);
        }
        
        hexEditor.setText(colour.toString().substring(2), notification);
        
        colourSpace.updateIfNeeded();
        brightnessSelector.updateIfNeeded();
        
        if (notification != dontSendNotification && !onlyCallBackOnClose)
            callback(getCurrentColour());
    }
    
    void changeColour() {
        if(hexEditor.isVisible())
        {
            setCurrentColour(Colour::fromString(hexEditor.getText()));
        }
        else {
            setCurrentColour (Colour ((uint8) sliders[0]->getValue(),
                                      (uint8) sliders[1]->getValue(),
                                      (uint8) sliders[2]->getValue()));
        }
    }
    
    void paint (Graphics& g) override
    {
        g.fillAll (findColour (PlugDataColour::popupMenuBackgroundColourId));
        
        g.setColour (findColour (PlugDataColour::popupMenuTextColourId));
        g.setFont (14.0f);
        
        for (auto& slider : sliders)
        {
            if (slider->isVisible())
                g.drawText (slider->getName() + ":",
                            0, slider->getY(),
                            slider->getX() - 8, slider->getHeight(),
                            Justification::centredRight, false);
        }
        
        if (hexEditor.isVisible())
        {
            g.drawText ("HEX:",
                        8, hexEditor.getY() + 1,
                        hexEditor.getX() - 8, hexEditor.getHeight(),
                        Justification::centredRight, false);
        }
    }
    
    void resized() override
    {
        auto numSliders = hexEditor.isVisible() ? 1 : 3;
        const int hueWidth = 16;
        
        auto bounds = getLocalBounds();
        
        auto sliderBounds = bounds.removeFromBottom(22 * numSliders + edgeGap);
        
        auto heightLeft = bounds.getHeight() - bounds.getWidth();
        
        auto controlSelectBounds = bounds.removeFromBottom(heightLeft).reduced(10, 6).translated(0, -12);
        auto colourSpaceBounds = bounds.removeFromLeft(bounds.getWidth() - hueWidth);

        colourSpace.setBounds(colourSpaceBounds);
        brightnessSelector.setBounds(bounds.withTrimmedBottom(heightLeft).translated(-4, 8).expanded(0, 2));
        
        
        showEyedropper.setBounds(controlSelectBounds.removeFromRight(24).translated(2, 0));
        controlSelectBounds.removeFromRight(6);
        showHex.setBounds(controlSelectBounds.removeFromLeft(controlSelectBounds.getWidth() / 2));
        showRgb.setBounds(controlSelectBounds.withTrimmedLeft(-1));
        
        sliderBounds.removeFromLeft(30);
        
        auto sliderHeight = sliderBounds.proportionOfHeight(0.33333f);
        for (int i = 0; i < numSliders; ++i)
        {
            if(sliders[i]->isVisible()) {
                sliders[i]->setBounds(sliderBounds.removeFromTop(sliderHeight));
            }
        }
        
        hexEditor.setBounds(sliderBounds.reduced(5, 2).translated(4, -4));
    }
    
    class ColourSpaceView  : public Component
    {
        
    public:
        ColourSpaceView (ColourPicker& cp, float& hue, float& sat, float& val)
        : owner (cp), h (hue), s (sat), v (val), marker(cp)
        {
            addAndMakeVisible (marker);
            setMouseCursor (MouseCursor::CrosshairCursor);
        }
        
        void updateImage()
        {
            const float antiAliasingRadius = 2.0f;
            const int circleRadius = imageSize / 2;
            
            colourWheelHSV = Image(Image::PixelFormat::ARGB, imageSize, imageSize, true);
            
            Graphics g(colourWheelHSV);

            for (int y = 0; y < imageSize; ++y)
            {
                for (int x = 0; x < imageSize; ++x)
                {
                    // calculate the distance of this pixel from the center
                    const float dx = x - imageSize / 2.0f;
                    const float dy = y - imageSize / 2.0f;
                    const float distance = std::sqrt(dx * dx + dy * dy);
                    
                    // only draw within the circle
                    if (distance <= circleRadius)
                    {
                        // calculate the color at this pixel using HSV color space
                        const float hue = std::atan2(dy, dx);
                        const float saturation = distance / circleRadius;
                        const float value = 1.0f;
                        
                        // convert the HSV color to RGB
                        Colour colour = Colour::fromHSV(hue / MathConstants<float>::twoPi, saturation, value, 1.0f);
                        
                        // calculate the alpha for anti-aliasing
                        const float alpha = std::min(1.0f, (circleRadius - distance) / antiAliasingRadius);
                        
                        // apply the alpha to the color
                        colour = colour.withAlpha(alpha);
                        
                        // set the pixel to the calculated color
                        colourWheelHSV.setPixelAt(x, y, colour);
                    }
                }
            }
        }
        
        void paint(Graphics& g) override
        {
            // draw the image
            g.drawImageAt(colourWheelHSV, margin, margin);
            
            g.setColour(findColour(PlugDataColour::outlineColourId));
            g.drawEllipse(imageBounds.toFloat().reduced(0.5f), 1.0f);
        }
        
        void mouseDown (const MouseEvent& e) override
        {
            mouseDrag (e);
        }
        
        void mouseDrag (const MouseEvent& e) override
        {
            auto area = imageBounds;
            auto markerSize = 20;
            auto centerX = area.getX() + area.getWidth() * 0.5f;
            auto centerY = area.getY() + area.getHeight() * 0.5f;
            
            // Calculate the distance from the center of the wheel to the specified position
            float dx = (e.x) - centerX;
            float dy = (e.y) - centerY;
            float dist = std::sqrt(dx * dx + dy * dy);
            
            float hue, saturation;
            
            // Calculate the maximum distance from the center of the wheel to the edge of the color wheel
            float maxDist = jmin(area.getWidth(), area.getHeight()) * 0.5f - markerSize * 0.5f;
            
            // Calculate the hue as an angle in radians
            float hueAngle = std::atan2(dy, dx);
            
            // Normalize the hue angle to the range [0, 1]
            hue = hueAngle / MathConstants<float>::twoPi;
            if(hue < 0.0f) hue += 1.0f;
            
            // Calculate the saturation as a percentage of the maximum distance from the center
            saturation = dist / maxDist;
            
            owner.setHS(hue, saturation);
            
            owner.brightnessSelector.repaint();
        }
        
        void updateIfNeeded()
        {
            if (lastHue != h)
            {
                lastHue = h;
                repaint();
            }
            
            updateMarker();
            owner.brightnessSelector.repaint();
        }
        
        void resized() override
        {
            imageSize = getWidth() - (margin * 2);
            imageBounds = Rectangle<int>(margin, margin, imageSize, imageSize);
            updateImage();
            updateMarker();
        }
        
    private:
        ColourPicker& owner;
        float& h;
        float& s;
        float& v;
        float lastHue = 0;
        
        static inline constexpr int margin = 10;
        int imageSize;
        Image colourWheelHSV;
        Rectangle<int> imageBounds;
        
        struct ColourSpaceMarker  : public Component
        {
            ColourPicker& owner;
            
            ColourSpaceMarker(ColourPicker& parent) : owner(parent)
            {
                setInterceptsMouseClicks (false, false);
            }
            
            void paint (Graphics& g) override
            {
                auto bounds = getLocalBounds().reduced(4).toFloat();
    
                Path shadowPath;
                shadowPath.addEllipse(bounds);
                StackShadow::renderDropShadow(g, shadowPath, Colours::black.withAlpha(0.75f), 5, {0, 0}, 0);

                                
                auto hs = owner.getHS();
                auto colour = Colour::fromHSV(hs.first, hs.second, 1.0f, 1.0f);
                
                g.setColour (colour);
                g.fillEllipse (bounds);
                
                g.setColour (Colour::greyLevel (0.9f));
                g.drawEllipse (bounds, 2.0f);
            }
        };
        
        ColourSpaceMarker marker;
        
        void updateMarker()
        {
            auto markerSize = 20;
            auto area = imageBounds;
            
            // Calculate the position of the marker based on the current hue and saturation values
            float centerX = area.getX() + area.getWidth() * 0.5f;
            float centerY = area.getY() + area.getHeight() * 0.5f;
            float radius = jmin(area.getWidth(), area.getHeight()) * 0.5f - markerSize * 0.5f;
            float angle = h * MathConstants<float>::twoPi;
            float x = centerX + std::cos(angle) * radius * s;
            float y = centerY + std::sin(angle) * radius * s;
            
            // Set the position of the marker
            marker.setBounds((int) x - markerSize / 2, (int) y - markerSize / 2, markerSize, markerSize);
        }
        
        JUCE_DECLARE_NON_COPYABLE (ColourSpaceView)
    };
    
    class BrightnessSelectorComp  : public Component
    {
    public:
        BrightnessSelectorComp (ColourPicker& cp, float& value)
        : owner (cp), v (value), edge(5), marker(cp)
        {
            addAndMakeVisible (marker);
        }
        
        void paint (Graphics& g) override
        {
            auto hs = owner.getHS();
            auto colour = Colour::fromHSV(hs.first, hs.second, 1.0f, 1.0f);
            
            auto bounds = getLocalBounds().toFloat().reduced(edge);
            auto radius = jmin(Corners::smallCornerRadius, bounds.getWidth() / 2.0f);

            g.setGradientFill(ColourGradient(colour, 0.0f, 0.0f, Colours::black, bounds.getHeight() / 2, bounds.getHeight() / 2, false));
            g.fillRoundedRectangle(bounds, radius);
            
            g.setColour(findColour(PlugDataColour::outlineColourId));
            g.drawRoundedRectangle(bounds, radius, 1.0f);
        }
        
        void resized() override
        {
            auto markerSize = 20;
            auto area = getLocalBounds().reduced (edge);
            
            marker.setBounds (Rectangle<int> (markerSize, markerSize)
                              .withCentre (area.getRelativePoint (0.5f, 1.0f - v)));
        }
        
        void mouseDown (const MouseEvent& e) override
        {
            mouseDrag (e);
        }
        
        void mouseDrag (const MouseEvent& e) override
        {
            owner.setBrightness(1.0f - (float) (e.y - edge) / (float) (getHeight() - edge * 2));
        }
        
        void updateIfNeeded()
        {
            resized();
        }
        
    private:
        ColourPicker& owner;
        float& v;
        const int edge;
        
        struct BrightnessSelectorMarker  : public Component
        {
            ColourPicker& owner;
            
            BrightnessSelectorMarker(ColourPicker& parent) : owner(parent)
            {
                setInterceptsMouseClicks (false, false);
            }
            
            void paint (Graphics& g) override
            {
                auto bounds = getLocalBounds().reduced(4).toFloat();
    
                Path shadowPath;
                shadowPath.addEllipse(bounds.reduced(2));
                StackShadow::renderDropShadow(g, shadowPath, Colours::black.withAlpha(0.75f), 5, {0, 2}, 0);
                                
                g.setColour (owner.getCurrentColour());
                g.fillEllipse (bounds);
                
                g.setColour (Colour::greyLevel (0.9f));
                g.drawEllipse (bounds, 2.0f);
            }
        };
        
        BrightnessSelectorMarker marker;
        
        JUCE_DECLARE_NON_COPYABLE (BrightnessSelectorComp)
    };
    
    Colour colour;
    float h, s, v;
    
    OwnedArray<Slider> sliders = {new ColourComponentSlider("R"), new ColourComponentSlider("G"), new ColourComponentSlider("B")};
    
    ColourSpaceView colourSpace;
    BrightnessSelectorComp brightnessSelector;
    Label hexEditor;
    
    int edgeGap;
    
    TextButton showHex = TextButton("HEX"), showRgb = TextButton("RGB");
    Eyedropper::EyedropperButton showEyedropper;
    
    Eyedropper eyedropper;

    bool onlyCallBackOnClose;
    
    static inline bool isShowing = false;
    
    std::function<void(Colour)> callback = [](Colour) {};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColourPicker)
};

