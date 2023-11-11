
// vim: ai sw=4 ts=4 expandtab

#ifdef _MSC_VER

#include "mediafoundation_player.h"
#include <vector>
#include <string>

#pragma comment(lib, "Mf")
#pragma comment(lib, "Mfplat")
#pragma comment(lib, "Mfuuid")
#pragma comment(lib, "Strmiids")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "Mfreadwrite")

#define DBG_REFCNT(x)
#define DBG_CAPTURE(x)
#define DBG_READER(x)

namespace {

com_ptr<IMFTopologyNode> create_src_node(com_ptr<IMFMediaSource> const& src,
    com_ptr<IMFPresentationDescriptor> const& pdesc,
    com_ptr<IMFStreamDescriptor> const& stream_desc)
{
    com_ptr<IMFTopologyNode> topo_node;
    throw_failure(MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE,
        topo_node.put()));
    throw_failure(topo_node->SetUnknown(MF_TOPONODE_SOURCE, src.get()));
    throw_failure(topo_node->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR,
        pdesc.get()));
    throw_failure(topo_node->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR,
        stream_desc.get()));
    return topo_node;
}

com_ptr<IMFActivate> create_sink_activate(
    com_ptr<IMFStreamDescriptor> const& stream_desc, HWND hwnd_video)
{
    com_ptr<IMFMediaTypeHandler> media_type_handler;
    throw_failure(stream_desc->GetMediaTypeHandler(media_type_handler.put()));
    GUID guid_major_type = { };
    throw_failure(media_type_handler->GetMajorType(&guid_major_type));
    com_ptr<IMFActivate> activate;
    if (guid_major_type == MFMediaType_Audio) {
        throw_failure(MFCreateAudioRendererActivate(activate.put()));
    } else if (guid_major_type == MFMediaType_Video) {
        throw_failure(MFCreateVideoRendererActivate(hwnd_video,
            activate.put()));
    } else {
        throw hresult_exception(E_FAIL);
    }
    return activate;
}

com_ptr<IMFTopologyNode> create_sink_node(com_ptr<IMFActivate> const& activate,
    DWORD stream_id)
{
    com_ptr<IMFTopologyNode> topo_node;
    throw_failure(MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE,
        topo_node.put()));
    throw_failure(topo_node->SetObject(activate.get()));
    throw_failure(topo_node->SetUINT32(MF_TOPONODE_STREAMID, stream_id));
    throw_failure(topo_node->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE,
        FALSE));
    return topo_node;
}

com_ptr<IMFTopology> create_topology(com_ptr<IMFMediaSource> const& src,
    com_ptr<IMFPresentationDescriptor> const& pdesc, HWND hwnd_video)
{
    com_ptr<IMFTopology> topo;
    throw_failure(MFCreateTopology(topo.put()));
    DWORD num_stream_desc = 0;
    throw_failure(pdesc->GetStreamDescriptorCount(&num_stream_desc));
    for (DWORD i = 0; i < num_stream_desc; ++i) {
        BOOL selected = FALSE;
        com_ptr<IMFStreamDescriptor> stream_desc;
        throw_failure(pdesc->GetStreamDescriptorByIndex(i, &selected,
            stream_desc.put()));
        if (selected) {
            com_ptr<IMFTopologyNode> src_node = create_src_node(src,
                pdesc, stream_desc);
            com_ptr<IMFActivate> sink_activate = create_sink_activate(
                stream_desc, hwnd_video);
            com_ptr<IMFTopologyNode> sink_node = create_sink_node(
                sink_activate, 0);
            throw_failure(topo->AddNode(src_node.get()));
            throw_failure(topo->AddNode(sink_node.get()));
            src_node->ConnectOutput(0, sink_node.get(), 0);
        }
    }
    return topo;
}

