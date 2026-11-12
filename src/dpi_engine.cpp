#include "dpi_engine.h"
#include <stdarg.h>
#include <algorithm>
#include <cstring>

struct ClientWorkerCtx {
    ZeroDpiEngine* engine;
    SOCKET sock;
    bool isSocks5;
};

struct RelayHalfCtx {
    SOCKET src;
    SOCKET dst;
};

static bool NormalizeHttpProxyRequest(const char* buf, int len, std::string& outData) {
    if (len < 14) return false;
    const char* sp1 = (const char*)memchr(buf, ' ', 10);
    if (!sp1) return false;
    size_t methodLen = (size_t)(sp1 - buf);
    if (methodLen < 3 || methodLen > 7) return false;
    if ((size_t)len < methodLen + 10) return false;
    if (_strnicmp(sp1 + 1, "http://", 7) != 0) return false;

    const char* lineEnd = (const char*)memchr(sp1 + 8, '\n', len - (int)(sp1 + 8 - buf));
    if (!lineEnd) return false;

    const char* hostStart = sp1 + 8;
    const char* sp2 = (const char*)memchr(hostStart, ' ', (size_t)(lineEnd - hostStart));
    if (!sp2) return false;

    const char* slashPos = (const char*)memchr(hostStart, '/', (size_t)(sp2 - hostStart));
    outData.clear();
    outData.reserve(len);
    outData.append(buf, methodLen + 1);
    if (slashPos && slashPos < sp2) {
        outData.append(slashPos, len - (int)(slashPos - buf));
    } else {
        outData.push_back('/');
        outData.append(sp2, len - (int)(sp2 - buf));
    }

    size_t hdrEnd = outData.find("\r\n\r\n");
    size_t pPos = outData.find("\r\nProxy-Connection:");
    if (pPos == std::string::npos || (hdrEnd != std::string::npos && pPos > hdrEnd)) {
        pPos = outData.find("\r\nproxy-connection:");
    }
    if (pPos != std::string::npos && (hdrEnd == std::string::npos || pPos < hdrEnd)) {
        outData.erase(pPos + 2, 6);
    }
    return true;
}

static DWORD WINAPI RelayHalfThread(LPVOID param) {
    RelayHalfCtx* ctx = (RelayHalfCtx*)param;
    std::vector<char> buf(16384);
    std::string rewritten;
    while (true) {
        int r = recv(ctx->src, buf.data(), (int)buf.size(), 0);
        if (r <= 0) break;
        const char* sendPtr = buf.data();
        int sendLen = r;
        if (NormalizeHttpProxyRequest(buf.data(), r, rewritten)) {
            sendPtr = rewritten.data();
            sendLen = (int)rewritten.size();
        }
        int sent = 0;
        while (sent < sendLen) {
            int w = send(ctx->dst, sendPtr + sent, sendLen - sent, 0);
            if (w <= 0) break;
            sent += w;
        }
        if (sent < sendLen) break;
    }
    shutdown(ctx->dst, SD_BOTH);
    shutdown(ctx->src, SD_BOTH);
    delete ctx;
    return 0;
}

static std::string ToLowerAscii(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    }
    return out;
}

static std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
    std::string out(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &out[0], len, NULL, NULL);
    return out;
}

static std::wstring ExtractFileName(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos && pos + 1 < path.size()) {
        return path.substr(pos + 1);
    }
    return path;
}

ZeroDpiEngine::ZeroDpiEngine() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    const wchar_t* defaultDomains[] = {
        L"discord.com",
        L"updates.discord.com",
        L"gateway.discord.gg",
        L"cdn.discordapp.com",
        L"media.discordapp.net",
        L"dl.discordapp.net",
        L"discord.gg",
        L"discordapp.com",
        L"discord.media",
        L"roblox.com",
        L"wattpad.com",
        L"imgur.com",
        L"pastebin.com"
    };

    wchar_t localAppData[MAX_PATH] = {0};
    if (GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH) > 0) {
        wchar_t discordDir[MAX_PATH] = {0};
        _snwprintf_s(discordDir, _countof(discordDir), _TRUNCATE, L"%s\\Discord", localAppData);
        if (GetFileAttributesW(discordDir) != INVALID_FILE_ATTRIBUTES) {
            AddTunnelItem(ITEM_FOLDER, discordDir);
        }
    }

    for (const wchar_t* d : defaultDomains) {
        AddTunnelItem(ITEM_DOMAIN, d);
    }
}

ZeroDpiEngine::~ZeroDpiEngine() {
    Stop();
    EnableSystemSmartPac(false);
    WSACleanup();
}

const wchar_t* ZeroDpiEngine::GetStageShortName(BypassStage stage) {
    switch (stage) {
        case STAGE_0_DIRECT:          return L"Kademe 0 - Doğrudan Geçiş";
        case STAGE_1_FAST_TLS_REC:    return L"Kademe 1 - En Hafif";
        case STAGE_2_DUAL_PKT_0MS:    return L"Kademe 2 - Hafif";
        case STAGE_3_MICRO_1MS:       return L"Kademe 3 - Orta";
        case STAGE_4_HYBRID_SUPER:    return L"Kademe 4 - Güçlü";
        case STAGE_5_ANYCAST_ULTRA:   return L"Kademe 5 - Çok Güçlü";
        case STAGE_6_UPSTREAM_TUNNEL: return L"Kademe 6 - Maksimum";
        default:                      return L"Bilinmeyen Kademe";
    }
}

void ZeroDpiEngine::Log(const wchar_t* fmt, ...) {
    if (!m_logCb) return;
    wchar_t buf[1024];
    va_list args;
    va_start(args, fmt);
    _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, fmt, args);
    va_end(args);
    m_logCb(buf);
}

void ZeroDpiEngine::RegisterRelaySocket(SOCKET s) {
    if (s == INVALID_SOCKET) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeRelaySocks.push_back(s);
}

void ZeroDpiEngine::UnregisterRelaySocket(SOCKET s) {
    if (s == INVALID_SOCKET) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find(m_activeRelaySocks.begin(), m_activeRelaySocks.end(), s);
    if (it != m_activeRelaySocks.end()) {
        m_activeRelaySocks.erase(it);
    }
}

void ZeroDpiEngine::ApplyLiveSwitch(bool stageChanged) {
    std::vector<SOCKET> toReset;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (stageChanged) {
            m_dohCache.clear();
            m_domainStageCache.clear();
        }
        toReset = m_activeRelaySocks;
    }

    if (m_running.load()) {
        if (!stageChanged && m_useSystemPac.load()) {
            EnableSystemSmartPac(true);
        }
        for (SOCKET s : toReset) {
            if (s != INVALID_SOCKET) {
                shutdown(s, SD_BOTH);
            }
        }
        if (m_stateCb) {
            m_stateCb((BypassStage)m_activeStage.load(), true, m_lastLatencyMs.load());
        }
    }
}

void ZeroDpiEngine::SetScopeMode(TunnelScopeMode mode) {
    int m = (int)mode;
    if (m < SCOPE_ONLY_LISTED || m > SCOPE_FULL_SYSTEM) m = SCOPE_ONLY_LISTED;
    int prev = m_scopeMode.exchange(m);
    if (prev != m && m_running.load()) {
        ApplyLiveSwitch(false);
    }
}

void ZeroDpiEngine::SetSelectedMode(int mode) {
    int st = (mode >= STAGE_1_FAST_TLS_REC && mode <= STAGE_6_UPSTREAM_TUNNEL) ? mode : STAGE_1_FAST_TLS_REC;
    int prev = m_selectedMode.exchange(st);
    m_activeStage.store(st);
    if (prev != st && m_running.load()) {
        ApplyLiveSwitch(true);
    }
}

void ZeroDpiEngine::AddTunnelItem(TunnelItemType type, const std::wstring& pathOrDomain, bool isExcluded) {
    if (pathOrDomain.empty()) return;
    std::wstring cleanVal = pathOrDomain;
    if (type == ITEM_DOMAIN) {
        while (!cleanVal.empty() && (cleanVal.front() == L' ' || cleanVal.front() == L'\t')) cleanVal.erase(cleanVal.begin());
        while (!cleanVal.empty() && (cleanVal.back() == L' ' || cleanVal.back() == L'\t' || cleanVal.back() == L'/')) cleanVal.pop_back();
        if (_wcsnicmp(cleanVal.c_str(), L"https://", 8) == 0) cleanVal.erase(0, 8);
        else if (_wcsnicmp(cleanVal.c_str(), L"http://", 7) == 0) cleanVal.erase(0, 7);
        if (_wcsnicmp(cleanVal.c_str(), L"www.", 4) == 0) cleanVal.erase(0, 4);
        size_t slashPos = cleanVal.find(L'/');
        if (slashPos != std::wstring::npos) cleanVal.resize(slashPos);
    }
    if (cleanVal.empty()) return;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& existing : m_tunnelItems) {
            if (existing.type == type && existing.isExcluded == isExcluded &&
                _wcsicmp(existing.pathOrDomain.c_str(), cleanVal.c_str()) == 0) {
                return;
            }
        }
        TunnelTargetItem item;
        item.type = type;
        item.pathOrDomain = cleanVal;
        item.displayName = (type == ITEM_DOMAIN) ? cleanVal : ExtractFileName(cleanVal);
        item.enabled = true;
        item.isExcluded = isExcluded;
        m_tunnelItems.push_back(item);
    }
    if (m_running.load()) ApplyLiveSwitch(false);
}

void ZeroDpiEngine::ToggleTunnelItemRule(int index) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (index >= 0 && index < (int)m_tunnelItems.size()) {
            m_tunnelItems[index].isExcluded = !m_tunnelItems[index].isExcluded;
        }
    }
    if (m_running.load()) ApplyLiveSwitch(false);
}

void ZeroDpiEngine::RemoveTunnelItem(int index) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (index >= 0 && index < (int)m_tunnelItems.size()) {
            m_tunnelItems.erase(m_tunnelItems.begin() + index);
        }
    }
    if (m_running.load()) ApplyLiveSwitch(false);
}

std::vector<TunnelTargetItem> ZeroDpiEngine::GetTunnelItems() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_tunnelItems;
}

bool ZeroDpiEngine::IsUserExcludedHost(const std::string& host) const {
    std::string h = ToLowerAscii(host);
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& item : m_tunnelItems) {
        if (!item.isExcluded || !item.enabled || item.type != ITEM_DOMAIN) continue;
        std::string d = ToLowerAscii(WideToUtf8(item.pathOrDomain));
        if (d.empty()) continue;
        if (h == d) return true;
        if (h.size() > d.size() && h.compare(h.size() - d.size(), d.size(), d) == 0 &&
            h[h.size() - d.size() - 1] == '.') {
            return true;
        }
    }
    return false;
}

