#include "Widgets.h"
#include <queue>
#include <iostream>
#include <format>

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
		layout.width = rectangle.width;
		layout.height = rectangle.height;
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

		auto letterHeight = font.baseSize / GetWindowScaleDPI().y;
		auto contentHeight = static_cast<float>(lines.size()) * letterHeight + padding.top + padding.bottom;
		auto visibleHeight = layout.height;

		if (contentHeight > visibleHeight) {
			auto scrollBarWidth = 10.f;
			auto totalSpan = contentHeight + visibleHeight;
			auto scrollBarHeight = layout.height * (visibleHeight / totalSpan);
			auto scrollBarY = layout.y + (topOffset / totalSpan) * layout.height;
			DrawRectangle(
				static_cast<int>(layout.x + layout.width - scrollBarWidth),
				static_cast<int>(scrollBarY),
				static_cast<int>(scrollBarWidth),
				static_cast<int>(scrollBarHeight),
				LIGHTGRAY
			);
		}

		if (activeInput.get() == this) {
			float thickness = 2;
			DrawRectangleLinesEx(layout, thickness, Fade(BLUE, .5f));
		}
		DrawRectangleLinesEx(layout, 1, Fade(BLACK, .25));

		BeginScissorMode(layout);

		auto y = layout.y + padding.top - topOffset;
		auto whitespaces = string();
		for (auto i = 0; i < lines.size(); i++) {
			const auto& line = lines[i];
			whitespaces.clear();
			for (const char& c: line)
				whitespaces += isspace(c) ? '.' : ' ';
			//if (i != lines.size() - 1)
			//	whitespaces += "\\n";
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
				static_cast<int>(layout.y + padding.top + letterHeight * CursorLine() - topOffset),
				2, letterHeight, BLACK);
		}

		//auto s = std::format("Content height: {:.2f}, Visible height: {:.2f}, Top offset: {:.2f}",
		//					 contentHeight, visibleHeight, topOffset);
		//DrawText(s, static_cast<int>(layout.x + padding.left), static_cast<int>(layout.y + padding.top), LIGHTGRAY);

		EndScissorMode();
	}

	int Input::CursorLine() {
		m_cursorLine = clamp(m_cursorLine, 0, static_cast<int>(lines.size()) - 1);
		return m_cursorLine;
	}
	void Input::SetCursorLine(int line) {
		m_cursorLine = clamp(line, 0, static_cast<int>(lines.size()) - 1);
	}
	int Input::CursorColumn() {
		auto line = CursorLine();
		return clamp(m_cursorDesiredColumn, 0, static_cast<int>(lines[line].size()));
	}
	void Input::SetCursorColumn(int column) {
		auto line = CursorLine();
		m_cursorDesiredColumn = clamp(column, 0, static_cast<int>(lines[line].size()));
	}
	std::string& Input::Line() {
		auto line = CursorLine();
		return lines[line];
	}

	bool Input::HandleChar(int c) {
		auto& line = Line();
		auto column = CursorColumn();
		line.insert(line.begin() + column, static_cast<char>(c));
		SetCursorColumn(column + 1);
		if (onChange) onChange();
		return true;
	}

	bool Input::HandleKey(int key) {
		auto cursorLine = CursorLine();
		auto cursorColumn = CursorColumn();
		auto& line = Line();
		switch (key) {
			case KEY_LEFT:
				if (cursorColumn > 0) {
					SetCursorColumn(cursorColumn - 1);
					return true;
				}
				break;
			case KEY_RIGHT:
				if (cursorColumn < static_cast<int>(line.size())) {
					SetCursorColumn(cursorColumn + 1);
					return true;
				}
				break;
			case KEY_BACKSPACE:
				if (cursorColumn > 0) {
					line.erase(line.begin() + cursorColumn - 1);
					SetCursorColumn(cursorColumn - 1);
					if (onChange) onChange();
					return true;
				}
				else if (cursorLine > 0) {
					SetCursorLine(cursorLine - 1);
					auto previousLineLength = lines[cursorLine - 1].size();
					lines[cursorLine - 1] += lines[cursorLine];
					lines.erase(lines.begin() + cursorLine);
					SetCursorColumn(previousLineLength);
					if (onChange) onChange();
					return true;
				}
				break;
			case KEY_ENTER: {// we need to break current line in two
				auto newLine = line.substr(cursorColumn);
				line.erase(line.begin() + cursorColumn, line.end());
				lines.insert(lines.begin() + cursorLine + 1, newLine);
				SetCursorLine(cursorLine + 1);
				SetCursorColumn(0);
				if (onChange) onChange();
				return true;
			}
			case KEY_UP:
				if (cursorLine > 0) {
					SetCursorLine(cursorLine - 1);
					return true;
				}
				break;
			case KEY_DOWN:
				if (cursorLine < static_cast<int>(lines.size()) - 1) {
					SetCursorLine(cursorLine + 1);
					return true;
				}
				break;
			case KEY_TAB:
				for (auto i = 0; i < 4; i++)
					HandleChar(' ');
				if (onChange) onChange();
				return true;
		}

		return false;
	}

	void Input::SetCursorFromPosition(const Vector2& position) {
		if (visibility == Visibility::Collapsed)
			return;

		auto letterHeight = font.baseSize / GetWindowScaleDPI().y;
		auto letterWidth = MeasureText("A").x;
		auto relativeX = position.x - layout.x - padding.left;
		auto relativeY = position.y - layout.y - padding.top + topOffset;

		SetCursorLine(relativeY / letterHeight);
		SetCursorColumn(relativeX / letterWidth);
	}

	void Input::SetTopOffset(float newTopOffset) {
		auto lineHeight = font.baseSize / GetWindowScaleDPI().y;
		auto linesNum = static_cast<int>(lines.size());
		auto contentHeight = linesNum * lineHeight;
		auto contentHeightWithPadding = contentHeight + padding.top + padding.bottom;
		topOffset = clamp(newTopOffset, 0.f, contentHeightWithPadding);
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
			if (slot->expandRatio == 0)
				minHeight += minSize.y;
		}

		return Vector2 {minWidth, minHeight};
	}

	void VerticalBox::LayoutWidget(const Rectangle& rectangle) {

		layout.x = rectangle.x;
		layout.y = rectangle.y;
		layout.width = rectangle.width;
		layout.height = rectangle.height;

		auto freeSpaceLeft = max(0.f, layout.height - MinSize().y);
		auto currentY = layout.y;
		for (const auto& slot: slots) {
			auto childHeight = slot->expandRatio == 0
							   ? slot->widget->MinSize().y
							   : freeSpaceLeft * slot->expandRatio;
			auto childRect = Rectangle {layout.x, currentY, layout.width, childHeight};
			slot->widget->LayoutWidget(childRect);
			currentY += childHeight;
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
			if (slot->expandRatio == 0)
				width += size.x;
			height = max(height, size.y);
		}
		return Vector2 {width, height};
	}

	void HorizontalBox::LayoutWidget(const Rectangle& rectangle) {

		layout.x = rectangle.x;
		layout.y = rectangle.y;
		layout.width = rectangle.width;
		layout.height = rectangle.height;

		auto freeSpaceLeft = max(0.f, layout.width - MinSize().x);
		auto currentX = layout.x;
		for (const auto& slot: slots) {
			auto childWidth = slot->expandRatio == 0
							  ? slot->widget->MinSize().x
							  : freeSpaceLeft * slot->expandRatio;
			auto childRect = Rectangle {currentX, layout.y, childWidth, layout.height};
			slot->widget->LayoutWidget(childRect);
			currentX += childWidth;
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

	bool Tick(const std::shared_ptr<UIWidget>& root) {

		auto mouseDelta = GetMouseDelta();
		if (mouseDelta.x != 0 || mouseDelta.y != 0)
			mouseWasMovedAtLeastOnce = true;

		if (!mouseWasMovedAtLeastOnce)
			return false; // No mouse movement, no need to process input

		auto needRedraw = false;

		auto mousePosition = GetMousePosition();
		auto hoveredLeafWidget = FindLeafWidgetAtPosition(root, mousePosition);

		if (lastHoveredLeafWidget && lastHoveredLeafWidget != hoveredLeafWidget) {
			lastHoveredLeafWidget->isHovered = false;
			lastHoveredLeafWidget->isActive = false;
			needRedraw = true;
		}
		if (hoveredLeafWidget) {
			hoveredLeafWidget->isHovered = true;
			if (auto button = dynamic_pointer_cast<Button>(hoveredLeafWidget)) {
				if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
					mouseDownButton = button;
					button->isActive = true;
					needRedraw = true;
				}
				else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && mouseDownButton == button && button->isActive) {
					if (button->onClick)
						button->onClick();
					button->isActive = false;
					needRedraw = true;
				}
			}
			else if (auto input = dynamic_pointer_cast<Input>(hoveredLeafWidget)) {
				if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
					if (input != activeInput) {
						activeInput = input;
						needRedraw = true;
					}
					else {
						input->SetCursorFromPosition(mousePosition);
						needRedraw = true;
					}
				}
				auto wheelMove = GetMouseWheelMoveV().y;
				if (wheelMove != 0) {
					auto oldTopOffset = input->TopOffset();
					input->SetTopOffset(input->TopOffset() - wheelMove * 10);
					needRedraw = oldTopOffset != input->TopOffset() || needRedraw;
				}
			}
		}

		if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
			mouseDownButton = nullptr;
		}

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hoveredLeafWidget != activeInput) {
			ResetActiveInput();
		 	needRedraw = true;
		}

		if (activeInput) {
			while (auto c = GetCharPressed())
				needRedraw = activeInput->HandleChar(c) || needRedraw;
			while (auto key = GetKeyPressed())
				needRedraw = activeInput->HandleKey(key) || needRedraw;
		}

		lastHoveredLeafWidget = hoveredLeafWidget;

		return needRedraw;
	}
}
