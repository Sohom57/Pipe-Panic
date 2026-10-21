#include <windows.h>
#include <GL/glut.h>
#include <stdlib.h>
#include <math.h>
#include <bits/stdc++.h>

using namespace std;

//constants
const float PI = 3.1415926535f;
const int MAXN = 13;

const int PATH = 0;
const int WALL = 1;
const int EXIT = 2;
const int TRAP = 3;

const float BALL_RADIUS = 0.20f;
const float PIPE_RADIUS = 0.36f;
const float TRAP_RADIUS = 0.24f;
const float TRAP_HIT    = 0.34f;

const int TRAIL_LEN = 48;
const int TRAP_RETRY_FRAMES = 80;

enum GameState { STATE_PLAYING, STATE_TRAPPED };

const int DX6[6] = {1, -1, 0, 0, 0, 0};
const int DY6[6] = {0, 0, 1, -1, 0, 0};
const int DZ6[6] = {0, 0, 0, 0, 1, -1};

//state
int N = 7;
int maze[MAXN][MAXN][MAXN];

float ballX = 1.5f, ballY = 1.5f, ballZ = 1.5f;

float mazeAngleX = 0.0f;
float mazeAngleY = 0.0f;
float mazeAngleZ = 0.0f;

int startX = 1, startY = 1, startZ = 1;
int exitX = 1,  exitY = 1,  exitZ = 1;
int levelNumber = 1;
int startRank   = 0;

float moveSpeed  = 0.045f;
int   mazeNodes  = 3;
int   trapCount  = 0;
int   braidCount = 10;
const float ROTATE_SPEED = 1.6f;

GameState gameState = STATE_PLAYING;
int trapFrames = 0;

float camYaw   = 0.45f;
float camPitch = 0.35f;
float camDist  = 20.0f;

int winW = 900, winH = 700;

bool keyDown[256];
bool specialDown[256];

float trailPos[TRAIL_LEN][3];
int   trailCount = 0;
int   trailHead  = 0;

GLuint pipeFaceList = 0;
GLuint pipeRingList = 0;

void buildLevel();

//helpers
bool inBounds(int x, int y, int z) {
    return x >= 0 && x < N && y >= 0 && y < N && z >= 0 && z < N;
}

bool isOpen(int x, int y, int z) {
    return inBounds(x, y, z) && maze[y][z][x] != WALL;
}

int manhattan(int x1, int y1, int z1, int x2, int y2, int z2) {
    return abs(x1 - x2) + abs(y1 - y2) + abs(z1 - z2);
}

int cellCode(int x, int y, int z) { return (y * N + z) * N + x; }

//difficulty
int currentRank() { return startRank + levelNumber - 1; }

void applyLevelSettings() {
    int r = currentRank();

    mazeNodes = 3 + r / 2;
    if (mazeNodes > 6) mazeNodes = 6;

    trapCount = (r * 2) / 3;
    if (trapCount > 10) trapCount = 10;

    braidCount = 10 - r;
    if (braidCount < 2) braidCount = 2;

    moveSpeed = 0.045f + 0.0015f * (float)r;
    if (moveSpeed > 0.060f) moveSpeed = 0.060f;
}

