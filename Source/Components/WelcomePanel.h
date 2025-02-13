/*
 // Copyright (c) 2024 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once
#include "Utility/Autosave.h"
#include "Utility/CachedTextRender.h"
#include "Utility/NanoVGGraphicsContext.h"
#include "Components/BouncingViewport.h"

class WelcomePanel : public Component
    , public NVGComponent
    , public AsyncUpdater {
        
    class TopFillAllRect : public Component {
        Colour bgCol;

    public:
        TopFillAllRect() { };

        void setBGColour(Colour const& col)
        {
            bgCol = col;
            repaint();
        }

        void paint(Graphics& g) override
        {
            g.fillAll(bgCol);
        }
    };

    class WelcomePanelTile : public Component {
        WelcomePanel& parent;
        float snapshotScale;
        bool isHovered = false;
        String tileName, tileSubtitle;
        std::unique_ptr<Drawable> snapshot = nullptr;
        NVGImage titleImage, subtitleImage, snapshotImage;

        Image thumbnailImageData;

        int lastWidth = -1;
        int lastHeight = -1;

    public:
        bool isFavourited;
        std::function<void()> onClick = []() { };
        std::function<void(bool)> onFavourite = nullptr;

        WelcomePanelTile(WelcomePanel& welcomePanel, String name, String subtitle, String svgImage, Colour iconColour, float scale, bool favourited, Image const& thumbImage = Image())
            : parent(welcomePanel)
            , snapshotScale(scale)
            , tileName(name)
            , tileSubtitle(subtitle)
            , thumbnailImageData(thumbImage)
            , isFavourited(favourited)
        {
            if (!thumbImage.isValid()) {
                snapshot = Drawable::createFromImageData(svgImage.toRawUTF8(), svgImage.getNumBytesAsUTF8());
                if (snapshot) {
                    snapshot->replaceColour(Colours::black, iconColour);
                }
            }

            resized();
        }
        
        void setSearchQuery(String const& searchQuery)
        {
            setVisible(tileName.containsIgnoreCase(searchQuery));
        }

        void paint(Graphics& g) override
        {
            auto bounds = getLocalBounds().reduced(12);

            auto* nvg = dynamic_cast<NanoVGGraphicsContext&>(g.getInternalContext()).getContext();
            parent.drawShadow(nvg, getWidth(), getHeight());

            if (thumbnailImageData.isValid()) {
                if (!snapshotImage.isValid() || lastWidth != bounds.getWidth() || lastHeight != bounds.getHeight()) {
                    lastWidth = bounds.getWidth();
                    lastHeight = bounds.getHeight();

                    snapshotImage = NVGImage(nvg, bounds.getWidth() * 2, (bounds.getHeight() - 32) * 2, [this, bounds](Graphics& g) {
                        g.addTransform(AffineTransform::scale(2.0f));
                        if (thumbnailImageData.isValid()) {
                            auto imageWidth = thumbnailImageData.getWidth();
                            auto imageHeight = thumbnailImageData.getHeight();
                            auto componentWidth = bounds.getWidth();
                            auto componentHeight = bounds.getHeight();

                            float imageAspect = static_cast<float>(imageWidth) / imageHeight;
                            float componentAspect = static_cast<float>(componentWidth) / componentHeight;

                            int drawWidth, drawHeight;
                            int offsetX = 0, offsetY = 0;

                            if (componentAspect < imageAspect) {
                                // Component is wider than the image aspect ratio, scale based on height
                                drawHeight = componentHeight;
                                drawWidth = static_cast<int>(drawHeight * imageAspect);
                            } else {
                                // Component is taller than the image aspect ratio, scale based on width
                                drawWidth = componentWidth;
                                drawHeight = static_cast<int>(drawWidth / imageAspect);
                            }

                            // Calculate offsets to center the image
                            offsetX = (componentWidth - drawWidth) / 2;
                            offsetY = (componentHeight - drawHeight - 32) / 2;

                            g.drawImage(thumbnailImageData, offsetX, offsetY, drawWidth, drawHeight, 0, 0, imageWidth, imageHeight);
                        }
                    });
                }
            } else {
                if (snapshot && !snapshotImage.isValid()) {
                    snapshotImage = NVGImage(nvg, bounds.getWidth() * 2, (bounds.getHeight() - 32) * 2, [this](Graphics& g) {
                        g.addTransform(AffineTransform::scale(2.0f));
                        snapshot->drawAt(g, 0, 0, 1.0f);
                    });
                }
            }

            nvgSave(nvg);
            auto sB = bounds.toFloat().reduced(0.2f);
            nvgRoundedScissor(nvg, sB.getX(), sB.getY(), sB.getWidth(), sB.getHeight(), Corners::largeCornerRadius);

            auto lB = bounds.toFloat().expanded(0.5f);
            // Draw background even for images incase there is a transparent PNG
            nvgDrawRoundedRect(nvg, lB.getX(), lB.getY(), lB.getWidth(), lB.getHeight(), convertColour(findColour(PlugDataColour::canvasBackgroundColourId)), convertColour(findColour(PlugDataColour::toolbarOutlineColourId)), Corners::largeCornerRadius);
            if (thumbnailImageData.isValid()) {
                // Render the thumbnail image file that is in the root dir of the pd patch
                auto sB = bounds.toFloat().reduced(0.2f);
                snapshotImage.render(nvg, Rectangle<int>(sB.getX() + 12, sB.getY(), sB.getWidth(), sB.getHeight() - 32));
            } else {
                // Otherwise render the generated snapshot
                snapshotImage.render(nvg, bounds.withTrimmedBottom(32));
            }
            nvgRestore(nvg);

            // Draw border around
            nvgDrawRoundedRect(nvg, lB.getX(), lB.getY(), lB.getWidth(), lB.getHeight(), nvgRGBA(0, 0, 0, 0), convertColour(findColour(PlugDataColour::toolbarOutlineColourId)), Corners::largeCornerRadius);

            auto hoverColour = findColour(PlugDataColour::toolbarHoverColourId).interpolatedWith(findColour(PlugDataColour::toolbarBackgroundColourId), 0.5f);

            nvgBeginPath(nvg);
            nvgRoundedRectVarying(nvg, bounds.getX(), bounds.getHeight() - 32, bounds.getWidth(), 44, 0.0f, 0.0f, Corners::largeCornerRadius, Corners::largeCornerRadius);
            nvgFillColor(nvg, convertColour(isHovered ? hoverColour : findColour(PlugDataColour::toolbarBackgroundColourId)));
            nvgFill(nvg);
            nvgStrokeColor(nvg, convertColour(findColour(PlugDataColour::toolbarOutlineColourId)));
            nvgStroke(nvg);

            auto textWidth = bounds.getWidth() - 8;
            if (titleImage.needsUpdate(textWidth * 2, 24 * 2) || subtitleImage.needsUpdate(textWidth * 2, 16 * 2)) {
                titleImage = NVGImage(nvg, textWidth * 2, 24 * 2, [this, textWidth](Graphics& g) {
                    g.addTransform(AffineTransform::scale(2.0f, 2.0f));
                    g.setColour(Colours::white);
                    g.setFont(Fonts::getBoldFont().withHeight(14));
                    g.drawText(tileName, Rectangle<int>(0, 0, textWidth, 24), Justification::centredLeft, true);
                }, NVGImage::AlphaImage);

                subtitleImage = NVGImage(nvg, textWidth * 2, 16 * 2, [this, textWidth](Graphics& g) {
                    g.addTransform(AffineTransform::scale(2.0f, 2.0f));
                    g.setColour(Colours::white);
                    g.setFont(Fonts::getDefaultFont().withHeight(13.5f));
                    g.drawText(tileSubtitle, Rectangle<int>(0, 0, textWidth, 16), Justification::centredLeft, true);
                }, NVGImage::AlphaImage);
            }

            {
                auto textColour = findColour(PlugDataColour::panelTextColourId);
                
                NVGScopedState scopedState(nvg);
                nvgTranslate(nvg, 22, bounds.getHeight() - 30);
                titleImage.renderAlphaImage(nvg, Rectangle<int>(0, 0, bounds.getWidth() - 8, 24), convertColour(textColour));
                nvgTranslate(nvg, 0, 20);
                subtitleImage.renderAlphaImage(nvg, Rectangle<int>(0, 0, bounds.getWidth() - 8, 16), convertColour(textColour.withAlpha(0.75f)));
            }

            if (onFavourite) {
                auto favouriteIconBounds = getHeartIconBounds();
                nvgFontFace(nvg, "icon_font-Regular");

                if (isFavourited) {
                    nvgFillColor(nvg, nvgRGBA(250, 50, 40, 200));
                    nvgText(nvg, favouriteIconBounds.getX(), favouriteIconBounds.getY() + 14, Icons::HeartFilled.toRawUTF8(), nullptr);
                } else if (isMouseOver()) {
                    nvgFillColor(nvg, NVGComponent::convertColour(findColour(PlugDataColour::panelTextColourId)));
                    nvgText(nvg, favouriteIconBounds.getX(), favouriteIconBounds.getY() + 14, Icons::HeartStroked.toRawUTF8(), nullptr);
                }
            }
        }

        Rectangle<int> getHeartIconBounds()
        {
            return Rectangle<int>(20, getHeight() - 80, 16, 16);
        }

        void mouseEnter(MouseEvent const& e) override
        {
            isHovered = true;
            repaint();
        }

        void mouseExit(MouseEvent const& e) override
        {
            isHovered = false;
            repaint();
        }

        void mouseUp(MouseEvent const& e) override
        {
            if (!e.mods.isLeftButtonDown())
                return;

            if (onFavourite && getHeartIconBounds().contains(e.x, e.y)) {
                isFavourited = !isFavourited;
                onFavourite(isFavourited);
                repaint();
            } else {
                onClick();
            }
        }

        void resized() override
        {
            if (snapshot) {
                auto bounds = getLocalBounds().reduced(12).withTrimmedBottom(44);
                snapshot->setTransformToFit(bounds.withSizeKeepingCentre(bounds.getWidth() * snapshotScale, bounds.getHeight() * snapshotScale).toFloat(), RectanglePlacement::centred);
            }
        }
    };

public:
    enum Tab
    {
        Home,
        Library
    };

    WelcomePanel(PluginEditor* pluginEditor)
        : NVGComponent(this)
        , editor(pluginEditor)
    {
        viewport.setViewedComponent(&contentComponent, false);
        viewport.setScrollBarsShown(true, false, false, false);
        contentComponent.setVisible(true);
#if JUCE_IOS
        viewport.setVisible(OSUtils::isIPad());
#else
        viewport.setVisible(true);
#endif

        addChildComponent(viewport);

        // A top rectangle component that hides anything behind (we use this instead of scissoring)
        topFillAllRect.setBGColour(findColour(PlugDataColour::panelBackgroundColourId));

        setCachedComponentImage(new NVGSurface::InvalidationListener(editor->nvgSurface, this));
        triggerAsyncUpdate();
    }

    void drawShadow(NVGcontext* nvg, int width, int height)
    {
        // We only need one shadow image, because all tiles have the same size
        if (shadowImage.needsUpdate(width * 2.0f, height * 2.0f)) {
            shadowImage = NVGImage(nvg, width * 2.0f, height * 2.0f, [width, height](Graphics& g) {
                g.addTransform(AffineTransform::scale(2.0f, 2.0f));
                Path tilePath;
                tilePath.addRoundedRectangle(12.5f, 12.5f, width - 25.0f, height - 25.0f, Corners::largeCornerRadius);
                StackShadow::renderDropShadow(0, g, tilePath, Colours::white.withAlpha(0.12f), 6, { 0, 1 }); }, NVGImage::AlphaImage);
            repaint();
        }

        shadowImage.renderAlphaImage(nvg, Rectangle<int>(0, 0, width, height), nvgRGB(0, 0, 0));
    }
        
    void setSearchQuery(String const& newSearchQuery)
    {
        searchQuery = newSearchQuery;
        if(newPatchTile) newPatchTile->setVisible(searchQuery.isEmpty());
        if(openPatchTile) openPatchTile->setVisible(searchQuery.isEmpty());
        
        auto& tiles = currentTab == Home ? recentlyOpenedTiles : libraryTiles;
        for (auto* tile : tiles) {
            tile->setSearchQuery(searchQuery);
        }
        resized();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(20);
        auto rowBounds = bounds.removeFromTop(160);

        int const desiredTileWidth = 190;
        int const tileSpacing = 4;

        int totalWidth = bounds.getWidth();
        // Calculate the number of columns that can fit in the total width
        int numColumns = std::max(1, totalWidth / (desiredTileWidth + tileSpacing));
        // Adjust the tile width to fit within the available width
        int actualTileWidth = (totalWidth - (numColumns - 1) * tileSpacing) / numColumns;

        if (newPatchTile && currentTab == Home)
            newPatchTile->setBounds(rowBounds.removeFromLeft(actualTileWidth));
        rowBounds.removeFromLeft(4);
        if (openPatchTile && currentTab == Home)
            openPatchTile->setBounds(rowBounds.removeFromLeft(actualTileWidth));

        auto viewPos = viewport.getViewPosition();
        viewport.setBounds(getLocalBounds());

        auto& tiles = currentTab == Home ? recentlyOpenedTiles : libraryTiles;
        int numRows = (tiles.size() + numColumns - 1) / numColumns;
        int totalHeight = (numRows * 160) + 200;

        auto tilesBounds = Rectangle<int>(24, currentTab == Home ? 206 : 6, totalWidth + 24, totalHeight + 24);
        contentComponent.setBounds(tilesBounds);
        
        // Start positioning the recentlyOpenedTiles
        rowBounds = tilesBounds.removeFromTop(160);
        for (auto* tile : tiles) {
            if(!tile->isVisible()) continue;
            if (tile->isFavourited) {
                if (rowBounds.getWidth() < actualTileWidth) {
                    rowBounds = tilesBounds.removeFromTop(160);
                }
                tile->setBounds(rowBounds.removeFromLeft(actualTileWidth));
                rowBounds.removeFromLeft(tileSpacing);
            }
        }

        for (auto* tile : tiles) {
            if(!tile->isVisible()) continue;
            if (!tile->isFavourited) {
                if (rowBounds.getWidth() < actualTileWidth) {
                    rowBounds = tilesBounds.removeFromTop(160);
                }
                tile->setBounds(rowBounds.removeFromLeft(actualTileWidth));
                rowBounds.removeFromLeft(tileSpacing);
            }
        }
        viewport.setViewPosition(viewPos);
    }
        
    void setShownTab(WelcomePanel::Tab tab)
    {
        currentTab = tab;
        if(tab == Home)
        {
            newPatchTile->setVisible(true);
            openPatchTile->setVisible(true);
            for(auto* tile : recentlyOpenedTiles)
            {
                tile->setVisible(true);
            }
            for(auto* tile : libraryTiles)
            {
                tile->setVisible(false);
            }
        }
        else {
            newPatchTile->setVisible(false);
            openPatchTile->setVisible(false);
            for(auto* tile : recentlyOpenedTiles)
            {
                tile->setVisible(false);
            }
            for(auto* tile : libraryTiles)
            {
                tile->setVisible(true);
            }
        }
        
        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override
    {
        // TODO: why are we updating this??
        newPatchTile = std::make_unique<WelcomePanelTile>(*this, "New Patch", "Create a new empty patch", newIcon, findColour(PlugDataColour::panelTextColourId), 0.33f, false);
        openPatchTile = std::make_unique<WelcomePanelTile>(*this, "Open Patch", "Browse for a patch to open", openIcon, findColour(PlugDataColour::panelTextColourId), 0.33f, false);

        newPatchTile->onClick = [this]() { editor->getTabComponent().newPatch(); };
        openPatchTile->onClick = [this]() { editor->getTabComponent().openPatch(); };

        topFillAllRect.setBGColour(findColour(PlugDataColour::panelBackgroundColourId));
        addAndMakeVisible(topFillAllRect);

        contentComponent.addAndMakeVisible(*newPatchTile);
        contentComponent.addAndMakeVisible(*openPatchTile);

        recentlyOpenedTiles.clear();

        auto settingsTree = SettingsFile::getInstance()->getValueTree();
        auto recentlyOpenedTree = settingsTree.getChildWithName("RecentlyOpened");

        if (recentlyOpenedTree.isValid()) {
            // Place favourited patches at the top
            for (int i = 0; i < recentlyOpenedTree.getNumChildren(); i++) {

                auto subTree = recentlyOpenedTree.getChild(i);
                auto patchFile = File(subTree.getProperty("Path").toString());
                auto patchThumbnailBase = File(patchFile.getParentDirectory().getFullPathName() + "\\" + patchFile.getFileNameWithoutExtension() + "_thumb");

                auto favourited = subTree.hasProperty("Pinned") && static_cast<bool>(subTree.getProperty("Pinned"));
                auto snapshotColour = LookAndFeel::getDefaultLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId).withAlpha(0.3f);

                String silhoutteSvg;
                Image thumbImage;

                StringArray possibleExtensions { ".png", ".jpg", ".jpeg", ".gif" };

                for (auto& ext : possibleExtensions) {
                    auto patchThumbnail = patchThumbnailBase.withFileExtension(ext);
                    if (patchThumbnail.existsAsFile()) {
                        FileInputStream fileStream(patchThumbnail);
                        if (fileStream.openedOk()) {
                            thumbImage = ImageFileFormat::loadFrom(fileStream).convertedToFormat(Image::ARGB);
                            break;
                        }
                    }
                }
                if (thumbImage.isNull()) {
                    if (patchFile.existsAsFile()) {
                        silhoutteSvg = OfflineObjectRenderer::patchToSVG(patchFile.loadFileAsString());
                    }
                }

                auto openTime = Time(static_cast<int64>(subTree.getProperty("Time")));
                auto diff = Time::getCurrentTime() - openTime;
                String date;
                if (diff.inDays() == 0)
                    date = "Today";
                else if (diff.inDays() == 1)
                    date = "Yesterday";
                else
                    date = openTime.toString(true, false);
                String time = openTime.toString(false, true, false, true);
                String timeDescription = date + ", " + time;

                auto* tile = recentlyOpenedTiles.add(new WelcomePanelTile(*this, patchFile.getFileNameWithoutExtension(), timeDescription, silhoutteSvg, snapshotColour, 1.0f, favourited, thumbImage));
                tile->onClick = [this, patchFile]() mutable {
                    if (patchFile.existsAsFile()) {
                        editor->pd->autosave->checkForMoreRecentAutosave(patchFile, editor, [this, patchFile]() {
                            editor->getTabComponent().openPatch(URL(patchFile));
                            SettingsFile::getInstance()->addToRecentlyOpened(patchFile);
                        });
                    } else {
                        editor->pd->logError("Patch not found");
                    }
                };
                tile->onFavourite = [this, path = subTree.getProperty("Path")](bool shouldBeFavourite) mutable {
                    auto settingsTree = SettingsFile::getInstance()->getValueTree();
                    auto recentlyOpenedTree = settingsTree.getChildWithName("RecentlyOpened");

                    // Settings file could be reloaded, we can't assume the old recently opened tree is still valid!
                    // So look up the entry by file path
                    auto subTree = recentlyOpenedTree.getChildWithProperty("Path", path);
                    subTree.setProperty("Pinned", shouldBeFavourite, nullptr);
                    resized();
                };
                contentComponent.addAndMakeVisible(tile);
            }
        }

        findLibraryPatches();
        resized();
    }
        
    void findLibraryPatches()
    {
        libraryTiles.clear();
        
        auto addTile = [this](File& patchFile){
            auto patchThumbnailBase = File(patchFile.getParentDirectory().getFullPathName() + "\\" + patchFile.getFileNameWithoutExtension() + "_thumb");
            StringArray possibleExtensions { ".png", ".jpg", ".jpeg", ".gif" };
            
            float scale = 1.0f;
            Image thumbImage;
            for (auto& ext : possibleExtensions) {
                auto patchThumbnail = patchThumbnailBase.withFileExtension(ext);
                if (patchThumbnail.existsAsFile()) {
                    FileInputStream fileStream(patchThumbnail);
                    if (fileStream.openedOk()) {
                        thumbImage = ImageFileFormat::loadFrom(fileStream).convertedToFormat(Image::ARGB);
                        break;
                    }
                }
            }
            String placeholderIcon;
            if(!thumbImage.isValid())
            {
                scale = 0.6f;
                placeholderIcon = libraryPlaceholderIcon;
            }
            auto snapshotColour = LookAndFeel::getDefaultLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId).withAlpha(0.3f);

            auto* tile = libraryTiles.add(new WelcomePanelTile(*this, patchFile.getFileNameWithoutExtension(), "", placeholderIcon, snapshotColour, scale, false, thumbImage));
            tile->onClick = [this, patchFile]() mutable {
                if (patchFile.existsAsFile()) {
                    editor->pd->autosave->checkForMoreRecentAutosave(patchFile, editor, [this, patchFile]() {
                        editor->getTabComponent().openPatch(URL(patchFile));
                        SettingsFile::getInstance()->addToRecentlyOpened(patchFile);
                    });
                } else {
                    editor->pd->logError("Patch not found");
                }
            };
            contentComponent.addAndMakeVisible(tile);
        };
        auto patchesFolder = ProjectInfo::appDataDir.getChildFile("Patches");
        for(auto& file : OSUtils::iterateDirectory(patchesFolder, false, false))
        {
            if(OSUtils::isDirectoryFast(file.getFullPathName()))
            {
                for(auto& subfile : OSUtils::iterateDirectory(file, false, false))
                {
                    if(subfile.hasFileExtension("pd"))
                    {
                        addTile(subfile);
                        break;
                    }
                }
            }
            else {
                if(file.hasFileExtension("pd"))
                {
                    addTile(file);
                }
            }
        }
    }

    void show()
    {
        triggerAsyncUpdate();
        setVisible(true);
    }

    void hide()
    {
        setVisible(false);
    }

    void render(NVGcontext* nvg) override
    {
        if (!nvgContext || nvgContext->getContext() != nvg)
            nvgContext = std::make_unique<NanoVGGraphicsContext>(nvg);

        nvgFillColor(nvg, convertColour(findColour(PlugDataColour::panelBackgroundColourId)));
        nvgFillRect(nvg, 0, 0, getWidth(), getHeight());

        Graphics g(*nvgContext);
        g.reduceClipRegion(editor->nvgSurface.getInvalidArea());
        paintEntireComponent(g, false);

        auto gradient = nvgLinearGradient(nvg, 0, viewport.getY(), 0, viewport.getY() + 20, convertColour(findColour(PlugDataColour::panelBackgroundColourId)), nvgRGBA(255, 255, 255, 0));

        nvgFillPaint(nvg, gradient);
        nvgFillRect(nvg, viewport.getX() + 8, viewport.getY(), viewport.getWidth() - 16, 20);
    }

    void lookAndFeelChanged() override
    {
        triggerAsyncUpdate();
    }

    static inline String const newIcon = "<?xml version=\"1.0\" standalone=\"no\"?>\n"
                                         "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\" >\n"
                                         "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" viewBox=\"-10 0 2058 2048\">\n"
                                         "   <path fill=\"currentColor\"\n"
                                         "d=\"M1024 170v512q0 72 50 122t120 50h512v852q0 72 -50 122t-120 50h-1024q-70 0 -120 -50.5t-50 -121.5v-1364q0 -72 50 -122t120 -50h512zM1151 213l512 512h-469q-16 0 -29.5 -12.5t-13.5 -30.5v-469z\" />\n"
                                         "</svg>\n";

    static inline String const openIcon = "<?xml version=\"1.0\" standalone=\"no\"?>\n"
                                          "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\" >\n"
                                          "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" viewBox=\"-10 0 2058 2048\">\n"
                                          "   <path fill=\"currentColor\"\n"
                                          "d=\"M1180 555h506q72 0 126 47t64 118v13l2 14v768q0 76 -52 131t-128 60h-12h-1324q-76 0 -131 -51.5t-59 -127.5l-2 -12v-620l530 2l17 -2q51 -4 92 -33l4 -3t6 -5l4 -2zM700 342q59 0 109 32l14 11l181 149l-263 219l-8 4q-10 8 -24 11h-9h-530v-236q0 -76 52 -131\n"
                                          "t128 -59h12h338z\" />\n"
                                          "</svg>\n";
        
    static inline String const libraryPlaceholderIcon = "<svg width=\"864\" height=\"864\" viewBox=\"0 0 864 864\" fill=\"none\" xmlns=\"http://www.w3.org/2000/svg\">\n"
        "<path d=\"M538.114 201.488C550.72 201.488 560.94 191.268 560.94 178.662C560.94 166.055 550.72 155.836 538.114 155.836C525.507 155.836 515.288 166.055 515.288 178.662C515.288 191.268 525.507 201.488 538.114 201.488Z\" fill=\"black\"/>\n"
        "<path d=\"M178.662 560.94C191.268 560.94 201.488 550.72 201.488 538.114C201.488 525.507 191.268 515.288 178.662 515.288C166.055 515.288 155.836 525.507 155.836 538.114C155.836 550.72 166.055 560.94 178.662 560.94Z\" fill=\"black\"/>\n"
        "<path d=\"M695.922 201.488C708.528 201.488 718.748 191.268 718.748 178.662C718.748 166.055 708.528 155.836 695.922 155.836C683.315 155.836 673.096 166.055 673.096 178.662C673.096 191.268 683.315 201.488 695.922 201.488Z\" fill=\"black\"/>\n"
        "<path d=\"M336.47 560.94C349.076 560.94 359.296 550.72 359.296 538.114C359.296 525.507 349.076 515.288 336.47 515.288C323.863 515.288 313.644 525.507 313.644 538.114C313.644 550.72 323.863 560.94 336.47 560.94Z\" fill=\"black\"/>\n"
        "<path d=\"M695.922 359.296C708.528 359.296 718.748 349.076 718.748 336.47C718.748 323.863 708.528 313.644 695.922 313.644C683.315 313.644 673.096 323.863 673.096 336.47C673.096 349.076 683.315 359.296 695.922 359.296Z\" fill=\"black\"/>\n"
        "<path d=\"M336.47 718.748C349.076 718.748 359.296 708.528 359.296 695.922C359.296 683.315 349.076 673.096 336.47 673.096C323.863 673.096 313.644 683.315 313.644 695.922C313.644 708.528 323.863 718.748 336.47 718.748Z\" fill=\"black\"/>\n"
        "<path d=\"M538.114 359.296C550.72 359.296 560.94 349.076 560.94 336.47C560.94 323.863 550.72 313.644 538.114 313.644C525.507 313.644 515.288 323.863 515.288 336.47C515.288 349.076 525.507 359.296 538.114 359.296Z\" fill=\"black\"/>\n"
        "<path d=\"M178.662 718.748C191.268 718.748 201.488 708.528 201.488 695.922C201.488 683.315 191.268 673.096 178.662 673.096C166.055 673.096 155.836 683.315 155.836 695.922C155.836 708.528 166.055 718.748 178.662 718.748Z\" fill=\"black\"/>\n"
        "<path fill-rule=\"evenodd\" clip-rule=\"evenodd\" d=\"M216.158 112L287.842 112C324.06 112 337.194 115.771 350.434 122.852C363.675 129.933 374.066 140.325 381.148 153.566C388.229 166.806 392 179.94 392 216.158V287.842C392 324.06 388.229 337.194 381.148 350.434C374.066 363.675 363.675 374.066 350.434 381.148C337.194 388.229 324.06 392 287.842 392H216.158C179.94 392 166.806 388.229 153.566 381.148C140.325 374.066 129.933 363.675 122.852 350.434C115.771 337.194 112 324.06 112 287.842V216.158C112 179.94 115.771 166.806 122.852 153.566C129.933 140.325 140.325 129.933 153.566 122.852C166.806 115.771 179.94 112 216.158 112Z\" fill=\"black\"/>\n"
        "<path fill-rule=\"evenodd\" clip-rule=\"evenodd\" d=\"M576.158 472H647.842C684.06 472 697.194 475.771 710.434 482.852C723.675 489.933 734.066 500.325 741.148 513.566C748.229 526.806 752 539.94 752 576.158V647.842C752 684.06 748.229 697.194 741.148 710.434C734.066 723.675 723.675 734.066 710.434 741.148C697.194 748.229 684.06 752 647.842 752H576.158C539.94 752 526.806 748.229 513.566 741.148C500.325 734.066 489.933 723.675 482.852 710.434C475.771 697.194 472 684.06 472 647.842V576.158C472 539.94 475.771 526.806 482.852 513.566C489.933 500.325 500.325 489.933 513.566 482.852C526.806 475.771 539.94 472 576.158 472Z\" fill=\"black\"/>\n"
        "<rect x=\"30\" y=\"30\" width=\"804\" height=\"804\" rx=\"172\" stroke=\"black\" stroke-width=\"8\"/>\n"
        "</svg>\n";

    std::unique_ptr<WelcomePanelTile> newPatchTile;
    std::unique_ptr<WelcomePanelTile> openPatchTile;

    Component contentComponent;
    BouncingViewport viewport;

    TopFillAllRect topFillAllRect;

    std::unique_ptr<NanoVGGraphicsContext> nvgContext = nullptr;

    NVGImage shadowImage;
    OwnedArray<WelcomePanelTile> recentlyOpenedTiles;
    OwnedArray<WelcomePanelTile> libraryTiles;
    PluginEditor* editor;
        
    String searchQuery;
    Tab currentTab = Home;
        
};
