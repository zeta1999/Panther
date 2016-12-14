#pragma once

// A composite control that contains both a TextField and a TabControl
// This control exists because both the TabControl and TextField can be controlled at
// the same time using keys (e.g. Ctrl+Tab to move tabs while typing in a textfield)

#include "control.h"
#include "textfield.h"
#include "tabcontrol.h"
#include "windowfunctions.h"

#define TEXT_TAB_HEIGHT 20

class TabbedTextField : public Control {
public:
	TabbedTextField(PGWindowHandle window, TextFile* file);

	void MouseWheel(int x, int y, int distance, PGModifier modifier);
	bool KeyboardButton(PGButton button, PGModifier modifier);
	void KeyboardCharacter(char character, PGModifier modifier);
	void KeyboardUnicode(char* character, PGModifier modifier);
	void PeriodicRender(void);
	void Draw(PGRendererHandle, PGIRect*);

	void MouseClick(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseDoubleClick(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseMove(int x, int y, PGMouseButton buttons);

	void OnResize(PGSize old_size, PGSize new_size);
private:
	TextField* textfield;
	TabControl* tabs;
};