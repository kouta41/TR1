#include <Novice.h>
#include"Vector3.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include"imgui.h"

const char kWindowTitle[] = "TR1_ツシマ_コウタ";

struct Block{
	Vector3 pos;
	float radius;
	int color;
};

struct Circle {
	Vector3 center;     // 中心
	float radius;       // 半径
	float speed;        // 移動速度
	unsigned int color; // 色
};
struct Capsule {
	Vector3 start;
	Vector3 end;
	float radius;
	unsigned int color; // 色
};
struct Line {
	Vector3 start;
	Vector3 end;
	unsigned int color; // 色
};
Vector3 ToScreen(const Vector3* world) {
	// 今回のワールド座標系からスクリーン座標系は
	// 原点位置がyに500ずれていて、y軸が反転
	const Vector3 kWorldToScreenTranslate = { 0.0f,0.f };
	const Vector3 kWorldToScreenScale = { 1.0f, 1.0f };
	return {
	  (world->x * kWorldToScreenScale.x) + kWorldToScreenTranslate.x,
	  (world->y * kWorldToScreenScale.y) + kWorldToScreenTranslate.y };
}


float Dot(const Vector3* lhs, const Vector3* rhs) { return lhs->x * rhs->x + lhs->y * rhs->y; }

void DrawCircle(const Circle* circle) {
	Vector3 screenCenter = ToScreen(&circle->center);
	Novice::DrawEllipse(
		int(screenCenter.x), int(screenCenter.y), int(circle->radius), int(circle->radius), 0.0f,
		circle->color, kFillModeSolid);
}

void DrawLine(const Line* line) {
	Vector3 screenStart = ToScreen(&line->start);
	Vector3 screenEnd = ToScreen(&line->end);
	Novice::DrawLine(
		int(screenStart.x), int(screenStart.y), int(screenEnd.x), int(screenEnd.y), line->color);
}

Vector3 Perpendicular(const Vector3* vector) { return { -vector->y, vector->x }; }

Vector3 Normalize(const Vector3* original) {
	float dot = Dot(original, original);
	Vector3 result = *original;
	if (dot != 0.0f) {
		float length = sqrtf(dot);
		result.x /= length;
		result.y /= length;
	}
	return result;
}

void DrawCapsule(const Capsule* capsule) {
	Vector3 screenStart = ToScreen(&capsule->start);
	Vector3 screenEnd = ToScreen(&capsule->end);

#if DRAW_CAPSULE_ONE_LINE
	Novice::DrawLine(
		int(screenStart.x), int(screenStart.y), int(screenEnd.x), int(screenEnd.y), capsule->color);
#else
	Vector3 toEnd = { capsule->end.x - capsule->start.x, capsule->end.y - capsule->start.y };
	Vector3 unitVector = Normalize(&toEnd);
	Vector3 perpendicular = Perpendicular(&unitVector);
	Vector3 start1 = {
	  capsule->start.x + perpendicular.x * capsule->radius,
	  capsule->start.y + perpendicular.y * capsule->radius };
	Vector3 start2 = {
	  capsule->start.x - perpendicular.x * capsule->radius,
	  capsule->start.y - perpendicular.y * capsule->radius };
	Vector3 end1 = { start1.x + toEnd.x, start1.y + toEnd.y };
	Vector3 end2 = { start2.x + toEnd.x, start2.y + toEnd.y };
	start1 = ToScreen(&start1);
	start2 = ToScreen(&start2);
	end1 = ToScreen(&end1);
	end2 = ToScreen(&end2);
	Novice::DrawLine(int(start1.x), int(start1.y), int(end1.x), int(end1.y), capsule->color);
	Novice::DrawLine(int(start2.x), int(start2.y), int(end2.x), int(end2.y), capsule->color);

#endif
	Novice::DrawEllipse(
		int(screenStart.x), int(screenStart.y), int(capsule->radius), int(capsule->radius), 0.0f,
		capsule->color, kFillModeWireFrame);
	Novice::DrawEllipse(
		int(screenEnd.x), int(screenEnd.y), int(capsule->radius), int(capsule->radius), 0.0f,
		capsule->color, kFillModeWireFrame);
}

Vector3 Rotate(const Vector3* original, float angle) {
	return {
	  original->x * cosf(angle) - original->y * sinf(angle),
	  original->y * cosf(angle) + original->x * sinf(angle) };
}

