
#include "textfield.h"
#include <sstream>
#include <algorithm>
#include "logger.h"

TextField::TextField(PGWindowHandle window, std::string filename) :
	Control(window), textfile(filename, this), display_carets(true), display_carets_count(0), active_cursor(NULL) {
	RegisterRefresh(window, this);
	cursors.push_back(new Cursor(&textfile));
	line_height = 20;
	character_width = 9;
}

#define FLICKER_CARET_INTERVAL 15

void TextField::DisplayCarets() {
	// FIXME: thread safety on setting display_carets_count?
	display_carets_count = 0;
	display_carets = true;
}

void TextField::PeriodicRender(void) {
	// FIXME: thread safety on incrementing display_carets_count and cursors?
	display_carets_count++;
	if (display_carets_count % FLICKER_CARET_INTERVAL == 0) {
		display_carets_count = 0;
		display_carets = !display_carets;
		ssize_t start_line = LLONG_MAX, end_line = -LLONG_MAX;
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			start_line = std::min((*it)->start_line, start_line);
			end_line = std::max((*it)->start_line, end_line);
		}
		if (start_line <= end_line) {
			this->InvalidateBetweenLines(start_line, end_line);
		}
	}
}

void TextField::Draw(PGRendererHandle renderer, PGRect* rectangle) {
	// FIXME: get line height from renderer without drawing
	// FIXME: mouse = caret over textfield
	// FIXME: draw background of textfield

	// determine the width of the line numbers 
	ssize_t line_count = textfile.GetLineCount();
	auto line_number = std::to_string(std::max((ssize_t)10, textfile.GetLineCount() + 1));
	text_offset = 10 + GetRenderWidth(renderer, line_number.c_str(), line_number.size());
	// determine the render position
	int position_x = this->x + this->offset_x;
	int position_x_text = position_x + text_offset;
	int position_y = 0;
	character_width = GetRenderWidth(renderer, "a", 1);
	ssize_t linenr = lineoffset_y;
	TextLine *current_line;
	SetTextFont(renderer, NULL);
	SetTextColor(renderer, PGColor(0, 0, 255));
	while ((current_line = textfile.GetLine(linenr)) != NULL) {
		// only render lines that fall within the render rectangle
		if (this->offsets.size() <= linenr - lineoffset_y) {
			this->offsets.push_back(std::vector<short>());
		}
		if (position_y > rectangle->y + rectangle->height) break;
		if (!(position_y + line_height < rectangle->y)) {
			// determine render offsets for mouse selection
			GetRenderOffsets(renderer, current_line->GetLine(), current_line->GetLength(), this->offsets[linenr - lineoffset_y]);
			// render the actual text
			PGSize size = RenderText(renderer, current_line->GetLine(), current_line->GetLength(), position_x_text, position_y);
			line_height = size.height;

			// render the line number
			auto line_number = std::to_string(linenr + 1);
			SetTextAlign(renderer, PGTextAlignRight);
			RenderText(renderer, line_number.c_str(), line_number.size(), position_x, position_y);
		}

		linenr++;
		position_y += line_height;
	}
	// render the selection and carets
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		ssize_t startline = std::max((*it)->BeginLine(), lineoffset_y);
		ssize_t endline = std::min((*it)->EndLine(), linenr);
		position_y = (startline - lineoffset_y) * line_height;
		for (; startline <= endline; startline++) {
			current_line = textfile.GetLine(startline);
			assert(current_line);
			ssize_t start, end;
			if (startline == (*it)->BeginLine()) {
				if (startline == (*it)->EndLine()) {
					// start and end are on the same line
					start = (*it)->BeginCharacter();
					end = (*it)->EndCharacter();
				} else {
					start = (*it)->BeginCharacter();
					end = current_line->GetLength() + 1;
				}
			} else if (startline == (*it)->EndLine()) {
				start = 0;
				end = (*it)->EndCharacter();
			} else {
				start = 0;
				end = current_line->GetLength() + 1;
			}

			if (startline == (*it)->SelectedLine()) {
				RenderRectangle(renderer, PGRect(position_x, position_y, text_offset, line_height), PGColor(32,32,255,64));
				if (display_carets) {
					// render the caret on the selected line
					RenderCaret(renderer, current_line->GetLine(), current_line->GetLength(), position_x_text, position_y, (*it)->SelectedCharacter());
				}
			}

			RenderSelection(renderer,
				current_line->GetLine(),
				current_line->GetLength(),
				position_x_text,
				position_y,
				start,
				end);

			position_y += line_height;
		}
	}
}

