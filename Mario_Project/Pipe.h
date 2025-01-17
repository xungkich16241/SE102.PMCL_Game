﻿#pragma once

#include "GameObject.h"
#include "Animation.h"
#include "Animations.h"

// Bounding Box
#define PIPE_BBOX_WIDTH 32
#define PIPE_BBOX_HEIGHT 32

// Animation ID
// Pipe hướng lên trên
#define ID_ANI_PIPE 14000
// Pipe hướng xuóng
#define ID_ANI_PIPE_DOWN 14010


class CPipe: public CGameObject
{
public:
	CPipe(float x, float y, int type) : CGameObject(x, y, type) {}
	void virtual Render();
	void virtual Update(DWORD dt);
	void virtual GetBoundingBox(float& left, float& top, float& right, float& bottom);

	void virtual SetState(int state);
};

