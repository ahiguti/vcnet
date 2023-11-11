
// vim: ai sw=4 ts=4 expandtab

#ifndef MEDIAFOUNDATION_PLAYER_H
#define MEDIAFOUNDATION_PLAYER_H

#ifdef _MSC_VER

#include <windows.h>
#include <new>
#include <shobjidl.h> 
#include <shlwapi.h>
#include <assert.h>
#include <strsafe.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <evr.h>
#include <mfreadwrite.h>
#include <stdexcept>
#include <string>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <deque>

template <typename T> struct com_ptr {
    explicit com_ptr(T *ptr0 = nullptr) : ptr(ptr0) {
        addref_if();
    }
    com_ptr(com_ptr const& x) : ptr(x.ptr) {
        addref_if();
    }
    com_ptr(com_ptr&& x) noexcept : ptr(x.ptr) {
        x.ptr = nullptr;
    }
    com_ptr& operator =(com_ptr const& x) {
        x.addref_if();
        release_if();
        ptr = x.ptr;
        return *this;
    }
    com_ptr& operator =(com_ptr&& x) noexcept {
        release_if();
        ptr = x.ptr;
        x.ptr = nullptr;
        return *this;
    }
    ~com_ptr() {
        release_if();
    }
    explicit operator bool() const {
        return ptr != nullptr;
    }
    bool operator ==(com_ptr const& x) const {
        return ptr == x.ptr;
    }
    bool operator !=(com_ptr const& x) const {
        return ptr != x.ptr;
    }
    bool operator ==(T *x) const {
        return ptr == x;
    }
    bool operator !=(T *x) const {
        return ptr != x;
    }
    T *get() const {
        return ptr;
    }
    T *operator ->() const {
        return ptr;
    }
    void reset() {
        release_if();
        ptr = nullptr;
    }
    T **put() {
        reset();
        return &ptr;
    }
private:
    void addref_if() const {
        if (ptr != nullptr) { ptr->AddRef(); }
    }
    void release_if() const {
        if (ptr != nullptr) { ptr->Release(); }
    }
private:
    T *ptr = nullptr;
};

struct auto_handle {
    auto_handle(HANDLE h0 = INVALID_HANDLE_VALUE) : h(h0) { }
    auto_handle(auto_handle const&) = delete;
    auto_handle& operator =(auto_handle const&) = delete;
    ~auto_handle() {
      reset();
    }
    HANDLE get() const { return h; }
    void reset(HANDLE h0 = INVALID_HANDLE_VALUE) {
      if (h != INVALID_HANDLE_VALUE && h != NULL) {
        CloseHandle(h);
      }
      h = h0;
    }
private:
    HANDLE h = INVALID_HANDLE_VALUE;
};

template <typename T> struct auto_cotaskmem {
    auto_cotaskmem(T *ptr0 = nullptr) : ptr(ptr0) { }
    auto_cotaskmem(auto_cotaskmem const&) = delete;
    auto_cotaskmem& operator =(auto_cotaskmem const&) = delete;
    ~auto_cotaskmem() {
        reset();
    }
    T *get() const { return ptr; }
    void reset() {
        if (ptr) {
            CoTaskMemFree(ptr);
            ptr = nullptr;
        }
    }
    T **put() {
        reset();
        return &ptr;
    }
private:
    T *ptr = nullptr;
};

struct auto_propvariant {
    auto_propvariant() {
        PropVariantInit(&val);
    }
    auto_propvariant(auto_propvariant const&) = delete;
    auto_propvariant& operator =(auto_propvariant const&) = delete;
    ~auto_propvariant() {
        reset();
    }
    void reset() {
        PropVariantClear(&val);
    }
    PROPVARIANT const& get() const {
        return val;
    }
    PROPVARIANT& get() {
        return val;
    }
    PROPVARIANT *put() {
        reset();
        return &val;
    }
private:
    PROPVARIANT val = { };
};

