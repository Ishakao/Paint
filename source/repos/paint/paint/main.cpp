#include <iostream>
#include <GLFW/glfw3.h>
#include <raylib.h>
#include <sstream>
#include <vector>
#include <unordered_map>

#define and &&

using namespace std;

template <typename T>
T MAX(T a, T b) {
	return (a>b) ? a : b;
}

// Настройки

const float relativeSizeX = 0.8;
const float relativeSizeY = 0.7;

// #########

bool colorsMatch(Color first, Color second) {
	return first.r == second.r and first.g == second.g and first.b == second.b and first.a == second.a;
}

Vector3 getMonitorResolution() {
	if (!glfwInit()) {
		cerr << "GLFW not initialized" << endl;
		return {};
	}

	GLFWmonitor* screen = glfwGetPrimaryMonitor();
	if (!screen) {
		cerr << "Monitor doesnt exist" << endl;
		glfwTerminate();
		return {};
	}

	const GLFWvidmode* screenResolution = glfwGetVideoMode(screen);
	if (!screenResolution) {
		cerr << "Video mode not exist"  << endl;
		glfwTerminate();
		return {};
	}

	float w = screenResolution->width;
	float h = screenResolution->height;
	float rate = screenResolution->refreshRate;

	return {w, h, rate};
}

short sizeOfBrush = 5;
Color currentColor = RED;
short currentTransparency = 0;

bool painting = false;
bool changing_transparency = false;

bool painting_line = false;
bool painting_circle = false;
bool painting_rectangle = false;

Vector2 start_line = {};
Vector2 start_circle = {};
Vector2 start_rectangle = {};

string drawType = "brush";

int current_layer = 0;

class DrawObject {
public:
	string Type;
	int index;
	int startX, startY, sizeX, sizeY, Radius;
	int hold_index;
	Texture2D texture;
	RenderTexture2D brushTexture;
	bool isBrush = false;
	Vector2 MousePosition;
	Color color;

	void Draw() {
		if (isBrush) {
			DrawTextureRec(brushTexture.texture, { 0, 0, (float)brushTexture.texture.width, (float)-brushTexture.texture.height }, { 0, 0 }, WHITE);
		}
		else if (Type == "rectangle") {
			DrawRectangle(startX, startY, sizeX, sizeY, color);
		}
		else if (Type == "line") {
			DrawLineEx({ (float)startX, (float)startY }, { MousePosition.x, MousePosition.y }, Radius, color);
		}
		else if (Type == "pixel") {
			DrawPixel(startX, startY, color);
		}
		else if (Type == "circle") {
			int centerX = startX;
			int centerY = startY;
			int radius = Radius;
			DrawCircle(centerX, centerY, radius, color);
		}
		else if (Type == "image") {
			Rectangle src = { 0, 0, (float)texture.width, (float)texture.height };
			Rectangle dst = { (float)startX, (float)startY, (float)sizeX, (float)sizeY };
			Vector2 origin = { 0, 0 };
			DrawTexturePro(texture, src, dst, origin, 0.0f, WHITE);
		}
	}
};

unordered_map<int, Vector2> last_positions;

class Layer {
public:
	vector<DrawObject*> objects;

	int index;
	bool visible = true;
	int element_index = 0;

	void DrawAllElements() {
		if (not visible) return;
		for (DrawObject* obj : objects) obj->Draw();
	}

	void addElement(string Type, int startX, int startY, int sizeX, int sizeY, int Radius, Vector2 MousePosition, Color color, int hold_index) {
		if (Type == "brush" || Type == "eraser") {
			DrawObject* obj = nullptr;
			for (DrawObject* o : objects) {
				if (o->isBrush and o->hold_index == hold_index) {
					obj = o;
					break;
				}
			}

			if (not obj) {
				obj = new DrawObject;
				obj->Type = Type;
				obj->hold_index = hold_index;
				obj->isBrush = true;
				obj->index = element_index++;
				obj->brushTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
				BeginTextureMode(obj->brushTexture);
				ClearBackground(BLANK);
				EndTextureMode();
				objects.push_back(obj);
			}

			Vector2 prevPos = last_positions.count(hold_index) ? last_positions[hold_index] : Vector2{ (float)startX, (float)startY };

			BeginTextureMode(obj->brushTexture);
			float deltaX = startX - prevPos.x;
			float deltaY = startY - prevPos.y;
			float distnce = sqrt(deltaX * deltaX + deltaY * deltaY);
			int steps = (int)(distnce / (Radius / 2));

			for (int i = 0; i <= steps; i++) {
				float quantity = (steps == 0) ? 0.0f : (float)i / (float)steps;
				int endX = (int)(prevPos.x + quantity * deltaX);
				int endY = (int)(prevPos.y + quantity * deltaY);
				DrawCircle(endX, endY, Radius, color);
			}
			EndTextureMode();

			last_positions[hold_index] = Vector2{ (float)startX, (float)startY };
			return;
		}

		DrawObject* new_object = new DrawObject;
		new_object->Type = Type;
		new_object->startX = startX;
		new_object->startY = startY;
		new_object->sizeX = sizeX;
		new_object->sizeY = sizeY;
		new_object->Radius = Radius;
		new_object->MousePosition = MousePosition;
		new_object->color = color;
		new_object->hold_index = hold_index;
		new_object->index = element_index++;
		objects.push_back(new_object);
	}

