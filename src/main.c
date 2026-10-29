#include <GLFW/glfw3.h>
#include <GL/glu.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define WINDOW_WIDTH 1536
#define WINDOW_HEIGHT 960
#define MU 39.4784f
#define DT 0.00005f
#define DISPLAY_K 0.7f
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

static float yearsPerSec = 0.0f;
static float accumulator = 0.0f;

static float xRot = 0.0f;
static float yRot = 0.0f;

static int is_dragging = 0;
static double mouse_xpos = 0.0f;
static double mouse_ypos = 0.0f;

typedef struct {
    float au;
    float ecc;
    float drawRadius;
    float color[3];
    float startAngle;
    float x, z;
    float vx, vz;
} Planet;

typedef struct {
    float a, e, inc; 
    float mu;
    float visA, visK;
    float startAngle;
    float x, z, vx, vz;
} Moon;

static Moon moon = {
    .a = 0.0025696f, .e = 0.0549f, .inc = 5.145f,
    .mu = MU * 3.0404e-6f,
    .visA = 0.25f
};

Planet earth = {
    .au = 1.00f, .ecc = 0.017f, .drawRadius = 0.09f,
    .color = {0.2f, 0.4f, 1.0f}, .startAngle = 0.0f, .x = 0.0f,
    .z = 0.0f, .vx = 0.0f, .vz = 0.0f
};

static float camDist = -11.0f;

GLUquadricObj *pObj = NULL;

static void InitMoon(void)
{
    float rp = moon.a * (1.0f - moon.e);
    float vp = sqrtf(moon.mu / moon.a * (1.0f + moon.e) / (1.0f - moon.e));
    float th = 0.0f;

    moon.visK = moon.visA / moon.a;
    moon.startAngle = th;
    moon.x  = rp * cosf(th);
    moon.z  = rp * sinf(th);
    moon.vx = vp * sinf(th);
    moon.vz = -vp * cosf(th);
}

static void StepMoon(float dt)
{
    float r2 = moon.x * moon.x + moon.z * moon.z;
    float inv_r3 = 1.0f / (r2 * sqrtf(r2));
    moon.vx += -moon.mu * moon.x * inv_r3 * dt;
    moon.vz += -moon.mu * moon.z * inv_r3 * dt;
    moon.x  += moon.vx * dt;
    moon.z  += moon.vz * dt;
}

static void DrawMoonPath(void)
{
    float p = moon.a * (1.0f - moon.e * moon.e);
    glBegin(GL_LINE_LOOP);
    for (int j = 0; j < 128; j++)
    {
        float nu  = 2.0f * (float)M_PI * j / 128;
        float r   = p / (1.0f + moon.e * cosf(nu));
        float phi = moon.startAngle - nu;
        glVertex3f(r * cosf(phi) * moon.visK, 0.0f, r * sinf(phi) * moon.visK);
    }
    glEnd();
}
static void InitOrbit(void)
{
    float a = earth.au, e = earth.ecc;
    float rp = a * (1.0f - e);
    float vp = sqrtf(MU / a * (1.0f + e) / (1.0f - e));
    float th = 2.0f * 0.7f;
    earth.startAngle = th;

    earth.x = rp * cosf(th);
    earth.z = rp * sinf(th);
    earth.vx = vp * sinf(th);
    earth.vz = -vp * cosf(th);
}

static void StepOrbits(float dt)
{
    float r2 = earth.x * earth.x + earth.z * earth.z;
    float r = sqrtf(r2);
    float inv_r3 = 1.0f / (r * r * r);

    earth.vx += -MU * earth.x * inv_r3 * dt;
    earth.vz += -MU * earth.z * inv_r3 * dt;
    earth.x += earth.vx * dt;
    earth.z += earth.vz * dt;

    StepMoon(dt);
}

static void UpdateOrbits(float frameSeconds)
{
    if (frameSeconds > 0.1f)
        frameSeconds = 0.1f;

    accumulator += frameSeconds * yearsPerSec;
    while (accumulator >= DT)
    {
        StepOrbits(DT);
        accumulator -= DT;
    }
}

static void DrawOrbitPath()
{
    float a = earth.au, e = earth.ecc;
    float p = a * (1.0f - e * e);
    const int N = 128;
    
    glBegin(GL_LINE_LOOP);
    for (int j = 0; j < N; j++)
    {
        float nu = 2 * M_PI * j / N;
        float r = p / (1.0f + e * cosf(nu));
        float phi = earth.startAngle - nu;
        float s = DISPLAY_K / sqrtf(a);

        glVertex3f(r * cosf(phi) * s, 0.0f, r * sinf(phi) * s);
    }
    glEnd();
}

static void Render(float *sx, float *sz)
{
    float r = sqrtf(earth.x * earth.x + earth.z * earth.z);
    float s = DISPLAY_K / sqrtf(earth.au);
    *sx = earth.x * s;
    *sz = earth.z * s;
}

