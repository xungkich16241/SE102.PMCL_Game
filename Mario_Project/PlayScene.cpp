﻿#include <iostream>
#include <fstream>
#include "AssetIDs.h"

#include "PlayScene.h"
#include "Utils.h"
#include "Textures.h"
#include "Sprites.h"
#include "Portal.h"
#include "Coin.h"
#include "Platform.h"
#include "InvisiblePlatform.h"
#include "Mushroom.h"
#include "WingGoomba.h"
#include "Koopa.h"
#include "ChangeDirectionOnPlatform.h"

#include "SampleKeyEventHandler.h"

using namespace std;

CPlayScene::CPlayScene(int id, LPCWSTR filePath):CScene(id, filePath)
{
	player = NULL;
	key_handler = new CSampleKeyHandler(this);
	base_platform_pos_y = 0;
	old_base_platform_pos_y = 0;

	background = NULL;
	forgeground = NULL;
}


#define SCENE_SECTION_UNKNOWN -1
#define SCENE_SECTION_ASSETS	1
#define SCENE_SECTION_TILEMAP	2
#define SCENE_SECTION_OBJECTS	3

#define ASSETS_SECTION_UNKNOWN -1
#define ASSETS_SECTION_SPRITES 1
#define ASSETS_SECTION_ANIMATIONS 2

#define MAX_SCENE_LINE 1024

void CPlayScene::_ParseSection_SPRITES(string line)
{
	vector<string> tokens = split(line);

	if (tokens.size() < 6) return; // skip invalid lines

	int ID = atoi(tokens[0].c_str());
	int l = atoi(tokens[1].c_str());
	int t = atoi(tokens[2].c_str());
	int r = atoi(tokens[3].c_str());
	int b = atoi(tokens[4].c_str());
	int texID = atoi(tokens[5].c_str());

	LPTEXTURE tex = CTextures::GetInstance()->Get(texID);
	if (tex == NULL)
	{
		DebugOut(L"[ERROR] Texture ID %d not found!\n", texID);
		return;
	}

	CSprites::GetInstance()->Add(ID, l, t, r, b, tex);
}

void CPlayScene::_ParseSection_ASSETS(string line)
{
	vector<string> tokens = split(line);

	if (tokens.size() < 1) return;

	wstring path = ToWSTR(tokens[0]);

	LoadAssets(path.c_str());
}


void CPlayScene::_ParseSection_TILEMAP(string line)
{
	vector<string> tokens = split(line);

	if (tokens.size() == 0) return;

	string path = tokens[0];

	LoadTilemap(path);
}

void CPlayScene::LoadTilemap(string tilemapFile)
{

	CTileMap* tile_map = new CTileMap(tilemapFile);
	this->background = tile_map->GetBackground();
	this->forgeground = tile_map->GetForgeground();
	//string scene_path = "scenes//";
	//string xml_path = "NES - Super Mario Bros 3 - Stage Tiles_transparent.xml";
	//string file_path = scene_path + xml_path;
	//TiXmlDocument doc_img(file_path.c_str());
	//if (!doc_img.LoadFile())
	//{
	//	printf("%s", doc_img.ErrorDesc());
	//	return;
	//}

	DebugOut(L"Da load dc file xml\n");
}


void CPlayScene::_ParseSection_ANIMATIONS(string line)
{
	vector<string> tokens = split(line);

	if (tokens.size() < 3) return; // skip invalid lines - an animation must at least has 1 frame and 1 frame time

	//DebugOut(L"--> %s\n",ToWSTR(line).c_str());

	LPANIMATION ani = new CAnimation();

	int ani_id = atoi(tokens[0].c_str());
	for (int i = 1; i < tokens.size(); i += 2)	// why i+=2 ?  sprite_id | frame_time  
	{
		int sprite_id = atoi(tokens[i].c_str());
		int frame_time = atoi(tokens[i + (long long int)1].c_str());
		ani->Add(sprite_id, frame_time);
	}

	CAnimations::GetInstance()->Add(ani_id, ani);
}

