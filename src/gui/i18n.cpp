#include "gui/i18n.h"

#include <cstring>
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

        // 将 UTF-8 字符串中的全部码点加入字形构建器
        void AddUtf8StringToBuilder(ImFontGlyphRangesBuilder& builder, const std::string& s) {
            size_t i = 0;
            while (i < s.size()) {
                unsigned char c = (unsigned char)s[i];
                unsigned cp = 0;
                size_t len = 0;
                if (c < 0x80) { cp = c; len = 1; }
                else if ((c & 0xE0) == 0xC0) { cp = c & 0x1Fu; len = 2; }
                else if ((c & 0xF0) == 0xE0) { cp = c & 0x0Fu; len = 3; }
                else if ((c & 0xF8) == 0xF0) { cp = c & 0x07u; len = 4; }
                else { ++i; continue; } // 非法字节，跳过

                if (i + len > s.size()) break;
                for (size_t k = 1; k < len; ++k)
                    cp = (cp << 6) | ((unsigned char)s[i + k] & 0x3Fu);

                if (cp < 0x10000) // ImWchar 为 16 位
                    builder.AddChar((ImWchar)cp);
                i += len;
            }
        }

        // 字形范围必须存活到 io.Fonts->Build()，故声明为静态
        ImVector<ImWchar> g_glyphRanges;

        // 字体数据：静态存储，存活至进程结束（atlas 不接管所有权）
        std::vector<char> g_fontData;

        // 通过 std::filesystem::path 读取文件（宽路径，不经 ANSI 转换），
        // 支持含中文/emoji 等任意字符的安装目录名
        bool ReadFileToBuffer(const std::filesystem::path& path, std::vector<char>& out) {
            std::ifstream file(path, std::ios::binary);
            if (!file) return false;
            file.seekg(0, std::ios::end);
            const auto size = file.tellg();
            if (size <= 0) return false;
            file.seekg(0, std::ios::beg);
            out.resize(static_cast<size_t>(size));
            return static_cast<bool>(file.read(out.data(), size));
        }

        // 检查 sfnt 魔数：TrueType(00 01 00 00)/true/ttc 集合；
        // 排除 OTTO（CFF 轮廓，stb_truetype 不支持）
        bool IsSupportedFont(const char* data, size_t size) {
            if (size < 4) return false;
            return memcmp(data, "\x00\x01\x00\x00", 4) == 0
                || memcmp(data, "true", 4) == 0
                || memcmp(data, "ttcf", 4) == 0;
        }

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
            fontConfig.FontDataOwnedByAtlas = false; // 数据由 g_fontData 持有

            // 字形范围 = ASCII + 常用简体字兜底 + 映射表中实际用到的所有字符。
            // 「常用 2500 字」不含"帧/轴/弧"等字，若不加映射表字符会显示为 '?'
            ImFontGlyphRangesBuilder builder;
            builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
            builder.AddRanges(io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
            if (g_mapLoaded) {
                for (const auto& [key, value] : g_map) {
                    AddUtf8StringToBuilder(builder, value);
                    AddUtf8StringToBuilder(builder, key);
                }
            }
            builder.BuildRanges(&g_glyphRanges);
            const ImWchar* glyphRanges = g_glyphRanges.Data;

            for (const auto& path : candidates) {
                std::error_code ec;
                if (!std::filesystem::exists(path, ec) || ec) continue;

                // 宽路径读入内存后注册，规避 AddFontFromFileTTF 的窄字符 fopen
                // 在含特殊字符目录名下打开失败的问题
                if (!ReadFileToBuffer(path, g_fontData)) {
                    LOG_WARN("Failed to read font file: %s", path.string().c_str());
                    continue;
                }
                if (!IsSupportedFont(g_fontData.data(), g_fontData.size())) {
                    LOG_WARN("Unsupported font format (CFF/OTF outlines not supported): %s", path.string().c_str());
                    continue;
                }

                ImFont* font = io.Fonts->AddFontFromMemoryTTF(g_fontData.data(), (int)g_fontData.size(), fontSize, &fontConfig, glyphRanges);
                if (font) {
                    g_fontLoaded = true;
                    LOG_INFO("Chinese font loaded: %s (%d bytes)", path.string().c_str(), (int)g_fontData.size());
                    return true;
                }
                LOG_WARN("Failed to add font: %s", path.string().c_str());
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

    bool Has(const char* key) {
        if (!g_mapLoaded) return false;
        return g_map.find(key) != g_map.end();
    }
}