com_ptr<IMFMediaSource> create_media_source_from_url(PCWSTR url)
{
    com_ptr<IMFSourceResolver> pSourceResolver;
    throw_failure(MFCreateSourceResolver(pSourceResolver.put()));
    MF_OBJECT_TYPE obj_type = MF_OBJECT_INVALID;
    com_ptr<IUnknown> src_unk;
    throw_failure(pSourceResolver->CreateObjectFromURL(url,
        MF_RESOLUTION_MEDIASOURCE, nullptr, &obj_type, src_unk.put()));
    com_ptr<IMFMediaSource> src;
    throw_failure(src_unk->QueryInterface(IID_PPV_ARGS(src.put())));
    return src;
}

}; // anonymous namespace

long mediafoundation_player::s_instance_count = 0;

com_ptr<mediafoundation_player>
mediafoundation_player::create(HWND hwnd_video)
{
    com_ptr<mediafoundation_player> p(new mediafoundation_player(hwnd_video));
    return p;
}

mediafoundation_player::mediafoundation_player(HWND hwnd_video)
    : m_hwnd_video(hwnd_video)
{
    InterlockedIncrement(&s_instance_count);
    DBG_REFCNT(printf("CPlayer::Cplayer %p %lu\r\n", this, m_ref_count));
    throw_failure(MFStartup(MF_VERSION));
}

mediafoundation_player::~mediafoundation_player()
{
    DBG_REFCNT(printf("CPlayer::~Cplayer %p %lu\r\n", this, m_ref_count));
    assert(m_session.get() == nullptr);
    this->shutdown();
    MFShutdown();
    InterlockedDecrement(&s_instance_count);
}

HRESULT
mediafoundation_player::QueryInterface(REFIID riid, void **pp)
{
    static const QITAB qitab[] = {
        QITABENT(mediafoundation_player, IMFAsyncCallback),
        QITABENT(mediafoundation_player, IMFSourceReaderCallback),
        { 0 }
    };
    return QISearch(this, qitab, riid, pp);
}

ULONG
mediafoundation_player::AddRef()
{
    DBG_REFCNT(printf("CPlayer::AddRef %p %lu\r\n", this, m_ref_count));
    return InterlockedIncrement(&m_ref_count);
}

ULONG
mediafoundation_player::Release()
{
    DBG_REFCNT(printf("CPlayer::Release %p %lu\r\n", this, m_ref_count));
    ULONG const c = InterlockedDecrement(&m_ref_count);
    if (c == 0) {
        delete this;
    }
    return c;
}

void
mediafoundation_player::open_url(const WCHAR *sURL)
{
    std::unique_lock<std::mutex> g(m_mtx);

    this->create_session(g);
    m_source = create_media_source_from_url(sURL);
    com_ptr<IMFPresentationDescriptor> src_pdesc;
    throw_failure(m_source->CreatePresentationDescriptor(src_pdesc.put()));
    com_ptr<IMFTopology> topo = create_topology(m_source, src_pdesc,
        m_hwnd_video);
        // これはpartial topology？
    throw_failure(m_session->SetTopology(0, topo.get()));
        // 成功するとMESessionTopologySetが送られる
}

