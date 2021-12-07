﻿#include <algorithm>
#include "debug.h"

#include "Mario.h"
#include "Game.h"

#include "Goomba.h"
#include "Coin.h"
#include "Portal.h"
#include "QuestionBrick.h"
#include "WingGoomba.h"
#include "Koopa.h"

#include "Collision.h"

void CMario::Update(DWORD dt, vector<LPGAMEOBJECT>* coObjects)
{
	vy += ay * dt;
	vx += ax * dt;

	if (abs(vx) > abs(maxVx)) vx = maxVx;
	if (vy > MARIO_FALL_DOWN_SPEED_Y) vy = MARIO_FALL_DOWN_SPEED_Y;



	// reset untouchable timer if untouchable time has passed
	if (GetTickCount64() - untouchable_start > MARIO_UNTOUCHABLE_TIME)
	{
		untouchable_start = 0;
		untouchable = 0;
	}

	if (GetTickCount64() - attackTail_start > MARIO_ATTACK_TAIL_TIME)
		attackTail_start = 0;

	// Nếu vẫn còn trong thời gian rơi chậm
	// Thì giảm tốc độ vx và vy bằng tốc độ khi rơi chậm
	if ((0 <= (GetTickCount64() - fallSlow_start)) &&
		((GetTickCount64() - fallSlow_start) <= MARIO_FALL_SLOW_TIME))
	{
		if (nx > 0)
			maxVx = MARIO_SPEED_FALL_SLOW_X;
		else
			maxVx = -MARIO_SPEED_FALL_SLOW_X;
		vy = MARIO_SPEED_FALL_SLOW_Y;

		DebugOut(L"Kich hoat fallSlow_start\n");
	}
	// Hoặc nếu thời gian rơi chậm đã hết
	// Thì không cho rơi chậm nữa
	else if ((GetTickCount64() - fallSlow_start) > MARIO_FALL_SLOW_TIME)
	{

		fallSlow_start = 0;

		//DebugOut(L"Tat fallSlow_start\n");
	}


	// Nếu vẫn đang trong thời gian bay
	// Thì sử dụng tốc độ di chuyển (theo trục x) đã được trừ đi lực cản không khí theo trục x
	if (this->CanContinueFly())
	{
		if (nx > 0)
			maxVx = MARIO_SPEED_FLYING_X;
		else
			maxVx = -MARIO_SPEED_FLYING_X;

		DebugOut(L"Đang bay\n");
	}
	// Nếu không
	// Thì không cho khả năng bay khi nhấm phím nhảy nữa
	else
	{
		flying_start = 0;

		//DebugOut(L"Đang không bay\n");
	}
	//DebugOutTitle(L"Mario flying speed: vx = %0.2f vy = %0.2f max_vx = %0.2f", vx, vy, maxVx);


	isOnPlatform = false;

	CCollision::GetInstance()->Process(this, dt, coObjects);

	//this->SetState(MARIO_STATE_FLYING);

}

void CMario::OnNoCollision(DWORD dt)
{
	x += vx * dt;
	y += vy * dt;
}

void CMario::OnCollisionWithInvisiblePlatform(LPCOLLISIONEVENT e)
{
	//if (e->ny < 0)	// Mario đang đứng trên Invisible Platform
	//{
	//	this->y += (this->vy * e->dt) * e->t +
	//		e->ny * BLOCK_PUSH_FACTOR;		// Đẩy Mario ra khỏi va chạm

	//	this->vy = 0;												// Vì cũng là đang đứng trên platform, nên phải đặt lại bằng 0
	//	this->isOnPlatform = true;
	//}
}