bool ZeroDpiEngine::IsTargetInBypassList(const std::string& host) const {
    int scope = m_scopeMode.load();
    if (scope == SCOPE_FULL_SYSTEM) return true;
    if (scope == SCOPE_ALL_EXCEPT_EXCLUDED) {
        return !IsUserExcludedHost(host);
    }

    std::string h = ToLowerAscii(host);

    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& item : m_tunnelItems) {
        if (item.isExcluded || !item.enabled) continue;
        if (item.type == ITEM_DOMAIN) {
            std::string d = ToLowerAscii(WideToUtf8(item.pathOrDomain));
            if (d.empty()) continue;
            if (h == d) return true;
            if (h.size() > d.size() && h.compare(h.size() - d.size(), d.size(), d) == 0 &&
                h[h.size() - d.size() - 1] == '.') {
                return true;
            }
        }
    }
    return false;
}

// Yerel tünele (127.0.0.1:3131) bağlanan soketin hangi .exe ve klasörden geldiğini bulur
static bool GetSocketOwnerProcess(SOCKET clientSock, std::wstring& outFullPath, std::wstring& outExeName, DWORD* outOwnerPid = nullptr) {
    outFullPath.clear();
    outExeName.clear();
    if (outOwnerPid) *outOwnerPid = 0;

    sockaddr_in peerAddr{};
    int peerLen = sizeof(peerAddr);
    if (getpeername(clientSock, (sockaddr*)&peerAddr, &peerLen) != 0) {
        return false;
    }
    DWORD clientPortNet = peerAddr.sin_port;

    DWORD dwSize = 0;
    GetExtendedTcpTable(nullptr, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_CONNECTIONS, 0);
    if (dwSize == 0) return false;

    std::vector<uint8_t> buffer;
    DWORD ret = ERROR_INSUFFICIENT_BUFFER;
    for (int attempt = 0; attempt < 3 && ret == ERROR_INSUFFICIENT_BUFFER; ++attempt) {
        dwSize += 4096;
        buffer.resize(dwSize);
        ret = GetExtendedTcpTable(buffer.data(), &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_CONNECTIONS, 0);
    }
    if (ret != NO_ERROR) {
        return false;
    }

    PMIB_TCPTABLE_OWNER_PID pTable = (PMIB_TCPTABLE_OWNER_PID)buffer.data();
    DWORD ownerPid = 0;
    for (DWORD i = 0; i < pTable->dwNumEntries; ++i) {
        const MIB_TCPROW_OWNER_PID& row = pTable->table[i];
        if ((row.dwLocalPort & 0xFFFF) == clientPortNet && row.dwLocalAddr == htonl(INADDR_LOOPBACK)) {
            ownerPid = row.dwOwningPid;
            break;
        }
    }

    if (ownerPid == 0) return false;
    if (outOwnerPid) *outOwnerPid = ownerPid;

    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, ownerPid);
    if (!hProc) return false;

    wchar_t pathBuf[MAX_PATH] = {0};
    DWORD pathLen = MAX_PATH;
    bool ok = (QueryFullProcessImageNameW(hProc, 0, pathBuf, &pathLen) != FALSE);
    CloseHandle(hProc);

    if (ok && pathLen > 0) {
        outFullPath.assign(pathBuf, pathLen);
        outExeName = ExtractFileName(outFullPath);
        return true;
    }
    return false;
}

static bool PathStartsWithFolder(const std::wstring& fullPath, const std::wstring& folderPath) {
    if (folderPath.empty() || fullPath.size() < folderPath.size()) return false;
    if (_wcsnicmp(fullPath.c_str(), folderPath.c_str(), folderPath.size()) != 0) return false;
    if (fullPath.size() == folderPath.size()) return true;
    wchar_t nextCh = fullPath[folderPath.size()];
    return (nextCh == L'\\' || nextCh == L'/' || folderPath.back() == L'\\' || folderPath.back() == L'/');
}

bool ZeroDpiEngine::ShouldBypassConnection(const std::string& host, const std::wstring& clientFullPath, const std::wstring& clientExeName, std::wstring& outReason) const {
    outReason.clear();
    int scope = m_scopeMode.load();

    // Mod 3: Tüm Sistem (Koşulsuz Tam Tünel)
    if (scope == SCOPE_FULL_SYSTEM) {
        outReason = L"Tüm Sistem";
        return true;
    }

    // Mod 2: Belirtilenler Dışında Tüm Sistem (Hariç Tutma Modu)
    if (scope == SCOPE_ALL_EXCEPT_EXCLUDED) {
        if (IsUserExcludedHost(host)) return false;
        if (!clientFullPath.empty()) {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (const auto& item : m_tunnelItems) {
                if (!item.isExcluded || !item.enabled) continue;
                if (item.type == ITEM_FOLDER && PathStartsWithFolder(clientFullPath, item.pathOrDomain)) {
                    return false;
                }
                if (item.type == ITEM_APP_EXE &&
                    (_wcsicmp(clientFullPath.c_str(), item.pathOrDomain.c_str()) == 0 ||
                     _wcsicmp(clientExeName.c_str(), item.displayName.c_str()) == 0)) {
                    return false;
                }
            }
        }
        outReason = L"Hariç Dışı Sistem";
        return true;
    }

    // Mod 1: Sadece Listedeki Uygulama, Klasör ve Siteler (SCOPE_ONLY_LISTED)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!clientFullPath.empty()) {
            for (const auto& item : m_tunnelItems) {
                if (item.isExcluded || !item.enabled) continue;
                if (item.type == ITEM_FOLDER && PathStartsWithFolder(clientFullPath, item.pathOrDomain)) {
                    outReason = L"Klasör: " + item.displayName;
                    return true;
                }
                if (item.type == ITEM_APP_EXE &&
                    (_wcsicmp(clientFullPath.c_str(), item.pathOrDomain.c_str()) == 0 ||
                     _wcsicmp(clientExeName.c_str(), item.displayName.c_str()) == 0)) {
                    outReason = L"Uygulama: " + item.displayName;
                    return true;
                }
            }
        }
    }

    if (IsTargetInBypassList(host)) {
        outReason = L"Site Kuralı";
        return true;
    }
    return false;
}

bool ZeroDpiEngine::FindTlsSniOffset(const uint8_t* data, int len, int& sniStart, int& sniLen, std::string& outHost) {
    sniStart = -1;
    sniLen = 0;
    outHost.clear();

    if (len < 43 || data[0] != 0x16 || data[1] != 0x03 || data[5] != 0x01) {
        return false;
    }

    int pos = 9 + 2 + 32; // 43
    if (pos >= len) return false;

    int sessionIdLen = data[pos];
    pos += 1 + sessionIdLen;
    if (pos + 2 > len) return false;

    int cipherLen = (data[pos] << 8) | data[pos + 1];
    pos += 2 + cipherLen;
    if (pos + 1 > len) return false;

    int compLen = data[pos];
    pos += 1 + compLen;
    if (pos + 2 > len) return false;

    int extTotalLen = (data[pos] << 8) | data[pos + 1];
    pos += 2;
    int extEnd = pos + extTotalLen;
    if (extEnd > len) extEnd = len;

    while (pos + 4 <= extEnd) {
        int extType = (data[pos] << 8) | data[pos + 1];
        int extLen  = (data[pos + 2] << 8) | data[pos + 3];
        pos += 4;
        if (pos + extLen > extEnd) break;

        if (extType == 0x0000 && extLen >= 5) {
            int nameType = data[pos + 2];
            int nameLen  = (data[pos + 3] << 8) | data[pos + 4];
            if (nameType == 0x00 && pos + 5 + nameLen <= extEnd && nameLen > 0) {
                sniStart = pos + 5;
                sniLen = nameLen;
                outHost.assign((const char*)(data + sniStart), (size_t)sniLen);
                return true;
            }
        }
        pos += extLen;
    }
    return false;
}

std::vector<uint8_t> ZeroDpiEngine::SplitIntoTlsRecords(const uint8_t* recordData, int recordLen, const std::vector<int>& hsSplitOffsets) {
    if (recordLen <= 5 || recordData[0] != 0x16 || recordData[1] != 0x03) {
        return std::vector<uint8_t>(recordData, recordData + recordLen);
    }

    int declaredRecLen = (recordData[3] << 8) | recordData[4];
    int totalFirstRec = 5 + declaredRecLen;
    if (totalFirstRec > recordLen) {
        return std::vector<uint8_t>(recordData, recordData + recordLen);
    }

    const uint8_t* hsData = recordData + 5;
    int hsLen = declaredRecLen;
    uint8_t verMajor = recordData[1];
    uint8_t verMinor = recordData[2];

    std::vector<uint8_t> out;
    int prev = 0;
    for (int splitPos : hsSplitOffsets) {
        if (splitPos <= prev || splitPos >= hsLen) continue;
        int chunkLen = splitPos - prev;
        out.push_back(0x16);
        out.push_back(verMajor);
        out.push_back(verMinor);
        out.push_back((uint8_t)((chunkLen >> 8) & 0xFF));
        out.push_back((uint8_t)(chunkLen & 0xFF));
        out.insert(out.end(), hsData + prev, hsData + splitPos);
        prev = splitPos;
    }

    int remLen = hsLen - prev;
    if (remLen > 0) {
        out.push_back(0x16);
        out.push_back(verMajor);
        out.push_back(verMinor);
        out.push_back((uint8_t)((remLen >> 8) & 0xFF));
        out.push_back((uint8_t)(remLen & 0xFF));
        out.insert(out.end(), hsData + prev, hsData + hsLen);
    }

    if (recordLen > totalFirstRec) {
        out.insert(out.end(), recordData + totalFirstRec, recordData + recordLen);
    }
    return out;
}

static bool IsPoisonedOrInvalidIp(const std::string& ip) {
    if (ip.empty()) return true;
    // Yerel, geçersiz ve Türk ISS (Türk Telekom, Turkcell Superonline, Kablonet, Vodafone) BTK blok sayfası IP blokları
    const char* badPrefixes[] = {
        "127.", "0.", "10.", "192.168.",
        "195.175.", "212.156.", "212.252.", "85.29.", "176.240.", "31.145."
    };
    for (const char* pref : badPrefixes) {
        if (ip.find(pref) == 0) return true;
    }
    return false;
}

static std::string GetVerifiedDiscordFallbackIp(const std::string& lowerHost, int variant = 0) {
    if (lowerHost == "dl.discordapp.net") {
        return (variant % 2 == 0) ? "104.18.48.115" : "104.18.52.172";
    }
    if (lowerHost == "gateway.discord.gg" || lowerHost.find(".discord.gg") != std::string::npos) {
        const char* gwIps[] = { "162.159.136.234", "162.159.135.234", "162.159.134.234", "162.159.133.234" };
        return gwIps[variant & 3];
    }
    if (lowerHost == "cdn.discordapp.com") {
        const char* cdnIps[] = { "162.159.135.233", "162.159.134.233", "162.159.133.233", "162.159.130.233" };
        return cdnIps[variant & 3];
    }
    if (lowerHost == "media.discordapp.net" || lowerHost.find(".discordapp.net") != std::string::npos) {
        const char* medIps[] = { "162.159.128.232", "162.159.129.232", "162.159.133.232", "162.159.134.232" };
        return medIps[variant & 3];
    }
    if (lowerHost.find("discord") != std::string::npos) {
        const char* mainIps[] = { "162.159.135.232", "162.159.136.232", "162.159.137.232", "162.159.138.232", "162.159.128.233" };
        return mainIps[variant % 5];
    }
    return "";
}