//maze
void carveTunnels() {
    for (int y = 0; y < N; y++)
        for (int z = 0; z < N; z++)
            for (int x = 0; x < N; x++)
                maze[y][z][x] = WALL;

    int L = mazeNodes;
    int sx = 2 * (rand() % L) + 1;
    int sy = 2 * (rand() % L) + 1;
    int sz = 2 * (rand() % L) + 1;

    maze[sy][sz][sx] = PATH;
    vector<int> stk;
    stk.push_back(cellCode(sx, sy, sz));

    while (!stk.empty()) {
        int code = stk.back();
        int cx = code % N;
        int cz = (code / N) % N;
        int cy = code / (N * N);

        int options[6];
        int nOpt = 0;
        for (int d = 0; d < 6; d++) {
            int nx = cx + 2 * DX6[d];
            int ny = cy + 2 * DY6[d];
            int nz = cz + 2 * DZ6[d];
            if (nx < 1 || nx > N - 2 || ny < 1 || ny > N - 2 || nz < 1 || nz > N - 2) continue;
            if (maze[ny][nz][nx] == WALL) options[nOpt++] = d;
        }

        if (nOpt == 0) {
            stk.pop_back();
        } else {
            int d = options[rand() % nOpt];
            maze[cy + DY6[d]][cz + DZ6[d]][cx + DX6[d]] = PATH;
            int nx = cx + 2 * DX6[d];
            int ny = cy + 2 * DY6[d];
            int nz = cz + 2 * DZ6[d];
            maze[ny][nz][nx] = PATH;
            stk.push_back(cellCode(nx, ny, nz));
        }
    }

    //loops
    for (int i = 0; i < braidCount; i++) {
        for (int tries = 0; tries < 300; tries++) {
            int x = 1 + rand() % (N - 2);
            int y = 1 + rand() % (N - 2);
            int z = 1 + rand() % (N - 2);
            int evens = (x % 2 == 0) + (y % 2 == 0) + (z % 2 == 0);
            if (evens != 1) continue;
            if (maze[y][z][x] != WALL) continue;
            maze[y][z][x] = PATH;
            break;
        }
    }
}

void bfsFromStart(vector<int>& dist, vector<int>& parent) {
    dist.assign(N * N * N, -1);
    parent.assign(N * N * N, -1);
    vector<int> q;
    int s = cellCode(startX, startY, startZ);
    dist[s] = 0;
    q.push_back(s);

    for (size_t i = 0; i < q.size(); i++) {
        int code = q[i];
        int cx = code % N;
        int cz = (code / N) % N;
        int cy = code / (N * N);
        for (int d = 0; d < 6; d++) {
            int nx = cx + DX6[d], ny = cy + DY6[d], nz = cz + DZ6[d];
            if (!inBounds(nx, ny, nz)) continue;
            if (maze[ny][nz][nx] == WALL) continue;
            int nc = cellCode(nx, ny, nz);
            if (dist[nc] != -1) continue;
            dist[nc] = dist[code] + 1;
            parent[nc] = code;
            q.push_back(nc);
        }
    }
}

void generateMaze() {
    carveTunnels();

    int L = mazeNodes;
    do {
        startX = 2 * (rand() % L) + 1;
        startY = 2 * (rand() % L) + 1;
        startZ = 2 * (rand() % L) + 1;
    } while (maze[startY][startZ][startX] != PATH);

    //exit
    vector<int> dist, parent;
    bfsFromStart(dist, parent);
    int best = -1, bestCode = cellCode(startX, startY, startZ);
    for (int c = 0; c < N * N * N; c++) {
        if (dist[c] > best) { best = dist[c]; bestCode = c; }
    }
    exitX = bestCode % N;
    exitZ = (bestCode / N) % N;
    exitY = bestCode / (N * N);

    vector<char> onRoute(N * N * N, 0);
    for (int c = bestCode; c != -1; c = parent[c]) onRoute[c] = 1;

    //traps
    int placed = 0, tries = 0;
    while (placed < trapCount && tries < 3000) {
        tries++;
        int x = 1 + rand() % (N - 2);
        int y = 1 + rand() % (N - 2);
        int z = 1 + rand() % (N - 2);
        if (maze[y][z][x] != PATH) continue;
        if (onRoute[cellCode(x, y, z)]) continue;
        if (manhattan(x, y, z, startX, startY, startZ) < 3) continue;
        maze[y][z][x] = TRAP;
        placed++;
    }

    maze[exitY][exitZ][exitX] = EXIT;
}

//glass
void pipeColor(float alpha) {
    glColor4f(0.02f, 0.02f, 0.03f, alpha);
}

