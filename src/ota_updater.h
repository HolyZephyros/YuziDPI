#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <mutex>
#include <cstdio>

#define YUZIDPI_VERSION_STR   "1.1.0"
#define YUZIDPI_VERSION_WSTR  L"v1.1.0"
#define WM_OTA_AVAILABLE      (WM_USER + 150)

struct UpdateNoticeInfo {
    std::wstring tag;
    std::wstring pageUrl;
};

static inline std::mutex& GetUpdateNoticeMutex() {
    static std::mutex mtx;
    return mtx;
}

static inline UpdateNoticeInfo& GetUpdateNoticeStorage() {
    static UpdateNoticeInfo info;
    return info;
}

static inline bool GetAvailableUpdateInfo(std::wstring& outTag, std::wstring& outUrl) {
    std::lock_guard<std::mutex> lock(GetUpdateNoticeMutex());
    const auto& info = GetUpdateNoticeStorage();
    if (info.tag.empty() || info.pageUrl.empty()) return false;
    outTag = info.tag;
    outUrl = info.pageUrl;
    return true;
}

static inline void ParseSemVer(const std::string& s, int& maj, int& min, int& pat) {
    maj = 0; min = 0; pat = 0;
    size_t i = 0;
    while (i < s.size() && (s[i] < '0' || s[i] > '9')) ++i;
    if (i < s.size()) {
        sscanf_s(s.c_str() + i, "%d.%d.%d", &maj, &min, &pat);
    }
}

static inline bool IsRemoteVersionNewer(const std::string& localVer, const std::string& remoteTag) {
    int lMaj = 0, lMin = 0, lPat = 0;
    int rMaj = 0, rMin = 0, rPat = 0;
    ParseSemVer(localVer, lMaj, lMin, lPat);
    ParseSemVer(remoteTag, rMaj, rMin, rPat);
    if (rMaj != lMaj) return rMaj > lMaj;
    if (rMin != lMin) return rMin > lMin;
    return rPat > lPat;
}

static inline std::string ExtractJsonStringField(const std::string& json, const char* key, size_t startPos = 0) {
    std::string pattern = std::string("\"") + key + "\"";
    size_t pos = json.find(pattern, startPos);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + pattern.size());
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return "";
    size_t endPos = json.find('"', pos + 1);
    if (endPos == std::string::npos) return "";
    return json.substr(pos + 1, endPos - (pos + 1));
}

static inline std::wstring Utf8ToWideSimple(const std::string& s) {
    if (s.empty()) return L"";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    if (wlen <= 0) return L"";
    std::wstring out(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], wlen);
    return out;
}

static inline std::string HttpGetReleaseJson(const wchar_t* host, const wchar_t* path) {
    const DWORD accessTypes[2] = { WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY };
    for (int attempt = 0; attempt < 2; ++attempt) {
        std::string result;
        HINTERNET hSession = WinHttpOpen(L"YuziDPI/1.1",
            accessTypes[attempt], WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) continue;

        WinHttpSetTimeouts(hSession, 5000, 5000, 7000, 7000);

        HINTERNET hConnect = WinHttpConnect(hSession, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (hConnect) {
            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path,
                NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
            if (hRequest) {
                DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
                WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

                const wchar_t* hdrs = L"Accept: application/vnd.github+json\r\nUser-Agent: YuziDPI/1.1\r\n";
                if (WinHttpSendRequest(hRequest, hdrs, (DWORD)-1L, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                    WinHttpReceiveResponse(hRequest, NULL)) {
                    DWORD statusCode = 0;
                    DWORD sz = sizeof(statusCode);
                    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &sz, WINHTTP_NO_HEADER_INDEX);
                    if (statusCode == 200) {
                        DWORD dwSize = 0;
                        do {
                            dwSize = 0;
                            if (!WinHttpQueryDataAvailable(hRequest, &dwSize) || dwSize == 0) break;
                            std::vector<char> buf(dwSize);
                            DWORD dwRead = 0;
                            if (WinHttpReadData(hRequest, buf.data(), dwSize, &dwRead) && dwRead > 0) {
                                result.append(buf.data(), dwRead);
                            }
                        } while (dwSize > 0 && result.size() < 256 * 1024);
                    }
                }
                WinHttpCloseHandle(hRequest);
            }
            WinHttpCloseHandle(hConnect);
        }
        WinHttpCloseHandle(hSession);
        if (!result.empty()) return result;
    }
    return "";
}

// Yalnızca ilk açılışta 1 kez yeni sürüm kontrolü yapar (diske hiçbir dosya yazmaz, kendini değiştirmez)
static inline DWORD WINAPI SilentOtaWorkerThreadProc(LPVOID param) {
    HWND hMainWnd = (HWND)param;
    Sleep(1500);

    std::string json = HttpGetReleaseJson(L"api.github.com", L"/repos/HolyZephyros/YuziDPI/releases/latest");
    if (!json.empty()) {
        std::string latestTag = ExtractJsonStringField(json, "tag_name");
        if (!latestTag.empty() && IsRemoteVersionNewer(YUZIDPI_VERSION_STR, latestTag)) {
            std::string pageUrl = ExtractJsonStringField(json, "html_url");
            if (pageUrl.empty()) {
                pageUrl = "https://github.com/HolyZephyros/YuziDPI/releases/latest";
            }
            {
                std::lock_guard<std::mutex> lock(GetUpdateNoticeMutex());
                GetUpdateNoticeStorage().tag = Utf8ToWideSimple(latestTag);
                GetUpdateNoticeStorage().pageUrl = Utf8ToWideSimple(pageUrl);
            }
            if (hMainWnd && IsWindow(hMainWnd)) {
                PostMessageW(hMainWnd, WM_OTA_AVAILABLE, 0, 0);
            }
        }
    }

    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
    EmptyWorkingSet(GetCurrentProcess());
    return 0;
}