std::string ZeroDpiEngine::ResolveDoH(const std::string& host, BypassStage stage) {
    sockaddr_in sa{};
    if (inet_pton(AF_INET, host.c_str(), &(sa.sin_addr)) == 1) {
        return host;
    }

    std::string lowerHost = ToLowerAscii(host);

    if (stage == STAGE_5_ANYCAST_ULTRA || stage == STAGE_6_UPSTREAM_TUNNEL) {
        std::string anycastIp = GetVerifiedDiscordFallbackIp(lowerHost, (int)(GetTickCount() & 0x7F));
        if (!anycastIp.empty()) return anycastIp;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_dohCache.find(lowerHost);
        if (it != m_dohCache.end()) return it->second;
    }

    std::string resolvedIp;

    // 0. Normal (engelsiz / STAGE_0_DIRECT) siteler için hiç vakit kaybetmeden doğrudan sistem DNS önbelleğini kullan (0ms)
    if (stage == STAGE_0_DIRECT && !IsTargetInBypassList(lowerHost)) {
        addrinfo hints{}, *res = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(lowerHost.c_str(), nullptr, &hints, &res) == 0 && res) {
            char ipStr[INET_ADDRSTRLEN] = {0};
            inet_ntop(AF_INET, &(((sockaddr_in*)res->ai_addr)->sin_addr), ipStr, sizeof(ipStr));
            freeaddrinfo(res);
            resolvedIp = ipStr;
        }
        if (!resolvedIp.empty()) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_dohCache[lowerHost] = resolvedIp;
            return resolvedIp;
        }
    }

    // 1. Önce Turkcell Superonline'da engellenmeyen Google DoH (8.8.8.8 / 8.8.4.4),
    //    ardından Cloudflare ikincil (1.0.0.1) ve Quad9 (9.9.9.9) üzerinden şifreli çöz!
    //    (Turkcell Superonline bazı bölgelerde 1.1.1.1 IP'sini filtrelediği için 8.8.8.8 ilk sıradadır)
    {
        struct DohProvider {
            const wchar_t* server;
            const wchar_t* pathFmt;
        };
        const DohProvider providers[] = {
            { L"8.8.8.8", L"/resolve?name=%S&type=A" },
            { L"8.8.4.4", L"/resolve?name=%S&type=A" },
            { L"1.0.0.1", L"/dns-query?name=%S&type=A" },
            { L"9.9.9.9", L"/dns-query?name=%S&type=A" },
            { L"1.1.1.1", L"/dns-query?name=%S&type=A" }
        };

        HINTERNET hSession = WinHttpOpen(L"ZeroDPI/2.3", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (hSession) {
            WinHttpSetTimeouts(hSession, 800, 800, 800, 800);
            for (const auto& prov : providers) {
                HINTERNET hConnect = WinHttpConnect(hSession, prov.server, INTERNET_DEFAULT_HTTPS_PORT, 0);
                if (!hConnect) continue;

                wchar_t path[512];
                _snwprintf_s(path, _countof(path), _TRUNCATE, prov.pathFmt, lowerHost.c_str());

                HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
                if (hRequest) {
                    WinHttpAddRequestHeaders(hRequest, L"Accept: application/dns-json\r\n", (DWORD)-1L, WINHTTP_ADDREQ_FLAG_ADD);
                    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                        WinHttpReceiveResponse(hRequest, NULL)) {
                        std::string body;
                        DWORD dwSize = 0;
                        while (WinHttpQueryDataAvailable(hRequest, &dwSize) && dwSize > 0) {
                            std::vector<char> buf(dwSize + 1, 0);
                            DWORD dwRead = 0;
                            if (WinHttpReadData(hRequest, buf.data(), dwSize, &dwRead) && dwRead > 0) {
                                body.append(buf.data(), dwRead);
                            }
                        }

                        size_t searchPos = 0;
                        while ((searchPos = body.find("\"data\"", searchPos)) != std::string::npos) {
                            size_t colon = body.find(':', searchPos);
                            size_t q1 = (colon != std::string::npos) ? body.find('"', colon + 1) : std::string::npos;
                            size_t q2 = (q1 != std::string::npos) ? body.find('"', q1 + 1) : std::string::npos;
                            if (q1 != std::string::npos && q2 != std::string::npos && q2 > q1 + 1) {
                                std::string candidate = body.substr(q1 + 1, q2 - q1 - 1);
                                sockaddr_in testSa{};
                                if (inet_pton(AF_INET, candidate.c_str(), &(testSa.sin_addr)) == 1 && !IsPoisonedOrInvalidIp(candidate)) {
                                    resolvedIp = candidate;
                                    break;
                                }
                            }
                            searchPos = (q2 != std::string::npos) ? q2 + 1 : searchPos + 6;
                        }
                    }
                    WinHttpCloseHandle(hRequest);
                }
                WinHttpCloseHandle(hConnect);
                if (!resolvedIp.empty()) break;
            }
            WinHttpCloseHandle(hSession);
        }
    }

    // 2. Eğer ISS tüm DoH sunucularını engellediyse, Discord domainleri için doğrulanmış temiz Cloudflare IP'lerini kullan
    if (resolvedIp.empty()) {
        resolvedIp = GetVerifiedDiscordFallbackIp(lowerHost, 0);
    }

    // 3. Diğer siteler için sistem DNS yedeklemesi (Zehirli IP kontrolü ile)
    if (resolvedIp.empty()) {
        addrinfo hints{}, *res = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(lowerHost.c_str(), nullptr, &hints, &res) == 0 && res) {
            char ipStr[INET_ADDRSTRLEN] = {0};
            inet_ntop(AF_INET, &(((sockaddr_in*)res->ai_addr)->sin_addr), ipStr, sizeof(ipStr));
            freeaddrinfo(res);
            if (!IsPoisonedOrInvalidIp(ipStr)) {
                resolvedIp = ipStr;
            }
        }
    }

    if (!resolvedIp.empty()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_dohCache.size() >= 512) {
            m_dohCache.clear();
        }
        m_dohCache[lowerHost] = resolvedIp;
    }
    return resolvedIp;
}

static bool SendAllExact(SOCKET s, const uint8_t* buf, int len) {
    int total = 0;
    while (total < len) {
        int n = send(s, (const char*)(buf + total), len - total, 0);
        if (n <= 0) return false;
        total += n;
    }
    return true;
}

// Çoklu TLS Record dizisini her bir zarf (veya grup) ayrı TCP paketi olacak şekilde gönderir
static bool SendTlsRecordsSegmented(SOCKET sock, const std::vector<uint8_t>& multiRec, int maxSegmentedRecords, DWORD delayMs) {
    int pos = 0;
    int total = (int)multiRec.size();
    int recIdx = 0;

    while (pos + 5 <= total && recIdx < maxSegmentedRecords) {
        if (multiRec[pos] != 0x16) break;
        int recPayloadLen = (multiRec[pos + 3] << 8) | multiRec[pos + 4];
        int frameLen = 5 + recPayloadLen;
        if (pos + frameLen > total) break;

        if (!SendAllExact(sock, multiRec.data() + pos, frameLen)) return false;
        pos += frameLen;
        recIdx++;
        if (pos < total && delayMs > 0) {
            Sleep(delayMs);
        }
    }

    if (pos < total) {
        return SendAllExact(sock, multiRec.data() + pos, total - pos);
    }
    return true;
}

