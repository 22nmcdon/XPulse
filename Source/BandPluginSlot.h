#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginPool.h"
#include "BinaryData.h"

// A single slot: shows current plugin name, click -> popup to add/remove/open editor
class BandPluginSlot : public juce::Component
{
public:
    using InstanceId = PluginPool::InstanceId;

    BandPluginSlot()
    {
        imgEmpty = juce::ImageCache::getFromMemory(BinaryData::ComboBoxBlueOff_png, BinaryData::ComboBoxBlueOff_pngSize);
        imgEmptyHover = juce::ImageCache::getFromMemory(BinaryData::ComboBoxBlueOn_png, BinaryData::ComboBoxBlueOn_pngSize);
        imgLoaded = juce::ImageCache::getFromMemory(BinaryData::ComboBoxRedOff_png, BinaryData::ComboBoxRedOff_pngSize);
        imgLoadedHover = juce::ImageCache::getFromMemory(BinaryData::ComboBoxRedOn_png, BinaryData::ComboBoxRedOn_pngSize);

        addAndMakeVisible(slotButton);
        slotButton.onClick = [this] { showMenu(); };

        typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::ARCADECLASSIC_TTF,
            BinaryData::ARCADECLASSIC_TTFSize);

        slotFont = juce::Font(typeface).withHeight(12.0f);

        menuLnF = std::make_unique<MenuLookAndFeel>(typeface);

        refreshImages();
        updateButtonText();
    }
    void resized() override
    {
        slotButton.setBounds(getLocalBounds());
    }

    // Callbacks (set these from your editor)
    std::function<void(int bandIndex, int slotIndex)> onRequestRebuildMenuList;
    std::function<void(int bandIndex, int slotIndex, const juce::PluginDescription& desc)> onAddReplace;
    std::function<void(int bandIndex, int slotIndex)> onRemove;
    std::function<void(int bandIndex, int slotIndex)> onOpenEditor;

    void setBandIndex(int idx) { bandIndex = idx; }
    void setSlotIndex(int idx) { slotIndex = idx; }

	int getBandIndex() const { return bandIndex; }
	int getSlotIndex() const { return slotIndex; }

    void setPluginName(const juce::String& name) { pluginName = name; updateButtonText(); }
    void setHasPlugin(bool has)
    {
        hasPlugin = has;
        refreshImages();
        updateButtonText();
    }

