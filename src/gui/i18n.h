#pragma once
#include <filesystem>

// 轻量中文汉化模块：
// - 启动时从 <配置目录>/zh-CN.json 加载扁平键值映射（UTF-8）
// - 加载中文字体（优先模组自带 AlibabaHealthFont2.0CN-85B.ttf，回退系统微软雅黑）
// - TR() 查表返回中文；映射或字体任一未就绪时回退英文原文，保证界面永远可读
// 性能：映射仅启动加载一次，TR() 为 O(1) 哈希查找，每帧开销可忽略
namespace I18N {
    // 在 ImGui 上下文创建之后、io.Fonts->Build() 之前调用（GUI::Initialize 内）
    void Initialize(const std::filesystem::path& configDir);

    // 中文界面是否完全就绪（映射表 + 中文字体均已加载成功）
    bool IsChineseReady();

    // 通用翻译：key 为英文原文；查不到或未就绪时原样返回 key
    const char* TR(const char* key);

    // ConVar 翻译：优先查 "<section>.<name>" 复合键，其次裸 name，最后回退 name
    const char* TR(const char* section, const char* name);
}
