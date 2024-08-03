#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <raylib.h>
#include <raymath.h>
#include <rcamera.h>

#include "lookuptables.h"

// #define WINDOW_WIDTH 			1920
// #define WINDOW_HEIGHT 			1080
#define WINDOW_WIDTH 			800
#define WINDOW_HEIGHT 			800

#define GRID_SIZE   16
#define CELL_SIZE   1.0f

#define CAMERA_MOUSE_SCROLL_SENSITIVITY 3.5f
#define CAMERA_MOUSE_MOVE_SENSITIVITY   0.003f
#define CAMERA_ROTATION_SPEED           0.03f
#define CAMERA_PAN_SPEED                0.2f
#define CAMERA_MOVE_SPEED               0.25f

typedef struct {
    Vector3 position;
    Vector3 color;
} Vertex;

static inline int cube_cord_index(int size, int x, int y, int z) {
    return x + size * (y + size * z);
}

static inline float min(float a, float b) {
    return (a <= b) ? a: b;
}

static inline float max(float a, float b) {
    return (a >= b) ? a: b;
}

static inline float interpolate_values(float v0, float v1, float isovalue) {
	float a = min(v0, v1);
	float b = max(v0, v1);

	float result = (isovalue - a) / (b - a);
    return result;
}

Vector3 interpolate_vertex(Vector3 p1, Vector3 p2, float v1, float v2, float isovalue) {
    float mu;
    Vector3 p;

    if (fabs(isovalue - v1) < 0.00001)
        return p1;
    if (fabs(isovalue - v2) < 0.00001)
        return p2;
    if (fabs(v1 - v2) < 0.00001)
        return p1;

    mu = (isovalue - v1) / (v2 - v1);
    p.x = p1.x + mu * (p2.x - p1.x);
    p.y = p1.y + mu * (p2.y - p1.y);
    p.z = p1.z + mu * (p2.z - p1.z);

    return p;
}

static inline float sphere(float x, float y, float z) {
    return sqrtf(x * x + y * y + z * z) - 0.5f;
}

static void generate_scalar_field(float *field, int size, float (*func)(float, float, float)) {
    for (int z = 0; z < size; z++) {
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float fx = (float)x / (size - 1) * 2.0f - 1.0f;
                float fy = (float)y / (size - 1) * 2.0f - 1.0f;
                float fz = (float)z / (size - 1) * 2.0f - 1.0f;
                field[z * size * size + y * size + x] = func(fx, fy, fz);
            }
        }
    }
}

