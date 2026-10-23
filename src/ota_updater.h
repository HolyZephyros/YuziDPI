#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <cstdio>

#define YUZIDPI_VERSION_STR  "1.0.0"
#define YUZIDPI_VERSION_WSTR L"v1.0.0"
#define WM_OTA_RESTART       (WM_USER + 150)

static inline std::wstring GetCurrentExeFullPath() {
    wchar_t buf[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, buf, MAX_PATH);
    return std::wstring(buf);
}

static inline void CleanupOldOtaFiles() {
    std::wstring exePath = GetCurrentExeFullPath();
    if (exePath.empty()) return;
    std::wstring oldPath = exePath + L".old";
    std::wstring updPath = exePath + L".update";
    DeleteFileW(oldPath.c_str());
    DeleteFileW(updPath.c_str());
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

static inline std::string FindExeDownloadUrlInReleaseJson(const std::string& json) {
    size_t pos = 0;
    while (true) {
        std::string url = ExtractJsonStringField(json, "browser_download_url", pos);
        if (url.empty()) break;
        if (url.find(".exe") != std::string::npos || url.find("YuziDPI") != std::string::npos) {
            return url;
        }
        size_t next = json.find(url, pos);
        if (next == std::string::npos) break;
        pos = next + url.size();
    }
    return "";
}

static inline std::string HttpGetTextWinHttp(const wchar_t* host, const wchar_t* path) {
    std::string result;
    HINTERNET hSession = WinHttpOpen(L"YuziDPI-OTA/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return result;

    WinHttpSetTimeouts(hSession, 6000, 6000, 8000, 8000);

    HINTERNET hConnect = WinHttpConnect(hSession, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (hConnect) {
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path,
            NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (hRequest) {
            DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
            WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

            const wchar_t* hdrs = L"Accept: application/vnd.github+json\r\nUser-Agent: YuziDPI-OTA/1.0\r\n";
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
                    } while (dwSize > 0 && result.size() < 512 * 1024);
                }
            }
            WinHttpCloseHandle(hRequest);
        }
        WinHttpCloseHandle(hConnect);
    }
    WinHttpCloseHandle(hSession);
    return result;
}

static inline bool DownloadBinaryToFileWinHttp(const std::wstring& urlW, const std::wstring& destPath) {
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    wchar_t hostName[256] = {0};
    wchar_t urlPath[2048] = {0};
    uc.lpszHostName = hostName;
    uc.dwHostNameLength = _countof(hostName);
    uc.lpszUrlPath = urlPath;
    uc.dwUrlPathLength = _countof(urlPath);

    if (!WinHttpCrackUrl(urlW.c_str(), (DWORD)urlW.size(), 0, &uc)) {
        return false;
    }

    HINTERNET hSession = WinHttpOpen(L"YuziDPI-OTA/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    WinHttpSetTimeouts(hSession, 8000, 8000, 15000, 30000);

    bool ok = false;
    HINTERNET hConnect = WinHttpConnect(hSession, hostName, uc.nPort, 0);
    if (hConnect) {
        DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath,
            NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (hRequest) {
            DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
            WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

            if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                WinHttpReceiveResponse(hRequest, NULL)) {
                DWORD statusCode = 0;
                DWORD sz = sizeof(statusCode);
                WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                    WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &sz, WINHTTP_NO_HEADER_INDEX);

                if (statusCode == 200) {
                    HANDLE hFile = CreateFileW(destPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hFile != INVALID_HANDLE_VALUE) {
                        DWORD totalWritten = 0;
                        BYTE firstTwo[2] = {0, 0};
                        DWORD dwSize = 0;
                        bool writeErr = false;

                        do {
                            dwSize = 0;
                            if (!WinHttpQueryDataAvailable(hRequest, &dwSize) || dwSize == 0) break;
                            std::vector<BYTE> buf(dwSize);
                            DWORD dwRead = 0;
                            if (WinHttpReadData(hRequest, buf.data(), dwSize, &dwRead) && dwRead > 0) {
                                if (totalWritten == 0 && dwRead >= 2) {
                                    firstTwo[0] = buf[0];
                                    firstTwo[1] = buf[1];
                                }
                                DWORD dwWritten = 0;
                                if (!WriteFile(hFile, buf.data(), dwRead, &dwWritten, NULL) || dwWritten != dwRead) {
                                    writeErr = true;
                                    break;
                                }
                                totalWritten += dwWritten;
                            }
                        } while (dwSize > 0);

                        CloseHandle(hFile);

                        // Geçerli bir Windows PE (.exe) başlığı ('MZ') ve en az 100 KB boyut doğrulaması
                        if (!writeErr && totalWritten > 100 * 1024 && firstTwo[0] == 'M' && firstTwo[1] == 'Z') {
                            ok = true;
                        } else {
                            DeleteFileW(destPath.c_str());
                        }
                    }
                }
            }
            WinHttpCloseHandle(hRequest);
        }
        WinHttpCloseHandle(hConnect);
    }
    WinHttpCloseHandle(hSession);
    return ok;
}

static inline DWORD WINAPI SilentOtaWorkerThreadProc(LPVOID param) {
    HWND hMainWnd = (HWND)param;
    // Başlangıçta ağ ve yerel tünelin oturması için kısa bekleme
    Sleep(4500);

    std::string json = HttpGetTextWinHttp(L"api.github.com", L"/repos/HolyZephyros/YuziDPI/releases/latest");
    if (json.empty()) return 0;

    std::string latestTag = ExtractJsonStringField(json, "tag_name");
    if (latestTag.empty() || !IsRemoteVersionNewer(YUZIDPI_VERSION_STR, latestTag)) {
        return 0;
    }

    std::string downloadUrl = FindExeDownloadUrlInReleaseJson(json);
    if (downloadUrl.empty()) return 0;

    int wlen = MultiByteToWideChar(CP_UTF8, 0, downloadUrl.c_str(), (int)downloadUrl.size(), NULL, 0);
    if (wlen <= 0) return 0;
    std::wstring downloadUrlW(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, downloadUrl.c_str(), (int)downloadUrl.size(), &downloadUrlW[0], wlen);

    std::wstring exePath = GetCurrentExeFullPath();
    std::wstring updPath = exePath + L".update";
    std::wstring oldPath = exePath + L".old";

    if (!DownloadBinaryToFileWinHttp(downloadUrlW, updPath)) {
        return 0;
    }

    // Çalışan exe'yi .old olarak yeniden adlandır ve yeni indirilen .update dosyasını asıl exe yerine taşı
    DeleteFileW(oldPath.c_str());
    if (!MoveFileExW(exePath.c_str(), oldPath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        DeleteFileW(updPath.c_str());
        return 0;
    }

    if (!MoveFileExW(updPath.c_str(), exePath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        // Geri al
        MoveFileExW(oldPath.c_str(), exePath.c_str(), MOVEFILE_REPLACE_EXISTING);
        return 0;
    }

    if (hMainWnd && IsWindow(hMainWnd)) {
        PostMessageW(hMainWnd, WM_OTA_RESTART, 0, 0);
    }
    return 0;
}
