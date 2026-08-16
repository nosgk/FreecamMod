#include "gui/i18n.h"

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "imgui.h"

#include "utils/debug.h"

namespace I18N {
    namespace {
        std::unordered_map<std::string, std::string> g_map;
        bool g_mapLoaded = false;
        bool g_fontLoaded = false;

        // ---- 极简 JSON 解析：仅支持扁平对象 {"key":"value",...}（够用且零依赖）----

        bool ParseHex4(const std::string& s, size_t pos, unsigned& out) {
            out = 0;
            for (size_t k = 0; k < 4; ++k) {
                char c = s[pos + k];
                out <<= 4;
                if (c >= '0' && c <= '9') out |= (unsigned)(c - '0');
                else if (c >= 'a' && c <= 'f') out |= (unsigned)(c - 'a' + 10);
                else if (c >= 'A' && c <= 'F') out |= (unsigned)(c - 'A' + 10);
                else return false;
            }
            return true;
        }

        void AppendUtf8(std::string& out, unsigned cp) {
            if (cp < 0x80) {
                out += (char)cp;
            }
            else if (cp < 0x800) {
                out += (char)(0xC0 | (cp >> 6));
                out += (char)(0x80 | (cp & 0x3F));
            }
            else if (cp < 0x10000) {
                out += (char)(0xE0 | (cp >> 12));
                out += (char)(0x80 | ((cp >> 6) & 0x3F));
                out += (char)(0x80 | (cp & 0x3F));
            }
            else {
                out += (char)(0xF0 | (cp >> 18));
                out += (char)(0x80 | ((cp >> 12) & 0x3F));
                out += (char)(0x80 | ((cp >> 6) & 0x3F));
                out += (char)(0x80 | (cp & 0x3F));
            }
        }

        // 解析 JSON 字符串字面量，s[i] 进入时指向开头的 '"'
        bool ParseJsonString(const std::string& s, size_t& i, std::string& out) {
            ++i; // skip '"'
            while (i < s.size()) {
                char c = s[i];
                if (c == '"') { ++i; return true; }

                if (c == '\\') {
                    if (++i >= s.size()) return false;
                    char e = s[i];
                    switch (e) {
                        case '"':  out += '"';  break;
                        case '\\': out += '\\'; break;
                        case '/':  out += '/';  break;
                        case 'n':  out += '\n'; break;
                        case 't':  out += '\t'; break;
                        case 'r':  out += '\r'; break;
                        case 'b':  out += '\b'; break;
                        case 'f':  out += '\f'; break;
                        case 'u': {
                            unsigned cp = 0;
                            if (i + 4 >= s.size() || !ParseHex4(s, i + 1, cp)) return false;
                            i += 4;
                            // UTF-16 代理对
                            if (cp >= 0xD800 && cp <= 0xDBFF
                                && i + 6 < s.size() && s[i + 1] == '\\' && s[i + 2] == 'u') {
                                unsigned lo = 0;
                                if (ParseHex4(s, i + 3, lo) && lo >= 0xDC00 && lo <= 0xDFFF) {
                                    cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                                    i += 6;
                                }
                            }
                            AppendUtf8(out, cp);
                            break;
                        }
                        default: return false;
                    }
                    ++i;
                }
                else {
                    out += c;
                    ++i;
                }
            }
            return false;
        }

        bool ParseFlatJsonObject(const std::string& text) {
            size_t i = 0;
            auto skipWs = [&] {
                while (i < text.size() && (text[i] == ' ' || text[i] == '\t' || text[i] == '\r' || text[i] == '\n')) ++i;
            };

            // 跳过 UTF-8 BOM
            if (text.size() >= 3 && (unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB && (unsigned char)text[2] == 0xBF) i = 3;

            skipWs();
            if (i >= text.size() || text[i] != '{') return false;
            ++i;
            skipWs();
            if (i < text.size() && text[i] == '}') return true;

            while (true) {
                skipWs();
                if (i >= text.size() || text[i] != '"') return false;

                std::string key;
                if (!ParseJsonString(text, i, key)) return false;

                skipWs();
                if (i >= text.size() || text[i] != ':') return false;
                ++i;
                skipWs();
                if (i >= text.size() || text[i] != '"') return false;

                std::string value;
                if (!ParseJsonString(text, i, value)) return false;

                g_map[std::move(key)] = std::move(value);

                skipWs();
                if (i < text.size() && text[i] == ',') { ++i; continue; }
                if (i < text.size() && text[i] == '}') { ++i; break; }
                return false;
            }
            return true;
        }

        bool LoadTranslations(const std::filesystem::path& path) {
            std::ifstream file(path, std::ios::binary);
            if (!file) return false;

            std::stringstream ss;
            ss << file.rdbuf();
            return ParseFlatJsonObject(ss.str());
        }

        // ---- 中文字体加载：模组字体 -> DLL 同目录 -> 系统微软雅黑 ----
        bool LoadChineseFont(const std::filesystem::path& configDir) {
            ImGuiIO& io = ImGui::GetIO();

            const std::wstring fontFileName = L"AlibabaHealthFont2.0CN-85B.ttf";
            const float fontSize = 16.0f;

            std::vector<std::filesystem::path> candidates = {
                configDir / fontFileName,                 // <DLL目录>/Freecam/
                configDir.parent_path() / fontFileName,   // DLL 同目录
                std::filesystem::path(L"C:\\Windows\\Fonts\\msyh.ttc"), // 系统微软雅黑回退
            };

            ImFontConfig fontConfig;
            fontConfig.OversampleH = 2;
            fontConfig.OversampleV = 2;

            const ImWchar* glyphRanges = io.Fonts->GetGlyphRangesChineseSimplifiedCommon();

            for (const auto& path : candidates) {
                std::error_code ec;
                if (!std::filesystem::exists(path, ec) || ec) continue;

                const std::string fontPathStr = path.string();
                ImFont* font = io.Fonts->AddFontFromFileTTF(fontPathStr.c_str(), fontSize, &fontConfig, glyphRanges);
                if (font) {
                    g_fontLoaded = true;
                    LOG_INFO("Chinese font loaded: %s", fontPathStr.c_str());
                    return true;
                }
                LOG_WARN("Failed to load font file: %s", fontPathStr.c_str());
            }

            LOG_WARN("No Chinese font available, GUI falls back to English");
            return false;
        }
    }

    void Initialize(const std::filesystem::path& configDir) {
        const std::filesystem::path jsonPath = configDir / "zh-CN.json";

        if (LoadTranslations(jsonPath)) {
            g_mapLoaded = true;
            LOG_INFO("zh-CN.json loaded: %d entries", (int)g_map.size());
        }
        else {
            LOG_WARN("Failed to load zh-CN.json (%s), GUI falls back to English", jsonPath.string().c_str());
        }

        LoadChineseFont(configDir);
    }

    bool IsChineseReady() {
        return g_mapLoaded && g_fontLoaded;
    }

    const char* TR(const char* key) {
        if (!IsChineseReady()) return key;
        auto it = g_map.find(key);
        return it != g_map.end() ? it->second.c_str() : key;
    }

    const char* TR(const char* section, const char* name) {
        if (!IsChineseReady()) return name;
        if (section && *section) {
            std::string combined = std::string(section) + "." + name;
            auto it = g_map.find(combined);
            if (it != g_map.end()) return it->second.c_str();
        }
        auto it = g_map.find(name);
        return it != g_map.end() ? it->second.c_str() : name;
    }
}
