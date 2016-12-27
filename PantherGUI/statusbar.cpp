
#include "statusbar.h"
#include "encoding.h"
#include "style.h"

StatusBar::StatusBar(PGWindowHandle window, TextField* textfield) :
	active_textfield(textfield), PGContainer(window) {
	font = PGCreateFont("myriad", false, false);
	SetTextFontSize(font, 13);
	SetTextColor(font, PGStyleManager::GetColor(PGColorStatusBarText));
	textfield->OnSelectionChanged([](Control* c, void* data) {
		((StatusBar*)(data))->SelectionChanged();
	}, (void*) this);

	Button* buttons[4];
	for (int i = 0; i < 4; i++) {
		buttons[i] = new Button(window, this);
		buttons[i]->background_color = PGColor(0, 0, 0, 0);
		buttons[i]->background_stroke_color = PGColor(0, 0, 0, 0);
		this->AddControl(buttons[i]);
	}

	unicode_button = buttons[0];
	language_button = buttons[1];
	lineending_button = buttons[2];
	tabwidth_button = buttons[3];

	for (auto it = controls.begin(); it != controls.end(); it++) {
		(*it)->SetPosition(PGPoint(0, 0));
		(*it)->SetSize(PGSize(this->width, this->height));
		(*it)->SetAnchor(PGAnchorTop | PGAnchorBottom);
	}

	unicode_button->OnPressed([](Button* button) {
		Control* c = button->parent;
		PGPopupMenuHandle menu = PGCreatePopupMenu(button->window, c);
		PGPopupMenuHandle reopen_menu = PGCreatePopupMenu(button->window, c);
		PGPopupMenuHandle savewith_menu = PGCreatePopupMenu(button->window, c);
		{
			PGPopupMenuInsertEntry(reopen_menu, "UTF-8", [](Control* control) {
				// FIXME: reopen with encoding
			}, PGPopupMenuGrayed);
			PGPopupMenuInsertEntry(savewith_menu, "UTF-8", [](Control* control) {
				// FIXME: save with encoding and change active encoding
			}, PGPopupMenuGrayed);
		}
		PGPopupMenuInsertSubmenu(menu, reopen_menu, "Reopen with Encoding...");
		PGPopupMenuInsertSubmenu(menu, savewith_menu, "Save with Encoding...");

		PGDisplayPopupMenu(menu, ConvertWindowToScreen(button->window,
			PGPoint(button->X() + button->width - 1, button->Y())),
			PGTextAlignRight | PGTextAlignBottom);
	});

	language_button->OnPressed([](Button* button) {
		Control* c = button->parent;
		TextFile& file = ((StatusBar*)c)->active_textfield->GetTextFile();
		PGPopupMenuHandle menu = PGCreatePopupMenu(c->window, c);
		auto languages = PGLanguageManager::GetLanguages();
		auto active_language = file.GetLanguage();
		for (auto it = languages.begin(); it != languages.end(); it++) {
			PGPopupMenuInsertEntry(menu, (*it)->GetName().c_str(), [](Control* control) {
				// FIXME: switch highlighter language
			}, active_language == *it ? PGPopupMenuChecked : PGPopupMenuFlagsNone);
		}
		PGDisplayPopupMenu(menu, ConvertWindowToScreen(button->window,
			PGPoint(button->X() + button->width - 1, button->Y())),
			PGTextAlignRight | PGTextAlignBottom);
	});

	lineending_button->OnPressed([](Button* button) {
		Control* c = button->parent;
		TextFile& file = ((StatusBar*)c)->active_textfield->GetTextFile();
		PGPopupMenuHandle menu = PGCreatePopupMenu(button->window, c);
		PGPopupMenuInsertEntry(menu, "Windows (\\r\\n)", [](Control* control) {
			TextFile& file = dynamic_cast<StatusBar*>(control)->active_textfield->GetTextFile();
			file.ChangeLineEnding(PGLineEndingWindows);
		}, file.GetLineEnding() == PGLineEndingWindows ? PGPopupMenuChecked : PGPopupMenuFlagsNone);
		PGPopupMenuInsertEntry(menu, "Unix (\\n)", [](Control* control) {
			TextFile& file = dynamic_cast<StatusBar*>(control)->active_textfield->GetTextFile();
			file.ChangeLineEnding(PGLineEndingUnix);
		}, file.GetLineEnding() == PGLineEndingUnix ? PGPopupMenuChecked : PGPopupMenuFlagsNone);
		PGPopupMenuInsertEntry(menu, "Mac OS 9 (\\r)", [](Control* control) {
			TextFile& file = dynamic_cast<StatusBar*>(control)->active_textfield->GetTextFile();
			file.ChangeLineEnding(PGLineEndingMacOS);
		}, file.GetLineEnding() == PGLineEndingMacOS ? PGPopupMenuChecked : PGPopupMenuFlagsNone);

		PGDisplayPopupMenu(menu, ConvertWindowToScreen(button->window,
			PGPoint(button->X() + button->width - 1, button->Y())),
			PGTextAlignRight | PGTextAlignBottom);
	});
	tabwidth_button->OnPressed([](Button* button) {
		Control* c = button->parent;
		TextFile& file = ((StatusBar*)c)->active_textfield->GetTextFile();
		PGPopupMenuHandle menu = PGCreatePopupMenu(c->window, c);

		PGPopupMenuInsertEntry(menu, "Indent Using Spaces", [](Control* control) {
			// FIXME: change indentation of file and convert
		}, file.GetLineIndentation() == PGIndentionSpaces ? PGPopupMenuChecked : PGPopupMenuFlagsNone);
		PGPopupMenuInsertSeparator(menu);
		for (int i = 1; i <= 8; i++) {
			std::string header = "Tab Width: " + std::to_string(i);
			PGPopupMenuInsertEntry(menu, header, [](Control* control) {
				// FIXME: change tab width
			});
		}
		PGPopupMenuInsertSeparator(menu);
		PGPopupMenuInsertEntry(menu, "Convert Indentation To Spaces", [](Control* control) {
			// FIXME: change indentation of file
		});
		PGPopupMenuInsertEntry(menu, "Convert Indentation To Tabs", [](Control* control) {
			// FIXME: change indentation of file
		});
		PGDisplayPopupMenu(menu, ConvertWindowToScreen(button->window,
			PGPoint(button->X() + button->width - 1, button->Y())),
			PGTextAlignRight | PGTextAlignBottom);
	});
}