void TextField::MouseClick(int x, int y, PGMouseButton button, PGModifier modifier) {
}

void TextField::GetLineCharacterFromPosition(int x, int y, ssize_t& line, ssize_t& character, bool clip_character) {
	// find the line position of the mouse
	ssize_t line_offset = std::max(std::min((ssize_t)y / this->line_height, textfile.GetLineCount() - this->lineoffset_y - 1), (ssize_t) 0);
	line = this->lineoffset_y + line_offset;
	// find the character position within the line
	x = x - this->text_offset - this->offset_x;
	ssize_t width = 0;
	if (clip_character) {
		character = textfile.GetLine(line)->GetLength();
		if (offsets.size() > line_offset) {
			for (int i = 0; i < this->offsets[line_offset].size(); i++) {
				if (width <= x && width + offsets[line_offset][i] > x) {
					character = i;
					break;
				}
				width += offsets[line_offset][i];
			}
		}
	} else {
		character = x / character_width;
	}
}

void TextField::ClearExtraCursors() {
	if (cursors.size() > 1) {
		for (auto it = cursors.begin() + 1; it != cursors.end(); it++) {
			delete *it;
		}
		cursors.erase(cursors.begin() + 1, cursors.end());
	}
	active_cursor = cursors.front();
}

void TextField::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {
	x = x - this->x;
	y = y - this->y;
	if (button == PGLeftMouseButton) {
		if (drag_selection_cursors) return;
		drag_selection = true;
		ssize_t line, character;
		GetLineCharacterFromPosition(x, y, line, character);
		
		time_t time = GetTime();
		if (time - last_click.time < DOUBLE_CLICK_TIME && 
			std::abs(x - last_click.x) < 2 && 
			std::abs(y - last_click.y) < 2) {
			last_click.clicks = last_click.clicks == 2 ? 0 : last_click.clicks + 1;
		} else {
			last_click.clicks = 0;
		}
		last_click.time = time;
		last_click.x = x;
		last_click.y = y;
		
		if (modifier == PGModifierNone && last_click.clicks == 0) {
			ClearExtraCursors();
			cursors[0]->SetCursorLocation(line, character);
			Logger::GetInstance()->WriteLogMessage(std::string("if (cursors.size() > 1) {\n\tcursors.erase(cursors.begin() + 1, cursors.end());\n}\ncursors[0]->SetCursorLocation(") + std::to_string(line) + std::string(", ") + std::to_string(character) + std::string(");"));
		} else if (modifier == PGModifierShift) {
			ClearExtraCursors();
			cursors[0]->SetCursorStartLocation(line, character);
			Logger::GetInstance()->WriteLogMessage(std::string("if (cursors.size() > 1) {\n\tcursors.erase(cursors.begin() + 1, cursors.end());\n}\ncursors[0]->SetCursorStartLocation(") + std::to_string(line) + std::string(", ") + std::to_string(character) + std::string(");"));
		} else if (modifier == PGModifierCtrl) {
			Logger::GetInstance()->WriteLogMessage(std::string("active_cursor = new Cursor(&textfile, ") + std::to_string(line) + std::string(", ") + std::to_string(character) + std::string(");\n") + std::string("cursors.push_back(active_cursor);"));
			
			cursors.push_back(new Cursor(&textfile, line, character));
			active_cursor = cursors.back();
			Cursor::NormalizeCursors(this, cursors, false);
		} else if (last_click.clicks == 1) {
			ClearExtraCursors();
			cursors[0]->SetCursorLocation(line, character);
			cursors[0]->SelectWord();
			Logger::GetInstance()->WriteLogMessage(std::string("if (cursors.size() > 1) {\n\tcursors.erase(cursors.begin() + 1, cursors.end());\n}\ncursors[0]->SetCursorLocation(") + std::to_string(line) + std::string(", ") + std::to_string(character) + std::string(");\ncursors[0].SelectWord();"));
		} else if (last_click.clicks == 2) {
			ClearExtraCursors();
			cursors[0]->SelectLine();
			Logger::GetInstance()->WriteLogMessage(std::string("if (cursors.size() > 1) {\n\tcursors.erase(cursors.begin() + 1, cursors.end());\n}\ncursors[0]->SelectLine();"));
		}
		this->Invalidate();
	} else if (button == PGMiddleMouseButton) {
		if (drag_selection) return;
		drag_selection_cursors = true;
		ssize_t line, character;
		GetLineCharacterFromPosition(x, y, line, character);
		ClearExtraCursors();
		cursors[0]->SetCursorLocation(line, character);
		active_cursor = cursors[0];
		this->Invalidate();
	}
}

