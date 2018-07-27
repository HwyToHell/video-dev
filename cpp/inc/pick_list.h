/// pick list class for selecting one item from list in console application
/// for windows and linux, depends on rlutil.h

#pragma once
#include <string>
#include <vector>
#include <windows.h>


// TODO check on console limits
class PickList {
public:
	typedef std::vector<std::string> StrArray;
	PickList(std::string itemName, 
			StrArray* pItemArray = nullptr, // default: empty item array 
			int offsetY = 0,				// default: continue at current y-position
			int idxSelect = 0);				// default: first index in array selected
	bool		addItem(std::string item);
	int			getSelection();
	bool		removeItem(int idx); // not implemented yet
	bool		setSelection(int idx);
private:
	const int	captionY;
	int			m_idxPrevious;
	int			m_idxActual;
	StrArray	m_itemArray;
	std::string	m_itemName;
	int			m_offsetY;

	bool		decIndex();
	bool		incIndex(); 
	void		printAllItems();
	void		printChangedSelection();
	bool		printItem(int idx);
};


// conio helper fcn
int wherex();
int wherey();
void clrln();