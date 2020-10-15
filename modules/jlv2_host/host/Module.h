/*
    Copyright (c) 2014-2019  Michael Fisher <mfisher@kushview.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
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

/** Representation of LV2 Scale Points */
class ScalePoints
{
public:
    using ValueMap  = HashMap<String, float>;
    using size_type = int;

    ScalePoints() = default;
    ~ScalePoints() = default;

    ScalePoints (const ScalePoints& o) { operator= (o); }
    ScalePoints& operator= (const ScalePoints& o)
    {
        ValueMap::Iterator iter (o.points);
        while (iter.next())
            points.set (iter.getKey(), iter.getValue());
        return *this;
    }

    /** Return true if empty */
    bool isEmpty()    const { return points.size() <= 0; }

    /** Return true if not empty */
    bool isNotEmpty() const { return ! isEmpty(); }

    /** Returns the total number of scale points */
    int  size()       const { return points.size(); }

    class Iterator
    {
    public:
        Iterator (const ScalePoints& o) 
            : iter (o.points) {}

        bool   next()           { return iter.next(); }
        float  getValue() const { return iter.getValue(); }
        String getLabel() const { return iter.getKey(); }

    private:
        ValueMap::Iterator iter;
    };

private:
    friend class Module;
    friend class Iterator;
    ValueMap points;
};

/** Description of a supported UI */
struct SupportedUI
{
    String URI;         // The UIs URI
    String plugin;      // the plugin URI
    String container;   // container type URI
    String widget;      // widget type URI
    String bundle;      // Bundle path
    String binary;      // Binary path

    // UI provides show interface
    bool useShowInterface { false };
};

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
    void* getHandle();

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

    /** Get the scale points for a port */
    ScalePoints getScalePoints (uint32 port) const;

    /** Returns true if the port should be enumerated */
    bool isPortEnumerated (uint32 index) const;

    /** Get the URI for this plugin */
    String getURI() const;

    /** Returns the world which crated this module */
    World& getWorld() { return world; }

    //=========================================================================

    /** Returns true if the Plugin has one or more UIs */
    bool hasEditor() const;

    /** Returns the best quality UI by URI */
    String getBestUI() const { return bestUI; }

    /** Create an editor for this plugin */
    ModuleUI* createEditor();

    /** @internal ModuleUI's should call this when being unloaded */
    void clearEditor();

    //=========================================================================

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

    //=========================================================================

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
        @note This is in the LV2 Instantiation Threading class
     */
    void activate();

    /** Activate the plugin
        @note This is in the LV2 Instantiation Threading class 
     */
    void cleanup();

    /** Deactivate the plugin
        @note This is in the LV2 Instantiation Threading class 
     */
    void deactivate();

    /** Returns true if the plugin has been activated
        @note This should NOT be used in a realtime thread
      */
    bool isActive() const;

    //=========================================================================

    /** Run / process the plugin for a cycle (realtime)
        @param nframes The number of samples to process
        @note If you need to process events only, then call this method 
              with nframes = 0.
      */
    void run (uint32 nframes);

    /** Connect a port to a data location (realtime)
        @param port The port index to connect
        @param data A pointer to the port buffer that should be used
      */
    void connectPort (uint32 port, void* data);

    /** Connect a channel to a data Location (realtime) 
        This simply converts the channel number to a port index then 
        calls Module::connectPort
      */
    void connectChannel (const PortType type, const int32 channel, void* data, const bool isInput);

    /** Connect an audio buffer setup for in place processing (realtime) */
    void referAudioReplacing (AudioSampleBuffer&);

    /** Returns a port buffer for port index (realtime) */    
    PortBuffer* getPortBuffer (uint32) const;

    //=========================================================================

    /** Loads the default state if available */
    void loadDefaultState();

    /** Returns an LV2 preset/state as a string */
    String getStateString() const;

    /** Restore from state created with getStateString()
        @see getStateString
     */
    void setStateString (const String&);

    //=========================================================================

    /** Write some data to a port
        This will send a PortEvent to the audio thread
     */
    void write (uint32 port, uint32 size, uint32 protocol, const void* buffer);

    /** Send port values to listeners now */
    void sendPortEvents();

    /** Returns a mapped LV2_URID */
    uint32 map (const String& uri) const;
    