void CMario::OnCollisionWith(LPCOLLISIONEVENT e)
{
	if (e->obj->IsBlocking())
	{
		if (e->ny != 0)
		{
			vy = 0;
			if (e->ny < 0) isOnPlatform = true;
		}
		else if (e->nx != 0)
		{
			vx = 0;
		}

	}


	if (e->obj->GetType() == OBJECT_TYPE_GOOMBA)
		OnCollisionWithGoomba(e);
	else if (e->obj->GetType() == OBJECT_TYPE_WING_GOOMBA)
		OnCollisionWithWingGoomba(e);
	else if (e->obj->GetType() == OBJECT_TYPE_COIN)
		OnCollisionWithCoin(e);
	else if (e->obj->GetType() == OBJECT_TYPE_PORTAL)
		OnCollisionWithPortal(e);
	else if (e->obj->GetType() == OBJECT_TYPE_INVISIBLE_PLATFORM)
		OnCollisionWithInvisiblePlatform(e);
	else if (e->obj->GetType() == OBJECT_TYPE_QUESTION_BRICK)
		OnCllisionWithQuestionBrick(e);
	else if (e->obj->GetType() == OBJECT_TYPE_MUSHROOM)
		OnCollisionWithMushroom(e);
	else if (e->obj->GetType() == OBJECT_TYPE_KOOPA)
		OnCollisionWithKoopa(e);
}

void CMario::OnCollisionWithKoopa(LPCOLLISIONEVENT e)
{
	CKoopa* koopa = dynamic_cast<CKoopa*>(e->obj);

	if (koopa->GetState() == KOOPA_STATE_WALKING)
	{
		if (e->ny < 0)	// Mario hit từ bên trên
		{
			koopa->SetState(KOOPA_STATE_SHELLING);
			vy = -MARIO_JUMP_DEFLECT_SPEED;
		}
		else  // Mario va chạm theo các hướng còn lại
		{
			if (untouchable == 0)
			{
				if (this->GetLevel() == MARIO_LEVEL_SMALL)
				{
					DebugOut(L">>> Mario DIE >>> \n");
					this->SetState(MARIO_STATE_DIE);
				}
				else if (this->GetLevel() == MARIO_LEVEL_BIG)
				{
					this->SetLevel(MARIO_LEVEL_SMALL);
					StartUntouchable();
				}
			}
		}
	}
	else if (koopa->GetState() == KOOPA_STATE_SHELLING)
	{
		float mario_x, mario_y;
		float koopa_x, koopa_y;
		this->GetPosition(mario_x, mario_y);
		koopa->GetPosition(koopa_x, koopa_y);

		if (mario_x < koopa_x)	// Mario ở phía bên trái của Koopa
		{
			koopa->SetNormalDirectionX(DIRECTION_RIGHT);	// Cho xoay sang phải
			koopa->SetState(KOOPA_STATE_SPIN_SHELL);	// Đặt lại state để có chỉ số vật lý sau: vận tốc x có trị tuyệt đối rất lớn và cùng dấu (dương) với vectoc normal
		}
		else  // Mario ở phía bên phải của Koopa
		{
			koopa->SetNormalDirectionX(DIRECTION_LEFT);		// Cho xoay sang trái
			koopa->SetState(KOOPA_STATE_SPIN_SHELL);	// Đặt lại state để có chỉ số vật lý sau: vận tốc x có giá trị tuyệt đối rất lớn và cùng dấu (âm) với vector normal
		}
	}
	else if (koopa->GetState() == KOOPA_STATE_SPIN_SHELL)
	{
		// Nếu Mario nhảy lên đầu Koopa khi Koopa đang xoay thì có thể khiến nó ngừng xoay
		if (e->ny == DIRECTION_UP)
		{
			koopa->SetState(KOOPA_STATE_SHELLING);
			vy = -MARIO_JUMP_DEFLECT_SPEED;
		}
		// Mario va chạm với Koopa theo các hướng còn lại (trái, phải và bên trên của Mario)
		// Nếu Mario có thể bị đụng vào, thì mới xử lý va chạm. Còn nếu đang trong thời gian "không thể bị đụng vào" thì thôi, không làm gì cả
		else if (this->untouchable == 0)
		{
			// Nếu Mario có 2 mạng (level Big) thì giảm thành còn 1 mạng (level Small) rồi bắt đầu tính giờ cho untouchable
			if (this->GetLevel() == MARIO_LEVEL_BIG)
			{
				this->SetLevel(MARIO_LEVEL_SMALL);
				StartUntouchable();
			}
			// Còn nếu chỉ còn 1 mạng (level Small) thì cho Mario die luôn
			else if (this->GetLevel() == MARIO_LEVEL_SMALL)
			{
				DebugOut(L">>> Marioo DIE >>> \n");
				this->SetState(MARIO_STATE_DIE);
			}
		}
	}
}