static void generate_cube_scalar_field(float *field, int size) {
    for (int z = 0; z < size; z++) {
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {

                if ((x != 0 && x < size-1) && (y != 0 && y < size-1) && (z != 0 && z < size-1))
                    field[z * size * size + y * size + x] = 1.0;
                else
                    field[z * size * size + y * size + x] = 0.0;
            }
        }
    }
}
static Mesh marching_cubes(float *field, int size, float isovalue) {
    Vector3 vertex_list[12];
    int vertex_count = 0;
    int max_vertices = size * size * size * 15;
    Vertex *vertices = (Vertex *)malloc(max_vertices * sizeof(Vertex));

    for (int z = 0; z < size - 1; z++) {
        for (int y = 0; y < size - 1; y++) {
            for (int x = 0; x < size - 1; x++) {
                
                int cube_index = 0;
                Vector3 p[8];
                float val[8];

				Vector3 cell_pos = {
					.x = (float)x,
					.y = (float)y,
					.z = (float)z
				};

                for (int i = 0; i < 8; i++) {
                    int ix = cube_cords[i][0];
                    int iy = cube_cords[i][1];
                    int iz = cube_cords[i][2];
                    p[i] = (Vector3){(float)ix, (float)iy, (float)iz};
                    
                    int idx = cube_cord_index(size, x + ix, y + iy, z + iz);
                    val[i] = field[idx];
                    
                    if (val[i] < isovalue)
                        cube_index |= (1 << i);
                }

                if (edge_table[cube_index] == 0)
                    continue;

				if (edge_table[cube_index] & 1)
					vertex_list[0] = interpolate_vertex(p[0],p[1],val[0],val[1], isovalue);
				if (edge_table[cube_index] & 2)
					vertex_list[1] = interpolate_vertex(p[1],p[2],val[1],val[2], isovalue);
				if (edge_table[cube_index] & 4)
					vertex_list[2] = interpolate_vertex(p[2],p[3],val[2],val[3], isovalue);
				if (edge_table[cube_index] & 8)
					vertex_list[3] = interpolate_vertex(p[3],p[0],val[3],val[0], isovalue);
				if (edge_table[cube_index] & 16)
					vertex_list[4] = interpolate_vertex(p[4],p[5],val[4],val[5], isovalue);
				if (edge_table[cube_index] & 32)
					vertex_list[5] = interpolate_vertex(p[5],p[6],val[5],val[6], isovalue);
				if (edge_table[cube_index] & 64)
					vertex_list[6] = interpolate_vertex(p[6],p[7],val[6],val[7], isovalue);
				if (edge_table[cube_index] & 128)
					vertex_list[7] = interpolate_vertex(p[7],p[4],val[7],val[4], isovalue);
				if (edge_table[cube_index] & 256)
					vertex_list[8] = interpolate_vertex(p[0],p[4],val[0],val[4], isovalue);
				if (edge_table[cube_index] & 512)
					vertex_list[9] = interpolate_vertex(p[1],p[5],val[1],val[5], isovalue);
				if (edge_table[cube_index] & 1024)
					vertex_list[10] = interpolate_vertex(p[2],p[6],val[2],val[6], isovalue);
				if (edge_table[cube_index] & 2048)
					vertex_list[11] = interpolate_vertex(p[3],p[7],val[3],val[7], isovalue);

                for (int i = 0; tri_table[cube_index][i] != -1; i += 3) {
					Vector3 p1 = Vector3Add(vertex_list[tri_table[cube_index][i]] , cell_pos);
					Vector3 p2 = Vector3Add(vertex_list[tri_table[cube_index][i + 1]], cell_pos);
					Vector3 p3 = Vector3Add(vertex_list[tri_table[cube_index][i + 2]], cell_pos);

					vertices[vertex_count++] = (Vertex){p1, (Vector3){1.0f, 0.0f, 0.0f}};
					vertices[vertex_count++] = (Vertex){p2, (Vector3){1.0f, 0.0f, 0.0f}};
					vertices[vertex_count++] = (Vertex){p3, (Vector3){1.0f, 0.0f, 0.0f}};
                }
            }
        }
    }

    Mesh mesh = {0};
    mesh.vertexCount = vertex_count;
    mesh.vertices = (float *)malloc(vertex_count * 3 * sizeof(float));
    mesh.colors = (unsigned char *)malloc(vertex_count * 4 * sizeof(unsigned char));

    for (int i = 0; i < vertex_count; i++) {
        mesh.vertices[i * 3 + 0] = vertices[i].position.x;
        mesh.vertices[i * 3 + 1] = vertices[i].position.y;
        mesh.vertices[i * 3 + 2] = vertices[i].position.z;
        mesh.colors[i * 4 + 0] = (unsigned char)(vertices[i].color.x * 255);
        mesh.colors[i * 4 + 1] = (unsigned char)(vertices[i].color.y * 255);
        mesh.colors[i * 4 + 2] = (unsigned char)(vertices[i].color.z * 255);
        mesh.colors[i * 4 + 3] = 255;
    }

    free(vertices);
    return mesh;
}

void update_camera_custom(Camera *camera)
{
    Vector2 mouse_position_delta = GetMouseDelta();

    bool move_in_world_plane = 0;
    bool rotate_around_target = 0;
    bool lock_view = 0;
    bool rotate_up = false;

    // Camera rotation
    if (IsKeyDown(KEY_DOWN)) CameraPitch(camera, -CAMERA_ROTATION_SPEED, lock_view, rotate_around_target, rotate_up);
    if (IsKeyDown(KEY_UP)) CameraPitch(camera, CAMERA_ROTATION_SPEED, lock_view, rotate_around_target, rotate_up);
    if (IsKeyDown(KEY_RIGHT)) CameraYaw(camera, -CAMERA_ROTATION_SPEED, rotate_around_target);
    if (IsKeyDown(KEY_LEFT)) CameraYaw(camera, CAMERA_ROTATION_SPEED, rotate_around_target);
    if (IsKeyDown(KEY_Z)) CameraRoll(camera, -CAMERA_ROTATION_SPEED);
    if (IsKeyDown(KEY_C)) CameraRoll(camera, CAMERA_ROTATION_SPEED);
    if (IsKeyDown(KEY_Q)) CameraMoveUp(camera, -CAMERA_PAN_SPEED);
    if (IsKeyDown(KEY_E)) CameraMoveUp(camera, CAMERA_PAN_SPEED);

    // Camera movement
    if (!IsGamepadAvailable(0))
    {
        // Camera pan (for CAMERA_FREE)
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
        {
            const Vector2 mouse_delta = GetMouseDelta();
            if (mouse_delta.x > 0.0f) CameraMoveRight(camera, CAMERA_PAN_SPEED, move_in_world_plane);
            if (mouse_delta.x < 0.0f) CameraMoveRight(camera, -CAMERA_PAN_SPEED, move_in_world_plane);
            if (mouse_delta.y > 0.0f) CameraMoveUp(camera, -CAMERA_PAN_SPEED);
            if (mouse_delta.y < 0.0f) CameraMoveUp(camera, CAMERA_PAN_SPEED);
        }
        else
        {
            // Mouse support
            CameraYaw(camera, -mouse_position_delta.x*CAMERA_MOUSE_MOVE_SENSITIVITY, rotate_around_target);
            CameraPitch(camera, -mouse_position_delta.y*CAMERA_MOUSE_MOVE_SENSITIVITY, lock_view, rotate_around_target, rotate_up);
        }

        // Keyboard support
        if (IsKeyDown(KEY_W)) CameraMoveForward(camera, CAMERA_MOVE_SPEED, move_in_world_plane);
        if (IsKeyDown(KEY_A)) CameraMoveRight(camera, -CAMERA_MOVE_SPEED, move_in_world_plane);
        if (IsKeyDown(KEY_S)) CameraMoveForward(camera, -CAMERA_MOVE_SPEED, move_in_world_plane);
        if (IsKeyDown(KEY_D)) CameraMoveRight(camera, CAMERA_MOVE_SPEED, move_in_world_plane);
    }

    if (IsKeyDown(KEY_SPACE)) CameraMoveUp(camera, CAMERA_MOVE_SPEED);
    if (IsKeyDown(KEY_LEFT_CONTROL)) CameraMoveUp(camera, -CAMERA_MOVE_SPEED);

    // Zoom target distance    
    CameraMoveToTarget(camera, -GetMouseWheelMove()*CAMERA_MOUSE_SCROLL_SENSITIVITY);
    if (IsKeyPressed(KEY_KP_SUBTRACT)) CameraMoveToTarget(camera, 2.0f);
    if (IsKeyPressed(KEY_KP_ADD)) CameraMoveToTarget(camera, -2.0f);
}

