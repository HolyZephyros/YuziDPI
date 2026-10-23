#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <winhttp.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>

#ifndef INTERNET_OPTION_REFRESH
#define INTERNET_OPTION_REFRESH 37
#endif
#ifndef INTERNET_OPTION_SETTINGS_CHANGED
#define INTERNET_OPTION_SETTINGS_CHANGED 39
#endif
extern "C" BOOL WINAPI InternetSetOptionW(LPVOID hInternet, DWORD dwOption, LPVOID lpBuffer, DWORD dwBufferLength);

// 6 Kademeli Optimize Manuel DPI Bypass Merdiveni (En hafif 0ms'den en güçlüye)
enum BypassStage : int {
    STAGE_0_DIRECT            = 0, // Doğrudan geçiş (%0 müdahale - Oyunlar ve normal siteler)
    STAGE_1_FAST_TLS_REC      = 1, // Kademe 1 (En Hafif - 0ms): Tek Paket TLS Record SNI Bölme (TTNET / TurkNet / Vodafone)
    STAGE_2_DUAL_PKT_0MS      = 2, // Kademe 2 (Çok Hafif - 0ms): 1-Byte Başlık + Çift Paket Mikro Zarf (0ms Gecikme)
    STAGE_3_MICRO_1MS         = 3, // Kademe 3 (Hafif+ - 1ms): 3'lü Mikro TLS Zarfı + Hızlı TCP Bölme
    STAGE_4_HYBRID_SUPER      = 4, // Kademe 4 (Dengeli Güçlü): Hibrit Mikro TLS Zarfı + Ayrı Segment (Superonline / Kablonet)
    STAGE_5_ANYCAST_ULTRA     = 5, // Kademe 5 (Maksimum Güç): Anycast Temiz IP + Tam Mikro TLS Desync
    STAGE_6_UPSTREAM_TUNNEL   = 6  // Kademe 6: Harici SOCKS5 Tünel Köprüsü (127.0.0.1:40000 vb.)
};

enum TunnelItemType : int {
    ITEM_APP_EXE = 0,
    ITEM_FOLDER  = 1,
    ITEM_DOMAIN  = 2
};

// 3 Kademeli Çalışma Kapsamı
enum TunnelScopeMode : int {
    SCOPE_ONLY_LISTED         = 0, // 1. Sadece Listedeki Uygulama, Klasör ve Siteler
    SCOPE_ALL_EXCEPT_EXCLUDED = 1, // 2. Belirtilenler Dışında Tüm Sistem (Hariç Tutma Modu)
    SCOPE_FULL_SYSTEM         = 2  // 3. Tüm Sistem (Her Şeyi Tünelle)
};

struct TunnelTargetItem {
    TunnelItemType type;
    std::wstring pathOrDomain; // .exe tam yolu, klasör yolu veya domain adı
    std::wstring displayName;
    bool enabled{true};
    bool isExcluded{false};    // false = Tünelle (Bypass), true = Hariç Tut (Doğrudan Geçiş)
};

typedef void (*EngineLogCallback)(const wchar_t* msg);
typedef void (*EngineStateCallback)(BypassStage activeStage, bool running, int latencyMs);

class ZeroDpiEngine {
public:
    ZeroDpiEngine();
    ~ZeroDpiEngine();

    bool Start(uint16_t httpPort = 3131, uint16_t socksPort = 1080);
    void Stop();
    bool IsRunning() const { return m_running.load(); }

    // Kademe 1'den başlayıp çalışan en düşük kademeyi bulur ve kilitler
    BypassStage RunAutoDiscovery(int& outLatencyMs);

    // Sistem geneli PAC yönlendirmesi (Tarayıcılar için)
    bool EnableSystemSmartPac(bool enable);
    void SetUseSystemPac(bool use) { m_useSystemPac.store(use); }
    bool GetUseSystemPac() const { return m_useSystemPac.load(); }
    void SetAutoLaunchApps(bool launch) { m_autoLaunchApps.store(launch); }
    bool GetAutoLaunchApps() const { return m_autoLaunchApps.load(); }

    // Kapsam Modu (0: Sadece Listedekiler, 1: Belirtilenler Dışı Tüm Sistem, 2: Tüm Sistem)
    void SetScopeMode(TunnelScopeMode mode);
    TunnelScopeMode GetScopeMode() const { return (TunnelScopeMode)m_scopeMode.load(); }

    // Kademe seçimi (1..6 = Manuel Kademe, varsayılan 1) — Çalışırken anında geçiş yapar
    void SetSelectedMode(int mode);
    void ApplyLiveSwitch(bool stageChanged = false);
    int GetSelectedMode() const { return m_selectedMode.load(); }
    BypassStage GetActiveStage() const { return (BypassStage)m_activeStage.load(); }
    int GetLastDiscoveryLatency() const { return m_lastLatencyMs.load(); }