private:
    void updateButtonText()
    {
        if (hasPlugin)
            slotText = pluginName.isEmpty() ? "Plugin" : pluginName;
        else
            slotText = "-None-";

        repaint();
    }
    struct MenuLookAndFeel : public juce::LookAndFeel_V4
    {
        MenuLookAndFeel(juce::Typeface::Ptr tf)
            : typeface(tf)
        {
        }

        // Theme knobs (change once; used everywhere in menu)
        juce::Colour bg = juce::Colour(0xFF141414);
        juce::Colour border = juce::Colour(0xFF3A3A3A);
        juce::Colour highlight = juce::Colour(0xFF2A2A2A);
        juce::Colour text = juce::Colour(0xFFEAEAEA);
        juce::Colour disabled = juce::Colour(0xFF7A7A7A);
        juce::Colour separator = juce::Colour(0xFF3A3A3A);

        juce::Font getPopupMenuFont() override
        {
            if (typeface != nullptr)
                return juce::Font(typeface).withHeight(14.0f);

            return juce::Font(14.0f);
        }

        void drawPopupMenuBackground(juce::Graphics& g, int w, int h) override
        {
            g.fillAll(bg);
            g.setColour(border);
            g.drawRect(0, 0, w, h);
        }

        void drawPopupMenuItem(juce::Graphics& g,
            const juce::Rectangle<int>& area,
            bool isSeparator,
            bool isActive,
            bool isHighlighted,
            bool isTicked,
            bool hasSubMenu,
            const juce::String& textIn,
            const juce::String& shortcutKeyText,
            const juce::Drawable* icon,
            const juce::Colour* /*textColourToUse*/) override
        {
            if (isSeparator)
            {
                g.setColour(separator);
                g.drawLine((float)area.getX() + 6.0f, (float)area.getCentreY(),
                    (float)area.getRight() - 6.0f, (float)area.getCentreY());
                return;
            }

            if (isHighlighted && isActive)
            {
                g.setColour(highlight);
                g.fillRect(area);
            }

            g.setFont(getPopupMenuFont());
            g.setColour(isActive ? text : disabled);

            auto r = area.reduced(8, 0);

            // Optional icon
            if (icon != nullptr)
            {
                auto iconArea = r.removeFromLeft(18).reduced(1);
                icon->drawWithin(g, iconArea.toFloat(), juce::RectanglePlacement::centred, 1.0f);
            }


            // Shortcut text 
            if (shortcutKeyText.isNotEmpty())
            {
                auto right = r.removeFromRight(70);
                g.drawText(shortcutKeyText, right, juce::Justification::centredRight, true);
            }

            // Submenu arrow
            if (hasSubMenu)
            {
                g.drawText(">", r.removeFromRight(16), juce::Justification::centred, true);
            }

            // Item text
            g.drawText(textIn, r, juce::Justification::centredLeft, true);
        }

        juce::Typeface::Ptr typeface;
    };

    void showMenu()
    {
        if (onRequestRebuildMenuList)
            onRequestRebuildMenuList(bandIndex, slotIndex);

        juce::PopupMenu menu;

        //Build plugin list submenu
		juce::PopupMenu pluginListMenu;

		menu.setLookAndFeel(menuLnF.get());
		pluginListMenu.setLookAndFeel(menuLnF.get());

        if (pluginNames.isEmpty())
        {
            pluginListMenu.addItem(999, "No plugins available", false);
		}
        else {
            for(int i = 0; i < pluginNames.size(); ++i)
            {
                pluginListMenu.addItem(2000 + i, pluginNames[i], true);
			}
        }


        // Top actions
        if (hasPlugin)
        {
            menu.addItem(1001, "Open Editor", true);
            menu.addItem(1002, "Remove", true);
            menu.addSeparator();

            // Replace submenu
            menu.addSubMenu("Replace", pluginListMenu);
        }
        else
        {
            // Add submenu
            menu.addSubMenu("Add", pluginListMenu);
        }


        menu.showMenuAsync(juce::PopupMenu::Options(),
            [this](int result)
            {
                if (result == 0) return;

                if (result == 1001) { if (onOpenEditor) onOpenEditor(bandIndex, slotIndex); return; }
                if (result == 1002) { if (onRemove) onRemove(bandIndex, slotIndex); return; }

                if (result >= 2000)
                {
                    int idx = result - 2000;
                    if (idx >= 0 && idx < pluginDescs.size())
                        if (onAddReplace) onAddReplace(bandIndex, slotIndex, pluginDescs[(size_t)idx]);
                }
            });
    }


public:
    // Set by editor whenever plugin list changes
    void setPluginList(const juce::Array<juce::PluginDescription>& descs)
    {
        pluginDescs.clear();
        pluginNames.clear();

        pluginDescs.ensureStorageAllocated(descs.size());
        pluginNames.ensureStorageAllocated(descs.size());

        for (auto& d : descs)
        {
            pluginDescs.add(d);
            pluginNames.add(d.name);
        }
    }

    void refreshImages()
    {
        const auto base = hasPlugin ? imgLoaded : imgEmpty;
        const auto over = hasPlugin ? imgLoadedHover : imgEmptyHover;

        // If hover image missing, fall back to base
        const auto overFinal = over.isValid() ? over : base;

        // No down image yet -> reuse hover
        const auto downFinal = overFinal;

        slotButton.setImages(false, true, true,
            base, 1.0f, juce::Colours::transparentBlack,
            overFinal, 1.0f, juce::Colours::transparentBlack,
            downFinal, 1.0f, juce::Colours::transparentBlack);
    }

    void paintOverChildren(juce::Graphics& g) override
    {
        g.setColour(hasPlugin ? textColourNoPlugin : textColour);        
        g.setFont(slotFont);         

        g.drawFittedText(slotText,
            getLocalBounds().reduced(6),
            juce::Justification::centred,
            1);
	}

private:
    int bandIndex = 0;
	int slotIndex = 0;
    bool hasPlugin = false;
    juce::String pluginName;

    juce::Typeface::Ptr typeface;
    juce::Font slotFont;
    std::unique_ptr<MenuLookAndFeel> menuLnF;
    juce::Colour textColour = juce::Colour(138, 0, 0);
	juce::Colour textColourNoPlugin = juce::Colour(0, 255, 255);

    juce::ImageButton slotButton;

	juce::Image imgEmpty, imgEmptyHover, imgLoaded, imgLoadedHover;
    juce::String slotText;

    juce::Array<juce::PluginDescription> pluginDescs;
    juce::StringArray pluginNames;
};