private:
    LilvInstance* instance;
    const LilvPlugin* plugin;
    World&    world;
    mutable String bestUI;
    mutable String nativeUI;

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

    OwnedArray<SupportedUI> supportedUIs;
    OwnedArray<ScalePoints> scalePoints;

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

    /** Set this to handle touch notifications from the UI */
    std::function<void(uint32, bool)> onTouch;

    /** Called when the UI invokes LV2UI_Resize::ui_resize 
        Use requestSize to request the UI changes it size from the host
        return zero on success
    */
    std::function<int()> onClientResize;

    ~ModuleUI ()
    {
        unload();
    }

    bool loaded() const { return instance != nullptr; }

    void unload()
    {
        module.clearEditor();
        
        if (instance)
        {
            suil_instance_free (instance);
            instance = nullptr;
        }
    }

    World& getWorld()   const { return world; }
    Module& getPlugin() const { return module; }

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

    bool isNative() const { return containerType == JLV2__NativeUI; }

    bool hasContainerType (const String& type) const
    {
        return containerType == type;
    }

    bool isA (const String& widgetTypeURI) const
    {
        return widgetTypeURI == widgetType;
    }

    void instantiate()
    {
        if (loaded())
            return;
        
        Array<const LV2_Feature*> features;
        world.getFeatures (features);

        // parent
        if (parent.data != nullptr)
            features.add (&parent);
        
        // resize host side
        hostResizeData.handle = this;
        hostResizeData.ui_resize = ModuleUI::hostResize;
        resizeFeature.data = (void*) &hostResizeData;
        features.add (&resizeFeature);

        // instance access
        if (auto handle = module.getHandle())
        {
            instanceFeature.data = (void*) handle;
            features.add (&instanceFeature);
        }

        // data access
        dataFeatureData.data_access = ModuleUI::dataAccess;
        dataFeature.data = &dataFeatureData;

        // terminate the array
        features.add (nullptr);

        instance = suil_instance_new (
            world.getSuilHost(), this,
            containerType.toRawUTF8(),
            plugin.toRawUTF8(),
            ui.toRawUTF8(),
            widgetType.toRawUTF8(),
            bundlePath.toRawUTF8(),
            binaryPath.toRawUTF8(),
            features.getRawDataPointer()
        );

        // Nullify all UI extensions
        uiResize = nullptr;
        uiIdle = nullptr;
        uiShow = nullptr;

        if (nullptr == instance)
            return;

        // resize - plugin side
        if (const auto* resizeData = suil_instance_extension_data (instance, LV2_UI__resize))
            uiResize = (LV2UI_Resize*) resizeData;
        
        // Idle Interface
        if (const auto* idleData = suil_instance_extension_data (instance, LV2_UI__idleInterface))
            uiIdle = (LV2UI_Idle_Interface*) idleData;
        
        // Show Interface
        if (const auto* showData = suil_instance_extension_data (instance, LV2_UI__showInterface))
            uiShow = (LV2UI_Show_Interface*) showData;
    }

    bool haveShowInterface() const
    { 
        return instance != nullptr &&
            uiShow != nullptr && 
            uiShow->show != nullptr &&
            uiShow->hide != nullptr;
    }

    bool requiresShowInterface() const { return requireShow; }

    bool show()
    {
        if (! haveShowInterface())
            return false;
        return 0 == uiShow->show ((LV2UI_Handle) suil_instance_get_handle (instance));
    }

    bool hide()
    {
        if (! haveShowInterface())
            return false;
        return 0 == uiShow->hide ((LV2UI_Handle) suil_instance_get_handle (instance));
    }

    /** Returns true if the plugin provided LV2_UI__idleInterface */
    bool haveIdleInterface() const { return nullptr != uiIdle && nullptr != instance; }
    
    void idle()
    {
        if (! haveIdleInterface())
            return;
        uiIdle->idle ((LV2UI_Handle) suil_instance_get_handle (instance));
    }

    void setParent (intptr_t ptr) 
    {
        parent.data = (void*) ptr;
    }

    /** returs true if the UI provided LV2_UI__resize */
    bool haveClientResize() const
    {
        return uiResize != nullptr &&
            uiResize->handle != nullptr &&
            uiResize->ui_resize != nullptr;
    }

    /** Request the UI update it's size. This is the Host side of
        LV2_UI__resize
     */
    bool requestSize (int width, int height)
    {
        return (haveClientResize()) ? uiResize->ui_resize (uiResize->handle, width, height)
                                    : false;
    }

    /** Returns the width as reported by UI-side LV2_UI__resize */
    int getClientWidth() const { return clientWidth; }

    /** Returns the width as reported by UI-side LV2_UI__resize */
    int getClientHeight() const { return clientHeight; }

private:
    friend class Module;
    friend class World;

    LV2UI_Idle_Interface *uiIdle = nullptr;
    LV2UI_Show_Interface *uiShow = nullptr;
    LV2UI_Resize* uiResize = nullptr;

    LV2_Feature parent { LV2_UI__parent, nullptr };
    LV2_Feature resizeFeature { LV2_UI__resize, nullptr };
    LV2UI_Resize hostResizeData;
    LV2_Feature instanceFeature { LV2_INSTANCE_ACCESS_URI, nullptr };
    LV2_Feature dataFeature { LV2_DATA_ACCESS_URI, nullptr };
    LV2_Extension_Data_Feature dataFeatureData;

    int clientWidth = 0;
    int clientHeight = 0;

    ModuleUI (World& w, Module& m)
        : world (w), module (m) { }

    World& world;
    Module& module;

    SuilInstance* instance = nullptr;
    String containerType {};
    String plugin {};
    String ui {};
    String widgetType {};
    String bundlePath {};
    String binaryPath {};
    bool requireShow = false;

    static const void* dataAccess (const char* uri)
    {
        ignoreUnused (uri);
        return nullptr;
    }

    static int hostResize (LV2UI_Feature_Handle handle, int width, int height)
    {
        auto* ui = static_cast<ModuleUI*> (handle);
        ui->clientWidth = width;
        ui->clientHeight = height;
        return (ui->onClientResize) ? ui->onClientResize() : 0;
    }

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

    static uint32_t portSubscribe (void* controller, uint32_t port_index, 
                                   uint32_t protocol, const LV2_Feature *const *features)
    {
        return 0;
    }

    static uint32_t portUnsubscribe (void* controller, uint32_t port_index, 
                                     uint32_t protocol, const LV2_Feature *const *features)
    {
        return 0;
    }

    static void touch (void* controller, uint32_t port_index, bool grabbed)
    {
        auto* ui = (static_cast<ModuleUI*> (controller));
        if (ui && ui->onTouch)
            ui->onTouch (port_index, grabbed);
    }
};

}