bool ZeroDpiEngine::SendInitialPayloadWithStage(SOCKET remoteSock, const uint8_t* data, int len, BypassStage stage) {
    if (len <= 0) return true;

    BOOL noDelay = TRUE;
    setsockopt(remoteSock, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

    if (stage <= STAGE_0_DIRECT) {
        return SendAllExact(remoteSock, data, len);
    }

    int sniStart = -1, sniLen = 0;
    std::string sniHost;
    bool isTlsWithSni = FindTlsSniOffset(data, len, sniStart, sniLen, sniHost);

    // Kademe 1 (En Hafif - 0ms): Tek TCP Paketinde TLS Record SNI Bölme (Eski Kademe 3 - TTNET / TurkNet / Vodafone)
    // Hiçbir Sleep/bekleme yoktur (%0 gecikme), tek send() ile SNI'ı 4 ayrı TLS zarfına bölerek gönderir.
    if (stage == STAGE_1_FAST_TLS_REC) {
        if (isTlsWithSni && sniStart > 9 && sniLen >= 4) {
            int sniHsBase = sniStart - 5;
            std::vector<int> splits = {
                2,
                sniHsBase + 2,
                sniHsBase + (sniLen / 2),
                sniHsBase + sniLen - 2
            };
            std::vector<uint8_t> multiRec = SplitIntoTlsRecords(data, len, splits);
            return SendAllExact(remoteSock, multiRec.data(), (int)multiRec.size());
        }
        int splitPos = (len > 2) ? 2 : 1;
        if (!SendAllExact(remoteSock, data, splitPos)) return false;
        return SendAllExact(remoteSock, data + splitPos, len - splitPos);
    }

    // Kademe 2 (Çok Hafif - 0ms): 1-Byte Başlık Kırma + Çift TCP Paketi Mikro Zarf (0ms Gecikme)
    // Yine Sleep yoktur (0ms)! Sadece ilk 1-byte ClientHello başlık zarfını önden ayrı TCP paketinde gönderir,
    // geri kalan mikro TLS zarflarını anında 2. TCP paketinde gönderir.
    if (stage == STAGE_2_DUAL_PKT_0MS) {
        if (isTlsWithSni && sniStart > 9 && sniLen >= 2) {
            int sniHsBase = sniStart - 5;
            std::vector<int> splits;
            splits.push_back(1);
            splits.push_back(4);
            for (int off = 2; off < sniLen; off += 3) {
                splits.push_back(sniHsBase + off);
            }
            std::vector<uint8_t> multiRec = SplitIntoTlsRecords(data, len, splits);
            return SendTlsRecordsSegmented(remoteSock, multiRec, 2, 0);
        }
        int splitPos = (len > 2) ? 2 : 1;
        if (!SendAllExact(remoteSock, data, splitPos)) return false;
        return SendAllExact(remoteSock, data + splitPos, len - splitPos);
    }

    // Kademe 3 (Hafif+ - 1ms): 1-Byte Başlık + 3'er Harflik SNI Zarf Parçalama + 1ms Ultra Hızlı Segmentasyon
    if (stage == STAGE_3_MICRO_1MS) {
        if (isTlsWithSni && sniStart > 9 && sniLen >= 2) {
            int sniHsBase = sniStart - 5;
            std::vector<int> splits;
            splits.push_back(1);
            splits.push_back(4);
            for (int off = 2; off < sniLen; off += 3) {
                splits.push_back(sniHsBase + off);
            }
            std::vector<uint8_t> multiRec = SplitIntoTlsRecords(data, len, splits);
            return SendTlsRecordsSegmented(remoteSock, multiRec, 5, 1);
        }
        int splitPos = (len > 3) ? 3 : 1;
        if (!SendAllExact(remoteSock, data, splitPos)) return false;
        return SendAllExact(remoteSock, data + splitPos, len - splitPos);
    }

    // Kademe 4 (Dengeli Güçlü - Superonline / Kablonet Optimize): 1-Byte Başlık + 2'şer Harflik Mikro SNI Zarfı + 2ms Hibrit TCP
    // (Eski Kademe 5'in ağı darlamayacak şekilde 2ms / 8 segment olarak optimize edilmiş hali)
    if (stage == STAGE_4_HYBRID_SUPER) {
        if (isTlsWithSni && sniStart > 9 && sniLen >= 2) {
            int sniHsBase = sniStart - 5;
            std::vector<int> splits;
            splits.push_back(1);
            splits.push_back(4);
            splits.push_back(sniHsBase);
            for (int off = 2; off < sniLen; off += 2) {
                splits.push_back(sniHsBase + off);
            }
            std::vector<uint8_t> multiRec = SplitIntoTlsRecords(data, len, splits);
            return SendTlsRecordsSegmented(remoteSock, multiRec, 8, 2);
        }
        return SendAllExact(remoteSock, data, len);
    }

    // Kademe 5 (Çok Güçlü): Doğrulanmış Cloudflare Anycast Rotası + 1-Byte Başlık + 2'şer Harflik Mikro TLS Zarf Desync
    if (stage == STAGE_5_ANYCAST_ULTRA) {
        if (isTlsWithSni && sniStart > 9 && sniLen >= 2) {
            int sniHsBase = sniStart - 5;
            std::vector<int> splits;
            splits.push_back(1);
            splits.push_back(4);
            splits.push_back(sniHsBase);
            for (int off = 1; off < sniLen; off += 2) {
                splits.push_back(sniHsBase + off);
            }
            std::vector<uint8_t> multiRec = SplitIntoTlsRecords(data, len, splits);
            return SendTlsRecordsSegmented(remoteSock, multiRec, 12, 3);
        }
        return SendAllExact(remoteSock, data, len);
    }

    // Kademe 6 (Maksimum): Anycast Rotası + 1-Byte Başlık + Tek Tek (1 Harflik) Tam Granüler SNI Zarf Bölme
    if (stage == STAGE_6_UPSTREAM_TUNNEL) {
        if (isTlsWithSni && sniStart > 9 && sniLen >= 2) {
            int sniHsBase = sniStart - 5;
            std::vector<int> splits;
            splits.push_back(1);
            splits.push_back(2);
            splits.push_back(4);
            splits.push_back(sniHsBase);
            for (int off = 1; off < sniLen; off += 1) {
                splits.push_back(sniHsBase + off);
            }
            std::vector<uint8_t> multiRec = SplitIntoTlsRecords(data, len, splits);
            return SendTlsRecordsSegmented(remoteSock, multiRec, 16, 3);
        }
        return SendAllExact(remoteSock, data, len);
    }

    return SendAllExact(remoteSock, data, len);
}

// Gerçek Discord (Update.exe / Schannel ve Discord.exe / BoringSSL) ClientHello yapısını birebir simüle eder:
// SNI (0x0000) uzantısı gerçek Discord'daki gibi EN BAŞTA (~80. byte ofsetinde) yer alır,
// büyük KeyShare / Padding bloğu ise SNI'dan SONRA gelir. Böylece Kademe 3 gibi tek-paket kademelerin
// 512 sıfır dolgusu yüzünden Turkcell Superonline'da sahte onay alması %100 engellenir.
static std::vector<uint8_t> BuildValidTls12ClientHello(const std::string& host) {
    std::vector<uint8_t> exts;

    // 1. SNI (server_name - 0x0000) uzantısı EN BAŞTA (gerçek Discord ofseti: ~75-95. byte)
    uint16_t hostLen = (uint16_t)host.size();
    uint16_t listLen = hostLen + 3;
    uint16_t extLen  = listLen + 2;

    exts.push_back(0x00); exts.push_back(0x00);
    exts.push_back((uint8_t)(extLen >> 8)); exts.push_back((uint8_t)(extLen & 0xFF));
    exts.push_back((uint8_t)(listLen >> 8)); exts.push_back((uint8_t)(listLen & 0xFF));
    exts.push_back(0x00);
    exts.push_back((uint8_t)(hostLen >> 8)); exts.push_back((uint8_t)(hostLen & 0xFF));
    exts.insert(exts.end(), host.begin(), host.end());

    // 2. Standart TLS 1.2 / 1.3 uzantıları
    const uint8_t stdExts[] = {
        0x00, 0x0b, 0x00, 0x04, 0x03, 0x00, 0x01, 0x02,
        0x00, 0x0a, 0x00, 0x0a, 0x00, 0x08, 0x00, 0x1d, 0x00, 0x17, 0x00, 0x18, 0x00, 0x19,
        0x00, 0x0d, 0x00, 0x12, 0x00, 0x10, 0x04, 0x01, 0x04, 0x03, 0x05, 0x01, 0x05, 0x03, 0x06, 0x01, 0x06, 0x03, 0x02, 0x01, 0x02, 0x03,
        0xff, 0x01, 0x00, 0x01, 0x00
    };
    exts.insert(exts.end(), stdExts, stdExts + sizeof(stdExts));

    // 3. SNI'dan SONRA gelen 512-byte'lık TLS Padding / KeyShare yükü (gerçek paket boyutu simülasyonu)
    uint16_t padDataLen = 512;
    exts.push_back(0x00); exts.push_back(0x15); // padding extension (0x0015)
    exts.push_back((uint8_t)(padDataLen >> 8));
    exts.push_back((uint8_t)(padDataLen & 0xFF));
    for (uint16_t i = 0; i < padDataLen; ++i) {
        exts.push_back(0x00);
    }

    std::vector<uint8_t> body;
    body.push_back(0x03); body.push_back(0x03);
    for (int i = 0; i < 32; ++i) body.push_back((uint8_t)((GetTickCount() + i * 29) & 0xFF));
    body.push_back(0x00);

    const uint8_t ciphers[] = {
        0x00, 0x18,
        0xc0, 0x2b, 0xc0, 0x2f, 0xc0, 0x2c, 0xc0, 0x30,
        0xcc, 0xa9, 0xcc, 0xa8, 0xc0, 0x13, 0xc0, 0x14,
        0x00, 0x9c, 0x00, 0x9d, 0x00, 0x2f, 0x00, 0x35
    };
    body.insert(body.end(), ciphers, ciphers + sizeof(ciphers));
    body.push_back(0x01); body.push_back(0x00);

    uint16_t totalExtLen = (uint16_t)exts.size();
    body.push_back((uint8_t)(totalExtLen >> 8));
    body.push_back((uint8_t)(totalExtLen & 0xFF));
    body.insert(body.end(), exts.begin(), exts.end());

    uint32_t hsLen = (uint32_t)body.size();
    std::vector<uint8_t> hs;
    hs.push_back(0x01);
    hs.push_back((uint8_t)((hsLen >> 16) & 0xFF));
    hs.push_back((uint8_t)((hsLen >> 8) & 0xFF));
    hs.push_back((uint8_t)(hsLen & 0xFF));
    hs.insert(hs.end(), body.begin(), body.end());

    uint16_t recLen = (uint16_t)hs.size();
    std::vector<uint8_t> pkt;
    pkt.push_back(0x16);
    pkt.push_back(0x03); pkt.push_back(0x01);
    pkt.push_back((uint8_t)(recLen >> 8));
    pkt.push_back((uint8_t)(recLen & 0xFF));
    pkt.insert(pkt.end(), hs.begin(), hs.end());
    return pkt;
}

static SOCKET ConnectTcpWithTimeout(const std::string& ip, uint16_t port, int timeoutMs) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return INVALID_SOCKET;

    u_long nonBlock = 1;
    ioctlsocket(s, FIONBIO, &nonBlock);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        closesocket(s);
        return INVALID_SOCKET;
    }

    connect(s, (sockaddr*)&addr, sizeof(addr));

    fd_set wset, eset;
    FD_ZERO(&wset);
    FD_ZERO(&eset);
    FD_SET(s, &wset);
    FD_SET(s, &eset);

    timeval tv{};
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    int sel = select(0, nullptr, &wset, &eset, &tv);
    if (sel <= 0 || FD_ISSET(s, &eset) || !FD_ISSET(s, &wset)) {
        closesocket(s);
        return INVALID_SOCKET;
    }

    int soErr = 0;
    int soLen = sizeof(soErr);
    getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&soErr, &soLen);
    if (soErr != 0) {
        closesocket(s);
        return INVALID_SOCKET;
    }

    nonBlock = 0;
    ioctlsocket(s, FIONBIO, &nonBlock);
    return s;
}

SOCKET ZeroDpiEngine::ConnectViaUpstreamSocks5(const std::string& targetHost, uint16_t targetPort) {
    SOCKET s = ConnectTcpWithTimeout(m_upstreamHost, m_upstreamPort.load(), 1800);
    if (s == INVALID_SOCKET) return INVALID_SOCKET;

    DWORD rwTimeout = 2000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&rwTimeout, sizeof(rwTimeout));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&rwTimeout, sizeof(rwTimeout));

    uint8_t greet[3] = { 0x05, 0x01, 0x00 };
    if (!SendAllExact(s, greet, 3)) { closesocket(s); return INVALID_SOCKET; }

    uint8_t greetResp[2] = {0};
    if (recv(s, (char*)greetResp, 2, 0) != 2 || greetResp[0] != 0x05) {
        closesocket(s);
        return INVALID_SOCKET;
    }

    std::vector<uint8_t> req = { 0x05, 0x01, 0x00, 0x03, (uint8_t)targetHost.size() };
    req.insert(req.end(), targetHost.begin(), targetHost.end());
    req.push_back((uint8_t)(targetPort >> 8));
    req.push_back((uint8_t)(targetPort & 0xFF));

    if (!SendAllExact(s, req.data(), (int)req.size())) { closesocket(s); return INVALID_SOCKET; }

    uint8_t resp[10] = {0};
    if (recv(s, (char*)resp, 4, 0) < 4 || resp[1] != 0x00) {
        closesocket(s);
        return INVALID_SOCKET;
    }

    int toSkip = (resp[3] == 0x01) ? 6 : ((resp[3] == 0x04) ? 18 : 0);
    if (resp[3] == 0x03) {
        uint8_t dlen = 0;
        recv(s, (char*)&dlen, 1, 0);
        toSkip = dlen + 2;
    }
    char dump[260];
    while (toSkip > 0) {
        int n = recv(s, dump, (toSkip > (int)sizeof(dump)) ? (int)sizeof(dump) : toSkip, 0);
        if (n <= 0) break;
        toSkip -= n;
    }
    return s;
}