	void addElement(string Type, int startX, int startY, int sizeX, int sizeY, int Radius, Vector2 MousePosition, Color color, int hold_index, Texture2D texture) {
		DrawObject* new_object = new DrawObject;
		new_object->Type = Type;
		new_object->startX = startX;
		new_object->startY = startY;
		new_object->sizeX = sizeX;
		new_object->sizeY = sizeY;
		new_object->Radius = Radius;
		new_object->MousePosition = MousePosition;
		new_object->color = color;
		new_object->hold_index = hold_index;
		new_object->texture = texture;
		new_object->index = element_index++;
		objects.push_back(new_object);
	}

	void deleteElement(int by_index) {
		if (by_index == -1) {
			if (not objects.empty()) {
				int hold = objects.back()->hold_index;
				for (size_t i = 0; i < objects.size();) {
					if (objects[i]->hold_index == hold) {
						delete objects[i];
						objects.erase(objects.begin() + i);
					}
					else {
						i++;
					}
				}
			}

			element_index = objects.size();

			return;
		}

		if (by_index >= 0 and by_index < (int)objects.size()) {
			delete objects[by_index];
			objects.erase(objects.begin() + by_index);
			for (size_t i = by_index; i < objects.size(); i++) {
				objects[i]->index = i;
			}
		}
		element_index = objects.size();
	}
	
	~Layer() {
		for (DrawObject* obj : objects) {
			delete obj;
		}

		objects.clear();
	}
};

class AllLayers {
public:
	vector<Layer*> Layers;

	Layer* addLayer() {
		Layer* new_layer = new Layer;
		Layers.push_back(new_layer);
		new_layer->index = Layers.size() - 1;
		current_layer = new_layer->index;
		return new_layer;
	}

	void deleteLayer(int index) {
		if (Layers.size() <= 1) return;

		int index_to_delete = (index == -1) ? Layers.size() - 1 : index;
		if (index_to_delete < 0 || index_to_delete >= Layers.size()) return;

		Layer* o = Layers[index_to_delete];

		while (!o->objects.empty()) {
			o->deleteElement(-1);
		}

		Layers.erase(Layers.begin() + index_to_delete);
		delete o;

		for (size_t i = index_to_delete; i < Layers.size(); ++i) {
			Layers[i]->index = i;
		}

		current_layer = min(current_layer, (int)Layers.size() - 1);
	}

	void DownLayer() {
		swap(Layers[current_layer], Layers[current_layer + 1]);
		Layers[current_layer]->index = current_layer;
		Layers[current_layer + 1]->index = current_layer + 1;
		current_layer++;
	}

	void UpLayer() {
		swap(Layers[current_layer], Layers[current_layer - 1]);
		Layers[current_layer]->index = current_layer;
		Layers[current_layer - 1]->index = current_layer - 1;
		current_layer--;
	}
};

AllLayers All_Layers;
Layer* startLayer = All_Layers.addLayer();

int hold_index = 0;

int from_index = 0;
int max_layers_on_list = 5;

void savePhoto(int winWidth) {
	float startX = winWidth * 0.1f;
	float endX = winWidth * 0.8f;

	Image screenshot = LoadImageFromScreen();

	Rectangle rec = { startX, 0, endX - startX, screenshot.height };
	Image img = ImageFromImage(screenshot, rec);

	ExportImage(img, "Paint_Image.png");

	UnloadImage(screenshot);
	UnloadImage(img);
}

/*

void loadPhoto(int winWidth, int winHeight) {
	Image img = LoadImage("C:/Users/illiy/Downloads/19901598.jpg");

	if (img.width == 0 || img.height == 0) return;

	ImageResize(&img, 1000, 1000);

	Texture2D texture = LoadTextureFromImage(img);

	int drawWidth = (int)(winWidth * 0.7f);
	int drawHeight = winHeight;
	int offsetX = (int)(winWidth * 0.1f);

	All_Layers.Layers[current_layer]->addElement("image", offsetX, 0, drawWidth, drawHeight, 0, { 0, 0 }, WHITE, hold_index, texture);

	UnloadImage(img);
}

*/