namespace {

std::string
to_std_string(PWSTR s, UINT32 slen)
{
    size_t const mbbuflen = (size_t)slen * 4;
    if (mbbuflen > INT_MAX) {
        return std::string();
    }
    std::vector<char> buf(mbbuflen);
    int const len = WideCharToMultiByte(CP_UTF8, 0, s, (int)slen, buf.data(),
    (int)mbbuflen, nullptr, nullptr);
    if (len <= 0) {
        return std::string();
    } else {
        return std::string(buf.data(), len);
    }
}

std::string
get_attr_string(com_ptr<IMFAttributes> const& attrs, REFGUID key)
{
    auto_cotaskmem<WCHAR> val;
    UINT32 len = 0;
    throw_failure(attrs->GetAllocatedString(key, val.put(), &len));
    return to_std_string(val.get(), len);
}

std::string
get_attr_friendly_name(com_ptr<IMFAttributes> const& attrs)
{
    return get_attr_string(attrs, MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME);
}

std::string
guid_to_std_string(GUID guid)
{
    OLECHAR buf[64] = { };
    int blen = StringFromGUID2(guid, &buf[0], 64);
    return to_std_string(&buf[0], blen > 0 ? blen - 1 : 0);
}

std::string
guid_fourcc_to_std_string(GUID guid)
{
    // guidがFourCC形式ならばASCII4文字で返す。ただしFourCC形式4文字に
    // コントロール文字を含むときは16進8文字で返す。FourCC形式でないとき
    // は完全なGUID文字列で返す。
    unsigned char pat[8] = { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 };
    if (guid.Data2 == 0x0000 && guid.Data3 == 0x0010 &&
        memcmp(guid.Data4, pat, 8) == 0) {
        char buf[4] = { };
        memcpy(buf, &guid.Data1, 4);
        bool has_ctrl = false;
        for (size_t i = 0; i < sizeof(buf); ++i) {
            has_ctrl |= (buf[i] >= 0 && buf[i] < 0x20);
        }
        if (has_ctrl) {
            std::string s;
            auto const to_hexchar = [](unsigned v) {
                return (v >= 10 && v < 16) ? ('a' + v - 10)
                    : (v < 10) ? ('0' + v)
                    : '0';
            };
            for (size_t i = 0; i < sizeof(buf); ++i) {
                s.push_back(to_hexchar(((unsigned)buf[i] >> 4) & 0x0f));
                s.push_back(to_hexchar((unsigned)buf[i] & 0x0f));
            }
            return s;
        } else {
            return std::string(buf, 4);
        }
    } else {
        return guid_to_std_string(guid);
    }
}

BOOL
device_get_media_types(std::string const& name,
    com_ptr<IMFMediaSource> const& src,
    mediafoundation_player::media_attribute const& filter,
    std::vector<mediafoundation_player::media_attribute>& attrs_append,
    BOOL set_default_if_found, com_ptr<IMFMediaType> *mediatype_r = nullptr)
{
    // srcで指定されたソースデバイスがfilterで指定された条件に合うフォー
    // マットをサポートするか調べ、条件に合うもののリストをattrs_append
    // に追加する。src_default_if_foundがTRUEなら見つかった最初のフォー
    // マットをSetCurrentMediaTypeでセットする。mediatypeがnullptrでない
    // なら見つかったフォーマットを返す。
    BOOL set_flag = FALSE;
    com_ptr<IMFPresentationDescriptor> pd;
    throw_failure(src->CreatePresentationDescriptor(pd.put()));
    DWORD dcnt = 0;
    throw_failure(pd->GetStreamDescriptorCount(&dcnt));
    DBG_CAPTURE(printf("stream descriptor count %u\r\n", (unsigned)dcnt));
    // dcntは大抵1のもよう
    for (DWORD k = 0; k < dcnt; ++k) {
        BOOL sel = FALSE;
        com_ptr<IMFStreamDescriptor> sd;
        throw_failure(pd->GetStreamDescriptorByIndex(k, &sel, sd.put()));
        com_ptr<IMFMediaTypeHandler> hnd;
        throw_failure(sd->GetMediaTypeHandler(hnd.put()));
        DWORD cnt = 0;
        throw_failure(hnd->GetMediaTypeCount(&cnt));
        BOOL found = FALSE;
        // サポートする各フォーマット(video/audio, RGB/YUV, 解像度など)に
        // ついてループ
        for (DWORD i = 0; i < cnt; ++i) {
            com_ptr<IMFMediaType> typ;
            throw_failure(hnd->GetMediaTypeByIndex(i, typ.put()));
            GUID majortype = { 0 };
            throw_failure(typ->GetMajorType(&majortype)); // video, audio
            auto majortype_str = guid_fourcc_to_std_string(majortype);
            bool m = TRUE; // このエントリがfilterに適合するかどうか
            m &= (filter.name.empty() || name == filter.name);
            m &= (majortype_str == filter.majortype);
            GUID subtype = { 0 };
            throw_failure(typ->GetGUID(MF_MT_SUBTYPE, &subtype));
            auto subtype_str = guid_fourcc_to_std_string(subtype);
            m &= (filter.subtype.empty() || subtype_str == filter.subtype);
            DBG_CAPTURE(printf("majortype %s %s subtype %s %s %d\r\n",
                majortype_str.c_str(), guid_to_std_string(majortype).c_str(),
                subtype_str.c_str(), guid_to_std_string(subtype).c_str(), m));
            mediafoundation_player::media_attribute a;
            a.name = name;
            a.majortype = majortype_str;
            a.subtype = subtype_str;
            UINT32 tcnt = 0;
            throw_failure(typ->GetCount(&tcnt));
            // 各属性についてループ
            for (UINT32 j = 0; j < tcnt; ++j) {
                GUID guid = { 0 };
                auto_propvariant var;
                throw_failure(typ->GetItemByIndex(j, &guid, var.put()));
                DBG_CAPTURE(printf("guid %s variant_vt=%d\r\n",
                    guid_fourcc_to_std_string(guid).c_str(),
                    (int)var.get().vt));
                if (guid == MF_MT_FRAME_SIZE) {
                    // 解像度
                    auto& v = var.get();
                    UINT32 uh = 0, ul = 0;
                    Unpack2UINT32AsUINT64(v.uhVal.QuadPart, &uh, &ul);
                    m &= (filter.width == 0 || filter.width == uh);
                    m &= (filter.height == 0 || filter.height == ul);
                    DBG_CAPTURE(printf("frame size %ux%u %d\r\n", uh, ul, m));
                    a.width = uh;
                    a.height = ul;
                } else if (guid == MF_MT_FRAME_RATE) {
                    // フレームレート
                    auto& v = var.get();
                    UINT32 uh = 0, ul = 0;
                    Unpack2UINT32AsUINT64(v.uhVal.QuadPart, &uh, &ul);
                    m &= (filter.fps_denominator == 0 ||
                        (filter.fps_numerator == uh &&
                            filter.fps_denominator == ul));
                        // fps_denominatorが0のときは、全てのフレームレートに
                        // マッチする
                    DBG_CAPTURE(printf("frame rate %u/%u %d\r\n", uh, ul, m));
                    a.fps_numerator = uh;
                    a.fps_denominator = ul;
                } else if (guid == MF_MT_AM_FORMAT_TYPE &&
                    var.get().vt == VT_CLSID) {
                    CLSID *puuid = var.get().puuid;
                    DBG_CAPTURE(printf("format type %s\r\n",
                        guid_fourcc_to_std_string(*puuid).c_str()));
                }
            }
            if (m) {
                DBG_CAPTURE(printf("found.\r\n"));
                if (set_default_if_found && !set_flag) {
                    throw_failure(hnd->SetCurrentMediaType(typ.get()));
                    DBG_CAPTURE(printf("set default.\r\n"));
                    set_flag = TRUE;
                }
                if (mediatype_r != nullptr) {
                    *mediatype_r = typ;
                }
                attrs_append.push_back(std::move(a));
            }
            DBG_CAPTURE(printf("(end)\r\n"));
        }
    }
    return set_flag;
}

BOOL
enumerate_source_devices(
    mediafoundation_player::media_attribute const& filter_attr,
    std::vector<mediafoundation_player::media_attribute>& attrs_append,
    com_ptr<IMFMediaSource> *src_r = nullptr,
    com_ptr<IMFMediaType> *mediatype_r = nullptr)
{
    // 利用可能なソースデバイスを列挙する。src_rがnullptrでなければ
    // 条件filter_attrに合うIMFMediaSourceをsrc_rにセットする。
    BOOL set_default_src_if = (src_r != nullptr);
    com_ptr<IMFAttributes> conf;
    throw_failure(MFCreateAttributes(conf.put(), 0));
    throw_failure(conf->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        filter_attr.majortype == "auds"
        ? MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID
        : MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID));
    auto_devices devices;
    throw_failure(MFEnumDeviceSources(conf.get(),
        devices.get_devices_address(), devices.get_count_address()));
    for (UINT32 i = 0; i < devices.get_count(); ++i) {
        try {
            auto const& device = devices[i];
            com_ptr<IMFMediaSource> src;
            throw_failure(device->ActivateObject(
                IID_PPV_ARGS(src.put())));
            auto name = get_attr_friendly_name(com_ptr<IMFAttributes>(device));
            DBG_CAPTURE(printf("device name: %s\r\n", name.c_str()));
            BOOL set_flag = device_get_media_types(name, src, filter_attr,
                attrs_append, set_default_src_if, mediatype_r);
            if (set_default_src_if && set_flag) {
                *src_r = src;
                set_default_src_if = FALSE;
                DBG_CAPTURE(printf("set_default_src\r\n"));
            } else {
                throw_failure(device->ShutdownObject());
                throw_failure(device->DetachObject());
            }
        } catch (hresult_exception const&) {
        }
    }
    return src_r != nullptr && !set_default_src_if;
}

