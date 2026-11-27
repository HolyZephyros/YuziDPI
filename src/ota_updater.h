#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <cstdio>

#define YUZIDPI_VERSION_STR  "1.0.9"
#define YUZIDPI_VERSION_WSTR L"v1.0.9"
#define WM_OTA_RESTART       (WM_USER + 150)

static inline std::wstring GetCurrentExeFullPath() {
    wchar_t buf[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, buf, MAX_PATH);
    return std::wstring(buf);
}

static inline bool MoveFileWithRetry(const std::wstring& src, const std::wstring& dst, int maxAttempts = 12, DWORD delayMs = 150) {
    for (int i = 0; i < maxAttempts; ++i) {
        if (MoveFileExW(src.c_str(), dst.c_str(), MOVEFILE_REPLACE_EXISTING)) {
            return true;
        }
        Sleep(delayMs);
    }
    return false;
}

static inline bool VerifyDownloadedPeBinary(const std::wstring& filePath) {
    // Antivirüs gerçek zamanlı taramasının yeni yazılan dosyayı incelemesi için bekle;
    // dosya karantinaya alındıysa veya kilitli/bozuksa asıl YuziDPI.exe'ye hiç dokunma!
    Sleep(1500);

    HANDLE hFile = INVALID_HANDLE_VALUE;
    for (int i = 0; i < 12; ++i) {
        hFile = CreateFileW(filePath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) break;
        DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND || err == 225 /* ERROR_VIRUS_INFECTED */) {
            return false;
        }
        Sleep(200);
    }
    if (hFile == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER fileSize{};
    bool valid = false;
    if (GetFileSizeEx(hFile, &fileSize) && fileSize.QuadPart > 100 * 1024 && fileSize.QuadPart < 50 * 1024 * 1024) {
        BYTE hdr[4096] = {0};
        DWORD bytesRead = 0;
        if (ReadFile(hFile, hdr, sizeof(hdr), &bytesRead, NULL) && bytesRead >= 64) {
            if (hdr[0] == 'M' && hdr[1] == 'Z') {
                DWORD peOff = *(DWORD*)(hdr + 0x3C);
                if (peOff <= bytesRead - 4) {
                    valid = (hdr[peOff] == 'P' && hdr[peOff + 1] == 'E' && hdr[peOff + 2] == 0 && hdr[peOff + 3] == 0);
                } else if (peOff < (DWORD)fileSize.QuadPart - 4) {
                    LARGE_INTEGER seekPos{};
                    seekPos.QuadPart = peOff;
                    BYTE peSig[4] = {0};
                    DWORD sigRead = 0;
                    if (SetFilePointerEx(hFile, seekPos, NULL, FILE_BEGIN) &&
                        ReadFile(hFile, peSig, 4, &sigRead, NULL) && sigRead == 4) {
                        valid = (peSig[0] == 'P' && peSig[1] == 'E' && peSig[2] == 0 && peSig[3] == 0);
                    }
                }
            }
        }
    }
    CloseHandle(hFile);
    return valid;
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
    const DWORD accessTypes[2] = { WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY };
    for (int attempt = 0; attempt < 2; ++attempt) {
        std::string result;
        HINTERNET hSession = WinHttpOpen(L"YuziDPI-OTA/1.0",
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
        if (!result.empty()) return result;
    }
    return "";
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

    const DWORD accessTypes[2] = { WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY };
    for (int attempt = 0; attempt < 2; ++attempt) {
        HINTERNET hSession = WinHttpOpen(L"YuziDPI-OTA/1.0",
            accessTypes[attempt], WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) continue;

        WinHttpSetTimeouts(hSession, 6000, 6000, 15000, 30000);

        bool ok = false;
        HINTERNET hConnect = WinHttpConnect(hSession, hostName, uc.nPort, 0);
        if (hConnect) {
            DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath,
                NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
            if (hRequest) {
                DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
                WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

                const wchar_t* hdrs = L"User-Agent: YuziDPI-OTA/1.0\r\nAccept: */*\r\n";
                if (WinHttpSendRequest(hRequest, hdrs, (DWORD)-1L, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
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
        if (ok) return true;
    }
    return false;
}

static inline DWORD WINAPI SilentOtaWorkerThreadProc(LPVOID param) {
    HWND hMainWnd = (HWND)param;
    // Başlangıçta arayüzün anında açılması için kısa bekleme (1.5 sn)
    Sleep(1500);

    std::string json = HttpGetTextWinHttp(L"api.github.com", L"/repos/HolyZephyros/YuziDPI/releases/latest");
    if (json.empty()) {
        SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
        EmptyWorkingSet(GetCurrentProcess());
        return 0;
    }

    std::string latestTag = ExtractJsonStringField(json, "tag_name");
    if (latestTag.empty() || !IsRemoteVersionNewer(YUZIDPI_VERSION_STR, latestTag)) {
        SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
        EmptyWorkingSet(GetCurrentProcess());
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

    // Antivirüs taramasından sağlam çıkıp tam PE başlığı doğrulanmadan asıl .exe'ye DOKUNMA!
    if (!VerifyDownloadedPeBinary(updPath)) {
        DeleteFileW(updPath.c_str());
        return 0;
    }

    // Çalışan exe'yi .old olarak yedekle ve yeni indirilen .update dosyasını asıl exe yerine taşı
    DeleteFileW(oldPath.c_str());
    if (!MoveFileWithRetry(exePath, oldPath, 10, 150)) {
        DeleteFileW(updPath.c_str());
        return 0;
    }

    if (!MoveFileWithRetry(updPath, exePath, 12, 150)) {
        // Kesin geri alma (rollback) — asıl exe asla kaybolmaz
        MoveFileWithRetry(oldPath, exePath, 20, 150);
        DeleteFileW(updPath.c_str());
        return 0;
    }

    // İsim değişikliği sonrası antivirüs anlık karantina kontrolü
    Sleep(300);
    if (GetFileAttributesW(exePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        MoveFileWithRetry(oldPath, exePath, 20, 150);
        return 0;
    }

    if (hMainWnd && IsWindow(hMainWnd)) {
        PostMessageW(hMainWnd, WM_OTA_RESTART, 0, 0);
    }
    return 0;
}