void emitPipeQuads(float cx, float cy, float cz, int axis, float r, int seg) {
    float dir[3] = {0, 0, 0};
    float u[3], v[3];
    dir[axis] = 1.0f;
    if (axis == 0) { u[0]=0; u[1]=1; u[2]=0;  v[0]=0; v[1]=0; v[2]=1; }
    else if (axis == 1) { u[0]=1; u[1]=0; u[2]=0;  v[0]=0; v[1]=0; v[2]=1; }
    else { u[0]=1; u[1]=0; u[2]=0;  v[0]=0; v[1]=1; v[2]=0; }

    for (int i = 0; i < seg; i++) {
        float a0 = 2.0f * PI * (float)i / (float)seg;
        float a1 = 2.0f * PI * (float)(i + 1) / (float)seg;
        float n0[3], n1[3];
        for (int k = 0; k < 3; k++) {
            n0[k] = cos(a0) * u[k] + sin(a0) * v[k];
            n1[k] = cos(a1) * u[k] + sin(a1) * v[k];
        }
        glNormal3f(n0[0], n0[1], n0[2]);
        glVertex3f(cx + r * n0[0], cy + r * n0[1], cz + r * n0[2]);
        glVertex3f(cx + r * n0[0] + dir[0], cy + r * n0[1] + dir[1], cz + r * n0[2] + dir[2]);
        glNormal3f(n1[0], n1[1], n1[2]);
        glVertex3f(cx + r * n1[0] + dir[0], cy + r * n1[1] + dir[1], cz + r * n1[2] + dir[2]);
        glVertex3f(cx + r * n1[0], cy + r * n1[1], cz + r * n1[2]);
    }
}

//ring
void emitPipeRing(float cx, float cy, float cz, int axis, float r, int seg) {
    float mid[3] = {cx, cy, cz};
    float u[3], v[3];
    if (axis == 0) { u[0]=0; u[1]=1; u[2]=0;  v[0]=0; v[1]=0; v[2]=1; }
    else if (axis == 1) { u[0]=1; u[1]=0; u[2]=0;  v[0]=0; v[1]=0; v[2]=1; }
    else { u[0]=1; u[1]=0; u[2]=0;  v[0]=0; v[1]=1; v[2]=0; }

    for (int i = 0; i < seg; i++) {
        float a0 = 2.0f * PI * (float)i / (float)seg;
        float a1 = 2.0f * PI * (float)(i + 1) / (float)seg;
        glVertex3f(mid[0] + r * (cos(a0) * u[0] + sin(a0) * v[0]),
                   mid[1] + r * (cos(a0) * u[1] + sin(a0) * v[1]),
                   mid[2] + r * (cos(a0) * u[2] + sin(a0) * v[2]));
        glVertex3f(mid[0] + r * (cos(a1) * u[0] + sin(a1) * v[0]),
                   mid[1] + r * (cos(a1) * u[1] + sin(a1) * v[1]),
                   mid[2] + r * (cos(a1) * u[2] + sin(a1) * v[2]));
    }
}

//joint
void emitJointQuads(float cx, float cy, float cz, float radius, int slices, int stacks, const bool openDir[6]) {
    for (int i = 0; i < stacks; i++) {
        float phi0 = PI * (float)i / (float)stacks - PI / 2.0f;
        float phi1 = PI * (float)(i + 1) / (float)stacks - PI / 2.0f;
        float phic = 0.5f * (phi0 + phi1);
        for (int j = 0; j < slices; j++) {
            float th0 = 2.0f * PI * (float)j / (float)slices;
            float th1 = 2.0f * PI * (float)(j + 1) / (float)slices;
            float thc = 0.5f * (th0 + th1);

            float cnx = cos(phic) * cos(thc);
            float cny = sin(phic);
            float cnz = cos(phic) * sin(thc);

            bool skip = false;
            for (int d = 0; d < 6; d++) {
                if (openDir[d] && (cnx * DX6[d] + cny * DY6[d] + cnz * DZ6[d]) > 0.01f) {
                    skip = true;
                    break;
                }
            }
            if (skip) continue;

            float p[4][2] = {{phi0, th0}, {phi0, th1}, {phi1, th1}, {phi1, th0}};
            for (int k = 0; k < 4; k++) {
                float nx = cos(p[k][0]) * cos(p[k][1]);
                float ny = sin(p[k][0]);
                float nz = cos(p[k][0]) * sin(p[k][1]);
                glNormal3f(nx, ny, nz);
                glVertex3f(cx + radius * nx, cy + radius * ny, cz + radius * nz);
            }
        }
    }
}