void TextField::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	if (button & PGLeftMouseButton) {
		drag_selection = false;
	} else if (button & PGMiddleMouseButton) {
		drag_selection_cursors = false;
	}
}

void TextField::MouseMove(int x, int y, PGMouseButton buttons) {
	if (drag_selection) {
		// FIXME: when having multiple cursors and we are altering the active cursor,
		// the active cursor can never "consume" the other selections (they should always stay)
		ssize_t line, character;
		GetLineCharacterFromPosition(x, y, line, character);
		if (active_cursor == NULL) {
			active_cursor = cursors.front();
		}
		if (active_cursor->start_line != line || active_cursor->start_character != character) {
			ssize_t old_line = active_cursor->start_line;
			active_cursor->SetCursorStartLocation(line, character);
			Logger::GetInstance()->WriteLogMessage(std::string("if (!active_cursor) active_cursor = cursors.front();\nactive_cursor->SetCursorStartLocation(") + std::to_string(line) + std::string(", ") + std::to_string(character) + std::string(");\nCursor::NormalizeCursors(textfield, cursors, false);"));
			Cursor::NormalizeCursors(this, cursors, false);
			this->InvalidateBetweenLines(old_line, line);
		}
	} else if (drag_selection_cursors) {
		ssize_t line, character;
		GetLineCharacterFromPosition(x, y, line, character, false);
		ssize_t start_character = active_cursor->end_character;
		ssize_t start_line = active_cursor->end_line;
		ssize_t increment = line > active_cursor->end_line ? 1 : -1;
		cursors[0] = active_cursor;
		ClearExtraCursors();
		for (auto it = active_cursor->end_line; ; it += increment) {
			if (it != active_cursor->end_line) {
				ssize_t line_length = textfile.GetLine(it)->GetLength();
				if (start_character == character || line_length >= start_character) {
					Cursor* cursor = new Cursor(&textfile, it, start_character);
					cursor->end_character = std::min(start_character, line_length);
					cursor->start_character = std::min(character, line_length);
					cursors.push_back(cursor);
				}
			}
			if (it == line) {
				break;
			}
		}

		cursors[0]->start_character = std::min(character, textfile.GetLine(cursors[0]->start_line)->GetLength());
		this->InvalidateBetweenLines(cursors[0]->start_line, line);
	}
}

int TextField::GetLineHeight() {
	return this->height / line_height + (this->height % line_height == 0 ? -1 : 0);
}