/*
	Parse a line in section [OBJECTS]
*/
void CPlayScene::_ParseSection_OBJECTS(string line)
{
	vector<string> tokens = split(line);

	// skip invalid lines - an object set must have at least id, x, y
	if (tokens.size() < 2) return;

	int object_type = atoi(tokens[0].c_str());
	float x = (float)atof(tokens[1].c_str());
	float y = (float)atof(tokens[2].c_str());

	CGameObject* obj = NULL;

	switch (object_type)
	{
	case OBJECT_TYPE_MARIO:
	{


		if (player != NULL)
		{
			DebugOut(L"[ERROR] MARIO object was created before!\n");
			return;
		}
		obj = new CMario(x, y, object_type);
		player = (CMario*)obj;

		DebugOut(L"[INFO] Player object has been created!\n");

		MakeCameraFollowMario();
		break;
	}
	case OBJECT_TYPE_HUD: obj = new CHUD(x, y, object_type); break;
	case OBJECT_TYPE_GOOMBA: obj = new CGoomba(x, y, object_type); break;
	case OBJECT_TYPE_BRICK: obj = new CBrick(x, y, object_type); break;
	case OBJECT_TYPE_COIN:
	{
		obj = new CCoin(x, y, object_type);
		break;
	}
	case OBJECT_TYPE_PBUTTON:
	{
		obj = new CPButton(x, y, object_type);
		break;
	}
	case OBJECT_TYPE_BREAKABLE_BRICK:
	{
		if (tokens.size() == 4)
			obj = new CBreakableBrick(x, y, object_type, true);
		else
			obj = new CBreakableBrick(x, y, object_type, false);

		break;
	}
	case OBJECT_TYPE_PIPE:
	{
		obj = new CPipe(x, y, object_type);
		break;
	}
	case OBJECT_TYPE_PIPE_GATE:
	{
		int pipe_des_id = atoi(tokens[3].c_str());
		int is_gate_in = atoi(tokens[4].c_str());

		if (is_gate_in == PIPE_GATE_WAY_IN)
			obj = new CPipeGate(x, y, object_type, pipe_des_id, true);
		else
			obj = new CPipeGate(x, y, object_type, pipe_des_id, false);

		break;
	}
	case OBJECT_TYPE_PIPE_TELEPORT_DESTINATION:
	{
		int id = atoi(tokens[3].c_str());
		int is_gate_out = atoi(tokens[4].c_str());

		// Pipe nằm trên mặt đất, là lối ra để về mặt đất
		if (is_gate_out == PIPE_TELE_DES_BACK_TO_SURFACE)
			obj = new CPipeTeleportDestination(x, y, object_type, id, true);
		else
			// Pipe trong Hidden Zone, thả rơi Mario
			obj = new CPipeTeleportDestination(x, y, object_type, id, false);

		itemsInside.insert(make_pair(id, obj));

		break;
	}
	case OBJECT_TYPE_KOOPA:
	{
		if (tokens.size() == 4)
		{
			int id_CDOP = atoi(tokens[3].c_str());
			obj = new CKoopa(x, y, object_type, true, id_CDOP);
		}
		else
		{
			obj = new CKoopa(x, y, object_type);
		}
			
		break;
	}
	case OBJECT_TYPE_CDOP:
	{
		int id = atoi(tokens[3].c_str());
		obj = new CChangeDirectionOnPlatform(x, y, object_type, id);

		itemsInside.insert(make_pair(id, obj));
		break;
	}
	case OBJECT_TYPE_WING_GOOMBA:
	{
		float walking_distance = (float)atof(tokens[3].c_str());

		obj = new CWingGoomba(x, y, object_type, walking_distance);
		break;
	}
	case OBJECT_TYPE_QUESTION_BRICK:
	{
		int item_id;
		if (tokens.size() > 3)
		{
			item_id = atoi(tokens[3].c_str());
			obj = new CQuestionBrick(x, y, object_type, item_id);
		}
		else
			obj = new CQuestionBrick(x, y, object_type);

		break;
	}
	case OBJECT_TYPE_MUSHROOM:
	{
		int mushroom_id = atoi(tokens[3].c_str());
		obj = new CMushroom(x, y, object_type, mushroom_id);

		itemsInside.insert(make_pair(mushroom_id, obj));
		break;
	}
	case OBJECT_TYPE_PLATFORM:
	{
		bool is_base = false;
		bool is_first_base = false;

		if (tokens.size() == 10)	// Nếu đây là platform base - camera không dịch chuyển theo trục y khi Mario đứng trên platform này (giúp player không bị chóng mặt)
			is_base = true;

		float cell_width = (float)atof(tokens[3].c_str());
		float cell_height = (float)atof(tokens[4].c_str());
		int length = atoi(tokens[5].c_str());
		int sprite_begin = atoi(tokens[6].c_str());
		int sprite_middle = atoi(tokens[7].c_str());
		int sprite_end = atoi(tokens[8].c_str());

		obj = new CPlatform(
			x, y, object_type, is_base,
			cell_width, cell_height, length,
			sprite_begin, sprite_middle, sprite_end
		);

		if (tokens.size() == 11)
		{
			int base_type = atoi(tokens[10].c_str());
			if (base_type == FIRST_BASE_PLATFORM)
				this->SetBasePlatformPosY(y);
			else if (base_type == HIDEEN_ZONE_BASE_PLATFORM)
			{
				itemsInside.insert(make_pair(DEFAULT_ID_HIDDEN_ZONE_BASE_PLATFORM, obj));
				DebugOut(L"Loaded HZ base platform\n");
			}
		}
		break;
	}
	case OBJECT_TYPE_INVISIBLE_PLATFORM:
	{
		int width = atoi(tokens[3].c_str());
		int height = atoi(tokens[4].c_str());
		obj = new CInvisiblePlatform(x, y, object_type, width, height);
		break;
	}
	case OBJECT_TYPE_IMAGEMAP:
	{
		int id = atoi(tokens[3].c_str());
		obj = new CImageMap(x, y, object_type, id);
		DebugOut(L"[INFO] Image map object has been created!\n");

		break;
	}
	case OBJECT_TYPE_PORTAL:
	{
		float r = (float)atof(tokens[3].c_str());
		float b = (float)atof(tokens[4].c_str());
		int scene_id = atoi(tokens[5].c_str());
		obj = new CPortal(x, y, r, b, scene_id, object_type);

		break;
	}
	/*case OBJECT_TYPE_TAIL:
	{
		float width = (float)atof(tokens[3].c_str());
		float height = (float)atof(tokens[4].c_str());
		obj = new CTail(x, y, width, height, object_type);

		DebugOut(L"[INFO] Tail object has been created!\n");

		break;
	}*/
	break;


	default:
		DebugOut(L"[ERROR] Invalid object type: %d\n", object_type);
		return;
	}

	// General object setup
	obj->SetPosition(x, y);


	objects.push_back(obj);
}

