# simple_vision API 参考

纯 C++20 的截图/位图颜色匹配库（要求 Xcode 16.3+ / NDK r25+ / GCC 10+，
语言核心只用到类类型非类型模板参数一项 C++20 特性）。不依赖 Lua、不依赖具体平台——宿主程序提供屏幕
像素缓冲（`Bitmap`），库完成取色、颜色判定、找色、特征匹配、找图。

- 命名空间：`vision`
- 主头文件：`src/vision_api.h`
- 像素格式：**RGBA 或 BGRA，每像素 4 字节**（`pixelStride_ = 4`），行距 `rowShift_ = width * 4`。
  格式是 `Bitmap::format_` 的运行时属性（`PIXEL_RGBA` / `PIXEL_BGRA`），内部按编译期特化的代码路径处理，两种格式性能一致

## 目录

1. [快速上手](#快速上手)
2. [类型](#类型)
3. [颜色字符串语法](#颜色字符串语法)
4. [特征字符串语法](#特征字符串语法)
5. [相似度语义](#相似度语义)
6. [READ_ORDER 扫描顺序](#read_order-扫描顺序)
7. [API 明细](#api-明细)
8. [错误处理约定](#错误处理约定)
9. [构建与测试](#构建与测试)

---

## 快速上手

```cpp
#include "vision_api.h"
using namespace vision;

// 1. 宿主准备好屏幕缓冲（Android framebuffer 常见 BGRA；MediaProjection 常见 RGBA）
Bitmap screen{buffer, 1080, 1920, 1080 * 4, 4, vision::PIXEL_BGRA};  // 或 PIXEL_RGBA

// 2. 判定某点颜色
if (isColor(&screen, 100, 200, "ff0000")) { /* 纯红 */ }

// 3. 全屏找色，从右上往左下扫
FindResult r = findColor(&screen, 0, 0, -1, -1, "ff0000|00ff00", 0.95,
                         UP_DOWN_RIGHT_LEFT);
if (r.found) { /* r.point.x, r.point.y, r.index(命中的是第几个备选) */ }

// 4. 找图（'|' 分隔多个模板路径，返回 1 起始的模板序号）
FindResult img = findImage(&screen, 0, 0, -1, -1,
                           "assets/button.png|assets/button2.png", 0.9);
```

加载图片：

```cpp
CommonBitmap tpl;
if (!tpl.load("assets/button.png")) {
    printf("load failed: %s\n", tpl.errorText());
}
```

宿主自定义资源来源（APK 内资产、压缩包、网络缓存等）：

```cpp
CommonBitmap::setResourceLoader([](const std::string& path, std::string& out) {
    if (!path.starts_with("res/")) return false;   // 返回 false 则回退直读文件
    return readFromApk(path, out);                  // 把图片字节填进 out
});
tpl.load("res/button.png");   // 相对路径会先走 loader；绝对路径直接读文件
```

---

## 类型

### `Bitmap`（`Bitmap.h`）

无所有权的外观类型，指向宿主的像素内存：

```cpp
enum PixelFormat { PIXEL_RGBA = 0, PIXEL_BGRA = 1 };

class Bitmap {
public:
    unsigned char* origin_;    // 首像素地址
    unsigned int   width_;
    unsigned int   height_;
    int            rowShift_;     // 行间距字节数（含 padding）
    int            pixelStride_;  // 恒为 4
    PixelFormat    format_ = PIXEL_RGBA;  // 内存字节序
};
```

宿主屏幕、截图缓冲都可以直接包一个 `Bitmap` 视图使用。库不会写这块内存。

**字节序说明**：

- `PIXEL_RGBA`：内存按 `[R,G,B,A]` 排列——lodepng 解码 PNG 的输出、
  `MediaProjection`/`ImageReader`（`RGBA_8888`）的截屏
- `PIXEL_BGRA`：内存按 `[B,G,R,A]` 排列——Android framebuffer 读取、
  很多设备的 `screencap`、部分 `ANativeWindow` 缓冲
- 屏幕与模板的格式可以不同（例如 BGRA 屏幕匹配 PNG 解出的 RGBA 模板），
  匹配按各自格式取通道，不做缓冲转换
- `CommonBitmap` 从 PNG 加载后自动标记为 `PIXEL_RGBA`；`load(Bitmap*,...)`
  克隆子区域时继承源的格式

### `CommonBitmap`（`CommonBitmap.h`）

自带像素存储的位图，负责 PNG 编解码（lodepng）：

| 成员 | 说明 |
|---|---|
| `bool load(const char* path)` | 从文件加载 PNG；相对路径先咨询 `ResourceLoader` |
| `bool load(const unsigned char* data, unsigned size)` | 从内存加载 PNG |
| `void load(Bitmap* src, int x, int y, int w, int h)` | 克隆 `src` 的一个子区域 |
| `const char* errorText()` | 最近一次加载失败的说明；成功时为 `nullptr` |
| `static void setResourceLoader(ResourceLoader)` | 安装用户资源装载器（见上文） |

### `FindResult`（`vision_api.h`）

```cpp
struct FindResult {
    Point point;   // 命中位置；未命中时为 (-1,-1)
    int   index;   // which/find 系列：命中第几个备选（1 起始）；未命中为 0
    bool  found;
};
```

---

## 颜色字符串语法

解码函数为 `decodeColor`（`vision_color.h`），四种基本形态用 `|` 组合：

| 写法 | 名称 | 语义 |
|---|---|---|
| `rrggbb` | 单色 | 逐通道等于该颜色（在相似度容差内） |
| `!rrggbb` | 排除色 | 与该颜色**差异超过**相似度容差时匹配 |
| `rrggbb-rrggbb` | 色域 | 前者是中心色，后者是逐通道半径；像素每通道都在带内即匹配（自带容差，相似度仍可再放宽） |
| `!rrggbb-rrggbb` | 排除色域 | 像素**每个通道都在带外**时匹配 |

组合 `a|b|c` 表示"任一匹配"：

- `isColor` → 布尔结果
- `whichColor` / `whichImage` / `findImage` 的 `index` → 命中第几个（1 起始）

示例：

```
"ff0000"                 纯红
"!000000"                非纯黑（相似度 1.0 时 = 与黑色有差异）
"ff8000-002020"          橙色为中心，R 通道 ±0，G/B 通道 ±32
"!808080-101010"         每通道都偏离中灰 ±16 以上（≈非灰区域）
"ff0000|00ff00|0000ff"   三原色任一
```

---

## 特征字符串语法

特征（`decodeFeature`，`vision_feature.h`）描述一组相对锚点：

```
"x1|y1|color1,x2|y2|color2,..."
```

- 每个锚点是 `x 偏移|y 偏移|颜色`（颜色可以是上面任意组合语法）
- 锚点之间用 `,` 分隔，坐标为 int16，可为负
- 匹配时所有锚点的色差**共享一个总预算**（见[相似度](#相似度语义)），所以特征点数越多、同样相似度下判得越松——单点色差可以在点间摊薄

示例：`"0|0|ff0000,10|0|00ff00,5|8|!000000"` 表示锚点处分别为红、绿、非黑。

---

## 相似度语义

所有 API 的 `similarity ∈ [0,1]`，默认 `1.0`（严格匹配）：

- 内部换算为**总色差预算** `shift = (1 - similarity) * 765`（765 = 255×3，即 `MAX_COLOR_SHIFT`）
- 像素与目标的曼哈顿色差 `≤` 预算即视为匹配
- **多像素场景预算是求和的**：
  - 特征：`shift × 765 × 锚点数`——锚点越多预算越大，单点容差仍约 `shift`
  - 找图：`shift × 模板像素数`——同理，模板越大单点容差不变
- 越界返回值：`similarityToShift()` 对非法输入返回 `-1`，各 API 直接判定不匹配，不抛异常

---

## READ_ORDER 扫描顺序

`findColor` / `findFeature` / `findImage` 的 `order` 参数（`vision_util.h`）：

| 值 | 名称 | 外层 | 内层 |
|---|---|---|---|
| 0 | `UP_DOWN_LEFT_RIGHT` | 列：左→右 | 行：上→下 |
| 1 | `UP_DOWN_RIGHT_LEFT` | 列：右→左 | 行：上→下 |
| 2 | `DOWN_UP_LEFT_RIGHT` | 列：左→右 | 行：下→上 |
| 3 | `DOWN_UP_RIGHT_LEFT` | 列：右→左 | 行：下→上 |
| 4 | `LEFT_RIGHT_UP_DOWN` | 行：上→下 | 列：左→右 |
| 5 | `LEFT_RIGHT_DOWN_UP` | 行：下→上 | 列：左→右 |
| 6 | `RIGHT_LEFT_UP_DOWN` | 行：上→下 | 列：右→左 |
| 7 | `RIGHT_LEFT_DOWN_UP` | 行：下→上 | 列：右→左 |

命名前半描述**内层**行走方向，后半描述外层。"先比列还是先比行"决定命中优先级：
列优先（`UP_DOWN_*` / `DOWN_UP_*`）先锁定靠左/右的列，行优先（`LEFT_RIGHT_*` / `RIGHT_LEFT_*`）先锁定靠上/下的行。

---

## API 明细

以下均声明于 `vision` 命名空间。矩形参数 `(x2,y2)` 传 `(-1,-1)` 表示扩展到位图右下角；
所有 API 对越界坐标、非法相似度、`nullptr` 一律返回"未命中"而不崩溃（见[错误处理](#错误处理约定)）。

### 取色

```cpp
Color getColor(Bitmap* bitmap, int x, int y);
```

返回 `0xRRGGBB`（`Color::data`）。坐标必须合法——越界读取属未定义行为，
调用方负责（配合 `isInBitmapScope` 使用，该函数在 `vision_util.h`）。

### 单点颜色判定

```cpp
bool isColor (Bitmap* b, int x, int y, const char* color, double similarity = 1.0);
int  whichColor(Bitmap* b, int x, int y, const char* color, double similarity = 1.0);
```

- `isColor`：该点是否匹配
- `whichColor`：命中组合里第几个备选（1 起始），未命中返回 `0`

### 区域计数 / 找色

```cpp
int getColorCount(Bitmap* b, int x, int y, int x2, int y2,
                  const char* color, double similarity = 1.0);

FindResult findColor(Bitmap* b, int x, int y, int x2, int y2,
                     const char* color, double similarity = 1.0,
                     int order = UP_DOWN_LEFT_RIGHT);
```

- `getColorCount`：矩形内匹配像素数；参数非法（如翻转矩形、坏颜色串）返回 `-1`
- `findColor`：按 `order` 扫描第一个匹配像素；`index` 为命中备选序号（单色恒为 1）

### 特征

```cpp
bool isFeature(Bitmap* b, int anchorX, int anchorY,
               const char* feature, double similarity = 1.0);

FindResult findFeature(Bitmap* b, int x, int y, int x2, int y2,
                       const char* feature, double similarity = 1.0,
                       int order = UP_DOWN_LEFT_RIGHT);
```

`isFeature` 以 `(anchorX, anchorY)` 为特征原点（各锚点相对它的偏移）判定。
`findFeature` 在区域内扫描使特征成立的原点。锚点落在位图外时按最大色差计。

### 找图

路径版（`templates` 为 `'|'` 分隔的图片路径，每次调用重新加载）：

```cpp
bool isImage  (Bitmap* b, int x, int y, const char* templates, double similarity = 1.0);
int  whichImage(Bitmap* b, int x, int y, const char* templates, double similarity = 1.0);
FindResult findImage(Bitmap* b, int x, int y, int x2, int y2,
                     const char* templates, double similarity = 1.0,
                     int order = UP_DOWN_LEFT_RIGHT);
```

预加载版（`CommonBitmap` 已加载，无 IO，推荐在循环里使用）：

```cpp
bool isImage  (Bitmap* b, int x, int y, Bitmap* templateImage, double similarity = 1.0);
int  whichImage(Bitmap* b, int x, int y,
                const std::vector<CommonBitmap>& templates, double similarity = 1.0);
FindResult findImage(Bitmap* b, int x, int y, int x2, int y2,
                     const std::vector<CommonBitmap>& templates,
                     double similarity = 1.0, int order = UP_DOWN_LEFT_RIGHT);
```

- 模板必须完整落在位图内，出界即不匹配
- `whichImage` / `findImage().index`：1 起始的模板序号
- 多模板时**扫描位置优先于模板顺序**：先确定扫描序里第一个可匹配的位置，再取该位置能匹配的第一个模板

### 辅助

```cpp
bool loadImages(const char* paths, size_t size, std::vector<CommonBitmap>& out);
bool loadImages(const std::string& paths,        std::vector<CommonBitmap>& out);

int similarityToShift(double similarity);
```

- `loadImages`：按 `'|'` 拆分批量加载。任一失败返回 `false`，成功前缀仍留在 `out`；
  空段（`a||b`、尾部 `|`）跳过
- `similarityToShift`：把相似度换算成总色差预算；非法输入返回 `-1`

---

## 错误处理约定

本库**不抛 C++ 异常**（绑定层友好：可以安全地跨 C 边界、Lua pcalls、JNI）：

| 情形 | 行为 |
|---|---|
| 坐标越界（`isColor`/`find*` 等） | 返回未命中 / `false` / `0` / `-1` |
| 颜色串、特征串解析失败 | 同上 |
| `similarity` 越界或 NaN | 同上 |
| PNG 加载失败 | `load` 返回 `false`，`errorText()` 给出原因 |
| 路径列表含缺失文件 | `loadImages` 返回 `false`，保留成功前缀 |

唯一需要调用方自律的是 `getColor` 的越界读取（性能热路径上不做检查）。

---

## 构建与测试

```bash
git submodule update --init     # lodepng

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

ctest --test-dir build --output-on-failure    # 运行测试
```

CMake 选项：`VISION_SHARED`（默认 ON）、`VISION_STATIC`（默认 OFF）、
`VISION_BUILD_TESTS`（默认 ON）。

测试（`tests/`）使用内置轻量断言框架，图像数据全部在运行时生成
（四象限板、噪声板、渐变、稀疏点四种图案），覆盖全部公共 API、
8 种扫描顺序、四种颜色语法、PNG 往返、`ResourceLoader` 注入等。
