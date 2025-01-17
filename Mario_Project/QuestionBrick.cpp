﻿#include "QuestionBrick.h"
#include "PlayScene.h"

void CQuestionBrick::Render()
{
	int ani_id = -1;
	if (state != QUESTION_BRICK_STATE_IDLE_NO_MUSHROOM)
		ani_id = ID_ANI_QUESTION_BRICK_QUESTION_MARK_MOVING;
	else
		ani_id = ID_ANI_QUESTION_BRICK_NO_QUESTION_MARK_MOVING;

	float left, top, right, bottom;
	this->GetBoundingBox(left, top, right, bottom);
	float width = right - left;
	float height = bottom - top;

	CAnimations::GetInstance()->Get(ani_id)->Render(x + width / 2, y + height / 2);

	RenderBoundingBox();
}

void CQuestionBrick::Update(DWORD dt, vector<LPGAMEOBJECT>* coObjects)
{
	if (state == QUESTION_BRICK_STATE_BOUNCING_UP)
	{
		vy += ay * dt;
		y += vy * dt;
	}

	if (y > default_pos_y)	// Nếu ? brick rớt xuống thâp hơn vị trí mặc định thì đặt lại vị trí mặt định, vy=0, ay=0
	{
		SetState(QUESTION_BRICK_STATE_IDLE_NO_MUSHROOM);
		if (itemInsideId > -1)
		{
			unordered_map<int, LPGAMEOBJECT> item_list = ((LPPLAYSCENE)CGame::GetInstance()->GetCurrentScene())->GetItemList();
			CGameObject* item = item_list[itemInsideId];
			if (item == NULL)
				return;
			item->SetState(MUSHROOM_STATE_RISING);
		}
	}


	//DebugOutTitle(L"? Brick: y: %0.2f", y);
}

void CQuestionBrick::SetState(int state)
{
	CBrick::SetState(state);

	switch (state)
	{
	case QUESTION_BRICK_STATE_IDLE_HAVE_MUSHROOM:
		vy = 0;
		ay = 0;
		break;
	case QUESTION_BRICK_STATE_BOUNCING_UP:
		vy = -QUESTION_BRICK_BOUNCING_UP_SPEED;
		ay = QUESTION_BRICK_GRAVITY;
		break;
	case QUESTION_BRICK_STATE_IDLE_NO_MUSHROOM:
		vy = 0;
		ay = 0;
		y = default_pos_y;
		break;
	}
}

void CQuestionBrick::GetBoundingBox(float& l, float& t, float& r, float& b)
{
	l = x;
	t = y;
	r = l + QUESTION_BRICK_BBOX_WIDTH;
	b = t + QUESTION_BRICK_BBOX_HEIGHT;
}