void CPlayScene::LoadAssets(LPCWSTR assetFile)
{
	DebugOut(L"[INFO] Start loading assets from : %s \n", assetFile);

	ifstream f;
	f.open(assetFile);

	int section = ASSETS_SECTION_UNKNOWN;

	char str[MAX_SCENE_LINE];
	while (f.getline(str, MAX_SCENE_LINE))
	{
		string line(str);

		if (line[0] == '#') continue;	// skip comment lines	

		if (line == "[SPRITES]") { section = ASSETS_SECTION_SPRITES; continue; };
		if (line == "[ANIMATIONS]") { section = ASSETS_SECTION_ANIMATIONS; continue; };
		if (line[0] == '[') { section = SCENE_SECTION_UNKNOWN; continue; }

		//
		// data section
		//
		switch (section)
		{
		case ASSETS_SECTION_SPRITES: _ParseSection_SPRITES(line); break;
		case ASSETS_SECTION_ANIMATIONS: _ParseSection_ANIMATIONS(line); break;
		}
	}

	f.close();

	DebugOut(L"[INFO] Done loading assets from %s\n", assetFile);
}

void CPlayScene::Load()
{
	DebugOut(L"[INFO] Start loading scene from : %s \n", sceneFilePath);

	ifstream f;
	f.open(sceneFilePath);

	// current resource section flag
	int section = SCENE_SECTION_UNKNOWN;

	char str[MAX_SCENE_LINE];
	while (f.getline(str, MAX_SCENE_LINE))
	{
		string line(str);

		if (line[0] == '#') continue;	// skip comment lines
		if (line.empty()) continue;	// skip empty lines
		if (line == "[ASSETS]") { section = SCENE_SECTION_ASSETS; continue; };
		if (line == "[TILEMAP]") { section = SCENE_SECTION_TILEMAP; continue; };
		if (line == "[OBJECTS]") { section = SCENE_SECTION_OBJECTS; continue; };
		if (line[0] == '[') { section = SCENE_SECTION_UNKNOWN; continue; }

		//
		// data section
		//
		switch (section)
		{
		case SCENE_SECTION_ASSETS: _ParseSection_ASSETS(line); break;
		case SCENE_SECTION_TILEMAP: _ParseSection_TILEMAP(line); break;
		case SCENE_SECTION_OBJECTS: _ParseSection_OBJECTS(line); break;
		}
	}

	f.close();

	DebugOut(L"[INFO] Done loading scene  %s\n", sceneFilePath);
}