void CMario::OnCollisionWithMushroom(LPCOLLISIONEVENT e)
{
	// Điều kiện kiểm tra state của Mushroom là để tránh Mario ăn được (va chạm) Mushroom trong khi Mushroom vẫn đang bị che bởi ? Brick
	// Vì dù Mario được push back bởi ? Brick, nhưng vẫn còn sự kiện Mario đã va chạm với Mushroom, nên Mushroom sẽ bị ăn bởi Mario
	// và bị delete mặc dù Mushroom vẫn chưa trồi lên
	if (e->obj->GetState() != MUSHROOM_STATE_IDLE)
	{
		//DebugOutTitle(L"Mario and Mushroom collided");
		e->obj->Delete();

		if (level == MARIO_LEVEL_SMALL)
		{
			SetLevel(MARIO_LEVEL_BIG);
		}
	}
}

void CMario::OnCllisionWithQuestionBrick(LPCOLLISIONEVENT e)
{
	if (e->ny > 0 &&
		e->obj->GetState() == QUESTION_BRICK_STATE_IDLE_HAVE_MUSHROOM)
	{
		e->obj->SetState(QUESTION_BRICK_STATE_BOUNCING_UP);
	}
}

void CMario::OnCollisionWithGoomba(LPCOLLISIONEVENT e)
{
	CGoomba* goomba = dynamic_cast<CGoomba*>(e->obj);

	// jump on top >> kill Goomba and deflect a bit 
	if (e->ny < 0)
	{
		if (goomba->GetState() != GOOMBA_STATE_DIE)
		{
			goomba->SetState(GOOMBA_STATE_DIE);
			vy = -MARIO_JUMP_DEFLECT_SPEED;
		}
	}
	else // hit by Goomba
	{
		if (untouchable == 0)
		{
			if (goomba->GetState() != GOOMBA_STATE_DIE)
			{
				if (level > MARIO_LEVEL_SMALL)
				{
					level = MARIO_LEVEL_SMALL;
					StartUntouchable();
				}
				else
				{
					DebugOut(L">>> Mario DIE >>> \n");
					SetState(MARIO_STATE_DIE);
				}
			}
		}
	}
}

void CMario::OnCollisionWithWingGoomba(LPCOLLISIONEVENT e)
{
	CWingGoomba* wing_goomba = dynamic_cast<CWingGoomba*>(e->obj);

	if (e->ny < 0)	// Mario nhảy lên đầu Wing Goomba
	{
		if (wing_goomba->GetState() != WING_GOOMBA_STATE_DIE)
		{
			if (wing_goomba->GetLevel() == WING_GOOMBA_LEVEL_HAVE_WING)	// Nếu Wing Goomba đang level "có cánh" thì hạ level xuống thành "không cánh"
			{
				wing_goomba->SetLevel(WING_GOOMBA_LEVEL_NO_WING);	// Set level "không cánh"
				wing_goomba->SetState(WING_GOOMBA_STATE_WALKING);	// Vì đang là level "không cánh" nên chỉ có thể có chỉ số vật lý của state walking
				vy = -MARIO_JUMP_DEFLECT_SPEED;
			}
			else  // Nếu Wing Goomba đang level "không cánh" thì cho die luôn
			{
				wing_goomba->SetState(WING_GOOMBA_STATE_DIE);
				vy = -MARIO_JUMP_DEFLECT_SPEED;
			}
		}
	}
	else  // Mario bị đụng bởi Wing Goomba
	{
		if (untouchable == 0)	// Nếu Mario có thể bị đụng vào
		{
			if (level > MARIO_LEVEL_SMALL)	// Mario đang ở level dạng Big thì chuyển lại thành level Small và bắt đầu khoảng thời gian Mario không thể bị đụng vào
			{
				SetLevel(MARIO_LEVEL_SMALL);
				StartUntouchable();
			}
			else  // Mario đang ở level Small thì cho chết luôn
			{
				DebugOut(L">>> Mario DIE >>> \n");
				SetState(MARIO_STATE_DIE);
			}
		}
	}
}

void CMario::OnCollisionWithCoin(LPCOLLISIONEVENT e)
{
	e->obj->Delete();
	coin++;
}