static void ChangeSize(GLFWwindow *window, int width, int height)
{
    (void)window;
    float fAspect;

    if (height == 0)
        height = 1;

    glViewport(0, 0, width, height);
    fAspect = (float)width / (float)height;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(20.0f, fAspect, 1.0, 400.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void SpecialKeys(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)window;
    (void)mods;

    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;

    if (key == GLFW_KEY_UP)
        xRot -= 5.0f;

    if (key == GLFW_KEY_DOWN)
        xRot += 5.0f;

    if (key == GLFW_KEY_LEFT)
        yRot -= 5.0f;

    if (key == GLFW_KEY_RIGHT)
        yRot += 5.0f;

    if (key == GLFW_KEY_M) {
        if (yearsPerSec > 1.0f)
            yearsPerSec = 1.0f;
        yearsPerSec += 0.1f;
    }
    xRot = (float)((const int)xRot % 360);
    yRot = (float)((const int)yRot % 360);
}

static void SpecialMouseScroll(GLFWwindow *window, double xoffset, double yoffset)
{
    (void)window;
    (void)xoffset;
    camDist += (float)yoffset * 0.8f;
    if (camDist > -2.0f)
        camDist = -2.0f;
    if (camDist < -50.0f)
        camDist = -50.0f;
}

static void SpecialMouseButton(GLFWwindow *window, int button, int action, int mods)
{
    (void)mods;
    if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            is_dragging = 1;
            glfwGetCursorPos(window, &mouse_xpos, &mouse_ypos);
        }
        else if (action == GLFW_RELEASE)
        {
            is_dragging = 0;
        }
    }
}

static void CursorPos(GLFWwindow *window, double xpos, double ypos)
{
    (void)window;

    if (!is_dragging)
        return;

    double dx = xpos - mouse_xpos;
    double dy = ypos - mouse_ypos;

    mouse_xpos = xpos;
    mouse_ypos = ypos;

    float sens = 0.5f;

    yRot += (float)dx * sens;
    xRot += (float)dy * sens;
}

void SetupRC()
{
    float whiteLight[] = { 0.05f, 0.05f, 0.05f, 1.0f };
    float sourceLight[] = { 0.25f, 0.25f, 0.25f, 1.0f };
    float lightPos[] = { -10.f, 5.0f, 5.0f, 1.0f };

    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);

    glEnable(GL_LIGHTING);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, whiteLight);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightfv(GL_LIGHT0, GL_AMBIENT, sourceLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sourceLight);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glEnable(GL_LIGHT0);

    glEnable(GL_COLOR_MATERIAL);

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    glClearColor(0.25f, 0.25f, 0.50f, 1.0f);

    pObj = gluNewQuadric();
    gluQuadricNormals(pObj, GLU_SMOOTH);
}

void RenderScene(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, camDist);
    glRotatef(xRot + 25.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(yRot, 0.0f, 1.0f, 0.0f);

    glColor3f(1.0f, 0.9f, 0.2f);
    gluSphere(pObj, 0.2f, 32, 16);
    
    glDisable(GL_LIGHTING);
    DrawOrbitPath();
    glEnable(GL_LIGHTING);
    float sx, sz;
    Render(&sx, &sz);

    glPushMatrix();
    glTranslatef(sx, 0.0f, sz);
    glColor3f(earth.color[0], earth.color[1], earth.color[2]);
    gluSphere(pObj, earth.drawRadius, 20, 10);

    glRotatef(moon.inc, 1.0f, 0.0f, 0.0f);
    glDisable(GL_LIGHTING);
    glColor3f(0.5f, 0.5f, 0.5f);
    DrawMoonPath();
    glEnable(GL_LIGHTING);

    glPushMatrix();
    glTranslatef(moon.x * moon.visK, 0.0f, moon.z * moon.visK);
    glColor3f(0.7f, 0.7f, 0.7f);
    gluSphere(pObj, 0.025f, 16, 8);

    glPopMatrix();
    glPopMatrix();
    glPopMatrix();
}

int main(void) {
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Model", NULL, NULL);

    if (!window)
    {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, ChangeSize);
    glfwSetKeyCallback(window, SpecialKeys);
    glfwSetMouseButtonCallback(window, SpecialMouseButton);
    glfwSetCursorPosCallback(window, CursorPos);
    glfwSetScrollCallback(window, SpecialMouseScroll);

    int fb_width, fb_height;
    glfwGetFramebufferSize(window, &fb_width, &fb_height);
    ChangeSize(window, fb_width, fb_height);

    SetupRC();
    InitOrbit();
    InitMoon();
    glfwSwapInterval(1);
    double last = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        double now = glfwGetTime();
        UpdateOrbits((float)(now - last));
        last = now;

        glfwPollEvents();
        RenderScene();
        glfwSwapBuffers(window);
    }
    gluDeleteQuadric(pObj);
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}