void CPlayScene::Update(DWORD dt)
{
	// We know that Mario is the first object in the list hence we won't add him into the colliable object list
	// TO-DO: This is a "dirty" way, need a more organized way
	/* "dirty" là vì chưa xử dụng phân hoạch không gian(Grid, Quad Tree, ...) để xử lý va chạm cho ít game object hơn.
		 Xử lý ít hơn là để cho game chạy nhanh hơn, mà cũng là vì những object không ở gần cửa sổ game thì cũng không cần update làm chi
		 (đỡ tốn thời gian tính toán cho CPU).
		 Nên mới cần phân hoạch không gian để chọn ra những object thực sự cần thiết để update.
	*/


	/*CMario* mario = dynamic_cast<CMario*>(objects[0]);

	if (mario->IsStopUpdate() == false)
	{*/
	vector<LPGAMEOBJECT> coObjects;
	CHUD* hud = NULL;
	for (size_t i = 0; i < objects.size(); i++)
	{
		// Nếu object này là Mario thì không bỏ vào danh sách collidable objects
		if (objects[i]->GetType() == OBJECT_TYPE_MARIO)
			continue;
		coObjects.push_back(objects[i]);
	}

	for (size_t i = 0; i < objects.size(); i++)
	{
		if (objects[i]->GetType() == OBJECT_TYPE_HUD)
		{
			hud = dynamic_cast<CHUD*>(objects[i]);
			continue;
		}
			
		objects[i]->Update(dt, &coObjects);
	}

	MakeCameraFollowMario();
	if (hud != NULL)
		hud->Update(dt, &coObjects);

	PurgeDeletedObjects();
	/*}*/
}

void CPlayScene::MakeCameraFollowMario()
{
	// skip the rest if scene was already unloaded (Mario::Update might trigger PlayScene::Unload)
	if (player == NULL) return;

	// Update camera to follow mario
	float cx, cy;
	player->GetPosition(cx, cy);

	CGame* game = CGame::GetInstance();
	cx -= game->GetBackBufferWidth() / 2;
	//cy -= game->GetBackBufferHeight() / 2;


	if (this->base_platform_pos_y != 0)
	{
		if (((this->base_platform_pos_y - game->GetBackBufferHeight()) <= cy) && (cy <= this->base_platform_pos_y))
			cy = base_platform_pos_y + BASE_PLATFORM_HEIGHT + HUD_BBOX_HEIGHT - game->GetBackBufferHeight();
	}

	if (cx < 0) cx = 0;

	CGame::GetInstance()->SetCamPos(cx, /*0.0f*/ cy);
	/*DebugOutTitle(L"Camera: %0.2f, %0.2f", cx, cy);*/
}

void CPlayScene::Render()
{
	if (this->background != NULL)
		this->background->Render();

	// Vẽ theo chiều ngược lại vì Mario thuộc vị trí đầu tiên của objects, theo sau là các object và cuối cùng là map
	for (int i = 0; i < objects.size(); i++)
		objects[i]->Render();

	if (this->forgeground != NULL)
		this->forgeground->Render();
}

/*
*	Clear all objects from this scene
*/
void CPlayScene::Clear()
{
	vector<LPGAMEOBJECT>::iterator it;
	for (it = objects.begin(); it != objects.end(); it++)
	{
		delete (*it);
	}
	objects.clear();
}

/*
	Unload scene

	TODO: Beside objects, we need to clean up sprites, animations and textures as well

*/
void CPlayScene::Unload()
{
	for (int i = 0; i < objects.size(); i++)
		delete objects[i];
	objects.clear();

	// Các item trong itemsInside đã được xóa cùng với các game object khác khi xóa các game object trong objects
	// Nên bây giờ chỉ cần clear itemsInside mà thôi
	itemsInside.clear();

	player = NULL;

	DebugOut(L"[INFO] Scene %d unloaded! \n", id);
}

bool CPlayScene::IsGameObjectDeleted(const LPGAMEOBJECT& o) { return o == NULL; }

void CPlayScene::PurgeDeletedObjects()
{
	vector<LPGAMEOBJECT>::iterator it;
	for (it = objects.begin(); it != objects.end(); it++)
	{
		LPGAMEOBJECT o = *it;
		if (o->IsDeleted())
		{
			delete o;
			*it = NULL;
		}
	}

	// NOTE: remove_if will swap all deleted items to the end of the vector
	// then simply trim the vector, this is much more efficient than deleting individual items
	objects.erase(
		std::remove_if(objects.begin(), objects.end(), CPlayScene::IsGameObjectDeleted),
		objects.end());
}