void CMario::OnCollisionWithPortal(LPCOLLISIONEVENT e)
{
	CPortal* p = (CPortal*)e->obj;
	CGame::GetInstance()->InitiateSwitchScene(p->GetSceneId());
}

//
// Get animation ID for small Mario
//
int CMario::GetAniIdSmall()
{
	int aniId = -1;
	if (!isOnPlatform)
	{
		if (abs(vx) == MARIO_RUNNING_SPEED)
		{
			if (nx >= 0)
				aniId = ID_ANI_MARIO_SMALL_JUMP_RUN_RIGHT;
			else
				aniId = ID_ANI_MARIO_SMALL_JUMP_RUN_LEFT;
		}
		else
		{
			if (nx >= 0)
				aniId = ID_ANI_MARIO_SMALL_JUMP_WALK_RIGHT;
			else
				aniId = ID_ANI_MARIO_SMALL_JUMP_WALK_LEFT;
		}
	}
	else
		if (isSitting)
		{
			if (nx > 0)
				aniId = ID_ANI_MARIO_SIT_RIGHT;
			else
				aniId = ID_ANI_MARIO_SIT_LEFT;
		}
		else
			if (vx == 0)
			{
				if (nx > 0) aniId = ID_ANI_MARIO_SMALL_IDLE_RIGHT;
				else aniId = ID_ANI_MARIO_SMALL_IDLE_LEFT;
			}
			else if (vx > 0)
			{
				if (ax < 0)
					aniId = ID_ANI_MARIO_SMALL_BRACE_RIGHT;
				else if (ax == MARIO_ACCEL_RUN_X)
					aniId = ID_ANI_MARIO_SMALL_RUNNING_RIGHT;
				else if (ax == MARIO_ACCEL_WALK_X)
					aniId = ID_ANI_MARIO_SMALL_WALKING_RIGHT;
			}
			else // vx < 0
			{
				if (ax > 0)
					aniId = ID_ANI_MARIO_SMALL_BRACE_LEFT;
				else if (ax == -MARIO_ACCEL_RUN_X)
					aniId = ID_ANI_MARIO_SMALL_RUNNING_LEFT;
				else if (ax == -MARIO_ACCEL_WALK_X)
					aniId = ID_ANI_MARIO_SMALL_WALKING_LEFT;
			}

	if (aniId == -1) aniId = ID_ANI_MARIO_SMALL_IDLE_RIGHT;

	return aniId;
}