void TextField::KeyboardButton(PGButton button, PGModifier modifier) {
	Logger::GetInstance()->WriteLogMessage(std::string("textField->KeyboardButton(") + GetButtonName(button) + std::string(", ") + GetModifierName(modifier) + std::string(");"));
	switch (button) {
		// FIXME: when moving up/down, count \t as multiple characters (equal to tab size)
		// FIXME: when moving up/down, maintain the current character number (even though a line is shorter than that number)
	case PGButtonDown:
		if (modifier == PGModifierCtrlShift) {
			textfile.MoveLines(cursors, 1);
		} else {
			for (auto it = cursors.begin(); it != cursors.end(); it++) {
				if (modifier == PGModifierNone) {
					(*it)->OffsetLine(1);
				} else if (modifier == PGModifierShift) {
					(*it)->OffsetSelectionLine(1);
				} else if (modifier == PGModifierCtrl) {
					SetScrollOffset(this->lineoffset_y + 1);
				}
			}
		}
		Cursor::NormalizeCursors(this, cursors);
		Invalidate();
		break;
	case PGButtonUp: 
		if (modifier == PGModifierCtrlShift) {
			textfile.MoveLines(cursors, -1);
		} else {
			for (auto it = cursors.begin(); it != cursors.end(); it++) {
				if (modifier == PGModifierNone) {
					(*it)->OffsetLine(-1);
				} else if (modifier == PGModifierShift) {
					(*it)->OffsetSelectionLine(-1);
				} else if (modifier == PGModifierCtrl) {
					SetScrollOffset(this->lineoffset_y - 1);
				}
			}
		}
		Cursor::NormalizeCursors(this, cursors);
		Invalidate();
		break;
	case PGButtonLeft:
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			if (modifier == PGModifierNone) {
				if ((*it)->SelectionIsEmpty()) {
					(*it)->OffsetCharacter(-1);
				} else {
					(*it)->SetCursorLocation((*it)->BeginLine(), (*it)->BeginCharacter());
				}
			} else if (modifier == PGModifierShift) {
				(*it)->OffsetSelectionCharacter(-1);
			} else if (modifier == PGModifierCtrl) {
				(*it)->OffsetWord(PGDirectionLeft);
			} else if (modifier == PGModifierCtrlShift) {
				(*it)->OffsetSelectionWord(PGDirectionLeft);
			}
		}
		Cursor::NormalizeCursors(this, cursors);
		Invalidate();
		break;
	case PGButtonRight:
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			if (modifier == PGModifierNone) {
				if ((*it)->SelectionIsEmpty()) {
					(*it)->OffsetCharacter(1);
				} else {
					(*it)->SetCursorLocation((*it)->EndLine(), (*it)->EndCharacter());
				}
			} else if (modifier == PGModifierShift) {
				(*it)->OffsetSelectionCharacter(1);
			} else if (modifier == PGModifierCtrl) {
				(*it)->OffsetWord(PGDirectionRight);
			} else if (modifier == PGModifierCtrlShift) {
				(*it)->OffsetSelectionWord(PGDirectionRight);
			}
		}
		Cursor::NormalizeCursors(this, cursors);
		Invalidate();
		break;
	case PGButtonEnd:
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			if (modifier == PGModifierNone) {
				(*it)->OffsetEndOfLine();
			} else if (modifier == PGModifierShift) {
				(*it)->SelectEndOfLine();
			} else if (modifier == PGModifierCtrl) {
				(*it)->OffsetEndOfFile();
			} else if (modifier == PGModifierCtrlShift) {
				(*it)->SelectEndOfFile();
			}
		}
		Cursor::NormalizeCursors(this, cursors);
		Invalidate();
		break;
	case PGButtonHome:
		for (auto it = cursors.begin(); it != cursors.end(); it++) {
			if (modifier == PGModifierNone) {
				(*it)->OffsetStartOfLine();
			} else if (modifier == PGModifierShift) {
				(*it)->SelectStartOfLine();
			} else if (modifier == PGModifierCtrl) {
				(*it)->OffsetStartOfFile();
			} else if (modifier == PGModifierCtrlShift) {
				(*it)->SelectStartOfFile();
			}
		}
		Cursor::NormalizeCursors(this, cursors);
		Invalidate();
		break;
	case PGButtonPageUp:
		if (modifier == PGModifierNone) {
			for (auto it = cursors.begin(); it != cursors.end(); it++) {
				(*it)->OffsetLine(-GetLineHeight());
			}
			Cursor::NormalizeCursors(this, cursors);
			Invalidate();
		}
		break;
	case PGButtonPageDown:
		if (modifier == PGModifierNone) {
			for (auto it = cursors.begin(); it != cursors.end(); it++) {
				(*it)->OffsetLine(GetLineHeight());
			}
			Cursor::NormalizeCursors(this, cursors);
			Invalidate();
		}
		break;
	case PGButtonDelete:
		if (modifier == PGModifierNone) {
			this->textfile.DeleteCharacter(cursors, PGDirectionRight);
		} else if (modifier == PGModifierCtrl) {
			this->textfile.DeleteWord(cursors, PGDirectionRight);
		} else if (modifier == PGModifierShift) {
			textfile.DeleteLines(cursors);
		} else if (modifier == PGModifierCtrlShift) {
			this->textfile.DeleteLine(cursors, PGDirectionRight);
		}
		this->Invalidate();
		break;
	case PGButtonBackspace:
		if (modifier == PGModifierNone || modifier == PGModifierShift) {
			this->textfile.DeleteCharacter(cursors, PGDirectionLeft);
		} else if (modifier == PGModifierCtrl) {
			this->textfile.DeleteWord(cursors, PGDirectionLeft);
		} else if (modifier == PGModifierCtrlShift) {
			this->textfile.DeleteLine(cursors, PGDirectionLeft);
		}
		this->Invalidate();
		break;
	case PGButtonEnter:
		if (modifier == PGModifierNone) {
			this->textfile.AddNewLine(cursors);
		} else if (modifier == PGModifierCtrl) {
			// FIXME: new line at end of every cursor's line
		} else if (modifier == PGModifierCtrlShift) {
			// FIXME: new line before every cursor's line
		}
		this->Invalidate();
	default:
		break;
	}
}

