/*
    Copyright (c) 2014-2019  Michael Fisher <mfisher@kushview.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

namespace jlv2 {

/** A wrapper around LilvPlugin/LilvInstance for running LV2 plugins
    Methods that are realtime/thread safe are excplicity documented as so.
    All other methods are NOT realtime safe
 */
class Module : private Timer
{
public:
    /** Create a new Module */
    Module (World& world, const void* plugin_);

    /** Destructor */
    ~Module();

    /** If set will be called on the message thread when a notification
        is received from the plugin. */
    PortNotificationFunction onPortNotify;

    /** Get the total number of ports for this plugin */
    uint32 getNumPorts() const;

    /** Get the number of ports for a given port type and flow (input or output) */
    uint32 getNumPorts (PortType type, bool isInput) const;

    /** Get the plugin's Author/Manufacturer name */
    String getAuthorName() const;

    /** Get a channel configuration */
    const ChannelConfig& getChannelConfig() const;

    /** Get the plugins class label (category) */
    String getClassLabel() const;

    /** Get the port intended to be used as a MIDI input */
    uint32 getMidiPort() const;

    /** Get the plugin's name */
    String getName() const;

    /** Get the plugin's notify port. e.g. the port intended to be
        used as a MIDI output */
    uint32 getNotifyPort() const;

    /** Get the underlying LV2_Handle */
    LV2_Handle getHandle();

    /** Get the LilvPlugin object for this Module */
    const LilvPlugin* getPlugin() const;

    /** Get the LilvPort for this Module (by index) */
    const LilvPort* getPort (uint32 index) const;

    /** Get a port's name */
    const String getPortName (uint32 port) const;

    /** Get a ports range (min, max and default value) */
    void getPortRange (uint32 port, float& min, float& max, float& def) const;

    /** Get the type of port for a port index */
    PortType getPortType (uint32 index) const;

    /** Get the URI for this plugin */
    String getURI() const;

    World& getWorld() { return world; }

    /** Returns true if the Plugin has one or more UIs */
    inline bool hasEditor() const;

    /** Returns the best quality UI by URI */
    String getBestUI() const { return bestUI; }

    /** Create an editor for this plugin */
    ModuleUI* createEditor();

    /** Returns the port index for a given symbol */
    uint32 getPortIndex (const String& symbol) const;

    /** Returns true if the port is an Input */
    bool isPortInput (uint32 port) const;

    /** Returns true if the port is an Output */
    bool isPortOutput (uint32 port) const;

    /** Set the sample rate for this plugin
        @param newSampleRate The new rate to use
        @note This will re-instantiate the plugin
      */
    void setSampleRate (double newSampleRate);

    /** Get the plugin's extension data
        @param uri The uri of the extesion
        @return A pointer to extension data or nullptr if not available
        @note This is in the LV2 Discovery Threading class 
      */
    const void* getExtensionData (const String& uri) const;

    /** Instantiate the Plugin
        @param samplerate The samplerate to use
        @note This is in the LV2 Instantiation Threading class */
    Result instantiate (double samplerate);

    /** Activate the plugin
        @note This is in the LV2 Instantiation Threading class */
    void activate();

    /** Activate the plugin
        @note This is in the LV2 Instantiation Threading class */
    void cleanup();

    /** Deactivate the plugin
        @note This is in the LV2 Instantiation Threading class */
    void deactivate();

    /** Returns true if the plugin has been activated
        @note This should NOT be used in a realtime thread
      */
    bool isActive() const;

    /** Run / process the plugin for a cycle
        @param nframes The number of samples to process
        @note If you need to process events only, then call this method 
              with nframes = 0.
      */
    void run (uint32 nframes);

    /** Connect a port to a data location
        @param port The port index to connect
        @param data A pointer to the port buffer that should be used
      */
    void connectPort (uint32 port, void* data);

    /** Connect a channel to a data Location.  
        This simply converts the channel number to a port index then 
        calls Module::connectPort
      */
    void connectChannel (const PortType type, const int32 channel, void* data, const bool isInput);

    void referAudioReplacing (AudioSampleBuffer&);
    
    PortBuffer* getPortBuffer (uint32) const;

    /** Returns an LV2 preset/state as a string */
    String getStateString() const;

    /** Restore from state created with getStateString()
        @see getStateString
      */
    void setStateString (const String&);

    /** Write some data to a port
        This will send a PortEvent to the audio thread
      */
    void write (uint32 port, uint32 size, uint32 protocol, const void* buffer);

    /** Send port values to listeners now */
    void sendPortEvents();

    /** Returns a mapped LV2_URID */
    uint32 map (const String& uri) const;

    /** @internal ModuleUI's should call this when being unloaded */
    void clearEditor();
    
private:
    LilvInstance* instance;
    const LilvPlugin* plugin;
    World&    world;
    mutable String bestUI;

    bool active;
    double currentSampleRate;
    uint32 numPorts;
    Array<const LV2_Feature*> features;

    std::unique_ptr<RingBuffer> events;
    HeapBlock<uint8> evbuf;
    uint32 evbufsize;

    std::unique_ptr<RingBuffer> notifications;
    HeapBlock<uint8> ntbuf;
    uint32 ntbufsize;

    Result allocateEventBuffers();
    void activatePorts();
    void freeInstance();
    void init();
    
    void timerCallback() override;

    class Private;
    ScopedPointer<Private>   priv;
    ScopedPointer<WorkerFeature> worker;

    /** @internal */
    bool isLoaded() const;
};

class ModuleUI final : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<ModuleUI>;

    ~ModuleUI ()
    {
        unload();
    }

    bool loaded() const { return instance != nullptr; }

    void unload()
    {
        plugin.clearEditor();
        
        if (instance)
        {
            suil_instance_free (instance);
            instance = nullptr;
        }
    }

    World& getWorld()   const { return world; }
    Module& getPlugin() const { return plugin; }

    LV2UI_Widget getWidget() const
    { 
        return instance != nullptr ? suil_instance_get_widget (instance)
                                   : nullptr;
    }

    void portEvent (uint32 port, uint32 size, uint32 format, const void* buffer)
    {
        if (instance == nullptr)
            return;
        suil_instance_port_event (instance, port, size, format, buffer);
    }

private:
    friend class Module;
    friend class World;

    ModuleUI (World& w, Module& m)
        : world (w), plugin (m) { }

    World& world;
    Module& plugin;
    SuilInstance* instance = nullptr;

    static void portWrite (void* controller, uint32_t port, uint32_t size,
                           uint32_t protocol, void const* buffer)
    {
        auto& plugin = (static_cast<ModuleUI*> (controller))->getPlugin();
        plugin.write (port, size, protocol, buffer);
    }

    static uint32_t portIndex (void* controller, const char* symbol)
    {
        auto& plugin = (static_cast<ModuleUI*> (controller))->getPlugin();
        return plugin.getPortIndex (symbol);
    }
};

}