void compilePipeLists() {
    if (pipeFaceList == 0) {
        pipeFaceList = glGenLists(2);
        pipeRingList = pipeFaceList + 1;
    }

    //glass
    glNewList(pipeFaceList, GL_COMPILE);
    glBegin(GL_QUADS);
    for (int y = 0; y < N; y++) {
        for (int z = 0; z < N; z++) {
            for (int x = 0; x < N; x++) {
                if (!isOpen(x, y, z)) continue;

                float cx = x + 0.5f, cy = y + 0.5f, cz = z + 0.5f;
                bool openDir[6];
                int deg = 0;
                for (int d = 0; d < 6; d++) {
                    openDir[d] = isOpen(x + DX6[d], y + DY6[d], z + DZ6[d]);
                    if (openDir[d]) deg++;
                }

                bool straight = (deg == 2) &&
                    ((openDir[0] && openDir[1]) || (openDir[2] && openDir[3]) || (openDir[4] && openDir[5]));

                if (!straight) {
                    pipeColor(0.17f);
                    emitJointQuads(cx, cy, cz, PIPE_RADIUS, 16, 12, openDir);
                }

                pipeColor(0.15f);
                if (openDir[0]) emitPipeQuads(cx, cy, cz, 0, PIPE_RADIUS, 16);
                if (openDir[2]) emitPipeQuads(cx, cy, cz, 1, PIPE_RADIUS, 16);
                if (openDir[4]) emitPipeQuads(cx, cy, cz, 2, PIPE_RADIUS, 16);
            }
        }
    }
    glEnd();
    glEndList();

    //rings
    glNewList(pipeRingList, GL_COMPILE);
    glBegin(GL_LINES);
    for (int y = 0; y < N; y++) {
        for (int z = 0; z < N; z++) {
            for (int x = 0; x < N; x++) {
                if (!isOpen(x, y, z)) continue;

                bool openDir[6];
                int deg = 0;
                for (int d = 0; d < 6; d++) {
                    openDir[d] = isOpen(x + DX6[d], y + DY6[d], z + DZ6[d]);
                    if (openDir[d]) deg++;
                }

                bool straight = (deg == 2) &&
                    ((openDir[0] && openDir[1]) || (openDir[2] && openDir[3]) || (openDir[4] && openDir[5]));
                if (straight) continue;

                float cx = x + 0.5f, cy = y + 0.5f, cz = z + 0.5f;
                glColor4f(0.0f, 0.0f, 0.0f, 0.75f);
                for (int axis = 0; axis < 3; axis++) {
                    if (openDir[2 * axis] || openDir[2 * axis + 1])
                        emitPipeRing(cx, cy, cz, axis, PIPE_RADIUS, 20);
                }
            }
        }
    }
    glEnd();
    glEndList();
}

//sphere
void drawParametricSphere(float cx, float cy, float cz, float radius, int slices, int stacks) {
    glBegin(GL_QUADS);
    for (int i = 0; i < stacks; i++) {
        float phi0 = PI * (float)i / (float)stacks - PI / 2.0f;
        float phi1 = PI * (float)(i + 1) / (float)stacks - PI / 2.0f;
        for (int j = 0; j < slices; j++) {
            float th0 = 2.0f * PI * (float)j / (float)slices;
            float th1 = 2.0f * PI * (float)(j + 1) / (float)slices;

            float p[4][2] = {{phi0, th0}, {phi0, th1}, {phi1, th1}, {phi1, th0}};
            for (int k = 0; k < 4; k++) {
                float nx = cos(p[k][0]) * cos(p[k][1]);
                float ny = sin(p[k][0]);
                float nz = cos(p[k][0]) * sin(p[k][1]);
                glNormal3f(nx, ny, nz);
                glVertex3f(cx + radius * nx, cy + radius * ny, cz + radius * nz);
            }
        }
    }
    glEnd();
}

//level
void resetView() {
    mazeAngleX = mazeAngleY = mazeAngleZ = 0.0f;
    camYaw = 0.45f;
    camPitch = 0.35f;
    camDist = N * 2.4f;
}

void placeBallAtStart() {
    ballX = startX + 0.5f;
    ballY = startY + 0.5f;
    ballZ = startZ + 0.5f;
    trailCount = 0;
    trailHead = 0;
    trapFrames = 0;
    gameState = STATE_PLAYING;
}

void buildLevel() {
    applyLevelSettings();
    N = 2 * mazeNodes + 1;
    generateMaze();
    compilePipeLists();
    placeBallAtStart();
    resetView();
}