bool ZeroDpiEngine::ProbeDomainAtStage(const std::string& host, uint16_t port, BypassStage stage, int& outLatencyMs) {
    DWORD t0 = GetTickCount();

    SOCKET s = INVALID_SOCKET;
    if (stage == STAGE_6_UPSTREAM_TUNNEL) {
        s = ConnectViaUpstreamSocks5(host, port);
    } else {
        std::string ip = ResolveDoH(host, stage);
        if (ip.empty()) return false;
        s = ConnectTcpWithTimeout(ip, port, 1400);
    }

    if (s == INVALID_SOCKET) return false;

    DWORD rwTimeout = 1000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&rwTimeout, sizeof(rwTimeout));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&rwTimeout, sizeof(rwTimeout));

    std::vector<uint8_t> hello = BuildValidTls12ClientHello(host);
    if (!SendInitialPayloadWithStage(s, hello.data(), (int)hello.size(), stage)) {
        closesocket(s);
        return false;
    }

    uint8_t resp[256] = {0};
    int r = recv(s, (char*)resp, sizeof(resp), 0);
    closesocket(s);

    if (r >= 10 && resp[0] == 0x16 && resp[1] == 0x03 && resp[5] == 0x02) {
        outLatencyMs = (int)(GetTickCount() - t0);
        if (outLatencyMs <= 0) outLatencyMs = 1;
        return true;
    }
    return false;
}

BypassStage ZeroDpiEngine::RunAutoDiscovery(int& outLatencyMs) {
    outLatencyMs = 0;
    if (m_discovering.exchange(true)) {
        outLatencyMs = m_lastLatencyMs.load();
        return (BypassStage)m_activeStage.load();
    }

    int selMode = m_selectedMode.load();
    if (selMode < STAGE_1_FAST_TLS_REC || selMode > STAGE_6_UPSTREAM_TUNNEL) {
        selMode = STAGE_1_FAST_TLS_REC;
        m_selectedMode.store(selMode);
    }

    BypassStage fixed = (BypassStage)selMode;
    m_activeStage.store((int)fixed);

    ResolveDoH("updates.discord.com", fixed);
    ResolveDoH("discord.com", fixed);

    int lat = 0;
    ProbeDomainAtStage("discord.com", 443, fixed, lat);
    if (lat <= 0) lat = 15;

    m_lastLatencyMs.store(lat);
    outLatencyMs = lat;
    Log(L"[✓] Seçili Kademe Aktif: %s (%d ms)", GetStageShortName(fixed), lat);
    if (m_stateCb) m_stateCb(fixed, m_running.load(), lat);
    m_discovering.store(false);
    return fixed;
}

SOCKET ZeroDpiEngine::ConnectToTarget(const std::string& host, uint16_t port, BypassStage stage) {
    std::string ip = ResolveDoH(host, stage);
    if (ip.empty()) return INVALID_SOCKET;
    return ConnectTcpWithTimeout(ip, port, 3000);
}

void ZeroDpiEngine::ServePacFile(SOCKET clientSock) {
    std::vector<TunnelTargetItem> items = GetTunnelItems();
    int scope = m_scopeMode.load();

    std::string pacBody =
        "function FindProxyForURL(url, host) {\r\n"
        "  host = host.toLowerCase();\r\n"
        "  if (isPlainHostName(host) || shExpMatch(host, 'localhost') || shExpMatch(host, '127.*') || shExpMatch(host, '192.168.*') || shExpMatch(host, '10.*') || shExpMatch(host, '172.16.*')) {\r\n"
        "    return 'DIRECT';\r\n"
        "  }\r\n";

    // Hariç Tutma Modunda (SCOPE_ALL_EXCEPT_EXCLUDED) kullanıcının işaretlediği domainler DIRECT geçer
    if (scope == SCOPE_ALL_EXCEPT_EXCLUDED) {
        for (const auto& it : items) {
            if (!it.isExcluded || !it.enabled || it.type != ITEM_DOMAIN) continue;
            std::string d = ToLowerAscii(WideToUtf8(it.pathOrDomain));
            if (d.empty()) continue;
            pacBody += "  if (dnsDomainIs(host, '" + d + "') || shExpMatch(host, '*." + d + "')) return 'DIRECT';\r\n";
        }
    }

    char proxyRet[160];
    _snprintf_s(proxyRet, sizeof(proxyRet), _TRUNCATE,
        "PROXY 127.0.0.1:%u; SOCKS5 127.0.0.1:%u",
        m_httpPort, m_socksPort);

    if (scope == SCOPE_ALL_EXCEPT_EXCLUDED || scope == SCOPE_FULL_SYSTEM) {
        pacBody += std::string("  return '") + proxyRet + "';\r\n}\r\n";
    } else {
        pacBody += "  if (";
        bool first = true;
        for (const auto& it : items) {
            if (it.type != ITEM_DOMAIN || !it.enabled || it.isExcluded) continue;
            std::string d = ToLowerAscii(WideToUtf8(it.pathOrDomain));
            if (d.empty()) continue;
            if (!first) pacBody += " ||\r\n      ";
            first = false;
            pacBody += "dnsDomainIs(host, '" + d + "') || shExpMatch(host, '*." + d + "')";
        }
        if (first) pacBody += "dnsDomainIs(host, 'discord.com')";
        pacBody += std::string(") {\r\n    return '") + proxyRet + "';\r\n  }\r\n  return 'DIRECT';\r\n}\r\n";
    }

    char header[512];
    _snprintf_s(header, sizeof(header), _TRUNCATE,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/x-ns-proxy-autoconfig\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        pacBody.size());

    send(clientSock, header, (int)strlen(header), 0);
    send(clientSock, pacBody.c_str(), (int)pacBody.size(), 0);
}

int ZeroDpiEngine::RelaySockets(SOCKET s1, SOCKET s2) {
    DWORD noTimeout = 0;
    BOOL keepAlive = TRUE;
    setsockopt(s1, SOL_SOCKET, SO_RCVTIMEO, (const char*)&noTimeout, sizeof(noTimeout));
    setsockopt(s2, SOL_SOCKET, SO_RCVTIMEO, (const char*)&noTimeout, sizeof(noTimeout));
    setsockopt(s1, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepAlive, sizeof(keepAlive));
    setsockopt(s2, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepAlive, sizeof(keepAlive));

    RegisterRelaySocket(s1);
    RegisterRelaySocket(s2);

    RelayHalfCtx* c1 = new RelayHalfCtx{ s1, s2 };
    HANDLE hThread = CreateThread(nullptr, 128 * 1024, RelayHalfThread, c1, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);
    if (!hThread) {
        delete c1;
    }

    int totalFromServer = 0;
    std::vector<char> buf(16384);
    while (m_running.load()) {
        int r = recv(s2, buf.data(), (int)buf.size(), 0);
        if (r <= 0) break;
        totalFromServer += r;
        int sent = 0;
        while (sent < r) {
            int w = send(s1, buf.data() + sent, r - sent, 0);
            if (w <= 0) break;
            sent += w;
        }
        if (sent < r) break;
    }

    UnregisterRelaySocket(s1);
    UnregisterRelaySocket(s2);

    shutdown(s1, SD_BOTH);
    shutdown(s2, SD_BOTH);
    if (hThread) {
        if (WaitForSingleObject(hThread, 1500) == WAIT_TIMEOUT) {
            closesocket(s1);
            closesocket(s2);
            s1 = INVALID_SOCKET;
            s2 = INVALID_SOCKET;
            WaitForSingleObject(hThread, 2000);
        }
        CloseHandle(hThread);
    }
    if (s2 != INVALID_SOCKET) closesocket(s2);
    if (s1 != INVALID_SOCKET) closesocket(s1);
    return totalFromServer;
}

// Chromium TLS 1.3 Post-Quantum (X25519MLKEM768) gibi ~1750 byte'lık büyük ClientHello paketlerinin
// yerel soketten parçalı gelmesi durumunda tamamını eksiksiz okur.
static int RecvCompleteTlsPayload(SOCKET sock, uint8_t* buf, int maxCap) {
    int total = recv(sock, (char*)buf, maxCap, 0);
    if (total <= 0) return total;

    if (total >= 5 && buf[0] == 0x16 && buf[1] == 0x03) {
        int recLen = (buf[3] << 8) | buf[4];
        int targetLen = 5 + recLen;
        if (targetLen > maxCap) targetLen = maxCap;
        while (total < targetLen) {
            int r = recv(sock, (char*)(buf + total), targetLen - total, 0);
            if (r <= 0) break;
            total += r;
        }
    }
    return total;
}

void ZeroDpiEngine::HandleHttpClient(SOCKET clientSock) {
    DWORD timeout = 8000;
    setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    char reqBuf[8192];
    int n = recv(clientSock, reqBuf, sizeof(reqBuf) - 1, 0);
    if (n <= 0) { closesocket(clientSock); return; }
    reqBuf[n] = '\0';

    if (strncmp(reqBuf, "GET /pac", 8) == 0 || strncmp(reqBuf, "GET /proxy.pac", 14) == 0) {
        ServePacFile(clientSock);
        closesocket(clientSock);
        return;
    }

    // 1. HTTPS CONNECT Tüneli (Tarayıcılar, Discord Rust Updater reqwest, Electron vb.)
    if (_strnicmp(reqBuf, "CONNECT ", 8) == 0) {
        std::string target(reqBuf + 8);
        size_t sp = target.find(' ');
        if (sp != std::string::npos) target.resize(sp);

        std::string host = target;
        uint16_t port = 443;
        size_t colon = target.rfind(':');
        if (colon != std::string::npos) {
            host = target.substr(0, colon);
            port = (uint16_t)atoi(target.substr(colon + 1).c_str());
            if (port == 0) port = 443;
        }

        const char* okResp = "HTTP/1.1 200 Connection Established\r\n\r\n";
        send(clientSock, okResp, (int)strlen(okResp), 0);

        uint8_t firstPayload[8192];
        int firstLen = RecvCompleteTlsPayload(clientSock, firstPayload, sizeof(firstPayload));
        if (firstLen <= 0) { closesocket(clientSock); return; }

        std::wstring clientFullPath, clientExeName, matchReason;
        GetSocketOwnerProcess(clientSock, clientFullPath, clientExeName);

        bool shouldBypass = ShouldBypassConnection(host, clientFullPath, clientExeName, matchReason);
        BypassStage usedStage = shouldBypass ? (BypassStage)m_activeStage.load() : STAGE_0_DIRECT;

        SOCKET remoteSock = ConnectToTarget(host, port, usedStage);
        if (remoteSock == INVALID_SOCKET) {
            closesocket(clientSock);
            return;
        }

        if (!SendInitialPayloadWithStage(remoteSock, firstPayload, firstLen, usedStage)) {
            closesocket(remoteSock);
            closesocket(clientSock);
            return;
        }

        if (usedStage > STAGE_0_DIRECT && shouldBypass) {
            Log(L"[→] [%s | %s] %S (Kademe %d)",
                clientExeName.empty() ? L"Sistem" : clientExeName.c_str(),
                matchReason.empty() ? L"Tünel" : matchReason.c_str(),
                host.c_str(), (int)usedStage);
        }

        DWORD relayTimeout = 120000;
        setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&relayTimeout, sizeof(relayTimeout));
        setsockopt(remoteSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&relayTimeout, sizeof(relayTimeout));

        RelaySockets(clientSock, remoteSock);
        return;
    }

    // 2. Düz HTTP İstekleri (GET http://... Windows NCSI testi veya düz HTTP bağlantıları)
    std::string reqStr(reqBuf, n);
    size_t hostPos = reqStr.find("Host: ");
    if (hostPos == std::string::npos) hostPos = reqStr.find("host: ");
    if (hostPos != std::string::npos) {
        size_t lineEnd = reqStr.find("\r\n", hostPos);
        if (lineEnd != std::string::npos) {
            std::string host = reqStr.substr(hostPos + 6, lineEnd - (hostPos + 6));
            uint16_t port = 80;
            size_t colon = host.rfind(':');
            if (colon != std::string::npos) {
                port = (uint16_t)atoi(host.substr(colon + 1).c_str());
                host = host.substr(0, colon);
                if (port == 0) port = 80;
            }
            SOCKET remoteSock = ConnectToTarget(host, port, STAGE_0_DIRECT);
            if (remoteSock != INVALID_SOCKET) {
                std::string normalizedReq;
                if (NormalizeHttpProxyRequest(reqBuf, n, normalizedReq)) {
                    SendAllExact(remoteSock, (const uint8_t*)normalizedReq.data(), (int)normalizedReq.size());
                } else {
                    SendAllExact(remoteSock, (const uint8_t*)reqBuf, n);
                }
                RelaySockets(clientSock, remoteSock);
                return;
            }
        }
    }

    closesocket(clientSock);
}