int CMario::GetAniRaccon()
{
	int ani_id = -1;

	float shift_x = 0;
	float shift_y = 0;

	float left, top, right, bottom;
	this->GetBoundingBox(left, top, right, bottom);
	float width = right - left;
	float height = bottom - top;

	if (this->IsTailAttacking())
	{
		if (nx > 0)
			return ID_ANI_MARIO_RACCON_ATTACK_TAIL_RIGHT;
		else
			return ID_ANI_MARIO_RACCON_ATTACK_TAIL_LEFT;
	}

	if (!isOnPlatform)	// Đang trong không trung
	{
		// Nếu đang trong thời gian bay
		if (this->CanContinueFly())
		{
			if (this->vy < 0)
			{
				if (nx > 0)
				{
					ani_id = ID_ANI_MARIO_RACCON_FLYING_RIGHT;
					CAnimations::GetInstance()->Get(ani_id)->Render(x + width / 2, y + height / 2);
				}

				else
					ani_id = ID_ANI_MARIO_RACCON_FLYING_LEFT;
			}
			else if (abs(maxVx) == MARIO_SPEED_FLYING_X)
			{
				if (nx > 0)
					ani_id = ID_ANI_MARIO_RACCON_JUMP_RUN_RIGHT;
				else
					ani_id = ID_ANI_MARIO_RACCON_JUMP_RUN_LEFT;
			}
		}

		// Đang dùng đuôi để rơi chậm hơn
		else if (abs(maxVx) == MARIO_SPEED_FALL_SLOW_X &&
			vy == MARIO_SPEED_FALL_SLOW_Y)
		{
			if (nx > 0)
				ani_id = ID_ANI_MARIO_RACCON_FALL_SLOW_RIGHT;
			else
				ani_id = ID_ANI_MARIO_RACCON_FALL_SLOW_LEFT;
		}
		else if (abs(vx) == MARIO_RUNNING_SPEED) // Đang di chuyển với tốc độ nhanh (gia tốc chạy) trong không trung
		{
			if (nx > 0)
				ani_id = ID_ANI_MARIO_RACCON_JUMP_RUN_RIGHT;
			else
				ani_id = ID_ANI_MARIO_RACCON_JUMP_RUN_LEFT;
		}
		else  // Đang di chuyển với tốc độ bình thường (gia tốc đi bộ) trong không trung
		{
			if (nx > 0)
			{
				ani_id = ID_ANI_MARIO_RACCON_JUMP_WALK_RIGHT;
			}

			else
				ani_id = ID_ANI_MARIO_RACCON_JUMP_WALK_LEFT;
		}
	}
	else
	{
		if (isSitting)	// Đang ngồi
		{
			if (nx > 0)	// Đang quay mặt sang bên phải
				ani_id = ID_ANI_MARIO_RACCON_SIT_RIGHT;
			else        // Đang quay mặt sang bên trái
				ani_id = ID_ANI_MARIO_RACCON_SIT_LEFT;
		}
		else  // Đang không ngồi == Đang {đứng yên, đi bộ, chạy}
		{
			if (vx == 0)  // Đang đứng yên
			{
				if (nx >= 0)
				{
					ani_id = ID_ANI_MARIO_RACCON_IDLE_RIGHT;
					shift_x = -7; // dư 7 pixel
				}
				else
				{
					ani_id = ID_ANI_MARIO_RACCON_IDLE_LEFT;
				}
					
			}
			else if (vx > 0)	// Đang chạy sang bên phải
			{
				if (ax < 0)		// Đang thắng
					ani_id = ID_ANI_MARIO_RACCON_BRACE_RIGHT;
				else if (ax == MARIO_ACCEL_RUN_X)	// Đang chạy
				{
					ani_id = ID_ANI_MARIO_RACCON_RUNNING_RIGHT;
					shift_x = -4;
				}
				else if (ax == MARIO_ACCEL_WALK_X) // Đang đi bộ
				{
					ani_id = ID_ANI_MARIO_RACCON_WALKING_RIGHT;
					shift_x = -4;
				}
			}
			else  // Đang chạy sang bên trái
			{
				if (ax > 0)		// Đang thắng
				{
					//shift_x = 4;
					ani_id = ID_ANI_MARIO_RACCON_BRACE_LEFT;
				}

				else if (ax == -MARIO_ACCEL_RUN_X)	// Đang chạy
					ani_id = ID_ANI_MARIO_RACCON_RUNNING_LEFT;
				else if (ax == -MARIO_ACCEL_WALK_X)	// Đang đi bộ
				{
					ani_id = ID_ANI_MARIO_RACCON_WALKING_LEFT;
				}

			}
		}
	}

	if (ani_id == -1)
		ani_id = ID_ANI_MARIO_RACCON_IDLE_RIGHT;




	CAnimations::GetInstance()->Get(ani_id)->Render(x + width / 2 + shift_x,
		y + height / 2 + shift_y);
	RenderBoundingBox();

	return ani_id;
}


//
// Get animdation ID for big Mario
//
int CMario::GetAniIdBig()
{
	int aniId = -1;
	if (!isOnPlatform)
	{
		if (abs(vx) == MARIO_RUNNING_SPEED)
		{
			if (nx >= 0)
				aniId = ID_ANI_MARIO_JUMP_RUN_RIGHT;
			else
				aniId = ID_ANI_MARIO_JUMP_RUN_LEFT;
		}
		else
		{
			if (nx >= 0)
				aniId = ID_ANI_MARIO_JUMP_WALK_RIGHT;
			else
				aniId = ID_ANI_MARIO_JUMP_WALK_LEFT;
		}
	}
	else
		if (isSitting)
		{
			if (nx > 0)
				aniId = ID_ANI_MARIO_SIT_RIGHT;
			else
				aniId = ID_ANI_MARIO_SIT_LEFT;
		}
		else
			if (vx == 0)
			{
				if (nx > 0) aniId = ID_ANI_MARIO_IDLE_RIGHT;
				else aniId = ID_ANI_MARIO_IDLE_LEFT;
			}
			else if (vx > 0)
			{
				if (ax < 0)
					aniId = ID_ANI_MARIO_BRACE_RIGHT;
				else if (ax == MARIO_ACCEL_RUN_X)
					aniId = ID_ANI_MARIO_RUNNING_RIGHT;
				else if (ax == MARIO_ACCEL_WALK_X)
					aniId = ID_ANI_MARIO_WALKING_RIGHT;
			}
			else // vx < 0
			{
				if (ax > 0)
					aniId = ID_ANI_MARIO_BRACE_LEFT;
				else if (ax == -MARIO_ACCEL_RUN_X)
					aniId = ID_ANI_MARIO_RUNNING_LEFT;
				else if (ax == -MARIO_ACCEL_WALK_X)
					aniId = ID_ANI_MARIO_WALKING_LEFT;
			}

	if (aniId == -1) aniId = ID_ANI_MARIO_IDLE_RIGHT;

	return aniId;
}

