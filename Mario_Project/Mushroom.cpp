﻿#include "Mushroom.h"


void CMushroom::Update(DWORD dt, vector<LPGAMEOBJECT>* coObjects)
{
	if (state != MUSHROOM_STATE_IDLE)
	{
		vy += ay * dt;
		if (vy >= MUSHROOM_SPEED_MAX_FALL_DOWN)
			vy = MUSHROOM_SPEED_MAX_FALL_DOWN;

		// Kiểm tra xem mushroom đã hết overlap với ? brick hay chưa
		// để có thể làm cho mushroom khỏi trồi lên nữa, sau đó sẽ cho mushroom di chuyển
		if ((y + DISTANCE_ADD_GRAVITY) < default_pos_y)
		{
			SetState(MUSHROOM_STATE_MOVING);
		}

		CCollision::GetInstance()->Process(this, dt, coObjects);
	}
	
	//DebugOutTitle(L"Mushroom: x: %0.2f y: %0.2f", x, y);
	//DebugOutTitle(L"Mushroom: vx: %0.2f vy: %0.2f ax: %0.2f ay: %0.2f", vx, vy, ax, ay);
	//DebugOutTitle(L"State: %d", state);
}

void CMushroom::Render()
{
	if (state == MUSHROOM_STATE_IDLE)
		return;

	CAnimations* animations = CAnimations::GetInstance();

	int ani_id = ID_ANI_MUSHROOM;

	float left, top, right, bottom;
	this->GetBoundingBox(left, top, right, bottom);
	float width = right - left;
	float height = bottom - top;

	CAnimations::GetInstance()->Get(ani_id)->Render(x + width / 2, y + height / 2);

	RenderBoundingBox();
}

void CMushroom::OnNoCollision(DWORD dt)
{
	x += vx * dt;
	y += vy * dt;
}

void CMushroom::OnCollisionWith(LPCOLLISIONEVENT e)
{
	if (e->obj->IsBlocking())
	{
		if (e->ny != 0)
		{
			vy = 0;
		}
		else if (e->nx != 0)
		{
			vx = -vx;
		}
	}
}

void CMushroom::SetState(int state)
{
	CGameObject::SetState(state);

	switch (state)
	{
		case MUSHROOM_STATE_IDLE:
			ax = 0;
			ay = 0;
			break;
		case MUSHROOM_STATE_RISING:
			vy = -MUSHROOM_RISING_UP_SPEED;
			break;
		case MUSHROOM_STATE_MOVING:
			vx = MUSHROOM_WALKING_SPEED;
			ay = MUSHROOM_GRAVITY;
			break;
	}
}

void CMushroom::GetBoundingBox(float& left, float& top, float& right, float& bottom)
{
	left = x;
	top = y;
	right = left + MUSHROOM_BBOX_WIDTH;
	bottom = top + MUSHROOM_BBOX_HEIGHT;
}
