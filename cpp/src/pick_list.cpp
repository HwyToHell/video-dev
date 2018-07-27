/// usage: instantiate PickList, optionally with item array (strings)  
/// console origin (1,1): upper left corner
#include "stdafx.h"
#include "../inc/pick_list.h"

#include <iostream>
#include <vector>
#include "../inc/rlutil.h"

///////////////////////////////////////////////////////////////////////////////
// conio helper functions
// console origin: x = 1, y = 1
///////////////////////////////////////////////////////////////////////////////

// get x position of cursor
int wherex() {
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hStdout, &csbi);
	int x = csbi.dwCursorPosition.X + 1;
	return x;
}

// get x position of cursor
int wherey() {
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hStdout, &csbi);
	int y = csbi.dwCursorPosition.Y + 1;
	return y;
}

// clear line
void clrln() {
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hStdout, &csbi);
	int lineWidth = csbi.dwSize.X;
	COORD coord = csbi.dwCursorPosition;
	coord.X = 0;
	SetConsoleCursorPosition(hStdout, coord);
	for (int i=0; i<lineWidth; ++i)
		std::cout << " ";
	coord.X = 0;
	SetConsoleCursorPosition(hStdout, coord);
}


///////////////////////////////////////////////////////////////////////////////
// PickList
// defaults: no item array, print at current cursor position (offsetY == 0)
// first item selected (idxSelect == 0)
///////////////////////////////////////////////////////////////////////////////
PickList::PickList(std::string itemName, StrArray* pItemArray, int offsetY, int idxSelect) :
	captionY(1), m_idxActual(idxSelect), m_idxPrevious(0), m_itemName(itemName), 
	m_offsetY(offsetY)  {
	
	// offsetY = 0 -> set current y position as offsetY
	if (offsetY == 0) 
		m_offsetY = wherey();
	rlutil::saveDefaultColor();
	
	// display initial array, if data available
	if (pItemArray) {
		m_itemArray = *pItemArray;
		printAllItems();
		printChangedSelection();
	}
}


bool PickList::addItem(std::string item) {
	m_itemArray.push_back(item);
	printAllItems();
	printChangedSelection();
	return true;
}


bool PickList::decIndex() {
	if (m_itemArray.size() == 0)
		return false;
	--m_idxActual;
	if (m_idxActual < 0)
		m_idxActual = (int)m_itemArray.size()-1;
	return true;
}


int PickList::getSelection() {
	using namespace rlutil;

	while(true) {
		if (kbhit()) {
			int k = getkey();
			if (k == KEY_UP) {
				if (decIndex())
					printChangedSelection();
			} else if (k == KEY_DOWN) {
				if (incIndex())
					printChangedSelection();
			} else if (k == KEY_ENTER) 
				break;
		}
	}

	// set cursor after last printed line
	gotoxy(1, m_offsetY + captionY + (int)m_itemArray.size() + 2);
	showcursor();
	return(m_idxActual);
}


bool PickList::incIndex() {
	if (m_itemArray.size() == 0)
		return false;
	++m_idxActual;
	if (m_idxActual >= (int)m_itemArray.size())
		m_idxActual = 0;
	return true;
}


void PickList::printAllItems() {
	using namespace std;

	gotoxy(1, m_offsetY);
	clrln();
	cout << "Please select " << m_itemName << ":" << endl;

	for (int idx = 0; idx < (int)m_itemArray.size(); ++idx) {
		clrln();
		printItem(idx);
	}

	cout << "Use <up> or <down> key to change selection," << endl;
	cout << "hit <enter> to continue with selected item" << endl;
	return;
}


void PickList::printChangedSelection() {
	using namespace std;
	using namespace rlutil;

	// print previous item in default color
	resetColor();
	gotoxy(1, m_idxPrevious + m_offsetY + captionY);
	printItem(m_idxPrevious);

	// print actual item in highlight color
	setBackgroundColor(GREY);
	setColor(BLACK);
	gotoxy(1, m_idxActual + m_offsetY + captionY);
	printItem(m_idxActual);

	// reset previous
	resetColor();
	hidecursor();
	m_idxPrevious = m_idxActual;
}


bool PickList::printItem(int idx) {
	using namespace std;
	if (idx < 0 || idx >= (int)m_itemArray.size())
		return false;
	else {
		gotoxy(1, idx + m_offsetY + captionY);
		cout << idx << " " << m_itemArray[idx] << endl;
		return true;
	}
}


bool PickList::removeItem(int idx) {
	return true;
}


bool PickList::setSelection(int idx) {
	using namespace std;
	
	if (idx < 0 || idx >= (int)m_itemArray.size()) {
		cerr << "index out of range" << endl;
		return false;
	} else {
		m_idxActual = idx;
		printChangedSelection();
		return true;
	}
}