StatusBar::~StatusBar() {
	// FIXME: unregister textfield function
}

void StatusBar::UpdateParentSize(PGSize old_size, PGSize new_size) {
	Control::UpdateParentSize(old_size, new_size);
	this->height = STATUSBAR_HEIGHT;
	this->y = parent->height - this->height;
}

void StatusBar::SelectionChanged() {
	this->Invalidate();
}

void StatusBar::Draw(PGRendererHandle renderer, PGIRect* rect) {
	using namespace std;
	PGScalar x = X(), y = Y();
	RenderRectangle(renderer, PGRect(x - rect->x, y - rect->y, this->width, this->height), PGStyleManager::GetColor(PGColorStatusBarBackground), PGDrawStyleFill);
	if (active_textfield) {
		TextFile& file = active_textfield->GetTextFile();
		if (file.IsLoaded()) {
			PGColor line_color = PGStyleManager::GetColor(PGColorStatusBarText);
			auto cursors = file.GetCursors();
			std::string str = "";
			if (cursors.size() == 1) {
				str = string_sprintf("Line %lld, Column %lld", cursors[0]->SelectedLine() + 1, cursors[0]->SelectedCharacter() + 1);

				if (!cursors[0]->SelectionIsEmpty()) {
					if (cursors[0]->BeginLine() == cursors[0]->EndLine()) {
						str += string_sprintf(" (%lld characters selected)", cursors[0]->EndCharacter() - cursors[0]->BeginCharacter());
					} else {
						str += string_sprintf(" (%lld lines selected)", cursors[0]->EndLine() - cursors[0]->BeginLine() + 1);
					}
				}
			} else {
				str = to_string(cursors.size()) + string(" selected regions");
			}
			RenderText(renderer, font, str.c_str(), str.size(), x + 10, y - rect->y);
			const int padding = 20;
			int right_position = 0;

			PGFileEncoding encoding = file.GetFileEncoding();
			str = PGEncodingToString(encoding);
			{
				PGScalar text_width = MeasureTextWidth(font, str.c_str(), str.size());
				unicode_button->x = this->width - 2 * padding - text_width - right_position;
				unicode_button->width = 2 * padding + text_width;
				unicode_button->Draw(renderer, rect);
				right_position += padding;
				right_position += RenderText(renderer, font, str.c_str(), str.size(), x + this->width - right_position, y - rect->y, PGTextAlignRight);
				right_position += padding;
				RenderLine(renderer, PGLine(x + this->width - right_position, y - rect->y, x + this->width - right_position, y - rect->y + this->height), line_color);
			}

			PGLanguage* language = file.GetLanguage();
			str = language ? language->GetName() : "Plain Text";
			{
				PGScalar text_width = MeasureTextWidth(font, str.c_str(), str.size());
				language_button->x = this->width - 2 * padding - text_width - right_position;
				language_button->width = 2 * padding + text_width;
				language_button->Draw(renderer, rect);
				right_position += padding;
				right_position += RenderText(renderer, font, str.c_str(), str.size(), x + this->width - right_position, y - rect->y, PGTextAlignRight);
				right_position += padding;
				RenderLine(renderer, PGLine(x + this->width - right_position, y - rect->y, x + this->width - right_position, y - rect->y + this->height), line_color);
			}

			PGLineEnding ending = file.GetLineEnding();
			switch (ending) {
			case PGLineEndingWindows:
				str = "CRLF";
				break;
			case PGLineEndingUnix:
				str = "LF";
				break;
			case PGLineEndingMacOS:
				str = "CR";
				break;
			default:
				str = "Mixed";
			}
			{
				PGScalar text_width = MeasureTextWidth(font, str.c_str(), str.size());
				lineending_button->x = this->width - 2 * padding - text_width - right_position;
				lineending_button->width = 2 * padding + text_width;
				lineending_button->Draw(renderer, rect);
				right_position += padding;
				right_position += RenderText(renderer, font, str.c_str(), str.size(), x + this->width - right_position, y - rect->y, PGTextAlignRight);
				right_position += padding;
				RenderLine(renderer, PGLine(x + this->width - right_position, y - rect->y, x + this->width - right_position, y - rect->y + this->height), line_color);
			}

			PGLineIndentation indentation = file.GetLineIndentation();
			switch (indentation) {
			case PGIndentionSpaces:
				str = "Spaces: ";
				break;
			case PGIndentionTabs:
				str = "Tab-Width: ";
				break;
			default:
				str = "Mixed: ";
			}
			str += to_string(4);
			{
				PGScalar text_width = MeasureTextWidth(font, str.c_str(), str.size());
				tabwidth_button->x = this->width - 2 * padding - text_width - right_position;
				tabwidth_button->width = 2 * padding + text_width;
				tabwidth_button->Draw(renderer, rect);
				right_position += padding;
				right_position += RenderText(renderer, font, str.c_str(), str.size(), x + this->width - right_position, y - rect->y, PGTextAlignRight);
				right_position += padding;
				RenderLine(renderer, PGLine(x + this->width - right_position, y - rect->y, x + this->width - right_position, y - rect->y + this->height), line_color);
			}
		}
	}
}