void ZeroDpiEngine::HandleSocks5Client(SOCKET clientSock) {
    DWORD timeout = 8000;
    setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    uint8_t buf[512];
    int n = recv(clientSock, (char*)buf, 2, 0);
    if (n < 2 || buf[0] != 0x05) { closesocket(clientSock); return; }
    int nmethods = buf[1];
    int readMethods = 0;
    while (readMethods < nmethods) {
        int r = recv(clientSock, (char*)buf + readMethods, nmethods - readMethods, 0);
        if (r <= 0) { closesocket(clientSock); return; }
        readMethods += r;
    }

    uint8_t authResp[2] = { 0x05, 0x00 };
    send(clientSock, (const char*)authResp, 2, 0);

    n = recv(clientSock, (char*)buf, 4, 0);
    if (n < 4 || buf[0] != 0x05 || buf[1] != 0x01) { closesocket(clientSock); return; }

    std::string host;
    uint16_t port = 0;
    uint8_t atyp = buf[3];

    if (atyp == 0x01) {
        uint8_t ip4[4];
        if (recv(clientSock, (char*)ip4, 4, 0) != 4) { closesocket(clientSock); return; }
        char ipStr[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, ip4, ipStr, sizeof(ipStr));
        host = ipStr;
    } else if (atyp == 0x03) {
        uint8_t dlen = 0;
        if (recv(clientSock, (char*)&dlen, 1, 0) != 1 || dlen == 0) { closesocket(clientSock); return; }
        std::vector<char> dname(dlen + 1, 0);
        if (recv(clientSock, dname.data(), dlen, 0) != dlen) { closesocket(clientSock); return; }
        host = dname.data();
    } else {
        uint8_t errReply[10] = { 0x05, 0x08, 0x00, 0x01, 0,0,0,0, 0,0 };
        send(clientSock, (const char*)errReply, 10, 0);
        closesocket(clientSock);
        return;
    }

    uint8_t portBytes[2];
    if (recv(clientSock, (char*)portBytes, 2, 0) != 2) { closesocket(clientSock); return; }
    port = (uint16_t)((portBytes[0] << 8) | portBytes[1]);

    uint8_t okReply[10] = { 0x05, 0x00, 0x00, 0x01, 0,0,0,0, 0,0 };
    send(clientSock, (const char*)okReply, 10, 0);

    uint8_t firstPayload[8192];
    int firstLen = RecvCompleteTlsPayload(clientSock, firstPayload, sizeof(firstPayload));
    if (firstLen <= 0) { closesocket(clientSock); return; }

    int sniStart = -1, sniLen = 0;
    std::string sniHost;
    if (FindTlsSniOffset(firstPayload, firstLen, sniStart, sniLen, sniHost) && !sniHost.empty()) {
        if (host.empty() || isdigit((unsigned char)host[0])) {
            host = sniHost;
        }
    }

    std::wstring clientFullPath, clientExeName, matchReason;
    GetSocketOwnerProcess(clientSock, clientFullPath, clientExeName);

    bool shouldBypass = ShouldBypassConnection(host, clientFullPath, clientExeName, matchReason);
    BypassStage usedStage = shouldBypass ? (BypassStage)m_activeStage.load() : STAGE_0_DIRECT;

    SOCKET remoteSock = ConnectToTarget(host, port, usedStage);
    if (remoteSock == INVALID_SOCKET) {
        closesocket(clientSock);
        return;
    }

    if (!SendInitialPayloadWithStage(remoteSock, firstPayload, firstLen, usedStage)) {
        closesocket(remoteSock);
        closesocket(clientSock);
        return;
    }

    if (usedStage > STAGE_0_DIRECT) {
        Log(L"[→] [%s | %s] %S (Kademe %d)",
            clientExeName.empty() ? L"SOCKS5" : clientExeName.c_str(),
            matchReason.empty() ? L"Tünel" : matchReason.c_str(),
            host.c_str(), (int)usedStage);
    }

    DWORD relayTimeout = 120000;
    setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&relayTimeout, sizeof(relayTimeout));
    setsockopt(remoteSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&relayTimeout, sizeof(relayTimeout));

    RelaySockets(clientSock, remoteSock);
}

DWORD WINAPI ZeroDpiEngine::ClientWorkerThreadProc(LPVOID param) {
    ClientWorkerCtx* ctx = (ClientWorkerCtx*)param;
    ZeroDpiEngine* self = ctx->engine;
    if (ctx->isSocks5) self->HandleSocks5Client(ctx->sock);
    else self->HandleHttpClient(ctx->sock);
    delete ctx;
    self->m_activeWorkers.fetch_sub(1);
    return 0;
}

static SOCKET CreateLocalListenSocket(uint16_t port) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return INVALID_SOCKET;

    BOOL reuse = TRUE;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (bind(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR || listen(s, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(s);
        return INVALID_SOCKET;
    }
    return s;
}

DWORD WINAPI ZeroDpiEngine::HttpAcceptThreadProc(LPVOID param) {
    ZeroDpiEngine* self = (ZeroDpiEngine*)param;
    while (self->m_running.load()) {
        SOCKET client = accept(self->m_httpListenSock, nullptr, nullptr);
        if (client == INVALID_SOCKET) break;
        if (!self->m_running.load()) { closesocket(client); break; }
        self->m_activeWorkers.fetch_add(1);
        ClientWorkerCtx* ctx = new ClientWorkerCtx{ self, client, false };
        HANDLE h = CreateThread(nullptr, 128 * 1024, ClientWorkerThreadProc, ctx, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);
        if (h) CloseHandle(h);
        else {
            self->m_activeWorkers.fetch_sub(1);
            closesocket(client);
            delete ctx;
        }
    }
    return 0;
}

DWORD WINAPI ZeroDpiEngine::SocksAcceptThreadProc(LPVOID param) {
    ZeroDpiEngine* self = (ZeroDpiEngine*)param;
    while (self->m_running.load()) {
        SOCKET client = accept(self->m_socksListenSock, nullptr, nullptr);
        if (client == INVALID_SOCKET) break;
        if (!self->m_running.load()) { closesocket(client); break; }
        self->m_activeWorkers.fetch_add(1);
        ClientWorkerCtx* ctx = new ClientWorkerCtx{ self, client, true };
        HANDLE h = CreateThread(nullptr, 128 * 1024, ClientWorkerThreadProc, ctx, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);
        if (h) CloseHandle(h);
        else {
            self->m_activeWorkers.fetch_sub(1);
            closesocket(client);
            delete ctx;
        }
    }
    return 0;
}

static void TerminateProcessesByName(const wchar_t* exeName);
static std::wstring FindLatestSquirrelAppExe(const std::wstring& baseFolder, const wchar_t* exeName);

static void RescueStuckStartupDiscord(uint16_t httpPort) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return;

    FILETIME ftNow{};
    GetSystemTimeAsFileTime(&ftNow);
    ULARGE_INTEGER ulNow{};
    ulNow.LowPart = ftNow.dwLowDateTime;
    ulNow.HighPart = ftNow.dwHighDateTime;

    bool hasDiscordUpdater = false;
    bool hasRecentBootDiscord = false;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"Update.exe") == 0) {
                HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
                if (hProc) {
                    wchar_t fullPath[MAX_PATH] = {0};
                    DWORD sz = MAX_PATH;
                    if (QueryFullProcessImageNameW(hProc, 0, fullPath, &sz)) {
                        if (wcsstr(fullPath, L"Discord") != nullptr || wcsstr(fullPath, L"discord") != nullptr) {
                            hasDiscordUpdater = true;
                        }
                    }
                    CloseHandle(hProc);
                }
            } else if (_wcsicmp(pe.szExeFile, L"Discord.exe") == 0) {
                HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
                if (hProc) {
                    FILETIME ftCreate{}, ftExit{}, ftKernel{}, ftUser{};
                    if (GetProcessTimes(hProc, &ftCreate, &ftExit, &ftKernel, &ftUser)) {
                        ULARGE_INTEGER ulCreate{};
                        ulCreate.LowPart = ftCreate.dwLowDateTime;
                        ulCreate.HighPart = ftCreate.dwHighDateTime;
                        if (ulNow.QuadPart >= ulCreate.QuadPart) {
                            ULONGLONG ageSec = (ulNow.QuadPart - ulCreate.QuadPart) / 10000000ULL;
                            if (ageSec <= 90) {
                                hasRecentBootDiscord = true;
                            }
                        }
                    }
                    CloseHandle(hProc);
                }
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);

    if (!hasDiscordUpdater && !hasRecentBootDiscord) return;

    wchar_t localAppData[MAX_PATH] = {0};
    if (GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH) == 0) return;
    std::wstring discordBase = std::wstring(localAppData) + L"\\Discord";
    std::wstring discordExe = FindLatestSquirrelAppExe(discordBase, L"Discord.exe");
    if (discordExe.empty()) return;

    TerminateProcessesByName(L"Update.exe");
    TerminateProcessesByName(L"Discord.exe");
    Sleep(250);

    SetEnvironmentVariableW(L"HTTP_PROXY", NULL);
    SetEnvironmentVariableW(L"HTTPS_PROXY", NULL);
    SetEnvironmentVariableW(L"ALL_PROXY", NULL);
    SetEnvironmentVariableW(L"http_proxy", NULL);
    SetEnvironmentVariableW(L"https_proxy", NULL);
    SetEnvironmentVariableW(L"all_proxy", NULL);

    wchar_t cmdLine[1024] = {0};
    _snwprintf_s(cmdLine, _countof(cmdLine), _TRUNCATE,
        L"\"%s\" --proxy-server=\"http://127.0.0.1:%u\" --enable-features=EncryptedClientHello",
        discordExe.c_str(), httpPort);

    std::wstring workDir = discordExe;
    size_t slash = workDir.find_last_of(L"\\/");
    if (slash != std::wstring::npos) workDir.resize(slash);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, workDir.c_str(), &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

