#pragma once

#include <string>

#include "../LE1-SDK/SdkHeaders.h"

/// <summary>
/// Abstract class for creating custom HUDs similar to StreamingLevelsHUD
/// </summary>
class CustomHUD {
public:
	bool ShouldDraw = true;
	CustomHUD(LPSTR windowName = "MassEffect", int left = 0, int top = 0) {
		_windowName = windowName;
	}
	virtual void Update(UCanvas* canvas) {
		_canvas = canvas;
	}
	virtual void Draw() = 0;

protected:
	UCanvas* _canvas;
	float textScale = 1.25f;
	float lineHeight = 14.0f;
	int yIndex;
	LPSTR _windowName;
	FVector2D topLeft;
	int m_indent = 0;

	void SetTextScale() {
		HWND activeWindow = FindWindowA(NULL, _windowName);
		if (activeWindow) {
			RECT rcOwner;
			if (GetWindowRect(activeWindow, &rcOwner)) {
				long width = rcOwner.right - rcOwner.left;
				long height = rcOwner.bottom - rcOwner.top;

				if (width > 2560 && height > 1440) {
					textScale = 3.0f;
				} else if (width > 1920 && height > 1080) {
					textScale = 2.5f;
				} else {
					textScale = 2.0f;
				}
				lineHeight = 14.0f * textScale;
			}
		}
	}

	void RenderText(std::wstring msg, const float x, const float y, const BYTE r, const BYTE g, const BYTE b, const float alpha) {
		if (_canvas) {
			_canvas->SetDrawColor(0, 0, 0, 255);
			_canvas->SetPos(topLeft.X + x + m_indent - 1, topLeft.Y + y + 64 - 1); //+ is Y start. To prevent overlay on top of the power bar thing
			_canvas->DrawTextW(FString{ const_cast<wchar_t*>(msg.c_str()) }, 1, textScale+0.01f, textScale+0.01f, nullptr);
			_canvas->SetDrawColor(r, g, b, (BYTE)(alpha * 255));
			_canvas->SetPos(topLeft.X + x + m_indent, topLeft.Y + y + 64); //+ is Y start. To prevent overlay on top of the power bar thing
			_canvas->DrawTextW(FString{ const_cast<wchar_t*>(msg.c_str()) }, 1, textScale, textScale, nullptr);
		}
	}

	void RenderTextLine(std::wstring msg, const BYTE r, const BYTE g, const BYTE b, const float alpha) {
		RenderText(msg, topLeft.X + 5 + m_indent, topLeft.Y + lineHeight * yIndex, r, g, b, alpha);
		yIndex++;
	}

	void RenderName(const wchar_t* label, FName name) {
		wchar_t output[512];
		swprintf_s(output, 512, L"%s: %S", label, name.GetName());
		RenderTextLine(output, 0, 255, 0, 1.0f);
	}

	void RenderFloat(const wchar_t* label, float yaw) {
		wchar_t output[512];
		swprintf_s(output, 512, L"%s: %f", label, yaw);
		RenderTextLine(output, 0, 255, 0, 1.0f);
	}

	void RenderString(const wchar_t* label, FString str) {
		wchar_t output[512];
		swprintf_s(output, 512, L"%s: %s", label, str.Data);
		RenderTextLine(output, 0, 255, 0, 1.0f);
	}

	void RenderInt(const wchar_t* label, int value) {
		wchar_t output[512];
		swprintf_s(output, 512, L"%s: %d", label, value);
		RenderTextLine(output, 0, 255, 0, 1.0f);
	}

	void RenderBool(const wchar_t* label, bool value) {
		wchar_t output[512];
		if (value) {
			swprintf_s(output, 512, L"%s: %s", label, L"True");
		} else {
			swprintf_s(output, 512, L"%s: %s", label, L"False");
		}
		RenderTextLine(output, 0, 255, 0, 1.0f);
	}

	void Indent() {
		m_indent += (int)(2 * lineHeight);
	}

	void Unindent() {
		m_indent -= (int)(2 * lineHeight);
		if (m_indent < 0)
			m_indent = 0;
	}

public:
	void SetTopLeft(int left, int top) {
		topLeft.X = (float)left;
		topLeft.Y = (float)top;
	}
};