struct auto_devices {
    auto_devices() : devices(nullptr), count(0) { }
    auto_devices(auto_devices const&) = delete;
    auto_devices& operator =(auto_devices const&) = delete;
    auto_devices(auto_devices&& x) noexcept
        : devices(x.devices), count(x.count) {
        x.devices = nullptr;
        x.count = 0;
    }
    ~auto_devices() {
        for (UINT32 i = 0; i < count; ++i) {
            devices[i]->Release();
        }
        CoTaskMemFree(devices);
    }
    IMFActivate ***get_devices_address() { return &devices; }
    UINT32* get_count_address() { return &count; }
    IMFActivate* operator [](UINT32 i) const { return devices[i]; }
    UINT32 get_count() const { return count; }
private:
    IMFActivate **devices = nullptr;
    UINT32 count = 0;
};

struct hresult_exception : std::exception {
    hresult_exception(HRESULT hr0, const char *msg0 = "")
        : hr(hr0),
        msg("HRESULT(" + std::to_string(hr) + ") " + std::string(msg0))
    {
    }
    const char *what() const { 
      return msg.c_str();
    }
    HRESULT get() const { return hr; }
private:
    HRESULT const hr = { };
    std::string const msg;
};

inline void throw_failure(HRESULT hr, const char *msg = "") {
    if (FAILED(hr)) {
        throw hresult_exception(hr, msg);
    }
}

#define return_failure(x) { HRESULT hr = (x); if (FAILED(hr)) { return hr; } }

struct mediafoundation_player : IMFAsyncCallback, IMFSourceReaderCallback {
public:
    struct media_attribute {
        std::string name;
        std::string majortype;   // "vids", "auds"
        std::string subtype;     // "YUY2", "NV12"
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t fps_denominator = 0;
        uint32_t fps_numerator = 1;
    };
    static com_ptr<mediafoundation_player> create(HWND hwnd_video);
    static std::vector<media_attribute> enumerate_capture_devices(
        media_attribute const& filter_attr = { });
    media_attribute open_capture_device(media_attribute const& attr_device,
        BOOL use_reader = FALSE);
    void open_url(const WCHAR *sURL);
    void play();
    void pause();
    void stop();
    void shutdown();
    void repaint_video();
    void set_video_position(MFVideoNormalizedRect *src_rect, RECT *dest_rect);
    void resize_video_window(WORD width, WORD height);
    SIZE get_native_video_size();
    BOOL has_video() const;
    BOOL is_playing() const;
    com_ptr<IMFSample> read_sample(BOOL *eof_r = nullptr);
    UINT32 get_audio_sps() const { return m_audio_sps; }
    UINT32 get_audio_channels() const { return m_audio_channels; }
    static long instance_count() { return s_instance_count; }
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void **);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    // IMFAsyncCallback
    STDMETHODIMP GetParameters(DWORD *, DWORD *);
    STDMETHODIMP Invoke(IMFAsyncResult *);
    // IMFSourceReaderCallback
    STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *);
    STDMETHODIMP OnFlush(DWORD);
    STDMETHODIMP OnReadSample(HRESULT, DWORD, DWORD, LONGLONG, IMFSample *);
private:
    mediafoundation_player(HWND hwnd_video);
    mediafoundation_player(mediafoundation_player const&) = delete;
    mediafoundation_player& operator =(mediafoundation_player const&) = delete;
    virtual ~mediafoundation_player();
    void create_session(std::unique_lock<std::mutex>& g);
    void close_session(std::unique_lock<std::mutex>& g);
    void start_session();
    HRESULT ev_topology_status(IMFMediaEvent *pEvent);
    HRESULT ev_end_of_presentation(IMFMediaEvent *pEvent);
    HRESULT ev_new_presentation(IMFMediaEvent *pEvent);
private:
    static long s_instance_count;
    long m_ref_count = 0;
    mutable std::mutex m_mtx;
    std::condition_variable m_cond;
    com_ptr<IMFMediaSession> m_session;
    com_ptr<IMFMediaSource> m_source;
    com_ptr<IMFVideoDisplayControl> m_video_display_control;
    HWND m_hwnd_video = { };
    BOOL m_session_playing = FALSE;
    BOOL m_session_closing = FALSE;
    com_ptr<IMFSourceReader> m_reader;
    com_ptr<IMFMediaType> m_media_type;
    std::deque<com_ptr<IMFSample>> m_samples;
    UINT32 m_samples_size_limit = 1;
    BOOL m_is_audio = FALSE;
    UINT32 m_audio_sps = 0;
    UINT32 m_audio_channels = 0;
    BOOL m_reader_eof = FALSE;
};

#endif // _MSC_VER

#endif

