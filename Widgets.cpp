#include "Widgets.h"
#include <queue>
#include <iostream>

using namespace std;

namespace UI {

	static shared_ptr <UIWidget> lastHoveredLeafWidget = nullptr;
	static shared_ptr <Button> mouseDownButton = nullptr;
	static shared_ptr <Input> activeInput = nullptr;

	static float GetScaledFontSize(float fontSize) {
		return fontSize / GetWindowScaleDPI().y;
	}
	static void BeginScissorMode(const Rectangle& rectangle) {
		::BeginScissorMode(static_cast<int>(rectangle.x), static_cast<int>(rectangle.y),
						   static_cast<int>(rectangle.width), static_cast<int>(rectangle.height));
	}

	Font font;

	Color YiqContrast(const Color& color) {
		auto yiq = (299 * color.r + 587 * color.g + 114 * color.b) / 1000;
		return (yiq >= 128) ? BLACK : WHITE;
	}

	Vector2 MeasureText(const string& text) {
		return MeasureTextEx(font, text.c_str(), GetScaledFontSize(font.baseSize), 0);
	}
	void DrawText(const string& text, int x, int y, Color color) {
		DrawTextEx(font, text.c_str(), Vector2 {static_cast<float>(x), static_cast<float>(y)}, GetScaledFontSize(font.baseSize), 0, color);
	}

	// Label implementation

	Label::Label(string text, Color color, const Margin& padding)
		: text(move(text)), color(color), padding(padding) {}

	Vector2 Label::MinSize() const {
		if (visibility == Visibility::Collapsed)
			return Vector2 {0, 0};
		auto textSize = MeasureText(Text());
		return Vector2 {
			static_cast<float>(textSize.x) + padding.left + padding.right,
			static_cast<float>(textSize.y) + padding.top + padding.bottom
		};
	}

	void Label::LayoutWidget(const Rectangle& rectangle) {
		layout.x = rectangle.x;
		layout.y = rectangle.y;
		auto size = MinSize();
		layout.width = max(rectangle.width, size.x);
		layout.height = max(rectangle.height, size.y);
	}

	void Label::Draw() {
		if (visibility == Visibility::Collapsed)
			return;
		DrawRectangleRec(layout, backgroundColor);
		BeginScissorMode(layout);
		DrawText(Text(), static_cast<int>(layout.x + padding.left), static_cast<int>(layout.y + padding.top), color);
		EndScissorMode();
	}

	// Button implementation

	Button::Button(string text, const Margin& padding)
		: Label(move(text), BLACK, padding) {}

	void Button::Draw() {
		if (visibility == Visibility::Collapsed)
			return;

		auto color = isActive ? BLACK : isHovered ? GRAY : LIGHTGRAY;
		auto fontColor = YiqContrast(color);

		DrawRectangleRec(layout, color);
		DrawRectangleLinesEx(layout, 1, Fade(BLACK, .25f));
		BeginScissorMode(layout);
		auto horizontalPadding = max(0.f, (layout.width - MeasureText(Text()).x) / 2);
		auto verticalPadding = max(0.f, (layout.height - font.baseSize / GetWindowScaleDPI().y) / 2);
		DrawText(text, static_cast<int>(layout.x + horizontalPadding),
				 static_cast<int>(layout.y + verticalPadding), fontColor);
		EndScissorMode();
	}

	// Input

	Input::Input(std::vector<std::string>& lines, Color color, const Margin& padding)
		: Label("a", color, padding), lines(lines) {}

	Vector2 Input::MinSize() const {
		if (visibility == Visibility::Collapsed)
			return Vector2 {0, 0};
		auto lineHeight = font.baseSize / GetWindowScaleDPI().y;
		auto minWidth = 0.f;
		auto minHeight = 0.f;
		for (const auto& line: lines) {
			minWidth = max(minWidth, static_cast<float>(MeasureText(line).x));
			minHeight += lineHeight;
		}
		minHeight = max(minHeight, lineHeight); // Ensure at least one line height
		minWidth += padding.left + padding.right;
		minHeight += padding.top + padding.bottom;
		return Vector2 {minWidth, minHeight};
	}

