#include "CApnsPostData.h"

// TODO: your implementation here

// Constructor implementation
//static int countttt = 0;
CApnsPostData::CApnsPostData()
	:nva(nullptr), body(nullptr)
{
	//log("new ApnsPostData:%p count:%d", this, ++countttt);
}

// Destructor implementation

CApnsPostData::~CApnsPostData()
{
	if (nva)
	{
		delete nva;
		nva = nullptr;

		navArrayLen = 0;
		//mapIndex = 0;
		;
	}

	if (body)
	{
		free(body);
		bodyLen = 0;

		//log("delet ~ApnsPostData:%p mapIndex:%u, count:%d\n",this, mapIndex, --countttt);
	}
}


bool CApnsPostData::Timeout(int iTimeRate)
{
	if (time(NULL) -sendTime > iTimeRate)
	{
		return true;
	}
	
	return false;
}
