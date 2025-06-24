#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>
#include <raylib.h>
#include <functional>
#include <format>
#include "Widgets.h"

using namespace std;
//using namespace UI;

static const int screenWidth = 800;
static const int screenHeight = 600;
static const char *windowTitle = "TextEd";
static const int targetFPS = 60;

struct FileInfo {
	vector<string> lines = {""};
	vector<string> originalLines = {""};
	optional<string> path;
	bool wasModified = false;
};

FileInfo fileInfo = FileInfo();

enum class FileDialogueType {
	Open, SaveAs
};

vector<string> filePath = {""};

shared_ptr<UI::VerticalBox> window;
shared_ptr<UI::VerticalBox> fileDialogue;
shared_ptr<UI::UIWidget> activeWidget;
shared_ptr<UI::Button> fileDialogueButton;
shared_ptr<UI::Input> textarea;
FileDialogueType fileDialogueType = FileDialogueType::Open;

void OpenDialogue(FileDialogueType type) {
	fileDialogueType = type;
	fileDialogueButton->text = (type == FileDialogueType::Open) ? "Load" : "Save";
	activeWidget = fileDialogue;
	filePath.clear();
	filePath.push_back(fileInfo.path.has_value() ? fileInfo.path.value() : "");
}

void PerformFileDialogueAction() {
	auto path = filePath[0];
	if (fileDialogueType == FileDialogueType::Open) {
		std::ifstream file(path);
		fileInfo.lines.clear();
		fileInfo.originalLines.clear();
		if (file.is_open()) {
			string line;
			while (getline(file, line)) {
				fileInfo.lines.push_back(line);
				fileInfo.originalLines.push_back(line);
			}
			file.close();
			fileInfo.path = path;
			fileInfo.wasModified = false;
			activeWidget = window;
		}
		else {
			std::cerr << "Failed to open file: " << path << std::endl;
		}
	}
	else {
		std::ofstream file(path, std::ios::out | std::ios::trunc);
		if (file.is_open()) {
			for (const auto& line: fileInfo.lines)
				file << line << "\n";
			file.close();
			fileInfo.path = path;
			fileInfo.wasModified = false;
			activeWidget = window;
		}
		else {
			std::cerr << "Failed to save file: " << path << std::endl;
		}
	}
}

int main() {

	filePath[0] = "/Users/user/file.cpp";
	PerformFileDialogueAction();

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(screenWidth, screenHeight, windowTitle);
	SetTargetFPS(targetFPS);
	SetExitKey(0);

	UI::font = LoadFontEx("Inconsolata-Regular.ttf", 16 * GetWindowScaleDPI().y, nullptr, 0);

	window = make_shared<UI::VerticalBox>();
	{
		auto horizontalBox = make_shared<UI::HorizontalBox>();
		auto slot = window->AddSlot(horizontalBox);
		{
			auto button = make_shared<UI::Button>("Open");
			button->onClick = []() { OpenDialogue(FileDialogueType::Open); };
			auto slot = horizontalBox->AddSlot(button);
		}
		{
			auto button = make_shared<UI::Button>("Save As...");
			button->onClick = []() { OpenDialogue(FileDialogueType::SaveAs); };
			auto slot = horizontalBox->AddSlot(button);
		}
		{
			auto label = make_shared<UI::Label>("File info");
			label->textLambda = []() -> string {
				auto modifiedSuffix = fileInfo.wasModified ? " (modified)" : "";
				return (fileInfo.path.has_value() ? fileInfo.path.value() : "<New File>") + modifiedSuffix;
			};
			label->backgroundColor = RAYWHITE;
			auto slot = horizontalBox->AddSlot(label);
			slot->expandRatio = 1;
		}
		{
			//auto slot = horizontalBox->AddSlot(make_shared<UI::Label>("[info]"));
		}
	}
	{
		textarea = make_shared<UI::Input>(fileInfo.lines, BLACK, UI::Margin {10, 10, 10, 10});
		textarea->onChange = []() {
			fileInfo.wasModified = fileInfo.lines != fileInfo.originalLines;
		};
		auto slot = window->AddSlot(textarea);
		slot->expandRatio = 1;
	}
	{
		auto label = make_shared<UI::Label>("");
		label->textLambda = []() -> string {
			return std::format("Line {}/{} : Column {}", textarea->CursorLine() + 1, textarea->lines.size(), textarea->CursorColumn() + 1);
		};
		window->AddSlot(label);
	}

	fileDialogue = make_shared<UI::VerticalBox>();
	{
		auto slot = fileDialogue->AddSlot(make_shared<UI::NullWidget>());
		slot->expandRatio = .5f;
	}
	{
		auto horizontalBox = make_shared<UI::HorizontalBox>();
		auto slot = fileDialogue->AddSlot(horizontalBox);
		{
			auto slot = horizontalBox->AddSlot(make_shared<UI::NullWidget>());
			slot->expandRatio = .25f;
		}
		{
			auto horizontalBox2 = make_shared<UI::HorizontalBox>();
			{
				auto input = make_shared<UI::Input>(filePath, BLACK);
				auto slot = horizontalBox2->AddSlot(input);
				slot->expandRatio = 1;
			}
			{
				fileDialogueButton = make_shared<UI::Button>("Load/Save");
				fileDialogueButton->onClick = []() { PerformFileDialogueAction(); };
				auto slot = horizontalBox2->AddSlot(fileDialogueButton);
			}
			{
				auto cancelButton = make_shared<UI::Button>("Cancel");
				cancelButton->onClick = []() { activeWidget = window; };
				auto slot = horizontalBox2->AddSlot(cancelButton);
			}
			auto slot = horizontalBox->AddSlot(horizontalBox2);
			slot->expandRatio = .5f;
		}
		{
			auto slot = horizontalBox->AddSlot(make_shared<UI::NullWidget>());
			slot->expandRatio = .25f;
		}
	}
	{
		auto slot = fileDialogue->AddSlot(make_shared<UI::NullWidget>());
		slot->expandRatio = .5f;
	}

	activeWidget = window;

	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(RAYWHITE);

		UI::Tick(activeWidget);

		// start with full screen rect
		auto screen = Rectangle {0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};

		window->LayoutWidget(screen);
		window->Draw();

		if (activeWidget == fileDialogue) {
			DrawRectangleRec(screen, Fade(BLACK, 0.25f)); // semi-transparent background
			fileDialogue->LayoutWidget(screen);
			fileDialogue->Draw();
		}

		EndDrawing();
	}

	CloseWindow();

	return 0;
}