	void Input::Draw() {
		if (visibility == Visibility::Collapsed)
			return;

		//auto backgroundColor = isActive || isHovered ? GRAY : LIGHTGRAY;

		DrawRectangleRec(layout, WHITE);
		if (activeInput.get() == this) {
			float thickness = 2;
			DrawRectangleLinesEx(layout, thickness, BLUE);
		}
		DrawRectangleLinesEx(layout, 1, Fade(BLACK, .25));

		BeginScissorMode(layout);

		auto letterHeight = font.baseSize / GetWindowScaleDPI().y;
		auto y = layout.y + padding.top;
		auto whitespaces = string();
		for (auto i = 0; i < lines.size(); i++) {
			const auto& line = lines[i];
			whitespaces.clear();
			for (const char& c: line)
				whitespaces += isspace(c) ? '.' : ' ';
			if (i != lines.size() - 1)
				whitespaces += "\\n";
			DrawText(whitespaces, static_cast<int>(layout.x + padding.left),
					 static_cast<int>(y), LIGHTGRAY);
			DrawText(line, static_cast<int>(layout.x + padding.left),
					 static_cast<int>(y), BLACK);
			y += letterHeight;
		}

		if (activeInput.get() == this) {
			auto letterWidth = MeasureText("A").x;
			DrawRectangle(
				static_cast<int>(layout.x + padding.left + letterWidth * CursorColumn()),
				static_cast<int>(layout.y + padding.top + letterHeight * cursorLine),
				2, letterHeight, BLACK);
		}

		EndScissorMode();
	}

	int Input::CursorColumn() const {
		const auto& line = lines[cursorLine];
		return clamp(cursorDesiredColumn, 0, static_cast<int>(line.size()));
	}

	void Input::HandleChar(int c) {
		auto& line = lines[cursorLine];
		auto column = CursorColumn();
		line.insert(line.begin() + column, static_cast<char>(c));
		//cout << line << '\n';
		cursorDesiredColumn = column + 1;
		if (onChange) onChange();
	}

	void Input::HandleKey(int key) {
		auto column = CursorColumn();
		switch (key) {
			case KEY_LEFT:
				if (column > 0)
					cursorDesiredColumn = column - 1;
				break;
			case KEY_RIGHT:
				if (column < static_cast<int>(lines[cursorLine].size()))
					cursorDesiredColumn = column + 1;
				break;
			case KEY_BACKSPACE:
				if (column > 0) {
					lines[cursorLine].erase(lines[cursorLine].begin() + column - 1);
					cursorDesiredColumn = column - 1;
					if (onChange) onChange();
				}
				else if (cursorLine > 0) {
					cursorDesiredColumn = static_cast<int>(lines[cursorLine - 1].size());
					lines[cursorLine - 1] += lines[cursorLine];
					lines.erase(lines.begin() + cursorLine);
					cursorLine--;
					if (onChange) onChange();
				}
				break;
			case KEY_ENTER:
				lines.insert(lines.begin() + cursorLine + 1, "");
				cursorLine++;
				cursorDesiredColumn = 0;
				if (onChange) onChange();
				break;
			case KEY_UP:
				if (cursorLine > 0)
					cursorLine--;
				break;
			case KEY_DOWN:
				if (cursorLine < static_cast<int>(lines.size()) - 1)
					cursorLine++;
				break;
			case KEY_TAB:
				for (auto i = 0; i < 4; i++)
					HandleChar(' ');
				if (onChange) onChange();
				break;
		}
	}

	// VerticalBox implementation

	Vector2 VerticalBox::MinSize() const {
		if (slots.empty() || visibility == Visibility::Collapsed)
			return Vector2 {0, 0};
		auto minWidth = 0.f;
		auto minHeight = 0.f;
		for (const auto& slot: slots) {
			auto minSize = slot->widget->MinSize();
			minWidth = max(minWidth, minSize.x);
			minHeight += minSize.y;
		}

		return Vector2 {minWidth, minHeight};
	}

	void VerticalBox::LayoutWidget(const Rectangle& rectangle) {

		layout.x = rectangle.x;
		layout.y = rectangle.y;
		auto size = MinSize();
		layout.width = max(rectangle.width, size.x);
		layout.height = max(rectangle.height, size.y);

		auto freeHeightSpace = max(0.f, layout.height - MinSize().y);
		auto currentY = layout.y;
		for (const auto& slot: slots) {
			auto childSize = slot->widget->MinSize();
			childSize.y += freeHeightSpace * slot->expandRatio;
			auto childRect = Rectangle {layout.x, currentY, layout.width, childSize.y};
			slot->widget->LayoutWidget(childRect);
			currentY += childSize.y;
		}

	}

	void VerticalBox::Draw() {
		if (slots.empty() || visibility == Visibility::Collapsed)
			return;
		for (const auto& slot: slots)
			slot->widget->Draw();
	}

	shared_ptr <VerticalBox::Slot> VerticalBox::AddSlot(const shared_ptr <UIWidget>& child) {
		slots.emplace_back(make_shared<Slot>());
		slots.back()->widget = child;
		return slots.back();
	}

