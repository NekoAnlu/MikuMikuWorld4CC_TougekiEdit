#pragma once
#include "ImGui/imgui.h"
#include <functional>
#include "NoteTypes.h"

namespace MikuMikuWorld
{
	struct Vector2
	{
		float x;
		float y;

		Vector2(float _x, float _y) : x{ _x }, y{ _y } {}

		Vector2() : x{ 0 }, y{ 0 } {}

		Vector2 operator+(const Vector2& v) { return Vector2(x + v.x, y + v.y); }

		Vector2 operator-(const Vector2& v) { return Vector2(x - v.x, y - v.y); }

		Vector2 operator*(const Vector2& v) { return Vector2(x * v.x, y * v.y); }
	};

	struct Color
	{
	  public:
		float r, g, b, a;

		Color(float _r = 0.0f, float _g = 0.0f, float _b = 0.0f, float _a = 1.0f)
		    : r{ _r }, g{ _g }, b{ _b }, a{ _a }
		{
		}

		inline bool operator==(const Color& c)
		{
			return r == c.r && g == c.g && b == c.b && a == c.a;
		}
		inline bool operator!=(const Color& c) { return !(*this == c); }
		inline Color operator*(const Color& c)
		{
			return Color{ r * c.r, g * c.g, b * c.b, a * c.a };
		}

		static inline int rgbaToInt(int r, int g, int b, int a)
		{
			return r << 24 | g << 16 | b << 8 | a;
		}
		static inline int abgrToInt(int a, int b, int g, int r)
		{
			return a << 24 | b << 16 | g << 8 | r;
		}

		inline ImVec4 toImVec4() const { return ImVec4{ r, g, b, a }; }
		static inline Color fromImVec4(const ImVec4& col)
		{
			return Color{ col.x, col.y, col.z, col.w };
		}

		static inline Color lerp(const Color& start, const Color& end, float ratio)
		{
			return Color{ start.r + (end.r - start.r) * ratio, start.g + (end.g - start.g) * ratio,
				          start.b + (end.b - start.b) * ratio,
				          start.a + (end.a - start.a) * ratio };
		}
		
		//mod HTML转Color
		static inline Color fromHex(const std::string& hex, float alpha = 0.9f)
		{
			Color color{ 1.0,1.0,1.0,1.0 };
			if (hex[0] != '#') return color;
			std::string processedHex = (hex[0] == '#') ? hex.substr(1) : hex;

			if (processedHex.length() != 6) {
				//throw std::invalid_argument("无效的十六进制颜色码，必须是6位。");
				return color;
			}

			unsigned int r_int, g_int, b_int;

			// 使用 sscanf 将十六进制字符串分段解析为整数
			sscanf(processedHex.c_str(), "%02x%02x%02x", &r_int, &g_int, &b_int);

			// 将 0-255 的整数范围归一化到 0.0-1.0 的浮点数范围
			color.r = r_int / 255.0f;
			color.g = g_int / 255.0f;
			color.b = b_int / 255.0f;
			color.a = alpha;

			return color;
		}

	};

	constexpr uint32_t roundUpToPowerOfTwo(uint32_t v)
	{
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v++;
		return v;
	}

	float lerp(float start, float end, float ratio);
	float easeIn(float start, float end, float ratio);
	float easeOut(float start, float end, float ratio);
	float easeInOut(float start, float end, float ratio);
	float easeOutIn(float start, float end, float ratio);
	float midpoint(float x1, float x2);
	bool isWithinRange(float x, float left, float right);

	std::function<float(float, float, float)> getEaseFunction(EaseType ease);

	uint32_t gcf(uint32_t a, uint32_t b);
}