int main(int argc, char** argv) {
	srand((unsigned int)time(0));

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "m-cubes");



    SetTargetFPS(60);

    float isovalue = 0.5f;

    float* scalar_field = (float *)malloc(GRID_SIZE * GRID_SIZE * GRID_SIZE * sizeof(float));
    generate_scalar_field(scalar_field, GRID_SIZE, sphere);
    // generate_cube_scalar_field(scalar_field, GRID_SIZE);

    Mesh mesh = marching_cubes(scalar_field, GRID_SIZE, isovalue);
	// Upload mesh data from CPU (RAM) to GPU (VRAM) memory
    UploadMesh(&mesh, true);
    Model model = LoadModelFromMesh(mesh);

    DisableCursor();                    
    SetMousePosition(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

    Camera3D camera = { 
        .position = {0, 0, 10},
        .target = {0, 0, 0},
        .up = {0, 1, 0},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE
    };

	Vector3 cube_position = { 0.0f, 0.0f, 0.0f };

	while (!WindowShouldClose()) {
        update_camera_custom(&camera);

        BeginDrawing();
		ClearBackground(GetColor(0x181818AA));
            BeginMode3D(camera);
                // DrawCubeWires(cube_position, 2.0f, 2.0f, 2.0f, RED);

                for (size_t i = 0; i < GRID_SIZE; ++i){
                    for (size_t j = 0; j < GRID_SIZE; ++j){
                        // for (size_t k = 0; k < 1; ++k){
                        for (size_t k = 0; k < GRID_SIZE; ++k){
                            
                            // Centered position
                            Vector3 cell_position = {
                                .x = i*CELL_SIZE + CELL_SIZE/2.0f - GRID_SIZE/2.0f,
                                .y = j*CELL_SIZE + CELL_SIZE/2.0f - GRID_SIZE/2.0f,
                                .z = k*CELL_SIZE + CELL_SIZE/2.0f - GRID_SIZE /2.0f,
                            };

                            DrawCubeWires(cell_position, CELL_SIZE, CELL_SIZE, CELL_SIZE, RED);
                        }
                    }
                }

                // Debug
                // DrawCube({0, 0, 0}, CELL_SIZE, CELL_SIZE, CELL_SIZE, WHITE); // center
                // DrawCube({-1, 0, 0}, CELL_SIZE, CELL_SIZE, CELL_SIZE, BLUE); // -x
                // DrawCube({1, 0, 0}, CELL_SIZE, CELL_SIZE, CELL_SIZE, GREEN); // +x
                // DrawGrid(6, 1.0f); // centered grid

                // Camera front
                Vector3 forward = GetCameraForward(&camera);
                Vector3 cubePos = Vector3Add(camera.position, Vector3Scale(forward, 2.0f));
                DrawCubeWires(cubePos, 0.1f, 0.1f, 0.1f, YELLOW);
                // DrawCube(camera.target, 0.1f, 0.1f, 0.1f, ORANGE);

                Vector3 model_position = {
                    .x = -GRID_SIZE / 2.0f + CELL_SIZE/2.0f,
                    .y = -GRID_SIZE / 2.0f + CELL_SIZE/2.0f,
                    .z = -GRID_SIZE / 2.0f + CELL_SIZE/2.0f
                };
                DrawModel(model, model_position, 1.0f, RED);

            EndMode3D();
		EndDrawing();
	}
	return 0;
}