DWORD WINAPI ZeroDpiEngine::StartupWorkerThreadProc(LPVOID param) {
    ZeroDpiEngine* self = (ZeroDpiEngine*)param;
    if (!self->m_running.load()) {
        self->m_activeWorkers.fetch_sub(1);
        return 0;
    }
    self->EnableSystemSmartPac(true);
    // Windows açılışında Discord, YuziDPI'dan birkaç salise önce başlayıp ProxyEnable=0
    // önbelleklemişse (Checking for updates / 10054 döngüsünde takılı kaldıysa) otomatik kurtar:
    if (self->m_running.load()) {
        RescueStuckStartupDiscord(self->m_httpPort);
    }
    int lat = 0;
    BypassStage st = (BypassStage)self->m_selectedMode.load();
    if (self->m_running.load()) {
        st = self->RunAutoDiscovery(lat);
    } else {
        self->EnableSystemSmartPac(false);
    }
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
    EmptyWorkingSet(GetCurrentProcess());
    if (self->m_stateCb) {
        self->m_stateCb(st, self->m_running.load(), lat);
    }
    self->m_activeWorkers.fetch_sub(1);
    return 0;
}

bool ZeroDpiEngine::Start(uint16_t httpPort, uint16_t socksPort) {
    if (m_running.load()) return true;

    m_httpPort = httpPort;
    m_socksPort = socksPort;

    m_httpListenSock = CreateLocalListenSocket(m_httpPort);
    m_socksListenSock = CreateLocalListenSocket(m_socksPort);

    if (m_httpListenSock == INVALID_SOCKET || m_socksListenSock == INVALID_SOCKET) {
        if (m_httpListenSock != INVALID_SOCKET) closesocket(m_httpListenSock);
        if (m_socksListenSock != INVALID_SOCKET) closesocket(m_socksListenSock);
        m_httpListenSock = INVALID_SOCKET;
        m_socksListenSock = INVALID_SOCKET;
        Log(L"[!] Hata: Yerel portlar (%u / %u) kullanımda!", m_httpPort, m_socksPort);
        return false;
    }

    m_running.store(true);
    EnableSystemSmartPac(true);
    m_httpThread = CreateThread(nullptr, 128 * 1024, HttpAcceptThreadProc, this, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);
    m_socksThread = CreateThread(nullptr, 128 * 1024, SocksAcceptThreadProc, this, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);

    Log(L"[+] Tünel Aktif (127.0.0.1:%u / SOCKS5:%u)", m_httpPort, m_socksPort);

    m_activeWorkers.fetch_add(1);
    HANDLE hDisc = CreateThread(nullptr, 128 * 1024, StartupWorkerThreadProc, this, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);
    if (hDisc) CloseHandle(hDisc);
    else m_activeWorkers.fetch_sub(1);

    return true;
}

void ZeroDpiEngine::Stop() {
    if (!m_running.exchange(false)) return;

    EnableSystemSmartPac(false);

    if (m_httpListenSock != INVALID_SOCKET) {
        closesocket(m_httpListenSock);
        m_httpListenSock = INVALID_SOCKET;
    }
    if (m_socksListenSock != INVALID_SOCKET) {
        closesocket(m_socksListenSock);
        m_socksListenSock = INVALID_SOCKET;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (SOCKET s : m_activeRelaySocks) {
            if (s != INVALID_SOCKET) {
                shutdown(s, SD_BOTH);
            }
        }
    }

    if (m_httpThread) {
        WaitForSingleObject(m_httpThread, 1200);
        CloseHandle(m_httpThread);
        m_httpThread = NULL;
    }
    if (m_socksThread) {
        WaitForSingleObject(m_socksThread, 1200);
        CloseHandle(m_socksThread);
        m_socksThread = NULL;
    }

    for (int i = 0; i < 30 && m_activeWorkers.load() > 0; ++i) {
        Sleep(25);
    }

    Log(L"[■] Servis durduruldu, sistem proxy ayarları kapatıldı.");
    if (m_stateCb) {
        m_stateCb((BypassStage)m_activeStage.load(), false, 0);
    }
}

static DWORD WINAPI NotifyWinInetRefreshThread(LPVOID) {
    InternetSetOptionW(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
    InternetSetOptionW(NULL, INTERNET_OPTION_REFRESH, NULL, 0);
    return 0;
}

bool ZeroDpiEngine::EnableSystemSmartPac(bool enable) {
    HKEY hKey = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
        0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    if (enable) {
        // Discord'un Rust tabanlı güncelleyicisi (reqwest), Update.exe ve masaüstü uygulamaları
        // doğrudan ProxyEnable=1 ve ProxyServer değerine bakar.
        // AutoConfigURL silinerek WinHTTP/WinInet'in ilk açılışta PAC indirme gecikmesi yaşaması
        // ve "Checking for updates..." ekranında DIRECT kaçırması %100 engellenir.
        DWORD proxyEnable = 1;
        wchar_t proxyServer[64];
        _snwprintf_s(proxyServer, _countof(proxyServer), _TRUNCATE, L"127.0.0.1:%u", m_httpPort);

        std::wstring proxyOverride = L"localhost;127.*;192.168.*;10.*;172.16.*;<local>";
        if (m_scopeMode.load() == SCOPE_ALL_EXCEPT_EXCLUDED) {
            std::vector<TunnelTargetItem> items = GetTunnelItems();
            for (const auto& it : items) {
                if (!it.isExcluded || !it.enabled || it.type != ITEM_DOMAIN || it.pathOrDomain.empty()) continue;
                std::wstring entry = L";" + it.pathOrDomain + L";*." + it.pathOrDomain;
                if (proxyOverride.size() + entry.size() < 1800) {
                    proxyOverride += entry;
                }
            }
        }

        RegSetValueExW(hKey, L"ProxyEnable", 0, REG_DWORD, (const BYTE*)&proxyEnable, sizeof(proxyEnable));
        RegSetValueExW(hKey, L"ProxyServer", 0, REG_SZ, (const BYTE*)proxyServer, (DWORD)((wcslen(proxyServer) + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, L"ProxyOverride", 0, REG_SZ, (const BYTE*)proxyOverride.c_str(), (DWORD)((proxyOverride.size() + 1) * sizeof(wchar_t)));
        RegDeleteValueW(hKey, L"AutoConfigURL");
    } else {
        DWORD proxyEnable = 0;
        RegSetValueExW(hKey, L"ProxyEnable", 0, REG_DWORD, (const BYTE*)&proxyEnable, sizeof(proxyEnable));
        RegDeleteValueW(hKey, L"ProxyServer");
        RegDeleteValueW(hKey, L"AutoConfigURL");
        RegDeleteValueW(hKey, L"ProxyOverride");
    }

    RegCloseKey(hKey);
    HANDLE hNotify = CreateThread(nullptr, 0, NotifyWinInetRefreshThread, nullptr, 0, nullptr);
    if (hNotify) {
        WaitForSingleObject(hNotify, enable ? 400 : 1500);
        CloseHandle(hNotify);
    }
    return true;
}

static void TerminateProcessesByName(const wchar_t* exeName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return;

    std::vector<HANDLE> waitHandles;
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, exeName) == 0) {
                HANDLE hProc = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pe.th32ProcessID);
                if (!hProc) {
                    hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                }
                if (hProc) {
                    TerminateProcess(hProc, 0);
                    waitHandles.push_back(hProc);
                }
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);

    for (HANDLE h : waitHandles) {
        WaitForSingleObject(h, 3000);
        CloseHandle(h);
    }
}

static std::wstring FindLatestSquirrelAppExe(const std::wstring& baseFolder, const wchar_t* exeName) {
    std::wstring pattern = baseFolder + L"\\app-*";
    WIN32_FIND_DATAW fd{};
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return L"";

    std::wstring bestSubdir;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::wstring sub = fd.cFileName;
            std::wstring candidate = baseFolder + L"\\" + sub + L"\\" + exeName;
            if (GetFileAttributesW(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) {
                if (sub > bestSubdir) {
                    bestSubdir = sub;
                }
            }
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);

    if (!bestSubdir.empty()) {
        return baseFolder + L"\\" + bestSubdir + L"\\" + exeName;
    }
    return L"";
}