void CMario::Render()
{
	int ani_id = -1;

	if (state == MARIO_STATE_DIE)	// Mario đã DIE
		ani_id = ID_ANI_MARIO_DIE;
	// Mario chưa DIE, vậy thì kiểm tra Level rồi lấy ra ani_id nằm trong những ani_id của level hiện tại,
	// rồi lấy ra ani_id phù hợp với các chỉ số vật lý hiện tại như: ax, nx và isSitting
	else
	{
		if (level == MARIO_LEVEL_BIG)
			ani_id = GetAniIdBig();
		else if (level == MARIO_LEVEL_SMALL)
			ani_id = GetAniIdSmall();
		else if (level == MARIO_LEVEL_RACCON)
		{
			ani_id = GetAniRaccon();
			return;
		}
	}

	float left, top, right, bottom;
	this->GetBoundingBox(left, top, right, bottom);
	float width = right - left;
	float height = bottom - top;

	CAnimations::GetInstance()->Get(ani_id)->Render(x + width / 2, y + height / 2);
	//CAnimations::GetInstance()->Get(ani_id)->Render(x, y);

	RenderBoundingBox();
	//DebugOutTitle(L"Mario: %0.2f, %0.2f, Bbox: %0.2f, %0.2f", x, y, left, top);

	//DebugOutTitle(L"Coins: %d", coin);
}