void TextField::ClearCursors(std::vector<Cursor*>& cursors) {
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		delete *it;
	}
	cursors.clear();
	active_cursor = NULL;
}

void TextField::KeyboardCharacter(char character, PGModifier modifier) {
	Logger::GetInstance()->WriteLogMessage(std::string("textField->KeyboardCharacter(") + std::to_string(character) + std::string(", ") + GetModifierName(modifier) + std::string(");"));

	if (modifier == PGModifierNone) {
		this->textfile.InsertText(character, cursors);
		this->Invalidate();
	} else {
		if (modifier & PGModifierCtrl) {
			switch (character) {
			case 'Z':
				this->textfile.Undo(cursors);
				this->Invalidate();
				break;
			case 'Y':
				this->textfile.Redo(cursors);
				Cursor::NormalizeCursors(this, cursors);
				this->Invalidate();
				break;
			case 'A':
				ClearCursors(cursors);
				this->cursors.push_back(new Cursor(&textfile, textfile.GetLineCount() - 1, textfile.GetLine(textfile.GetLineCount() - 1)->GetLength()));
				this->cursors.back()->end_character = 0;
				this->cursors.back()->end_line = 0;
				this->Invalidate();
				break;
			case 'C': {
				std::string text = textfile.CopyText(cursors);
				SetClipboardText(window, text);
				break;
			}
			case 'V': {
				if (modifier & PGModifierShift) {
					// FIXME: cycle through previous copy-pastes
				} else {
					textfile.PasteText(cursors, GetClipboardText(window));
				}
				this->Invalidate();
				break;
			}
			case 'S': {
				textfile.SaveChanges();
				break;
			}
			}
		}
	}
}

void TextField::KeyboardUnicode(char *character, PGModifier modifier) {
	// FIXME
}

void TextField::MouseWheel(int x, int y, int distance, PGModifier modifier) {
	if (modifier == PGModifierNone) {
		if (SetScrollOffset(this->lineoffset_y - (distance / 120) * 2)) {
			this->Invalidate();
		}
	}
}

bool
TextField::SetScrollOffset(ssize_t offset) {
	ssize_t new_y = std::min(std::max(offset, (ssize_t)0), (ssize_t)(textfile.GetLineCount() - 1));
	if (new_y != this->lineoffset_y) {
		this->lineoffset_y = new_y;
		return true;
	}
	return false;
}

void TextField::InvalidateLine(int line) {
	this->Invalidate(PGRect(0, (line - this->lineoffset_y) * line_height, this->width, line_height));
}

void TextField::InvalidateBeforeLine(int line) {
	this->Invalidate(PGRect(0, 0, this->width, (line - this->lineoffset_y) * line_height));
}

void TextField::InvalidateAfterLine(int line) {
	this->Invalidate(PGRect(0, (line - this->lineoffset_y) * line_height, this->width, this->height));
}

void TextField::InvalidateBetweenLines(int start, int end) {
	if (start > end) {
		InvalidateBetweenLines(end, start);
		return;
	}
	this->Invalidate(PGRect(0, (start - this->lineoffset_y) * line_height, this->width,
		(end - this->lineoffset_y) * line_height - (start - this->lineoffset_y) * line_height + line_height));
}