    // Uygulama / Klasör / Domain Tünel Listesi Yönetimi
    void AddTunnelItem(TunnelItemType type, const std::wstring& pathOrDomain, bool isExcluded = false);
    void ToggleTunnelItemRule(int index);
    void RemoveTunnelItem(int index);
    std::vector<TunnelTargetItem> GetTunnelItems() const;

    // Listedeki (veya seçili) uygulama/klasörü yerel tünel üzerinden başlatır
    bool LaunchTargetItemTunneled(int index);
    int LaunchAllAppTargetsTunneled();

    // Ayarları zerodpi.ini dosyasına kaydet / yükle
    void LoadConfig(const std::wstring& iniPath, bool& outAutoConnect, bool& outAutoLaunchApps, bool& outStartMinimized);
    void SaveConfig(const std::wstring& iniPath, bool autoConnect, bool autoLaunchApps, bool startMinimized);

    // Windows başlangıcında çalıştırma (HKCU\...\Run)
    static bool IsWindowsAutostartEnabled();
    static bool SetWindowsAutostart(bool enable, bool startMinimized = false);

    void SetCallbacks(EngineLogCallback logCb, EngineStateCallback stateCb) {
        m_logCb = logCb;
        m_stateCb = stateCb;
    }

    static const wchar_t* GetStageShortName(BypassStage stage);

    // TLS ClientHello içinden SNI (server_name) ofsetini bulur
    static bool FindTlsSniOffset(const uint8_t* data, int len, int& sniStart, int& sniLen, std::string& outHost);

    // Tek bir TLS ClientHello paketini birden fazla RFC-uyumlu TLS Record zarfına (0x16 0x03 0x01) böler
    static std::vector<uint8_t> SplitIntoTlsRecords(const uint8_t* recordData, int recordLen, const std::vector<int>& hsSplitOffsets);

    std::string ResolveDoH(const std::string& host, BypassStage stage = STAGE_1_FAST_TLS_REC);
    bool SendInitialPayloadWithStage(SOCKET remoteSock, const uint8_t* data, int len, BypassStage stage);
    bool ProbeDomainAtStage(const std::string& host, uint16_t port, BypassStage stage, int& outLatencyMs);

private:
    static DWORD WINAPI HttpAcceptThreadProc(LPVOID param);
    static DWORD WINAPI SocksAcceptThreadProc(LPVOID param);
    static DWORD WINAPI ClientWorkerThreadProc(LPVOID param);
    static DWORD WINAPI StartupWorkerThreadProc(LPVOID param);

    void HandleHttpClient(SOCKET clientSock);
    void HandleSocks5Client(SOCKET clientSock);
    void ServePacFile(SOCKET clientSock);
    int RelaySockets(SOCKET s1, SOCKET s2);

    SOCKET ConnectToTarget(const std::string& host, uint16_t port, BypassStage stage);
    SOCKET ConnectViaUpstreamSocks5(const std::string& targetHost, uint16_t targetPort);

    bool IsGameOrExcludedHost(const std::string& host) const;
    bool IsUserExcludedHost(const std::string& host) const;
    bool IsTargetInBypassList(const std::string& host) const;
    bool ShouldBypassConnection(const std::string& host, const std::wstring& clientFullPath, const std::wstring& clientExeName, std::wstring& outReason) const;
    bool LaunchSingleExeTunneled(const std::wstring& exePath);

    void RegisterRelaySocket(SOCKET s);
    void UnregisterRelaySocket(SOCKET s);
    void Log(const wchar_t* fmt, ...);

private:
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_discovering{false};
    std::atomic<bool> m_useSystemPac{true};
    std::atomic<bool> m_autoLaunchApps{true};
    std::atomic<int>  m_scopeMode{SCOPE_ONLY_LISTED};
    std::atomic<int>  m_selectedMode{STAGE_1_FAST_TLS_REC}; // 1..6 = Manuel Kademe
    std::atomic<int>  m_activeStage{STAGE_1_FAST_TLS_REC};
    std::atomic<int>  m_probeStage{STAGE_1_FAST_TLS_REC};
    std::atomic<int>  m_lastLatencyMs{0};
    std::atomic<int>  m_activeWorkers{0};

    uint16_t m_httpPort{3131};
    uint16_t m_socksPort{1080};

    SOCKET m_httpListenSock{INVALID_SOCKET};
    SOCKET m_socksListenSock{INVALID_SOCKET};
    HANDLE m_httpThread{NULL};
    HANDLE m_socksThread{NULL};

    std::string m_upstreamHost{"127.0.0.1"};
    std::atomic<uint16_t> m_upstreamPort{40000};

    mutable std::mutex m_mutex;
    std::map<std::string, std::string> m_dohCache;
    std::map<std::string, BypassStage> m_domainStageCache;
    std::vector<TunnelTargetItem> m_tunnelItems;
    std::vector<SOCKET> m_activeRelaySocks;

    EngineLogCallback m_logCb{nullptr};
    EngineStateCallback m_stateCb{nullptr};
};