void CMario::SetState(int state)
{
	// DIE is the end state, cannot be changed! 
	if (this->state == MARIO_STATE_DIE) return;

	switch (state)
	{
	case MARIO_STATE_RUNNING_RIGHT:
		if (isSitting) break;
		// To-do:
		// Phải xét nếu Mario có đang đứng trên platform hay blocking object hay không, nếu có thì mới cho tăng tốc độ di chuyển (maxVx và ax) được (vật lý)
		maxVx = MARIO_RUNNING_SPEED;
		ax = MARIO_ACCEL_RUN_X;
		nx = 1;
		break;
	case MARIO_STATE_RUNNING_LEFT:
		if (isSitting) break;
		// To-do:
		// Phải xét nếu Mario có đang đứng trên platform hay blocking object hay không, nếu có thì mới cho tăng tốc độ di chuyển (maxVx và ax) được (vật lý)
		maxVx = -MARIO_RUNNING_SPEED;
		ax = -MARIO_ACCEL_RUN_X;
		nx = -1;
		break;
	case MARIO_STATE_WALKING_RIGHT:
		if (isSitting) break;
		maxVx = MARIO_WALKING_SPEED;
		ax = MARIO_ACCEL_WALK_X;
		nx = 1;
		break;
	case MARIO_STATE_WALKING_LEFT:
		if (isSitting) break;
		maxVx = -MARIO_WALKING_SPEED;
		ax = -MARIO_ACCEL_WALK_X;
		nx = -1;
		break;
	case MARIO_STATE_JUMP:
		if (isSitting) break;
		if (isOnPlatform)
		{
			if (abs(this->vx) == MARIO_RUNNING_SPEED)
				vy = -MARIO_JUMP_RUN_SPEED_Y;
			else
			{
				vy = -MARIO_JUMP_SPEED_Y;
			}
		}
		else // Đang ở giữa không trung
		{
			// Nếu là level Raccon thì mới cho rơi chậm và bay
			if (this->GetLevel() == MARIO_LEVEL_RACCON)
			{
				/*if (abs(vx) == MARIO_RUNNING_SPEED)
					this->SetState(MARIO_STATE_FLYING);
				else*/
				this->SetState(MARIO_STATE_FALL_SLOW);
			}
		}
		break;

	case MARIO_STATE_FLYING:
		// Lực nhảy mạnh (khi chạy nhanh) của lần đầu nhấn nhảy thì đã có ở state jump rồi
		// Bây giờ là đang ở trên cao, nên chỉ cần cho Mario một lực bay nhỏ để bay lên mà thôi

		if (nx > 0)
			maxVx = MARIO_SPEED_FLYING_X;
		else
			maxVx = -MARIO_SPEED_FLYING_X;

		vy = -MARIO_SPEED_FLYING_Y;
		break;

	case MARIO_STATE_FALL_SLOW:
		if (nx > 0)
			maxVx = MARIO_SPEED_FALL_SLOW_X;
		else
			maxVx = -MARIO_SPEED_FALL_SLOW_X;

		vy = MARIO_SPEED_FALL_SLOW_Y;

		fallSlow_start = GetTickCount64();
		break;

	case MARIO_STATE_RELEASE_JUMP:
		if (vy < 0) vy += MARIO_JUMP_SPEED_Y / 2;
		break;

	case MARIO_STATE_SIT:
		if (isOnPlatform && level != MARIO_LEVEL_SMALL)
		{
			state = MARIO_STATE_IDLE;
			isSitting = true;
			vx = 0;
			y += MARIO_SIT_HEIGHT_ADJUST;
		}
		break;

	case MARIO_STATE_SIT_RELEASE:
		if (isSitting)
		{
			isSitting = false;
			state = MARIO_STATE_IDLE;
			y -= MARIO_SIT_HEIGHT_ADJUST;
		}
		break;

	case MARIO_STATE_IDLE:
		ax = 0.0f;
		vx = 0.0f;
		break;

	case MARIO_STATE_DIE:
		vy = -MARIO_JUMP_DEFLECT_SPEED;
		vx = 0;
		ax = 0;
		break;
	}

	CGameObject::SetState(state);
}

void CMario::GetBoundingBox(float& left, float& top, float& right, float& bottom)
{
	if (level == MARIO_LEVEL_BIG)
	{
		if (isSitting)
		{
			left = x;
			top = y;
			right = left + MARIO_BIG_SITTING_BBOX_WIDTH;
			bottom = top + MARIO_BIG_SITTING_BBOX_HEIGHT;
		}
		else
		{
			left = x;
			top = y;
			right = left + MARIO_BIG_BBOX_WIDTH;
			bottom = top + MARIO_BIG_BBOX_HEIGHT;
		}
	}
	else if (level == MARIO_LEVEL_SMALL)
	{
		left = x;
		top = y;
		right = left + MARIO_SMALL_BBOX_WIDTH;
		bottom = top + MARIO_SMALL_BBOX_HEIGHT;
	}
	else if (level == MARIO_LEVEL_RACCON)
	{
		if (isSitting)
		{
			left = x;
			top = y;
			right = x + MARIO_BIG_SITTING_BBOX_WIDTH;
			bottom = y + MARIO_BIG_SITTING_BBOX_HEIGHT;
		}
		else
		{
			left = x;
			top = y;
			right = left + MARIO_BIG_BBOX_WIDTH;
			bottom = top + MARIO_BIG_BBOX_HEIGHT;
		}
	}

	//DebugOutTitle(L"Mario: %0.2f, %0.2f   BBox: %0.2f, %0.2f", x, y, top, left);
}

void CMario::SetLevel(int l)
{
	// Adjust position to avoid falling off platform
	// Nếu đang ở dạng Small, thì khi chuyển sang các dạng lớn hơn thì điều chỉnh lại vị trí bằng khoảng chênh lệch giữ dạng Small và dạng Big
	if (this->level == MARIO_LEVEL_SMALL &&
		l != MARIO_LEVEL_SMALL)
	{
		y -= (MARIO_BIG_BBOX_HEIGHT + 1 - MARIO_SMALL_BBOX_HEIGHT);
	}

	level = l;
}

