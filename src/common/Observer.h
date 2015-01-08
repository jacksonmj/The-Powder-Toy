#ifndef Observer_h
#define Observer_h

#include <vector>

class ObserverSubject;

class Observer
{
	friend class ObserverSubject;
protected:
	ObserverSubject *subject;
public:
	Observer() : subject(NULL) {}
	virtual void Run() = 0;
	virtual ~Observer() {}
};

class ObserverSubject
{
protected:
	std::vector<Observer*> observers;
	bool running;// Whether Trigger() is currently being run
public:
	ObserverSubject() : running(false) {}

	void Attach(Observer *obs)
	{
		obs->subject = this;
		observers.push_back(obs);
	}
	void Detach(Observer *obs)
	{
		for (std::vector<Observer*>::iterator it=observers.begin(); it<observers.end(); ++it)
		{
			if (obs==(*it))
			{
				observers.erase(it);
			}
		}
	}
	// Runs all registered callback functions
	void Trigger()
	{
		if (running)
		{
			// Prevent recursion
			//cout << "Error: ObserverSubject::Trigger was called recursively" << endl;
			return;
		}
		running = true;
		for (std::vector<Observer*>::iterator it=observers.begin(); it<observers.end(); ++it)
		{
			(*it)->Run();
		}
		running = false;
	}
	// Return the number of registered callbacks
	int ObserversCount()
	{
		return observers.size();
	}
	virtual ~ObserverSubject()
	{
		for (std::vector<Observer*>::iterator it=observers.begin(); it<observers.end(); ++it)
		{
			(*it)->subject = NULL;
		}
	}
};

template <class T>
class Observer_ClassMember : public Observer
{
	// Using templates to allow member functions in different classes of the form "void SomeClass:func()"
	// to be called without needing a different derived class for each SomeClass
private:
	T *targetObject;//pointer to the object which will have a member function called
	void (T::*targetFunc)();//pointer to the member function to call
public:
	Observer_ClassMember(ObserverSubject &s, T *obj, void (T::*func)())
		: targetObject(obj), targetFunc(func)
	{
		s.Attach(this);
	}
	Observer_ClassMember(const Observer_ClassMember<T>& other)
		: targetObject(other.targetObject), targetFunc(other.targetFunc)
	{
		if (other.subject)
			other.subject->Attach(this);
	}
	virtual ~Observer_ClassMember()
	{
		if (subject)
			subject->Detach(this);
	}
	virtual void Run()
	{
		(targetObject->*targetFunc)();
	}
};

class Observer_PlainFunc : public Observer
{
private:
	void (*targetFunc)();//pointer to the function to call
public:
	Observer_PlainFunc(ObserverSubject &s, void (*func)())
		: targetFunc(func)
	{
		s.Attach(this);
	}
	Observer_PlainFunc(const Observer_PlainFunc& other)
		: targetFunc(other.targetFunc)
	{
		if (other.subject)
			other.subject->Attach(this);
	}
	virtual ~Observer_PlainFunc()
	{
		if (subject)
			subject->Detach(this);
	}
	virtual void Run()
	{
		(*targetFunc)();
	}
};

#endif