void retryLevel() {
    placeBallAtStart();
}

void startNewGame(int rank) {
    startRank = rank;
    levelNumber = 1;
    buildLevel();
}

void advanceLevel() {
    levelNumber++;
    buildLevel();
}

//collision
bool checkWallCollision(float px, float py, float pz, float r) {
    float mn[3] = {px - r, py - r, pz - r};
    float mx[3] = {px + r, py + r, pz + r};
    int lo[3], hi[3];

    for (int i = 0; i < 3; i++) {
        lo[i] = (int)floor(mn[i]);
        hi[i] = (int)floor(mx[i]);
        if (lo[i] < 0 || hi[i] >= N) return true;
    }

    for (int cy = lo[1]; cy <= hi[1]; cy++)
        for (int cz = lo[2]; cz <= hi[2]; cz++)
            for (int cx = lo[0]; cx <= hi[0]; cx++)
                if (maze[cy][cz][cx] == WALL)
                    if (mn[0] < cx + 1 && mx[0] > cx &&
                        mn[1] < cy + 1 && mx[1] > cy &&
                        mn[2] < cz + 1 && mx[2] > cz)
                        return true;
    return false;
}

void checkTrapCollision() {
    for (int y = 0; y < N; y++)
        for (int z = 0; z < N; z++)
            for (int x = 0; x < N; x++)
                if (maze[y][z][x] == TRAP) {
                    float dx = ballX - (x + 0.5f);
                    float dy = ballY - (y + 0.5f);
                    float dz = ballZ - (z + 0.5f);
                    if (dx * dx + dy * dy + dz * dz < TRAP_HIT * TRAP_HIT) {
                        gameState = STATE_TRAPPED;
                        trapFrames = 0;
                        return;
                    }
                }
}

void checkExitCollision() {
    int cx = (int)floor(ballX);
    int cy = (int)floor(ballY);
    int cz = (int)floor(ballZ);
    if (inBounds(cx, cy, cz) && maze[cy][cz][cx] == EXIT) advanceLevel();
}

//move
void pushTrail() {
    if (trailCount > 0) {
        int last = (trailHead + TRAIL_LEN - 1) % TRAIL_LEN;
        float dx = ballX - trailPos[last][0];
        float dy = ballY - trailPos[last][1];
        float dz = ballZ - trailPos[last][2];
        if (dx * dx + dy * dy + dz * dz < 0.04f * 0.04f) return;
    }
    trailPos[trailHead][0] = ballX;
    trailPos[trailHead][1] = ballY;
    trailPos[trailHead][2] = ballZ;
    trailHead = (trailHead + 1) % TRAIL_LEN;
    if (trailCount < TRAIL_LEN) trailCount++;
}

void updateMovement() {
    if (gameState == STATE_TRAPPED) {
        trapFrames++;
        if (trapFrames >= TRAP_RETRY_FRAMES) retryLevel();
        return;
    }

    float mv[3] = {0.0f, 0.0f, 0.0f};
    if (keyDown['d']) mv[0] += 1.0f;
    if (keyDown['a']) mv[0] -= 1.0f;
    if (keyDown['e']) mv[1] += 1.0f;
    if (keyDown['q']) mv[1] -= 1.0f;
    if (keyDown['s']) mv[2] += 1.0f;
    if (keyDown['w']) mv[2] -= 1.0f;

    float pos[3] = {ballX, ballY, ballZ};

    for (int i = 0; i < 3; i++) {
        if (mv[i] != 0.0f) {
            float np[3] = {pos[0], pos[1], pos[2]};
            np[i] += mv[i] * moveSpeed;
            if (!checkWallCollision(np[0], np[1], np[2], BALL_RADIUS)) pos[i] = np[i];
        } else {
            //center
            float centre = floor(pos[i]) + 0.5f;
            float diff = centre - pos[i];
            float np[3] = {pos[0], pos[1], pos[2]};
            np[i] = (fabs(diff) < 0.003f) ? centre : pos[i] + diff * 0.22f;
            if (!checkWallCollision(np[0], np[1], np[2], BALL_RADIUS)) pos[i] = np[i];
        }
    }

    ballX = pos[0];
    ballY = pos[1];
    ballZ = pos[2];

    pushTrail();
    checkTrapCollision();
    if (gameState == STATE_PLAYING) checkExitCollision();
}

