#pragma once
#include <memory>

namespace controls
{

class _ControlCollectionPrivate;

class _ControlCollection
{
private:
	std::unique_ptr<_ControlCollectionPrivate> data;

public:
	_ControlCollection();

	~_ControlCollection();

	void draw();
};

};