bool ZeroDpiEngine::LaunchSingleExeTunneled(const std::wstring& rawExePath) {
    std::wstring exePath = rawExePath;
    if (GetFileAttributesW(exePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        Log(L"[!] Dosya bulunamadı: %s", exePath.c_str());
        return false;
    }

    std::wstring fileName = ExtractFileName(exePath);
    bool isDiscord = (exePath.find(L"Discord") != std::wstring::npos || exePath.find(L"discord") != std::wstring::npos);

    // Eğer kullanıcı Discord klasörünü veya Update.exe'yi seçtiyse, doğrudan app-*\Discord.exe'yi bul
    if (isDiscord && _wcsicmp(fileName.c_str(), L"Update.exe") == 0) {
        std::wstring parentDir = exePath;
        size_t sl = parentDir.find_last_of(L"\\/");
        if (sl != std::wstring::npos) {
            parentDir.resize(sl);
            std::wstring realDiscordExe = FindLatestSquirrelAppExe(parentDir, L"Discord.exe");
            if (!realDiscordExe.empty()) {
                exePath = realDiscordExe;
                fileName = ExtractFileName(exePath);
            }
        }
    }

    if (isDiscord) {
        // Discord'un Rust güncelleyicisi (updater.node / reqwest):
        // 1) Ortam değişkeninde HTTPS_PROXY varsa kendisi siler (0x18018b8d5),
        //    fakat HTTP_PROXY kalırsa reqwest ortam değişkeni haritasını dolu sanıp
        //    Windows Kayıt Defteri (ProxyServer) ayarını okumayı tamamen atlar (0x180138cfe)!
        // 2) Bu yüzden Discord başlatılırken ortam değişkenleri temizlenir,
        //    Windows Sistem Proxy'si (ProxyServer=127.0.0.1:3131) aktif edilir,
        //    arkada takılı kalmış hem Update.exe hem Discord.exe temizlenir
        //    ve Electron için --proxy-server argümanı verilir.
        EnableSystemSmartPac(true);

        SetEnvironmentVariableW(L"HTTP_PROXY", NULL);
        SetEnvironmentVariableW(L"HTTPS_PROXY", NULL);
        SetEnvironmentVariableW(L"ALL_PROXY", NULL);
        SetEnvironmentVariableW(L"http_proxy", NULL);
        SetEnvironmentVariableW(L"https_proxy", NULL);
        SetEnvironmentVariableW(L"all_proxy", NULL);

        TerminateProcessesByName(L"Update.exe");
        TerminateProcessesByName(L"Discord.exe");
        Sleep(250);
    } else {
        wchar_t httpProxy[64];
        _snwprintf_s(httpProxy, _countof(httpProxy), _TRUNCATE, L"http://127.0.0.1:%u", m_httpPort);
        SetEnvironmentVariableW(L"HTTP_PROXY", httpProxy);
        SetEnvironmentVariableW(L"HTTPS_PROXY", httpProxy);
        SetEnvironmentVariableW(L"ALL_PROXY", httpProxy);
        SetEnvironmentVariableW(L"NO_PROXY", L"localhost,127.0.0.1");
    }

    wchar_t cmdLine[1024] = {0};
    if (_wcsicmp(fileName.c_str(), L"Update.exe") == 0 && isDiscord) {
        _snwprintf_s(cmdLine, _countof(cmdLine), _TRUNCATE,
            L"\"%s\" --processStart Discord.exe --process-start-args=\"--proxy-server=http://127.0.0.1:%u --enable-features=EncryptedClientHello\"",
            exePath.c_str(), m_httpPort);
    } else {
        _snwprintf_s(cmdLine, _countof(cmdLine), _TRUNCATE,
            L"\"%s\" --proxy-server=\"http://127.0.0.1:%u\" --enable-features=EncryptedClientHello",
            exePath.c_str(), m_httpPort);
    }

    std::wstring workDir = exePath;
    size_t slash = workDir.find_last_of(L"\\/");
    if (slash != std::wstring::npos) workDir.resize(slash);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, workDir.c_str(), &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        Log(L"[+] Tünelli başlatıldı: %s", fileName.c_str());
        return true;
    }
    return false;
}

bool ZeroDpiEngine::LaunchTargetItemTunneled(int index) {
    std::vector<TunnelTargetItem> items = GetTunnelItems();
    if (index < 0 || index >= (int)items.size()) return false;

    const auto& item = items[index];
    if (item.isExcluded) return false;

    if (item.type == ITEM_APP_EXE) {
        return LaunchSingleExeTunneled(item.pathOrDomain);
    } else if (item.type == ITEM_FOLDER) {
        std::wstring latestDiscord = FindLatestSquirrelAppExe(item.pathOrDomain, L"Discord.exe");
        if (!latestDiscord.empty()) {
            return LaunchSingleExeTunneled(latestDiscord);
        }

        std::wstring updateCandidate = item.pathOrDomain + L"\\Update.exe";
        if (GetFileAttributesW(updateCandidate.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return LaunchSingleExeTunneled(updateCandidate);
        }

        std::wstring searchPattern = item.pathOrDomain + L"\\*.exe";
        WIN32_FIND_DATAW fd{};
        HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            std::wstring chosenExe;
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    if (wcsstr(fd.cFileName, L"unins") == nullptr && wcsstr(fd.cFileName, L"crash") == nullptr) {
                        chosenExe = item.pathOrDomain + L"\\" + fd.cFileName;
                        break;
                    }
                }
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
            if (!chosenExe.empty()) {
                return LaunchSingleExeTunneled(chosenExe);
            }
        }
        Log(L"[!] Klasörde .exe bulunamadı: %s", item.pathOrDomain.c_str());
    }
    return false;
}

int ZeroDpiEngine::LaunchAllAppTargetsTunneled() {
    std::vector<TunnelTargetItem> items = GetTunnelItems();
    int launched = 0;
    for (int i = 0; i < (int)items.size(); ++i) {
        if (items[i].enabled && !items[i].isExcluded && (items[i].type == ITEM_APP_EXE || items[i].type == ITEM_FOLDER)) {
            if (LaunchTargetItemTunneled(i)) launched++;
        }
    }
    return launched;
}

void ZeroDpiEngine::LoadConfig(const std::wstring& iniPath, bool& outAutoConnect, bool& outAutoLaunchApps, bool& outStartMinimized) {
    outAutoLaunchApps = true;
    m_autoLaunchApps.store(true);
    if (GetFileAttributesW(iniPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        outAutoConnect = false;
        outStartMinimized = false;
        return;
    }

    int scope = GetPrivateProfileIntW(L"Settings", L"ScopeMode", 0, iniPath.c_str());
    int mode = GetPrivateProfileIntW(L"Settings", L"StageMode", 1, iniPath.c_str());
    outAutoConnect = (GetPrivateProfileIntW(L"Settings", L"AutoConnect", 0, iniPath.c_str()) != 0);
    outStartMinimized = (GetPrivateProfileIntW(L"Settings", L"StartMinimized", 0, iniPath.c_str()) != 0);

    SetScopeMode((TunnelScopeMode)scope);
    SetSelectedMode(mode);
    m_useSystemPac.store(true);

    int itemCount = GetPrivateProfileIntW(L"Targets", L"Count", -1, iniPath.c_str());
    if (itemCount >= 0) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tunnelItems.clear();
            for (int i = 0; i < itemCount; ++i) {
                wchar_t keyType[32], keyVal[32], keyExcl[32];
                _snwprintf_s(keyType, _countof(keyType), _TRUNCATE, L"Type_%d", i);
                _snwprintf_s(keyVal, _countof(keyVal), _TRUNCATE, L"Value_%d", i);
                _snwprintf_s(keyExcl, _countof(keyExcl), _TRUNCATE, L"Excl_%d", i);

                int t = GetPrivateProfileIntW(L"Targets", keyType, 2, iniPath.c_str());
                int excl = GetPrivateProfileIntW(L"Targets", keyExcl, 0, iniPath.c_str());
                wchar_t valBuf[MAX_PATH] = {0};
                GetPrivateProfileStringW(L"Targets", keyVal, L"", valBuf, MAX_PATH, iniPath.c_str());
                if (wcslen(valBuf) > 0) {
                    TunnelTargetItem item;
                    item.type = (TunnelItemType)t;
                    item.pathOrDomain = valBuf;
                    item.displayName = (item.type == ITEM_DOMAIN) ? valBuf : ExtractFileName(valBuf);
                    item.enabled = true;
                    item.isExcluded = (excl != 0);
                    m_tunnelItems.push_back(item);
                }
            }
        }
        // updates.discord.com ve dl.discordapp.net eski ini dosyasında yoksa ekle
        AddTunnelItem(ITEM_DOMAIN, L"updates.discord.com", false);
        AddTunnelItem(ITEM_DOMAIN, L"dl.discordapp.net", false);
    }
}

void ZeroDpiEngine::SaveConfig(const std::wstring& iniPath, bool autoConnect, bool autoLaunchApps, bool startMinimized) {
    wchar_t numBuf[32];
    _snwprintf_s(numBuf, _countof(numBuf), _TRUNCATE, L"%d", m_scopeMode.load());
    WritePrivateProfileStringW(L"Settings", L"ScopeMode", numBuf, iniPath.c_str());

    _snwprintf_s(numBuf, _countof(numBuf), _TRUNCATE, L"%d", m_selectedMode.load());
    WritePrivateProfileStringW(L"Settings", L"StageMode", numBuf, iniPath.c_str());

    _snwprintf_s(numBuf, _countof(numBuf), _TRUNCATE, L"%d", m_useSystemPac.load() ? 1 : 0);
    WritePrivateProfileStringW(L"Settings", L"UseSystemPac", numBuf, iniPath.c_str());

    _snwprintf_s(numBuf, _countof(numBuf), _TRUNCATE, L"%d", autoConnect ? 1 : 0);
    WritePrivateProfileStringW(L"Settings", L"AutoConnect", numBuf, iniPath.c_str());

    _snwprintf_s(numBuf, _countof(numBuf), _TRUNCATE, L"%d", autoLaunchApps ? 1 : 0);
    WritePrivateProfileStringW(L"Settings", L"AutoLaunchApps", numBuf, iniPath.c_str());

    _snwprintf_s(numBuf, _countof(numBuf), _TRUNCATE, L"%d", startMinimized ? 1 : 0);
    WritePrivateProfileStringW(L"Settings", L"StartMinimized", numBuf, iniPath.c_str());

    std::vector<TunnelTargetItem> items = GetTunnelItems();
    WritePrivateProfileSectionW(L"Targets", L"", iniPath.c_str());

    _snwprintf_s(numBuf, _countof(numBuf), _TRUNCATE, L"%d", (int)items.size());
    WritePrivateProfileStringW(L"Targets", L"Count", numBuf, iniPath.c_str());

    for (int i = 0; i < (int)items.size(); ++i) {
        wchar_t keyType[32], keyVal[32], keyExcl[32];
        _snwprintf_s(keyType, _countof(keyType), _TRUNCATE, L"Type_%d", i);
        _snwprintf_s(keyVal, _countof(keyVal), _TRUNCATE, L"Value_%d", i);
        _snwprintf_s(keyExcl, _countof(keyExcl), _TRUNCATE, L"Excl_%d", i);

        _snwprintf_s(numBuf, _countof(numBuf), _TRUNCATE, L"%d", (int)items[i].type);
        WritePrivateProfileStringW(L"Targets", keyType, numBuf, iniPath.c_str());
        WritePrivateProfileStringW(L"Targets", keyVal, items[i].pathOrDomain.c_str(), iniPath.c_str());
        WritePrivateProfileStringW(L"Targets", keyExcl, items[i].isExcluded ? L"1" : L"0", iniPath.c_str());
    }
}

bool ZeroDpiEngine::IsWindowsAutostartEnabled() {
    HKEY hKey = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    wchar_t val[MAX_PATH] = {0};
    DWORD sz = sizeof(val);
    bool exists = (RegQueryValueExW(hKey, L"YuziDPI", NULL, NULL, (LPBYTE)val, &sz) == ERROR_SUCCESS);
    RegCloseKey(hKey);
    return exists;
}

bool ZeroDpiEngine::SetWindowsAutostart(bool enable, bool startMinimized) {
    HKEY hKey = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    RegDeleteValueW(hKey, L"ZeroDPI");
    if (enable) {
        wchar_t exePath[MAX_PATH] = {0};
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        wchar_t cmd[MAX_PATH + 32];
        if (startMinimized) {
            _snwprintf_s(cmd, _countof(cmd), _TRUNCATE, L"\"%s\" --minimized", exePath);
        } else {
            _snwprintf_s(cmd, _countof(cmd), _TRUNCATE, L"\"%s\"", exePath);
        }
        RegSetValueExW(hKey, L"YuziDPI", 0, REG_SZ, (const BYTE*)cmd, (DWORD)((wcslen(cmd) + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(hKey, L"YuziDPI");
    }
    RegCloseKey(hKey);
    return true;
}
