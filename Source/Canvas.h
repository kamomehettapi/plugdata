/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <juce_opengl/juce_opengl.h>
using namespace juce::gl;

#include <nanovg.h>
#include <nanovg_gl_utils.h>

#include "ObjectGrid.h"          // move to impl
#include "Utility/RateReducer.h" // move to impl
#include "Utility/ModifierKeyListener.h"
#include "Components/CheckedTooltip.h"
#include "Pd/MessageListener.h"
#include "Pd/Patch.h"
#include "Constants.h"
#include "Objects/ObjectParameters.h"
#include "Utility/NVGComponent.h"
#include "Utility/GlobalMouseListener.h"

namespace pd {
class Patch;
}

class SuggestionComponent;
class GraphArea;
class Iolet;
class Object;
class Connection;
class PluginEditor;
class PluginProcessor;
class ConnectionPathUpdater;
class ConnectionBeingCreated;
class TabComponent;

struct ObjectDragState {
    bool wasDragDuplicated = false;
    bool didStartDragging = false;
    bool wasSelectedOnMouseDown = false;
    bool wasResized = false;
    Point<int> canvasDragStartPosition = { 0, 0 };
    Component::SafePointer<Object> componentBeingDragged;
    Component::SafePointer<Object> objectSnappingInbetween;
    Component::SafePointer<Connection> connectionToSnapInbetween;
};

class Canvas : public Component
    , public Value::Listener
    , public SettingsFileListener
    , public LassoSource<WeakReference<Component>>
    , public ModifierKeyListener
    , public pd::MessageListener
    , public AsyncUpdater 
    , public NVGComponent
{
public:
    Canvas(PluginEditor* parent, pd::Patch::Ptr patch, Component* parentGraph = nullptr);

    ~Canvas() override;

    PluginEditor* editor;
    PluginProcessor* pd;

    void mouseDown(MouseEvent const& e) override;
    void mouseDrag(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    
    Point<int> getLastMousePosition();
    
    void commandKeyChanged(bool isHeld) override;
    void spaceKeyChanged(bool isHeld) override;
    void middleMouseChanged(bool isHeld) override;
    void altKeyChanged(bool isHeld) override;

    void propertyChanged(String const& name, var const& value) override;

    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;
    
    void render(NVGcontext* nvg) override;
    void updateFramebuffers(NVGcontext* nvg);
    void finaliseRender(NVGcontext* nvg);
    bool performFramebufferUpdate(NVGcontext* nvg, Rectangle<int> invalidRegion, int maxUpdateTimeMs);
    void performRender(NVGcontext* nvg, Rectangle<int> invalidRegion);

    void resized() override;

    void renderAllObjects(NVGcontext* nvg, Rectangle<int> area);
    void renderAllConnections(NVGcontext* nvg, Rectangle<int> area);

    int getOverlays() const;
    void updateOverlays();

    void synchroniseSplitCanvas();
    void synchronise();
    void performSynchronise();
    void handleAsyncUpdate() override;

    void moveToWindow(PluginEditor* newWindow);

    void updateDrawables();

    bool keyPressed(KeyPress const& key) override;
    void valueChanged(Value& v) override;

    TabComponent* getTabbar();
    int getTabIndex();
    void tabChanged();

    void hideAllActiveEditors();

    void copySelection();
    void removeSelection();
    void removeSelectedConnections();
    void dragAndDropPaste(String const& patchString, Point<int> mousePos, int patchWidth, int patchHeight, String name = String());
    void pasteSelection();
    void duplicateSelection();

    void encapsulateSelection();

    bool canConnectSelectedObjects();
    bool connectSelectedObjects();

    void cancelConnectionCreation();

    void alignObjects(Align alignment);

    void undo();
    void redo();

    void jumpToOrigin();
    void zoomToFitAll();
    
    float getRenderScale() const;
    
    bool autoscroll(MouseEvent const& e);

    // Multi-dragger functions
    void deselectAll();
    void setSelected(Component* component, bool shouldNowBeSelected, bool updateCommandStatus = true);

    SelectedItemSet<WeakReference<Component>>& getLassoSelection() override;

    bool checkPanDragMode();
    bool setPanDragMode(bool shouldPan);

    void findLassoItemsInArea(Array<WeakReference<Component>>& itemsFound, Rectangle<int> const& area) override;

    void updateSidebarSelection();

    void orderConnections();

    void showSuggestions(Object* object, TextEditor* textEditor);
    void hideSuggestions();

    static bool panningModifierDown();

    ObjectParameters& getInspectorParameters();

    void receiveMessage(t_symbol* symbol, pd::Atom const atoms[8], int numAtoms) override;

    template<typename T>
    Array<T*> getSelectionOfType()
    {
        Array<T*> result;

        for (auto const& obj : selectedComponents) {
            if (auto* objOfType = dynamic_cast<T*>(obj.get())) {
                result.add(objOfType);
            }
        }

        return result;
    }

    std::unique_ptr<Viewport> viewport = nullptr;

    bool connectingWithDrag = false;
    bool connectionCancelled = false;
    SafePointer<Iolet> nearestIolet;

    std::unique_ptr<SuggestionComponent> suggestor;

    pd::Patch::Ptr refCountedPatch;
    pd::Patch& patch;

    // Needs to be allocated before object and connection so they can deselect themselves in the destructor
    SelectedItemSet<WeakReference<Component>> selectedComponents;
    OwnedArray<Object> objects;
    OwnedArray<Connection> connections;
    OwnedArray<ConnectionBeingCreated> connectionsBeingCreated;

    Value locked = SynchronousValue();
    Value commandLocked;
    Value presentationMode;
    Value showDirection;
    Value altMode;

    bool showOrigin = false;
    bool showBorder = false;
    bool connectionsBehind = true;
    
    bool isScrolling = false;
    
    bool isGraph = false;
    bool isDraggingLasso = false;
    
    bool needsSearchUpdate = false;

    Value isGraphChild = SynchronousValue(var(false));
    Value hideNameAndArgs = SynchronousValue(var(false));
    Value xRange = SynchronousValue();
    Value yRange = SynchronousValue();
    Value patchWidth = SynchronousValue();
    Value patchHeight = SynchronousValue();

    Value zoomScale;

    ObjectGrid objectGrid = ObjectGrid(this);

    Point<int> const canvasOrigin;

    std::unique_ptr<GraphArea> graphArea;

    SafePointer<Object> lastSelectedObject;         // For auto patching
    SafePointer<Connection> lastSelectedConnection; // For auto patching

    Point<int> pastedPosition;
    Point<int> pastedPadding;

    std::unique_ptr<ConnectionPathUpdater> pathUpdater;
    RateReducer objectRateReducer = RateReducer(90);

    ObjectDragState dragState;

    inline static constexpr int infiniteCanvasSize = 128000;

    Component objectLayer;
    Component connectionLayer;
    
    NVGLUframebuffer* ioletBuffer = nullptr;
    float ioletScale;

private:
    
    GlobalMouseListener globalMouseListener;
    
    int lastMouseX, lastMouseY;
    LassoComponent<WeakReference<Component>> lasso;

    RateReducer canvasRateReducer = RateReducer(90);

    // Properties that can be shown in the inspector by right-clicking on canvas
    ObjectParameters parameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Canvas)
};
