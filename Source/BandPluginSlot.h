#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginPool.h"

// A single slot: shows current plugin name, click -> popup to add/remove/open editor
class BandPluginSlot : public juce::Component
{
public:
    using InstanceId = PluginPool::InstanceId;

    BandPluginSlot()
    {
        addAndMakeVisible(slotButton);
        slotButton.onClick = [this] { showMenu(); };
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
    void setHasPlugin(bool has) { hasPlugin = has; updateButtonText(); }

private:
    void updateButtonText()
    {
        if (hasPlugin)
            slotButton.setButtonText(pluginName.isEmpty() ? "Plugin" : pluginName);
        else
            slotButton.setButtonText("-None-");
    }

    void showMenu()
    {
        if (onRequestRebuildMenuList)
            onRequestRebuildMenuList(bandIndex, slotIndex);

        juce::PopupMenu menu;

        //Build plugin list submenu
		juce::PopupMenu pluginListMenu;

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

private:
    int bandIndex = 0;
	int slotIndex = 0;
    bool hasPlugin = false;
    juce::String pluginName;

    juce::TextButton slotButton;

    juce::Array<juce::PluginDescription> pluginDescs;
    juce::StringArray pluginNames;
};