//axes
void drawAxisGizmo() {
    glDisable(GL_LIGHTING);
    glLineWidth(2.5f);
    glBegin(GL_LINES);
    glColor3f(0.90f, 0.15f, 0.15f); glVertex3f(0, 0, 0); glVertex3f(2, 0, 0);
    glColor3f(0.10f, 0.65f, 0.20f); glVertex3f(0, 0, 0); glVertex3f(0, 2, 0);
    glColor3f(0.15f, 0.35f, 0.90f); glVertex3f(0, 0, 0); glVertex3f(0, 0, 2);
    glEnd();
    glLineWidth(1.0f);

    //labels
    glColor3f(0.90f, 0.15f, 0.15f); glRasterPos3f(2.15f, 0.0f, 0.0f); glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, 'X');
    glColor3f(0.10f, 0.65f, 0.20f); glRasterPos3f(0.0f, 2.15f, 0.0f); glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, 'Y');
    glColor3f(0.15f, 0.35f, 0.90f); glRasterPos3f(0.0f, 0.0f, 2.15f); glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, 'Z');
}

//display
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //camera
    float ex = camDist * cos(camPitch) * sin(camYaw);
    float ey = camDist * sin(camPitch);
    float ez = camDist * cos(camPitch) * cos(camYaw);
    gluLookAt(ex, ey, ez, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    //light
    GLfloat lightPos[4] = {0.5f, 1.0f, 0.8f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    //rotate
    glPushMatrix();
    glRotatef(mazeAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(mazeAngleY, 0.0f, 1.0f, 0.0f);
    glRotatef(mazeAngleZ, 0.0f, 0.0f, 1.0f);
    glTranslatef(-N / 2.0f, -N / 2.0f, -N / 2.0f);

    drawAxisGizmo();

    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);

    //ball
    glColor3f(1.0f, 0.78f, 0.05f);
    drawParametricSphere(ballX, ballY, ballZ, BALL_RADIUS, 24, 16);

    //trap exit
    for (int y = 0; y < N; y++) {
        for (int z = 0; z < N; z++) {
            for (int x = 0; x < N; x++) {
                if (maze[y][z][x] == TRAP) {
                    float pulse = 0.5f + 0.5f * sin(t * 3.0f + x + y + z);
                    GLfloat em[4] = {0.35f + 0.35f * pulse, 0.02f, 0.02f, 1.0f};
                    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
                    glColor3f(0.90f, 0.10f, 0.10f);
                    drawParametricSphere(x + 0.5f, y + 0.5f, z + 0.5f, TRAP_RADIUS, 16, 12);
                } else if (maze[y][z][x] == EXIT) {
                    float s = 0.20f + 0.04f * sin(t * 4.0f);
                    GLfloat em[4] = {0.05f, 0.50f, 0.18f, 1.0f};
                    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
                    glColor3f(0.10f, 0.90f, 0.35f);
                    drawParametricSphere(x + 0.5f, y + 0.5f, z + 0.5f, s + 0.06f, 20, 14);
                }
            }
        }
    }
    GLfloat noEm[4] = {0, 0, 0, 1};
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, noEm);

    //glass
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glCallList(pipeFaceList);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

    //rings
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glLineWidth(1.5f);
    glCallList(pipeRingList);
    glLineWidth(1.0f);

    //glow
    glColor4f(1.0f, 0.70f, 0.10f, 0.25f);
    drawParametricSphere(ballX, ballY, ballZ, BALL_RADIUS * 1.8f, 16, 12);

    for (int y = 0; y < N; y++) {
        for (int z = 0; z < N; z++) {
            for (int x = 0; x < N; x++) {
                if (maze[y][z][x] == TRAP) {
                    glColor4f(1.0f, 0.10f, 0.10f, 0.16f);
                    drawParametricSphere(x + 0.5f, y + 0.5f, z + 0.5f, TRAP_RADIUS * 1.6f, 14, 10);
                } else if (maze[y][z][x] == EXIT) {
                    float pulse = 0.5f + 0.5f * sin(t * 4.0f);
                    glColor4f(0.10f, 0.90f, 0.35f, 0.18f + 0.12f * pulse);
                    drawParametricSphere(x + 0.5f, y + 0.5f, z + 0.5f, 0.42f + 0.08f * pulse, 16, 12);
                }
            }
        }
    }

    //trail
    glEnable(GL_POINT_SMOOTH);
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < trailCount; i++) {
        int idx = (trailHead - 1 - i + TRAIL_LEN * 2) % TRAIL_LEN;
        float a = 0.70f * (1.0f - (float)i / (float)TRAIL_LEN);
        glColor4f(1.0f, 0.55f, 0.05f, a);
        glVertex3f(trailPos[idx][0], trailPos[idx][1], trailPos[idx][2]);
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    glPopMatrix();

    glutSwapBuffers();
}

