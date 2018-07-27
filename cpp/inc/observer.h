/* Observer Pattern v0.3
 * 2018-07-22
 *
 * Subject	(base)				Observer (base)
 *  notify() --- update -------> Obs1 (derived)
 *		     ---- update ----------------------> Obs2 (dervied)		 
 *  <------------- getParam ---- update()
 *	<--- getParam ------------------------------ update()
 */

#pragma once
#include <list>
#include <string>
#include <algorithm>

void updateObserver(class Observer* pObserver);

class Subject {
protected:
	std::list<Observer*> mObservers;
public:
	Subject() {}
	void attach(Observer* pObserver) { mObservers.push_back(pObserver); }
	void detach(Observer* pObserver) { mObservers.remove(pObserver); }
	void notifyObservers() {
		std::for_each(mObservers.begin(), mObservers.end(), updateObserver);
	}
    virtual std::string getParam(std::string name) = 0;
    virtual bool setParam(std::string name, std::string value) = 0;
};

class Observer {
protected:
	Subject* mSubject;
public:
	Observer(Subject* pSubject) : mSubject(pSubject) {}
    virtual void update() = 0;
};