int main() {
	Vector3 screenResolution = getMonitorResolution();

	int winWidth = static_cast<int>(screenResolution.x * relativeSizeX);
	int winHeight = static_cast<int>(screenResolution.y * relativeSizeY);

	InitWindow(winWidth, winHeight, "Paint on C++");
	SetExitKey(KEY_NULL);
	SetTargetFPS(static_cast<int>(screenResolution.z));

	ClearBackground(WHITE);

	const Color colors[15] = { LIME, GREEN, DARKGREEN, RED, BLUE, YELLOW, ORANGE, PINK, BLACK, DARKGRAY, WHITE, BROWN, GOLD, MAROON, MAGENTA };

	while (!WindowShouldClose()) {
		BeginDrawing();

		ClearBackground(WHITE);

		for (Layer* layer : All_Layers.Layers) {
			layer->DrawAllElements();
		}

		Vector2 mousePosition = GetMousePosition();

		float wheel = GetMouseWheelMove();
		if (wheel > 0 and sizeOfBrush < 100 and mousePosition.x>winWidth*0.1) sizeOfBrush += 1;
		else if (wheel < 0 and sizeOfBrush > 3 and mousePosition.x > winWidth * 0.1) sizeOfBrush -= 1;

		//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
		//			Отрисовка фигур
		//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			if (mousePosition.x < winWidth * 0.8 and mousePosition.x > winWidth*0.1)
				painting = true;
		}

		Color endColor = { currentColor.r, currentColor.g, currentColor.b, currentColor.a * (100 - currentTransparency) / 100 };

		if (drawType == "eraser") {
			endColor = WHITE;
		}

		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) and painting and mousePosition.x > winWidth*0.1 and mousePosition.x < winWidth * 0.8 + sizeOfBrush and (drawType == "eraser" || drawType == "brush")) {
			DrawCircle(mousePosition.x, mousePosition.y, sizeOfBrush, endColor);
			All_Layers.Layers[current_layer]->addElement("brush", mousePosition.x, mousePosition.y, 0, 0, sizeOfBrush, { 0.0, 0.0 }, endColor, hold_index);
		}

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) and mousePosition.x > winWidth * 0.1 and mousePosition.x < winWidth * 0.8 + sizeOfBrush / 2) {
			if (drawType == "circle") {
				painting_circle = true;
				start_circle = { mousePosition.x, mousePosition.y };
			}
			else if (drawType == "rectangle") {
				painting_rectangle = true;
				start_rectangle = { mousePosition.x, mousePosition.y };
			}
			else if (drawType == "line") {
				painting_line = true;
				start_line = { mousePosition.x, mousePosition.y };
			}
		}

		if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
			float X_FOR_MOUSE = fminf(fmaxf(mousePosition.x, 0), winWidth);
			float Y_FOR_MOUSE = fminf(fmaxf(mousePosition.y, 0), winHeight);
			Vector2 mouseInWindow = { X_FOR_MOUSE, Y_FOR_MOUSE };

			float x = fabsf(mouseInWindow.x - start_circle.x);
			float y = fabsf(mouseInWindow.y - start_circle.y);
			float radius = fminf(x, y) / 2;

			if (drawType == "circle" and painting_circle) {
				DrawCircle((start_circle.x + mouseInWindow.x) / 2, (start_circle.y + mouseInWindow.y) / 2, radius, endColor);

				All_Layers.Layers[current_layer]->addElement("circle", (start_circle.x + mouseInWindow.x) / 2, (start_circle.y + mouseInWindow.y) / 2, 0, 0, radius, { 0.0, 0.0 }, endColor, hold_index);
			}
			else if (drawType == "rectangle" and painting_rectangle) {
				float endX = fminf(start_rectangle.x, mouseInWindow.x);
				float endY = fminf(start_rectangle.y, mouseInWindow.y);
				float endW = fminf(fabsf(mouseInWindow.x - start_rectangle.x), fabsf(winWidth - endX));
				float endH = fminf(fabsf(mouseInWindow.y - start_rectangle.y), fabsf(winHeight - endY));

				DrawRectangle(endX, endY, endW, endH, endColor);

				All_Layers.Layers[current_layer]->addElement("rectangle", endX, endY, endW, endH, 0, { 0.0, 0.0 }, endColor, hold_index);

			}
			else if (drawType == "line" and painting_line) {
				DrawLineEx(start_line, mouseInWindow, sizeOfBrush, endColor);

				All_Layers.Layers[current_layer]->addElement("line", start_line.x, start_line.y, 0, 0, sizeOfBrush, mouseInWindow, endColor, hold_index);
			}

			painting_line = false;
			painting_circle = false;
			painting_rectangle = false;
			painting = false;
			hold_index += 1;
		}


		DrawRectangle(winWidth * 0.8, 0, winWidth * 0.2, winHeight, GRAY);

		//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
		//			ЦВЕТА
		//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№

		DrawText("Colors", winWidth * 0.8 + (30 / (1920 * relativeSizeX) * winWidth), 0, winHeight * 0.1, BLACK);

		const Vector4 colorRectangleUDIM2 = { winWidth * 0.8, 0, winWidth * 0.2, winHeight };

		const short layer1 = winHeight * 0.1;
		const short layer2 = winHeight * 0.18;
		const short layer3 = winHeight * 0.26;

		const short sizeOfOneBlock = (winWidth * 0.2 - (20 / (1920 * relativeSizeX) * winWidth)) * 0.18;

		const Color colors[15] = { LIME, GREEN, DARKGREEN, RED, BLUE, YELLOW, ORANGE, PINK, BLACK, DARKGRAY, WHITE, BROWN, GOLD, MAROON, MAGENTA };
		const Vector2 positions[15] =
		{
			{(colorRectangleUDIM2.x + (10 / (1920 * relativeSizeX) * winWidth) + (colorRectangleUDIM2.z - (10 / (1080 * relativeSizeY) * winHeight)) * 0), layer1},
			{(colorRectangleUDIM2.x + (10 / (1920 * relativeSizeX) * winWidth) + (colorRectangleUDIM2.z - (10 / (1080 * relativeSizeY) * winHeight)) * 0.2), layer1},
			{(colorRectangleUDIM2.x + (10 / (1920 * relativeSizeX) * winWidth) + (colorRectangleUDIM2.z - (10 / (1080 * relativeSizeY) * winHeight)) * 0.4), layer1},
			{(colorRectangleUDIM2.x + (10 / (1920 * relativeSizeX) * winWidth) + (colorRectangleUDIM2.z - (10 / (1080 * relativeSizeY) * winHeight)) * 0.6), layer1},
			{(colorRectangleUDIM2.x + (10 / (1920 * relativeSizeX) * winWidth) + (colorRectangleUDIM2.z - (10 / (1080 * relativeSizeY) * winHeight)) * 0.8), layer1},

			{(colorRectangleUDIM2.x + (10 / (1920 * relativeSizeX) * winWidth) + (colorRectangleUDIM2.z - (10 / (1080 * relativeSizeY) * winHeight)) * 0), layer2},
			{(colorRectangleUDIM2.x + (10 / (1920 * relativeSizeX) * winWidth) + (colorRectangleUDIM2.z - (10 / (1080 * relativeSizeY) * winHeight)) * 0.2), layer2},
			{(colorRectangleUDIM2.x + (10 / (1920 * relativeSizeX) * winWidth) + (colorRectangleUDIM2.z - (10 / (1080 * relativeSizeY) * winHeight)) * 0.4), layer2},
			{(colorRectangleUDIM2.x + (10 / (1920 * relativeSizeX) * winWidth) + (colorRectangleUDIM2.z - (10 / (1080 * relativeSizeY) * winHeight)) * 0.6), layer2},
			{(colorRectangleUDIM2.x + (10 / (1920 * relativeSizeX) * winWidth) + (colorRectangleUDIM2.z - (10 / (1080 * relativeSizeY) * winHeight)) * 0.8), layer2},

			{(colorRectangleUDIM2.x + (10 / (1920 * relativeSizeX) * winWidth) + (colorRectangleUDIM2.z - (10 / (1080 * relativeSizeY) * winHeight)) * 0), layer3 },
			{(colorRectangleUDIM2.x + (10 / (1920 * relativeSizeX) * winWidth) + (colorRectangleUDIM2.z - (10 / (1080 * relativeSizeY) * winHeight)) * 0.2), layer3},
			{(colorRectangleUDIM2.x + (10 / (1920 * relativeSizeX) * winWidth) + (colorRectangleUDIM2.z - (10 / (1080 * relativeSizeY) * winHeight)) * 0.4), layer3},
			{(colorRectangleUDIM2.x + (10 / (1920 * relativeSizeX) * winWidth) + (colorRectangleUDIM2.z - (10 / (1080 * relativeSizeY) * winHeight)) * 0.6), layer3},
			{(colorRectangleUDIM2.x + (10 / (1920 * relativeSizeX) * winWidth) + (colorRectangleUDIM2.z - (10 / (1080 * relativeSizeY) * winHeight)) * 0.8), layer3}
		};

		Color LIGHT_BLUE = { 3, 227, 243, 255 };

		for (short i = 0; i < 15; i++) {
			if (colorsMatch(currentColor, colors[i])) {
				DrawRectangle(positions[i].x - (3 / (1920 * relativeSizeX) * winWidth), positions[i].y - (3 / (1080 * relativeSizeY) * winHeight), sizeOfOneBlock + (6 / (1920 * relativeSizeX) * winWidth), sizeOfOneBlock + (6 / (1920 * relativeSizeX) * winWidth), LIGHT_BLUE);
			}

			DrawRectangle(positions[i].x, positions[i].y, sizeOfOneBlock, sizeOfOneBlock, colors[i]);
		}

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			for (short i = 0; i < 15; i++) {
				if ((mousePosition.x > positions[i].x and mousePosition.x < positions[i].x + sizeOfOneBlock) and (mousePosition.y > positions[i].y and mousePosition.y < positions[i].y + sizeOfOneBlock)) {
					currentColor = colors[i];
					break;
				}
			}
		}

		//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
		//			ПРОЗРАЧНОСТЬ
		//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№

		DrawText("Transparency", winWidth * 0.8 + (15 / (1920 * relativeSizeX) * winWidth), winHeight * 0.35, winHeight * 0.05, BLACK);

		Vector4 sizeOfTransparency = { winWidth * 0.8 + (15 / (1920 * relativeSizeX) * winWidth), winHeight * 0.43, winWidth * 0.15 - (30 / (1920 * relativeSizeX) * winWidth), winHeight * 0.05 };

		char percentSTR[5];
		sprintf_s(percentSTR, "%d\%%", currentTransparency);
		DrawText(percentSTR, sizeOfTransparency.x + sizeOfTransparency.z + (20 / (1920 * relativeSizeX) * winWidth), sizeOfTransparency.y + (5 / (1080 * relativeSizeY) * winHeight), winHeight * 0.04, BLACK);

		DrawRectangle(sizeOfTransparency.x - (3 / (1920 * relativeSizeX) * winWidth), sizeOfTransparency.y - (3 / (1080 * relativeSizeY) * winHeight), sizeOfTransparency.z + (6 / (1920 * relativeSizeX) * winWidth), sizeOfTransparency.w + (6 / (1080 * relativeSizeY) * winHeight), BLACK);
		DrawRectangle(sizeOfTransparency.x, sizeOfTransparency.y, sizeOfTransparency.z, sizeOfTransparency.w, DARKGRAY);

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			if ((mousePosition.x > sizeOfTransparency.x and mousePosition.x < sizeOfTransparency.x + sizeOfTransparency.z) and (mousePosition.y > sizeOfTransparency.y and mousePosition.y < sizeOfTransparency.y + sizeOfTransparency.w)) {
				changing_transparency = true;
			}
		}

		if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
			changing_transparency = false;
			painting = false;
		}

		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
			if ((mousePosition.x > sizeOfTransparency.x and mousePosition.x < sizeOfTransparency.x + sizeOfTransparency.z) and (mousePosition.y > sizeOfTransparency.y and mousePosition.y < sizeOfTransparency.y + sizeOfTransparency.w)) {
				if (changing_transparency) {
					int where_mouse = (sizeOfTransparency.z - (sizeOfTransparency.x + sizeOfTransparency.z - mousePosition.x)) / (sizeOfTransparency.z) * 100;

					if (where_mouse == 99) {
						where_mouse = 100;
					}

					currentTransparency = where_mouse;
				}
			}
		}

		DrawRectangle(sizeOfTransparency.x, sizeOfTransparency.y, sizeOfTransparency.z * currentTransparency / 100, sizeOfTransparency.w, BLUE);

		//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
		//			ИНСТРУМЕНТЫ
		//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№

		DrawText("Tools", winWidth * 0.8 + (100 / (1920 * relativeSizeX) * winWidth), winHeight * 0.5, winHeight * 0.05, BLACK);

		const string tools[5] = {"brush", "eraser", "line", "rectangle", "circle"};
		const float sizesTools[5] = {0.035, 0.03, 0.04, 0.025, 0.03};

		const Vector4 toolsUDIM = { winWidth * 0.8, winHeight*0.57, winWidth * 0.2, winHeight*0.3};

		const short toolsBlockSizeX = (toolsUDIM.z) * 0.3;
		const short toolsBlockSizeY = (toolsUDIM.z) * 0.15;

		const Vector2 toolsPositions[5] = 
		{
			{toolsUDIM.x + toolsUDIM.z * 0.05, toolsUDIM.y},
			{toolsUDIM.x + toolsUDIM.z * 0.65, toolsUDIM.y},
			{toolsUDIM.x + toolsUDIM.z * 0.05, toolsUDIM.y + (60 / (1080 * relativeSizeY) * winHeight)},
			{toolsUDIM.x + toolsUDIM.z * 0.65, toolsUDIM.y + (60 / (1080 * relativeSizeY) * winHeight)},
			{toolsUDIM.x + toolsUDIM.z * 0.35, toolsUDIM.y + (120 / (1080 * relativeSizeY) * winHeight)}
		};

		for (short i = 0; i < 5; i++) {
			if (drawType == tools[i]) {
				DrawRectangle(toolsPositions[i].x - (3 / (1920 * relativeSizeX) * winWidth), toolsPositions[i].y - (3 / (1080 * relativeSizeY) * winHeight), toolsBlockSizeX + (6 / (1920 * relativeSizeX) * winWidth), toolsBlockSizeY + (6 / (1080 * relativeSizeY) * winHeight), LIGHT_BLUE);
			}
			else {
				DrawRectangle(toolsPositions[i].x - (3 / (1920 * relativeSizeX) * winWidth), toolsPositions[i].y - (3 / (1080 * relativeSizeY) * winHeight), toolsBlockSizeX + (6 / (1920 * relativeSizeX) * winWidth), toolsBlockSizeY + (6 / (1080 * relativeSizeY) * winHeight), BLACK);
			}

			DrawRectangle(toolsPositions[i].x, toolsPositions[i].y, toolsBlockSizeX, toolsBlockSizeY, WHITE);
			DrawText(tools[i].c_str(), toolsPositions[i].x + (5 / (1920 * relativeSizeX) * winWidth), toolsPositions[i].y+winHeight*0.015, winHeight * sizesTools[i], BLACK);
		}

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			Vector2 mousePosition = GetMousePosition();

			for (short i = 0; i < 15; i++) {
				if ((mousePosition.x > toolsPositions[i].x and mousePosition.x < toolsPositions[i].x + toolsBlockSizeX) and (mousePosition.y > toolsPositions[i].y and mousePosition.y < toolsPositions[i].y + toolsBlockSizeY)) {
					drawType = tools[i];
					break;
				}
			}
		}

		ostringstream oss;
		oss << " Mouse Wheel \nto change size\n      (" << sizeOfBrush << ")";
		string str = oss.str();
		
		DrawText(str.c_str(), winWidth * 0.83, winHeight * 0.85, winWidth * 0.02, BLACK);


		//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
		//			РАБОТА СО СЛОЯМИ
		//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№

		if (IsKeyDown(KEY_LEFT_CONTROL) and IsKeyPressed(KEY_Z)) {
			All_Layers.Layers[current_layer]->deleteElement(-1);
		}

		DrawRectangle(0, 0, winWidth * 0.1, winHeight, GRAY);

		DrawText("Layers", winWidth * 0.004, winHeight * 0.02, winWidth * 0.02, BLACK);

		if (from_index > All_Layers.Layers.size()-5) {
			from_index = 0;
		}

		if (wheel < 0 and from_index+5 < All_Layers.Layers.size() and mousePosition.x <= winWidth * 0.1) from_index += 1;
		else if (wheel > 0 and from_index > 0 and mousePosition.x <= winWidth * 0.1) from_index -= 1;

		int sizeOfLayerX = winWidth * 0.08;
		int sizeOfLayerY = winHeight * 0.08;

		for (int i = 0; i < max_layers_on_list; i++) {
			ostringstream num_of_layer;
			num_of_layer << from_index+i;
			string str_layer = num_of_layer.str();

			if (i > All_Layers.Layers.size() - 1) { break;  }

			Vector2 Position = { winWidth * 0.01, winHeight * (0.1 + 0.09 * (i)) };

			if (current_layer == All_Layers.Layers[from_index + i]->index) {
				DrawRectangle(Position.x - winWidth * 0.005, Position.y - winHeight * 0.005, sizeOfLayerX + winWidth * 0.01, sizeOfLayerY + winHeight * 0.01, LIGHT_BLUE);
			}
			else {
				DrawRectangle(Position.x - winWidth * 0.005, Position.y - winHeight * 0.005, sizeOfLayerX + winWidth * 0.01, sizeOfLayerY + winHeight * 0.01, LIGHTGRAY);
			}

			DrawRectangle(Position.x, Position.y, sizeOfLayerX, sizeOfLayerY, WHITE);


			// ОТОБРАЖЕНИЕ ЭЛЕМЕНТОВ СЛОЯ НА МИНИ СЛОЙ СПРАВА

			for (DrawObject* obj : All_Layers.Layers[from_index + i]->objects) {
				float size_multiplier = sizeOfLayerX / (winWidth * 0.7);

				int startX = (obj->startX * sizeOfLayerX / winWidth) + Position.x;
				int startY = (obj->startY * sizeOfLayerY / winHeight) + Position.y;

				int sizeX = (obj->sizeX * sizeOfLayerX / winWidth);
				int sizeY = (obj->sizeY * sizeOfLayerY / winHeight);

				Vector2 MousePosition = {
					(obj->MousePosition.x * sizeOfLayerX / winWidth) + Position.x,
					(obj->MousePosition.y * sizeOfLayerY / winHeight) + Position.y
				};

				if (obj->isBrush) {
					float miniWidth = sizeOfLayerX;
					float miniHeight = miniWidth * ((float)obj->brushTexture.texture.height / obj->brushTexture.texture.width);

					if (miniHeight > sizeOfLayerY) {
						miniHeight = sizeOfLayerY;
						miniWidth = miniHeight * ((float)obj->brushTexture.texture.width / obj->brushTexture.texture.height);
					}

					DrawTexturePro(obj->brushTexture.texture, {0, (float)obj->brushTexture.texture.height, (float)obj->brushTexture.texture.width, -(float)obj->brushTexture.texture.height}, {(float)Position.x, (float)Position.y, miniWidth, miniHeight}, {0, 0}, 0.0, WHITE);
				}

				else if (obj->Type == "brush" || obj->Type == "circle" || obj->Type == "eraser") {
					DrawCircle(startX, startY, MAX((double)obj->Radius * size_multiplier, 1.0), obj->color);
				}
				else if (obj->Type == "rectangle") {
					DrawRectangle(startX, startY, sizeX, sizeY, obj->color);
				}
				else if (obj->Type == "line") {
					DrawLineEx({ (float)startX, (float)startY }, { MousePosition.x, MousePosition.y }, obj->Radius * size_multiplier, obj->color);
				}
				else if (obj->Type == "pixel") {
					DrawPixel(startX, startY, obj->color);
				}
				else if (obj->Type == "image") {
					Rectangle source = { 0, 0, (float)obj->texture.width, (float)obj->texture.height };
					Rectangle dst = { (float)startX, (float)startY, (float)sizeX, (float)sizeY };
					DrawTexturePro(obj->texture, source, dst, { 0, 0 }, 0.0f, WHITE);
				}
			}


			// ########################################


			DrawText(str_layer.c_str(), Position.x + sizeOfLayerX * 0.05, Position.y + sizeOfLayerY * 0.7, winHeight * 0.02, BLACK);

			if (All_Layers.Layers[from_index + i]->visible) {
				// слой виден
			}
			else {
				DrawRectangle(Position.x, Position.y, sizeOfLayerX, sizeOfLayerY, Color{ 0,0,0,100 });
			}
		}

		// DrawRectangle(winWidth * 0.08, winHeight * 0.01, winWidth * 30 / 1920, winHeight * 35 / 1080, RED); // это чисто для дебага
		// DrawRectangle(winWidth * 0.08, winHeight * 0.055, winWidth * 30 / 1920, winHeight * 30 / 1080, RED); // это чисто для дебага

		DrawText("+", winWidth * 0.08, winHeight * 0, winWidth * 0.03, LIME);
		DrawText("-", winWidth * 0.081, winHeight * 0.04, winWidth * 0.03, RED);

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			if (mousePosition.x > winWidth * 0.08 and mousePosition.x < winWidth * 0.08 + winWidth * 30 / 1920
				and mousePosition.y > winHeight * 0.01 and mousePosition.y < winHeight * 0.01 + winHeight * 35 / 1080) {
				All_Layers.addLayer();
			}
			else if (mousePosition.x > winWidth * 0.08 and mousePosition.x < winWidth * 0.08 + winWidth * 30 / 1920
				and mousePosition.y > winHeight * 0.055 and mousePosition.y < winHeight * 0.055 + winHeight * 30 / 1080) {
				All_Layers.deleteLayer(current_layer);
			}

			for (int i = 0; i < max_layers_on_list; i++) {
				Vector2 Position = { winWidth * 0.01, winHeight * (0.1 + 0.09 * (i)) };
				if (mousePosition.x > Position.x and mousePosition.x < Position.x + sizeOfLayerX
					and mousePosition.y > Position.y and mousePosition.y < Position.y + sizeOfLayerY and All_Layers.Layers.size() > from_index + i) {
					current_layer = from_index + i;
					break;
				}
			}
		}

		if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
			for (int i = 0; i < max_layers_on_list; i++) {
				Vector2 Position = { winWidth * 0.01, winHeight * (0.1 + 0.09 * (i)) };
				if (mousePosition.x > Position.x and mousePosition.x < Position.x + sizeOfLayerX
					and mousePosition.y > Position.y and mousePosition.y < Position.y + sizeOfLayerY) {
					All_Layers.Layers[from_index + i]->visible = !All_Layers.Layers[from_index + i]->visible;
					break;
				}
			}
		}

		if (All_Layers.Layers.size() > max_layers_on_list) {
			Vector2 Size = { winWidth * 0.0025f, winHeight * 0.45f * max_layers_on_list / (float)All_Layers.Layers.size() };
			float progress = (float)from_index / fmaxf((float)All_Layers.Layers.size() - max_layers_on_list, 1.0f);
			Vector2 Position = { winWidth * 0.096f, winHeight * 0.095f + (winHeight * 0.45f - Size.y) * fminf(fmaxf(progress, 0.0f), 1.0f) };
			DrawRectangle(Position.x, Position.y, Size.x, Size.y, Color{ 60,60,60,255 });
		}

		//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
		//			СИСТЕМНАЯ ОТРИСОВАКА
		//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№

		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
			float X_FOR_MOUSE = fminf(fmaxf(mousePosition.x, 0), winWidth);
			float Y_FOR_MOUSE = fminf(fmaxf(mousePosition.y, 0), winHeight);
			Vector2 mouseInWindow = { X_FOR_MOUSE, Y_FOR_MOUSE };

			float x = fabsf(mouseInWindow.x - start_circle.x);
			float y = fabsf(mouseInWindow.y - start_circle.y);
			float radius = fminf(x, y) / 2;

			Color color_for_system_layer = Color{ 0, 179, 255, 100 };

			if (drawType == "circle" and painting_circle) {
				DrawCircle((start_circle.x + mouseInWindow.x) / 2, (start_circle.y + mouseInWindow.y) / 2, radius, color_for_system_layer);
			}
			else if (drawType == "rectangle" and painting_rectangle) {
				float endX = fminf(start_rectangle.x, mouseInWindow.x);
				float endY = fminf(start_rectangle.y, mouseInWindow.y);
				float endW = fminf(fabsf(mouseInWindow.x - start_rectangle.x), fabsf(winWidth - endX));
				float endH = fminf(fabsf(mouseInWindow.y - start_rectangle.y), fabsf(winHeight - endY));

				DrawRectangle(endX, endY, endW, endH, color_for_system_layer);
			}
			else if (drawType == "line" and painting_line) {
				DrawLineEx(start_line, mouseInWindow, sizeOfBrush, color_for_system_layer);
			}
		}

		/*
		
		if (IsKeyDown(KEY_LEFT_CONTROL) and IsKeyPressed(KEY_C)) {
			loadPhoto(winWidth, winHeight);
		}
		
		*/

		//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№
		//			СВАП СЛОЕВ
		//№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№№

		if (IsKeyPressed(KEY_DOWN)) {
			if (All_Layers.Layers.size() > 1 and current_layer != All_Layers.Layers.size() - 1) {
				All_Layers.DownLayer();
			}
		}

		if (IsKeyPressed(KEY_UP)) {
			if (All_Layers.Layers.size() > 1 and current_layer != 0) {
				All_Layers.UpLayer();
			}
		}
		
		// DrawRectangle(winWidth*0, winHeight*0.97, winWidth*0.03, winHeight*0.03, RED); // Дебаг
		DrawText("Info", winWidth * 0.001, winHeight * 0.97, winHeight * 0.03, BLACK);

		if (mousePosition.x < winWidth * 0.03 and mousePosition.y > winHeight * 0.97) {
			DrawRectangle(winWidth * 0.047, winHeight * 0.697, winWidth * 0.206, winHeight * 0.306, LIGHTGRAY);
			DrawRectangle(winWidth*0.05, winHeight*0.7, winWidth*0.2, winHeight*0.2995, Color{230,230,230,255});

			string Text[10] = {"Export - CTRL+S", "Undo - CTRL+Z", "Swap Layer Up - UP", "Swap Layer Down - DOWN", "Resize Brush - Mouse Wheel", "Toggle layer visible - MMB"};

			for (short i = 0; i < 10; i++) {
				DrawText(Text[i].c_str(), winWidth * 0.05, winHeight * 0.7 + ((winHeight * 0.3) * i / 10), winHeight*0.3*0.09, BLACK);
			}
		}

		EndDrawing();

		if (IsKeyDown(KEY_LEFT_CONTROL) and IsKeyPressed(KEY_S)) {
			savePhoto(winWidth);
		}
	}

	CloseWindow(); 
	return 0;
}