// FIXME
void
source_reader_set_format_video_noconv(com_ptr<IMFSourceReader> const& reader)
{
    // IMFSourceReaderのフォーマットとしてnativeの形式を指定する。
    // 必要？
    HRESULT hr = S_OK;
    for (DWORD i = 0; ; ++i) {
        com_ptr<IMFMediaType> mt;
        hr = reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            i, mt.put());
        if (FAILED(hr)) {
            break;
        }
        GUID subtype = { 0 };
        hr = mt->GetGUID(MF_MT_SUBTYPE, &subtype);
        if (FAILED(hr)) {
            continue;
        }
        hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 
            nullptr, mt.get());
        if (SUCCEEDED(hr)) {
            break;
        }
    }
    throw_failure(hr);
}

com_ptr<IMFMediaType>
source_reader_set_format_pcm(com_ptr<IMFSourceReader> const& reader)
{
    // IMFSourceReaderのフォーマットとしてPCM形式を指定する。
    throw_failure(reader->SetStreamSelection(
        (DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE));
    throw_failure(reader->SetStreamSelection(
        (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE));
    com_ptr<IMFMediaType> mt;
    throw_failure(MFCreateMediaType(mt.put()));
    throw_failure(mt->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio));
    throw_failure(mt->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM));
    throw_failure(reader->SetCurrentMediaType(
        (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, mt.get()));
    com_ptr<IMFMediaType> mt_got;
    throw_failure(reader->GetCurrentMediaType(
        (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, mt_got.put()));
    throw_failure(reader->SetStreamSelection(
        (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE));
    return mt_got;
}

}; // anonymous namespace

std::vector<mediafoundation_player::media_attribute>
mediafoundation_player::enumerate_capture_devices(
    media_attribute const& filter_attr)
{
    std::vector<media_attribute> attrs;
    enumerate_source_devices(filter_attr, attrs, nullptr, nullptr);
    return attrs;
}

mediafoundation_player::media_attribute
mediafoundation_player::open_capture_device(media_attribute const& attr_device,
    BOOL use_reader)
{
    std::unique_lock<std::mutex> g(m_mtx);

    m_reader.reset();
    m_media_type.reset();
    m_samples.clear();
    m_reader_eof = FALSE;
    m_audio_sps = 0;
    m_audio_channels = 0;

    this->create_session(g);

    // attr_deviceで指定されたデバイスからデータを取得するmedia sourceを作る。
    auto attr = attr_device;
    if (attr.majortype.empty()) {
        attr.majortype = "vids"; // 既定ではビデオ
    }
    std::vector<media_attribute> attrs;
    com_ptr<IMFMediaSource> src;
    com_ptr<IMFMediaType> mtyp;
    BOOL set_flag = enumerate_source_devices(attr, attrs, &src, &mtyp);
    #if 0
    if (set_flag) {
        UINT32 numerator = 0;
        UINT32 denominator = 0;
        if (SUCCEEDED(MFGetAttributeRatio(mtyp.get(), MF_MT_FRAME_RATE,
            &numerator, &denominator))) {
            printf("fps %u/%u\r\n", numerator, denominator);
        } else {
            printf("fps unknown\r\n");
        }
    }
    #endif
    media_attribute rattr;
    if (set_flag) {
        m_source = src;
        if (!attrs.empty()) {
            rattr = attrs.front();
        }
    } else {
        throw_failure(E_FAIL);
    }
    m_is_audio = (attr.majortype != "vids");

    if (!use_reader) {
        // media sessionを使って再生するプレイヤーを作る。
        com_ptr<IMFPresentationDescriptor> src_pdesc;
        throw_failure(m_source->CreatePresentationDescriptor(src_pdesc.put()));
        com_ptr<IMFTopology> topo = create_topology(m_source, src_pdesc,
            m_hwnd_video);
            // これはpartial topology？
        throw_failure(m_session->SetTopology(0, topo.get()));
            // 成功するとMESessionTopologySetが送られる
    } else {
        // サンプルを取得するsource readerを作る。media sessionは使わない。
        com_ptr<IMFAttributes> a;
        throw_failure(MFCreateAttributes(a.put(), 1));
        throw_failure(a->SetUINT32(MF_LOW_LATENCY, 1)); //効いてるのか？
        if (!m_is_audio) {
            throw_failure(a->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, TRUE));
                // 必要か？
        }
        IMFSourceReaderCallback *cb = this;
        throw_failure(a->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, cb));
        com_ptr<IMFSourceReader> reader;
        throw_failure(MFCreateSourceReaderFromMediaSource(m_source.get(),
            a.get(), reader.put()));
        m_reader = reader;
        if (!m_is_audio) {
            // 非同期モードでsampleを読み取る。OnReadSampleがコールバック
            // される。
            // source_reader_set_format_video_noconv(m_reader);
            DBG_READER(printf("video sample reader\r\n"));
            DBG_READER(fflush(stdout));
            throw_failure(m_reader->ReadSample(
                MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, nullptr,
                nullptr, nullptr));
        } else {
            m_media_type = source_reader_set_format_pcm(m_reader);
            throw_failure(m_media_type->GetUINT32(
                MF_MT_AUDIO_SAMPLES_PER_SECOND, &m_audio_sps));
            throw_failure(m_media_type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS,
                &m_audio_channels));
            // 非同期モードでsampleを読み取る。OnReadSampleがコールバック
            // される。
            throw_failure(m_reader->ReadSample(
                MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, nullptr,
                nullptr, nullptr));
        }
    }
    return rattr;
}