Vector3 ClosestPoint(const Line* line, const Vector3* point) {
	// 直線のベクトル
	Vector3 lineVector = { line->end.x - line->start.x, line->end.y - line->start.y };
	float length = sqrtf(lineVector.x * lineVector.x + lineVector.y * lineVector.y);

	// 単位ベクトル
	Vector3 unitVector = lineVector;
	if (length != 0.0f) {
		unitVector.x = lineVector.x / length;
		unitVector.y = lineVector.y / length;
	}

	// 始点からポイントへのベクトル
	Vector3 toCenter = { point->x - line->start.x, point->y - line->start.y };

	// 内積
	float dot = toCenter.x * unitVector.x + toCenter.y * unitVector.y;

	// 最近接点が円の内部にいるかどうかで判定
	dot = fmaxf(0.0f, fminf(dot, length));
	Vector3 closestPoint = { line->start.x + unitVector.x * dot, line->start.y + unitVector.y * dot };
	return closestPoint;
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	// ライブラリの初期化
	Novice::Initialize(kWindowTitle, 1280, 720);

	// キー入力結果を受け取る箱
	char keys[256] = { 0 };
	char preKeys[256] = { 0 };

	// 変数と定数
	const float kRotateAngle = 1.0f / 256.0f * float(M_PI);
	Vector3 kPointA{ 600, 400 };
	const Vector3 kBaseVector{ -190, -60 };
	const float kMaxScale = 2.0f;
	const float kMinScale = 0.5f;
	const float kScaleIncrement = 0.01f;
	float scale = 0.4f;
	float theta = 0.4f;
	float addScaleValue = 0.01f;
	

	int whiteTextureHandle = Novice::LoadTexture("white1x1.png");

	//カメラ視点
	Vector3 krPoint = { kPointA.x - 170, kPointA.y - 60 };
	Vector3 InitkrPoint = { kPointA.x - 170, kPointA.y - 60 };



	Vector3 krLeftTop = { krPoint.x - 65,krPoint.y - 25 };
	Vector3 krRightTop = { krPoint.x - 65,krPoint.y + 25 };
	Vector3 krLeftBottom = { krPoint.x + 65,krPoint.y - 25 };
	Vector3 krRightBottom = { krPoint.x + 65,krPoint.y + 25 };
	Vector3 Xlin[7];
	float XlinRadius = 10;

	float  speed = 2;
	int jp = true;
	int budS = true;
	int clocr = RED;

	Circle circle = {
	  {200.0f, 400.0f},
	  75.0f/2,
	  4.0f,
	  WHITE,
	};

	Capsule capsule = {
	  {100.0f,  100.0f},
	  {1000.0f, 400.0f},
	  10.0f, WHITE
	};

	Block block[50];
	for (int i = 0; i < 50; i++) {
		block[i].color = WHITE;
	}
	block[0].pos.x = 700;
	block[0].pos.y = 300;
	block[0].radius = 40;

	block[1].pos.x = 700;
	block[1].pos.y = 310;
	block[1].radius = 40;

	block[2].pos.x = 700;
	block[2].pos.y = 320;
	block[2].radius = 40;

	block[3].pos.x = 700;
	block[3].pos.y = 330;
	block[3].radius = 40;

	block[4].pos.x = 700;
	block[4].pos.y = 340;
	block[4].radius = 40;

	block[5].pos.x = 700;
	block[5].pos.y = 350;
	block[5].radius = 40;

	block[6].pos.x = 700;
	block[6].pos.y = 360;
	block[6].radius = 40;

	block[7].pos.x = 700;
	block[7].pos.y = 370;
	block[7].radius = 40;

	block[8].pos.x = 700;
	block[8].pos.y = 380;
	block[8].radius = 40;

	block[9].pos.x = 700;
	block[9].pos.y = 390;
	block[9].radius = 40;

	block[10].pos.x = 700;
	block[10].pos.y = 400;
	block[10].radius = 40;

	block[11].pos.x = 700;
	block[11].pos.y = 410;
	block[11].radius = 40;

	block[12].pos.x = 700;
	block[12].pos.y = 420;
	block[12].radius = 40;

	block[13].pos.x = 700;
	block[13].pos.y = 430;
	block[13].radius = 40;

	block[14].pos.x = 700;
	block[14].pos.y = 440;
	block[14].radius = 40;

	block[15].pos.x = 700;
	block[15].pos.y = 450;
	block[15].radius = 40;

	block[16].pos.x = 700;
	block[16].pos.y = 460;
	block[16].radius = 40;

	block[17].pos.x = 700;
	block[17].pos.y = 470;
	block[17].radius = 40;

	block[18].pos.x = 700;
	block[18].pos.y = 480;
	block[18].radius = 40;

	block[19].pos.x = 700;
	block[19].pos.y = 490;
	block[19].radius = 40;

	block[20].pos.x = 700;
	block[20].pos.y = 500;
	block[20].radius = 40;

	block[21].pos.x = 700;
	block[21].pos.y = 510;
	block[21].radius = 40;

	block[22].pos.x = 700;
	block[22].pos.y = 520;
	block[22].radius = 40;

	block[23].pos.x = 700;
	block[23].pos.y = 530;
	block[23].radius = 40;


	/*---------------------------------------------------------------------*/
	block[24].pos.x = 890;
	block[24].pos.y = 150;
	block[24].radius = 40;

	block[25].pos.x = 900;
	block[25].pos.y = 150;
	block[25].radius = 40;

	block[26].pos.x = 910;
	block[26].pos.y = 150;
	block[26].radius = 40;

	block[27].pos.x = 920;
	block[27].pos.y = 150;
	block[27].radius = 40;

	block[28].pos.x = 930;
	block[28].pos.y = 150;
	block[28].radius = 40;

	block[29].pos.x = 940;
	block[29].pos.y = 150;
	block[29].radius = 40;

	block[30].pos.x = 950;
	block[30].pos.y = 150;
	block[30].radius = 40;

	block[31].pos.x = 960;
	block[31].pos.y = 150;
	block[31].radius = 40;

	block[32].pos.x = 970;
	block[32].pos.y = 150;
	block[32].radius = 40;

	block[33].pos.x = 980;
	block[33].pos.y = 150;
	block[33].radius = 40;

	block[34].pos.x = 990;
	block[34].pos.y = 150;
	block[34].radius = 40;

	block[35].pos.x = 1000;
	block[35].pos.y = 150;
	block[35].radius = 40;

	block[36].pos.x = 1010;
	block[36].pos.y = 150;
	block[36].radius = 40;

	block[37].pos.x = 1020;
	block[37].pos.y = 150;
	block[37].radius = 40;

	block[38].pos.x = 1030;
	block[38].pos.y = 150;
	block[38].radius = 40;

	block[39].pos.x = 1040;
	block[39].pos.y = 150;
	block[39].radius = 40;

	block[40].pos.x = 1050;
	block[40].pos.y = 150;
	block[40].radius = 40;

	block[41].pos.x = 1060;
	block[41].pos.y = 150;
	block[41].radius = 40;

	block[42].pos.x = 1070;
	block[42].pos.y = 150;
	block[42].radius = 40;

	block[43].pos.x = 1080;
	block[43].pos.y = 150;
	block[43].radius = 40;

	block[44].pos.x = 1090;
	block[44].pos.y = 150;
	block[44].radius = 40;

	block[45].pos.x = 1100;
	block[45].pos.y = 150;
	block[45].radius = 40;

	block[46].pos.x = 1110;
	block[46].pos.y = 150;
	block[46].radius = 40;

	block[47].pos.x = 1120;
	block[47].pos.y = 150;
	block[47].radius = 40;
	/*for (int i = 0; i < 10; i++) {
		block[i].pos.x = 850;
		block[i].pos.y = 120;
	}*/
	int spfrag = true;
	float spSpeed = 2.0f;
	float i = 0.01;

	Vector3 rotatedVector = {
		  kBaseVector.x * cosf(theta) - kBaseVector.y * sinf(theta),
		  kBaseVector.y * cosf(theta) + kBaseVector.x * sinf(theta) };
	rotatedVector.x *= scale;
	rotatedVector.y *= scale;

	Vector3 pointB = { kPointA.x + rotatedVector.x, kPointA.y + rotatedVector.y,15 };

	// ウィンドウの×ボタンが押されるまでループ
	while (Novice::ProcessMessage() == 0) {
		// フレームの開始
		Novice::BeginFrame();

		// キー入力を受け取る
		memcpy(preKeys, keys, 256);
		Novice::GetHitKeyStateAll(keys);

		///
		/// ↓更新処理ここから
		///


		

		Vector3 rotatedVector = {
		  kBaseVector.x * cosf(theta) - kBaseVector.y * sinf(theta),
		  kBaseVector.y * cosf(theta) + kBaseVector.x * sinf(theta)
		};
		rotatedVector.x *= scale;
		rotatedVector.y *= scale;

		Vector3 pointB = { kPointA.x + rotatedVector.x, kPointA.y + rotatedVector.y,15-i};

		

		if(budS==true){
		if (keys[DIK_S]) {
			kPointA.y += 4;
			krPoint.y += 4;
		}
		}
		if (keys[DIK_A]) {
			kPointA.x -= 4;
			krPoint.x -= 4;
		}
		else if (keys[DIK_D]) {
			kPointA.x += 4;
			krPoint.x += 4;
		}


		if (keys[DIK_UP]) {
			krPoint.y -= 2;
			krPoint.x += 1.5;
			theta += kRotateAngle;
			speed -= 2;
		}
		else if (keys[DIK_DOWN]) {
			krPoint.y += 2;
			krPoint.x -= 1.5;
			theta -= kRotateAngle;
			speed += 2;
		}


		if (keys[DIK_1]) {
			i += 0.8;
		}
		else if (keys[DIK_2]) {
			i -= 0.8;
		}


		if (keys[DIK_RIGHT]) {
			scale -= addScaleValue;
		}
		else if (keys[DIK_LEFT]) {
			scale += addScaleValue;
		}


		//移動制限
		if (kPointA.x <= 20) {
			kPointA.x = 20;
		}
		if (kPointA.x >= 1260) {
			kPointA.x = 1260;
		}
		if (kPointA.y <= 20) {
			kPointA.y = 20;
		}
		if (kPointA.y >= 680) {
			kPointA.y = 680;
		}

		//重力
		
			kPointA.y += 9.8f;
		

		//壁もぼり
		if (jp == true) {
			if (kPointA.x >= 830) {
				clocr = GREEN;
				kPointA.x = 830;
				kPointA.y -= 9.8;
				if (keys[DIK_W]) {
					kPointA.y -= 4;
					krPoint.y -= 4;
				}
			}
		}

		if (kPointA.y <= 290&&kPointA.x>=830) {
			 jp = false;
			 kPointA.y = 280;
			 clocr = RED;
			 budS = false;
		 }
		 else {
			 jp = true;
			 budS = true;
		 }
		


		capsule.start = { kPointA };
		capsule.end = { kPointA.x + rotatedVector.x, kPointA.y + rotatedVector.y };
		Line capsuleLine = { capsule.start, capsule.end, WHITE };

		for (int i=0; i < 50; i++) {
			Vector3 closestPoint = ClosestPoint(&capsuleLine, &block[i].pos);
			Vector3 closestPointToCenter = {
		 block[i].pos.x - closestPoint.x, block[i].pos.y - closestPoint.y};
			float sumRadius = block[i].radius + capsule.radius;
			block[i].color = WHITE;
			if (Dot(&closestPointToCenter, &closestPointToCenter) < sumRadius * sumRadius) {
				block[i].color = RED;
				scale -= addScaleValue;
				theta -= kRotateAngle;
			}
		}
		


		if (kMaxScale < scale) {
			scale = kMaxScale - (scale - kMaxScale);
			addScaleValue = -kScaleIncrement;
		}
		else if (scale < kMinScale) {
			scale = kMinScale + (kMinScale - scale);
			addScaleValue = kScaleIncrement;
		}


		///
		/// 
		/// ↑更新処理ここまで

		///
		/// ↓描画処理ここから
		///
		Novice::DrawBox(840, 300, 1000, 1000, 0.0f, WHITE, kFillModeSolid);
		
		for (int i = 0; i < 50; i++) {
			Novice::DrawEllipse(block[i].pos.x, block[i].pos.y, block[i].radius, block[i].radius, 0.0f, block[i].color, kFillModeSolid);
		}
		Novice::DrawBox(830, 300, 20, 1000,0.0f, BLUE, kFillModeSolid);
		Novice::DrawBox(0, 710, 1000, 15, 0.0f, WHITE, kFillModeSolid);

		// 線の描画
		//player
		Novice::DrawEllipse(int(kPointA.x), int(kPointA.y), 20, 20, 0.0f, clocr, kFillModeSolid);

		Novice::DrawLine(int(kPointA.x), int(kPointA.y), int(pointB.x), int(pointB.y), WHITE);

		
		DrawCapsule(&capsule);


		Novice::DrawEllipse(int(pointB.x), int(pointB.y), int(pointB.z), int(pointB.z), 0.0f, RED, kFillModeSolid);


		// キャラクターの座標を画面表示する処理
		ImGui::Begin("Player pos");
		// float3入力ボックス
		ImGui::InputFloat3("point", &pointB.x);

		ImGui::End();
		


		///
		/// ↑描画処理ここまで
		///

		// フレームの終了
		Novice::EndFrame();

		// ESCキーが押されたらループを抜ける
		if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
			break;
		}
	}

	// ライブラリの終了
	Novice::Finalize();
	return 0;
}