	// HorizontalBox implementation

	Vector2 HorizontalBox::MinSize() const {
		if (slots.empty() || visibility == Visibility::Collapsed)
			return Vector2 {0, 0};
		auto width = 0.f;
		auto height = 0.f;
		for (const auto& slot: slots) {
			auto size = slot->widget->MinSize();
			width += size.x;
			height = max(height, size.y);
		}
		return Vector2 {width, height};
	}

	void HorizontalBox::LayoutWidget(const Rectangle& rectangle) {

		layout.x = rectangle.x;
		layout.y = rectangle.y;
		auto size = MinSize();
		layout.width = max(rectangle.width, size.x);
		layout.height = max(rectangle.height, size.y);

		auto freeWidthSpace = max(0.f, layout.width - MinSize().x);
		auto currentX = layout.x;
		for (const auto& slot: slots) {
			auto childSize = slot->widget->MinSize();
			childSize.x += freeWidthSpace * slot->expandRatio;
			auto childRect = Rectangle {currentX, layout.y, childSize.x, layout.height};
			slot->widget->LayoutWidget(childRect);
			currentX += childSize.x;
		}
	}

	void HorizontalBox::Draw() {
		if (slots.empty() || visibility == Visibility::Collapsed)
			return;
		for (const auto& slot: slots)
			slot->widget->Draw();
	}

	shared_ptr <HorizontalBox::Slot> HorizontalBox::AddSlot(const shared_ptr <UIWidget>& child) {
		slots.emplace_back(make_shared<Slot>());
		slots.back()->widget = child;
		return slots.back();
	}

	static queue <shared_ptr<UIWidget>> widgetsToProcess;

	std::shared_ptr<UIWidget> FindLeafWidgetAtPosition(const std::shared_ptr<UIWidget>& root, const Vector2& position) {

		while (!widgetsToProcess.empty())
			widgetsToProcess.pop();

		widgetsToProcess.push(root);

		while (!widgetsToProcess.empty()) {

			auto widget = widgetsToProcess.front();
			widgetsToProcess.pop();

			if (widget->visibility == Visibility::Collapsed)
				continue;

			if (CheckCollisionPointRec(position, widget->layout)) {

				if (widget->IsLeaf()) {
					return widget;
				}

				if (auto verticalBox = dynamic_pointer_cast<VerticalBox>(widget))
					for (const auto& slot: verticalBox->slots)
						widgetsToProcess.push(slot->widget);
				else if (auto horizontalBox = dynamic_pointer_cast<HorizontalBox>(widget))
					for (const auto& slot: horizontalBox->slots)
						widgetsToProcess.push(slot->widget);
			}
		}

		return nullptr;
	}

	static void ResetActiveInput() {
		if (activeInput) {
			activeInput->isActive = false;
			activeInput = nullptr;
		}
	}

	static bool mouseWasMovedAtLeastOnce = false;

	void Tick(const std::shared_ptr<UIWidget>& root) {

		auto mouseDelta = GetMouseDelta();
		if (mouseDelta.x != 0 || mouseDelta.y != 0)
			mouseWasMovedAtLeastOnce = true;

		if (!mouseWasMovedAtLeastOnce)
			return; // No mouse movement, no need to process input

		auto mousePosition = GetMousePosition();
		auto hoveredLeafWidget = FindLeafWidgetAtPosition(root, mousePosition);

		if (lastHoveredLeafWidget && lastHoveredLeafWidget != hoveredLeafWidget) {
			lastHoveredLeafWidget->isHovered = false;
			lastHoveredLeafWidget->isActive = false;
		}
		if (hoveredLeafWidget) {
			hoveredLeafWidget->isHovered = true;
			if (auto button = dynamic_pointer_cast<Button>(hoveredLeafWidget)) {
				if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
					mouseDownButton = button;
					button->isActive = true;
				}
				else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && mouseDownButton == button && button->isActive) {
					if (button->onClick)
						button->onClick();
					button->isActive = false;
				}
			}
			else if (auto input = dynamic_pointer_cast<Input>(hoveredLeafWidget)) {
				if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
					activeInput = input;
			}
		}

		if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
			mouseDownButton = nullptr;
		}

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hoveredLeafWidget != activeInput)
			ResetActiveInput();

		if (activeInput) {
			while (auto c = GetCharPressed())
				activeInput->HandleChar(c);
			while (auto key = GetKeyPressed())
				activeInput->HandleKey(key);
		}

		lastHoveredLeafWidget = hoveredLeafWidget;
	}
}