com_ptr<IMFSample>
mediafoundation_player::read_sample(BOOL *eof_r)
{
    std::lock_guard<std::mutex> g(m_mtx);

    // 非同期モードで読み取ったサンプルを返す。この関数はブロックしない。
    if (eof_r) {
        *eof_r = FALSE;
    }
    com_ptr<IMFSample> sample;
    if (m_samples.empty() && m_reader_eof) {
        if (eof_r) {
            *eof_r = TRUE;
        }
        return sample;
    }
    if (m_samples.empty()) {
        return sample;
    }
    sample = m_samples.front();
    m_samples.pop_front();
    return sample;
}

HRESULT
mediafoundation_player::OnEvent(DWORD stream_idx, IMFMediaEvent *ev)
{
    return S_OK;
}

HRESULT
mediafoundation_player::OnFlush(DWORD stream_idx)
{
    return S_OK;
}

HRESULT
mediafoundation_player::OnReadSample(HRESULT hr, DWORD stream_idx,
    DWORD stream_flags, LONGLONG timestamp, IMFSample *sample)
{
    std::lock_guard<std::mutex> g(m_mtx);

    DBG_READER(printf("OnReadSample %p %d %u\r\n", sample, (int)m_is_audio,
        m_audio_channels));
    DBG_READER(fflush(stdout));
    // throw_failure(hr);
    if (sample != nullptr) {
        com_ptr<IMFSample> s(sample);
        if (m_samples.size() >= (size_t)m_samples_size_limit) {
            m_samples.pop_front();
        }
        m_samples.push_back(s);
    }
    if ((stream_flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) {
        m_reader_eof = TRUE;
    } else {
        hr = m_reader->ReadSample(m_is_audio
                ? MF_SOURCE_READER_FIRST_AUDIO_STREAM
                : MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0, nullptr, nullptr, nullptr, nullptr);
        // throw_failure(hr);
    }
    return S_OK;
}

void
mediafoundation_player::pause()
{
    std::lock_guard<std::mutex> g(m_mtx);

    if (m_session == nullptr || m_source == nullptr) {
        return;
    }
    throw_failure(m_session->Pause());
    m_session_playing = FALSE;
}

void
mediafoundation_player::stop()
{
    std::lock_guard<std::mutex> g(m_mtx);

    if (m_session == nullptr) {
        return;
    }
    throw_failure(m_session->Stop());
    m_session_playing = FALSE;
}

BOOL
mediafoundation_player::has_video() const
{
    std::lock_guard<std::mutex> g(m_mtx);

    return (m_video_display_control != nullptr);
}

BOOL
mediafoundation_player::is_playing() const
{
    std::lock_guard<std::mutex> g(m_mtx);

    return m_session_playing;
}

void
mediafoundation_player::repaint_video()
{
    std::lock_guard<std::mutex> g(m_mtx);

    if (m_video_display_control != nullptr) {
        throw_failure(m_video_display_control->RepaintVideo());
    }
}

void
mediafoundation_player::set_video_position(
    MFVideoNormalizedRect *src_rect, RECT *dest_rect)
{
    std::lock_guard<std::mutex> g(m_mtx);

    if (m_video_display_control != nullptr) {
        throw_failure(m_video_display_control->SetVideoPosition(src_rect,
            dest_rect));
    }
}

void
mediafoundation_player::resize_video_window(WORD width, WORD height)
{
    std::lock_guard<std::mutex> g(m_mtx);

    if (m_video_display_control != nullptr) {
        RECT rc = { 0, 0, width, height };
        throw_failure(m_video_display_control->SetVideoPosition(nullptr, &rc));
    }
}

SIZE
mediafoundation_player::get_native_video_size()
{
    std::lock_guard<std::mutex> g(m_mtx);

    SIZE r = { };
    if (m_video_display_control != nullptr) {
        throw_failure(m_video_display_control->GetNativeVideoSize(&r,
            nullptr));
    }
    return r;
}

HRESULT
mediafoundation_player::GetParameters(DWORD *, DWORD *)
{
    return E_NOTIMPL;
}

HRESULT
mediafoundation_player::Invoke(IMFAsyncResult* aresult)
{
    // m_sessionに非同期イベントが来たときにワーカスレッド上で
    // コールバックされる。
    std::lock_guard<std::mutex> g(m_mtx);

    com_ptr<IMFMediaEvent> ev;
    return_failure(m_session->EndGetEvent(aresult, ev.put()));
    MediaEventType meType = MEUnknown;
    return_failure(ev->GetType(&meType));

    if (meType == MESessionClosed) {
        // m_pSession->Close()が呼ばれた。Closeが完了したのでClose呼び出し元
        // へm_condで知らせる。
        m_session_closing = FALSE;
        m_cond.notify_one();
        return S_OK;
    }

    HRESULT hr_status = S_OK;
    return_failure(ev->GetStatus(&hr_status));
    return_failure(hr_status);

    switch (meType) {
    case MENewPresentation:
        return_failure(ev_new_presentation(ev.get()));
        break;
    case MESessionTopologyStatus:
        return_failure(ev_topology_status(ev.get()));
        break;
    case MEEndOfPresentation:
        return_failure(ev_end_of_presentation(ev.get()));
        break;
    default:
        break;
    }
    // 次のイベントを待つ
    return_failure(m_session->BeginGetEvent(this, NULL));
    return S_OK;
}

void
mediafoundation_player::shutdown()
{
    std::unique_lock<std::mutex> g(m_mtx);

    close_session(g);

    m_samples.clear();
    m_media_type.reset();
    m_reader.reset();
    m_video_display_control.reset();
    m_source.reset();
    m_session.reset();
}

HRESULT
mediafoundation_player::ev_topology_status(IMFMediaEvent *ev)
{
    try {
        // どういうときに起きるのか？
        UINT32 status = 0;
        throw_failure(ev->GetUINT32(MF_EVENT_TOPOLOGY_STATUS, &status));
        if (status == MF_TOPOSTATUS_READY) {
            (void)MFGetService(m_session.get(), MR_VIDEO_RENDER_SERVICE,
                IID_PPV_ARGS(m_video_display_control.put()));
                // videoを含んでいないときはエラーになるが無視してよい
          start_session();
        }
        return S_OK;
    } catch (hresult_exception const &e) {
        return e.get();
    }
}

HRESULT
mediafoundation_player::ev_end_of_presentation(IMFMediaEvent *ev)
{
    m_session_playing = FALSE;
    return S_OK;
}

HRESULT
mediafoundation_player::ev_new_presentation(IMFMediaEvent *ev)
{
    try {
        // どういうときに起きるのか？
        auto_propvariant var;
        throw_failure(ev->GetValue(var.put()));
        com_ptr<IMFPresentationDescriptor> pdesc;
        if (var.get().vt == VT_UNKNOWN) {
            throw_failure(var.get().punkVal->QueryInterface(pdesc.put()));
        } else {
            throw hresult_exception(MF_E_INVALIDTYPE);
        }
        com_ptr<IMFTopology> topo = create_topology(m_source, pdesc,
            m_hwnd_video);
        throw_failure(m_session->SetTopology(0, topo.get()));
        return S_OK;
    } catch (hresult_exception const &e) {
        return e.get();
    }
}

void
mediafoundation_player::create_session(std::unique_lock<std::mutex>& g)
{
    close_session(g);
    assert(m_session == nullptr);

    com_ptr<IMFAttributes> conf;
    throw_failure(MFCreateAttributes(conf.put(), 0));
    throw_failure(conf->SetUINT32(MF_LOW_LATENCY, 1)); //効いてるのか？
    // throw_failure(conf->SetGUID(MF_MT_AM_FORMAT_TYPE, MFVideoFormat_YUY2));
    throw_failure(MFCreateMediaSession(conf.get(), m_session.put()));

    throw_failure(m_session->BeginGetEvent((IMFAsyncCallback*)this, NULL));
        // BeginGetEventはthisをAddRefし、this->Invokeがコールバックされた
        // あとにReleaseする？
        // またReleaseされるまでは循環参照になっているので注意。
}

void
mediafoundation_player::close_session(std::unique_lock<std::mutex>& g)
{
    m_video_display_control.reset();
    #if 0
    if (m_session) {
        // m_sessionをCloseし、非同期にとじられるまで待つ。これを
        // 本当に実行する必要があるのかはよくわからない(しなくてもリークは
        // 起きることはないように見える)が、サンプルがそのようにしているので
        // 真似しておく。
        HRESULT hr = m_session->Close();
            // MESessionStoppedイベントがthis(IMFAsyncCallback)へ送られる。
        if (SUCCEEDED(hr)) {
            // MESessionStoppedが届いたらm_condで通知される。
            m_session_closing = TRUE;
            while (m_session_closing) {
                m_cond.wait(g);
            }
        }
    }
    #endif
    if (m_source) {
        (void)m_source->Shutdown();
    }
    if (m_session) {
        (void)m_session->Shutdown();
    }
    m_source.reset();
    m_session.reset();
}

void
mediafoundation_player::start_session()
{
    if (m_session == nullptr) {
        return;
    }
    auto_propvariant var;
    throw_failure(m_session->Start(&GUID_NULL, var.put()));
    m_session_playing = TRUE;
}

void
mediafoundation_player::play()
{
    std::lock_guard<std::mutex> g(m_mtx);

    if (m_session == nullptr || m_source == nullptr) {
        return;
    }
    start_session();
}

#endif