//timer
void timer(int value) {
    //rotate
    if (keyDown['i']) mazeAngleX += ROTATE_SPEED;
    if (keyDown['k']) mazeAngleX -= ROTATE_SPEED;
    if (keyDown['j']) mazeAngleY += ROTATE_SPEED;
    if (keyDown['l']) mazeAngleY -= ROTATE_SPEED;
    if (keyDown['u']) mazeAngleZ += ROTATE_SPEED;
    if (keyDown['o']) mazeAngleZ -= ROTATE_SPEED;

    //zoom
    if (keyDown['+'] || keyDown['=']) camDist *= 0.985f;
    if (keyDown['-'] || keyDown['_']) camDist *= 1.015f;
    if (specialDown[GLUT_KEY_PAGE_UP])   camDist *= 0.985f;
    if (specialDown[GLUT_KEY_PAGE_DOWN]) camDist *= 1.015f;
    if (camDist < 6.0f)  camDist = 6.0f;
    if (camDist > 90.0f) camDist = 90.0f;

    //orbit
    if (specialDown[GLUT_KEY_LEFT])  camYaw   -= 0.03f;
    if (specialDown[GLUT_KEY_RIGHT]) camYaw   += 0.03f;
    if (specialDown[GLUT_KEY_UP])    camPitch += 0.025f;
    if (specialDown[GLUT_KEY_DOWN])  camPitch -= 0.025f;
    if (camPitch > 1.5f)  camPitch = 1.5f;
    if (camPitch < -1.5f) camPitch = -1.5f;

    updateMovement();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

//input
void keyboard(unsigned char key, int x, int y) {
    unsigned char k = (unsigned char)tolower(key);
    keyDown[k] = true;

    switch (k) {
        case '1': startNewGame(0); break;
        case '2': startNewGame(4); break;
        case '3': startNewGame(8); break;

        case 'n': buildLevel();  break;
        case 'r': retryLevel();  break;
        case 't': resetView();   break;

        case 27: exit(0); break;
        default: break;
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    unsigned char k = (unsigned char)tolower(key);
    keyDown[k] = false;
    if (k == '+' || k == '=') { keyDown['+'] = false; keyDown['='] = false; }
    if (k == '-' || k == '_') { keyDown['-'] = false; keyDown['_'] = false; }
}

void specialKeys(int key, int x, int y) {
    if (key >= 0 && key < 256) specialDown[key] = true;
}

void specialKeysUp(int key, int x, int y) {
    if (key >= 0 && key < 256) specialDown[key] = false;
}

void reshape(int width, int height) {
    if (height == 0) height = 1;
    winW = width;
    winH = height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)width / (double)height, 0.5, 300.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

//init
void initGL() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LINE_SMOOTH);

    glEnable(GL_LIGHT0);
    GLfloat amb[4]  = {0.35f, 0.35f, 0.38f, 1.0f};
    GLfloat diff[4] = {0.95f, 0.95f, 1.00f, 1.0f};
    GLfloat spec[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    GLfloat matSpec[4] = {0.9f, 0.9f, 0.9f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 70.0f);
}

int main(int argc, char** argv) {
    FreeConsole();

    srand((unsigned int)time(NULL));
    for (int i = 0; i < 256; i++) { keyDown[i] = false; specialDown[i] = false; }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("");

    glutIgnoreKeyRepeat(1);

    initGL();
    startNewGame(0);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeys);
    glutSpecialUpFunc(specialKeysUp);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}
