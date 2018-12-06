//========= Copyright Valve Corporation ============//
#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1

#include <stdio.h>
#include <cstdlib>
#include <string>
#include "glad.h"

#include <OpenVR/openvr.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <glm.hpp>

#include <gtc/type_ptr.hpp>
#include "bounding_box.h"
#include "camera.h"
#include "map.h"
#include "map_loader.h"
#include "model_manager.h"
#include "player.h"
#include "shader_manager.h"
#include "texture_manager.h"
const char *INSTRUCTIONS =
    "***************\n"
    "This is a game made by Jackson Kruger for CSCI 5607 at the University of Minnesota.\n"
    "\n"
    "Controls:\n"
    "WASD - Player movement\n"
    "Space - Player jump\n"
    "Left ctrl - Player crouch\n"
    "g - Drop key\n"
    "Esc - Quit\n"
    "F11 - Fullscreen\n"
    "***************\n";

const char *USAGE =
    "Usage:\n"
    "-w \'width\'x\'height\'\n"
    "   Example: -m 800x600\n"
    "-m map\n"
    "   This map must be in the root of the directory the game's being run from.\n"
    "   Example: -m map1.txt\n";

#if defined(POSIX)
#include "unistd.h"
#endif

#ifndef _WIN32
#define APIENTRY
#endif

#ifndef _countof
#define _countof(x) (sizeof(x) / sizeof((x)[0]))
#endif

void ThreadSleep(unsigned long nMilliseconds) {
#if defined(_WIN32)
    Sleep(nMilliseconds);
#elif defined(POSIX)
    usleep(nMilliseconds * 1000);
#endif
}

static bool g_bPrintf = true;
using glm::mat4;
using glm::vec2;
using glm::vec3;

//-----------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
class CMainApplication {
   public:
    CMainApplication(int argc, char *argv[]);
    virtual ~CMainApplication();

    bool BInit();
    bool BInitGL();
    bool BInitCompositor();

    void Shutdown();

    void RunMainLoop();
    void ProcessVREvent(const vr::VREvent_t &event);
    void RenderFrame();

    void SetupScene();

    bool SetupStereoRenderTargets();
    void SetupCompanionWindow();
    void SetupCameras();

    void RenderStereoTargets();
    void RenderCompanionWindow();
    void RenderScene(vr::Hmd_Eye nEye);

    mat4 GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye);
    mat4 GetHMDMatrixPoseEye(vr::Hmd_Eye nEye);
    mat4 GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye);
    void UpdateHMDMatrixPose();

    mat4 ConvertSteamVRMatrixToMat4(const vr::HmdMatrix34_t &matPose);

   private:
    MapLoader map_loader;
    Map *map;
    Camera camera;
    Player *player;

    vr::IVRSystem *m_pHMD;
    std::string m_strDriver;
    std::string m_strDisplay;
    vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
    mat4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];

    struct ControllerInfo_t {
        vr::VRInputValueHandle_t m_source = vr::k_ulInvalidInputValueHandle;
        vr::VRActionHandle_t m_actionPose = vr::k_ulInvalidActionHandle;
        vr::VRActionHandle_t m_actionHaptic = vr::k_ulInvalidActionHandle;
        mat4 m_rmat4Pose;
        std::string m_sRenderModelName;
        bool m_bShowController;
    };

    enum EHand {
        Left = 0,
        Right = 1,
    };
    ControllerInfo_t m_rHand[2];

   private:  // SDL bookkeeping
    SDL_Window *m_pCompanionWindow;
    uint32_t m_nCompanionWindowWidth;
    uint32_t m_nCompanionWindowHeight;

    SDL_GLContext m_pContext;

   private:  // OpenGL bookkeeping
    int m_iValidPoseCount;
    vec2 m_vAnalogValue;

    std::string m_strPoseClasses;                         // what classes we saw poses for this frame
    char m_rDevClassChar[vr::k_unMaxTrackedDeviceCount];  // for each device, a character representing its class

    int m_iSceneVolumeWidth;
    int m_iSceneVolumeHeight;
    int m_iSceneVolumeDepth;
    float m_fScaleSpacing;
    float m_fScale;

    int m_iSceneVolumeInit;  // if you want something other than the default 20x20x20

    float m_fNearClip;
    float m_fFarClip;

    GLuint m_iTexture;

    unsigned int m_uiVertcount;

    GLuint m_unSceneVAO;
    GLuint m_unCompanionWindowVAO;
    GLuint m_glCompanionWindowIDVertBuffer;
    GLuint m_glCompanionWindowIDIndexBuffer;
    unsigned int m_uiCompanionWindowIndexSize;

    mat4 m_mat4HMDPose;
    mat4 m_mat4eyePosLeft;
    mat4 m_mat4eyePosRight;

    mat4 m_mat4ProjectionCenter;
    mat4 m_mat4ProjectionLeft;
    mat4 m_mat4ProjectionRight;

    struct VertexDataScene {
        vec3 position;
        vec2 texCoord;
    };

    struct VertexDataWindow {
        vec2 position;
        vec2 texCoord;

        VertexDataWindow(const vec2 &pos, const vec2 tex) : position(pos), texCoord(tex) {}
    };

    GLint m_nSceneMatrixLocation;

    struct FramebufferDesc {
        GLuint m_nDepthBufferId;
        GLuint m_nRenderTextureId;
        GLuint m_nRenderFramebufferId;
        GLuint m_nResolveTextureId;
        GLuint m_nResolveFramebufferId;
    };
    FramebufferDesc leftEyeDesc;
    FramebufferDesc rightEyeDesc;

    bool CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc &framebufferDesc);

    uint32_t m_nRenderWidth;
    uint32_t m_nRenderHeight;
};

//---------------------------------------------------------------------------------------------------------------------
// Purpose: Returns true if the action is active and had a rising edge
//---------------------------------------------------------------------------------------------------------------------
bool GetDigitalActionRisingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t *pDevicePath = nullptr) {
    vr::InputDigitalActionData_t actionData;
    vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
    if (pDevicePath) {
        *pDevicePath = vr::k_ulInvalidInputValueHandle;
        if (actionData.bActive) {
            vr::InputOriginInfo_t originInfo;
            if (vr::VRInputError_None ==
                vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo))) {
                *pDevicePath = originInfo.devicePath;
            }
        }
    }
    return actionData.bActive && actionData.bChanged && actionData.bState;
}

//---------------------------------------------------------------------------------------------------------------------
// Purpose: Returns true if the action is active and had a falling edge
//---------------------------------------------------------------------------------------------------------------------
bool GetDigitalActionFallingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t *pDevicePath = nullptr) {
    vr::InputDigitalActionData_t actionData;
    vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
    if (pDevicePath) {
        *pDevicePath = vr::k_ulInvalidInputValueHandle;
        if (actionData.bActive) {
            vr::InputOriginInfo_t originInfo;
            if (vr::VRInputError_None ==
                vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo))) {
                *pDevicePath = originInfo.devicePath;
            }
        }
    }
    return actionData.bActive && actionData.bChanged && !actionData.bState;
}

//---------------------------------------------------------------------------------------------------------------------
// Purpose: Returns true if the action is active and its state is true
//---------------------------------------------------------------------------------------------------------------------
bool GetDigitalActionState(vr::VRActionHandle_t action, vr::VRInputValueHandle_t *pDevicePath = nullptr) {
    vr::InputDigitalActionData_t actionData;
    vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
    if (pDevicePath) {
        *pDevicePath = vr::k_ulInvalidInputValueHandle;
        if (actionData.bActive) {
            vr::InputOriginInfo_t originInfo;
            if (vr::VRInputError_None ==
                vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo))) {
                *pDevicePath = originInfo.devicePath;
            }
        }
    }
    return actionData.bActive && actionData.bState;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CMainApplication::CMainApplication(int argc, char *argv[])
    : m_pCompanionWindow(NULL),
      m_pContext(NULL),
      m_nCompanionWindowWidth(1280),
      m_nCompanionWindowHeight(640),
      m_pHMD(NULL),
      m_unSceneVAO(0),
      m_nSceneMatrixLocation(-1),
      m_iValidPoseCount(0),
      m_iSceneVolumeInit(20),
      m_strPoseClasses("") {
    // other initialization tasks are done in BInit
    memset(m_rDevClassChar, 0, sizeof(m_rDevClassChar));
};

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CMainApplication::~CMainApplication() {}

//-----------------------------------------------------------------------------
// Purpose: Helper to get a string from a tracked device property and turn it
//			into a std::string
//-----------------------------------------------------------------------------
std::string GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop,
                                   vr::TrackedPropertyError *peError = NULL) {
    uint32_t unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
    if (unRequiredBufferLen == 0) return "";

    char *pchBuffer = new char[unRequiredBufferLen];
    unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
    std::string sResult = pchBuffer;
    delete[] pchBuffer;
    return sResult;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::BInit() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        printf("%s - SDL could not initialize! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
        return false;
    }

    // Loading the SteamVR Runtime
    vr::EVRInitError eError = vr::VRInitError_None;
    m_pHMD = VR_Init(&eError, vr::VRApplication_Scene);

    if (eError != vr::VRInitError_None) {
        m_pHMD = NULL;
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "Unable to init VR runtime: %s", VR_GetVRInitErrorAsEnglishDescription(eError));
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "VR_Init Failed", buf, NULL);
        return false;
    }

    int nWindowPosX = 100;
    int nWindowPosY = 100;
    Uint32 unWindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    // SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);

    m_pCompanionWindow =
        SDL_CreateWindow("hellovr", nWindowPosX, nWindowPosY, m_nCompanionWindowWidth, m_nCompanionWindowHeight, unWindowFlags);
    if (m_pCompanionWindow == NULL) {
        printf("%s - Window could not be created! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
        return false;
    }

    m_pContext = SDL_GL_CreateContext(m_pCompanionWindow);
    if (m_pContext == NULL) {
        printf("%s - OpenGL context could not be created! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
        return false;
    }

    if (gladLoadGLLoader(SDL_GL_GetProcAddress)) {
        printf("\nOpenGL loaded\n");
        printf("Vendor:   %s\n", glGetString(GL_VENDOR));
        printf("Renderer: %s\n", glGetString(GL_RENDERER));
        printf("Version:  %s\n\n", glGetString(GL_VERSION));
    } else {
        printf("ERROR: Failed to initialize OpenGL context.\n");
        return -1;
    }

    SDL_SetWindowTitle(m_pCompanionWindow, "MazeGameVR");

    // cube array
    m_iSceneVolumeWidth = m_iSceneVolumeInit;
    m_iSceneVolumeHeight = m_iSceneVolumeInit;
    m_iSceneVolumeDepth = m_iSceneVolumeInit;

    m_fScale = 0.3f;
    m_fScaleSpacing = 4.0f;

    m_fNearClip = 0.1f;
    m_fFarClip = 500.0f;

    m_iTexture = 0;
    m_uiVertcount = 0;

    if (!BInitGL()) {
        printf("%s - Unable to initialize OpenGL!\n", __FUNCTION__);
        return false;
    }

    if (!BInitCompositor()) {
        printf("%s - Failed to initialize VR Compositor!\n", __FUNCTION__);
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize OpenGL. Returns true if OpenGL has been successfully
//          initialized, false if shaders could not be created.
//          If failure occurred in a module other than shaders, the function
//          may return true or throw an error.
//-----------------------------------------------------------------------------
bool CMainApplication::BInitGL() {
    SetupScene();
    SetupCameras();
    SetupStereoRenderTargets();
    SetupCompanionWindow();

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize Compositor. Returns true if the compositor was
//          successfully initialized, false otherwise.
//-----------------------------------------------------------------------------
bool CMainApplication::BInitCompositor() {
    vr::EVRInitError peError = vr::VRInitError_None;

    if (!vr::VRCompositor()) {
        printf("Compositor initialization failed. See log file for details\n");
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::Shutdown() {
    if (m_pHMD) {
        vr::VR_Shutdown();
        m_pHMD = NULL;
    }

    if (m_pCompanionWindow) {
        SDL_DestroyWindow(m_pCompanionWindow);
        m_pCompanionWindow = NULL;
    }

    SDL_Quit();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RunMainLoop() {
    bool quit = false;

    SDL_StartTextInput();
    SDL_ShowCursor(SDL_DISABLE);

    SDL_Event windowEvent;
    while (!quit) {
        while (SDL_PollEvent(&windowEvent)) {  // inspect all events in the queue
            if (windowEvent.type == SDL_QUIT) quit = true;
            // List of keycodes: https://wiki.libsdl.org/SDL_Keycode - You can catch many special keys
            // Scancode refers to a keyboard position, keycode refers to the letter (e.g., EU keyboards)
            if (windowEvent.type == SDL_KEYUP) {  // Exit event loop
                if (windowEvent.key.keysym.sym == SDLK_ESCAPE) {
                    quit = true;
                }
            }

            if (windowEvent.type == SDL_MOUSEMOTION && SDL_GetRelativeMouseMode() == SDL_TRUE) {
                // printf("Mouse movement (xrel, yrel): (%i, %i)\n", windowEvent.motion.xrel, windowEvent.motion.yrel);
                float factor = 0.002f;
                camera.Rotate(0, -windowEvent.motion.xrel * factor);
            }

            switch (windowEvent.window.event) {
                case SDL_WINDOWEVENT_FOCUS_LOST:
                    SDL_Log("Window focus lost");
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                    break;
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    SDL_Log("Window focus gained");
                    SDL_SetRelativeMouseMode(SDL_TRUE);
                    break;
            }
        }
        RenderFrame();
    }

    SDL_StopTextInput();
}

//-----------------------------------------------------------------------------
// Purpose: Processes a single VR event
//-----------------------------------------------------------------------------
void CMainApplication::ProcessVREvent(const vr::VREvent_t &event) {
    switch (event.eventType) {
        case vr::VREvent_TrackedDeviceDeactivated: {
            printf("Device %u detached.\n", event.trackedDeviceIndex);
        } break;
        case vr::VREvent_TrackedDeviceUpdated: {
            printf("Device %u updated.\n", event.trackedDeviceIndex);
        } break;
    }
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RenderFrame() {
    // for now as fast as possible
    if (m_pHMD) {
        RenderStereoTargets();
        RenderCompanionWindow();

        vr::Texture_t leftEyeTexture = {(void *)(uintptr_t)leftEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma};
        vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
        vr::Texture_t rightEyeTexture = {(void *)(uintptr_t)rightEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma};
        vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
    }

    SDL_GL_SwapWindow(m_pCompanionWindow);

    UpdateHMDMatrixPose();
}

//-----------------------------------------------------------------------------
// Purpose: create a sea of cubes
//-----------------------------------------------------------------------------
void CMainApplication::SetupScene() {
    std::string map_file = "map2.txt";
    map = map_loader.LoadMap(map_file);
    player = new Player(&camera, map);
    map->Add(player);

    TextureManager::InitTextures();

    glGenVertexArrays(1, &m_unSceneVAO);  // Create a VAO
    glBindVertexArray(m_unSceneVAO);      // Bind the above created VAO to the current context

    ModelManager::InitVBO();

    ShaderManager::InitShaders();
    m_nSceneMatrixLocation = ShaderManager::Attributes.projection;

    TextureManager::InitTextures();

    glBindVertexArray(0);  // Unbind the VAO in case we want to create a new one

    glEnable(GL_DEPTH_TEST);

    printf("%s\n", INSTRUCTIONS);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::SetupCameras() {
    m_mat4ProjectionLeft = GetHMDMatrixProjectionEye(vr::Eye_Left);
    m_mat4ProjectionRight = GetHMDMatrixProjectionEye(vr::Eye_Right);
    m_mat4eyePosLeft = GetHMDMatrixPoseEye(vr::Eye_Left);
    m_mat4eyePosRight = GetHMDMatrixPoseEye(vr::Eye_Right);
}

//-----------------------------------------------------------------------------
// Purpose: Creates a frame buffer. Returns true if the buffer was set up.
//          Returns false if the setup failed.
//-----------------------------------------------------------------------------
bool CMainApplication::CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc &framebufferDesc) {
    glGenFramebuffers(1, &framebufferDesc.m_nRenderFramebufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nRenderFramebufferId);

    glGenRenderbuffers(1, &framebufferDesc.m_nDepthBufferId);
    glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, nWidth, nHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);

    glGenTextures(1, &framebufferDesc.m_nRenderTextureId);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, nWidth, nHeight, true);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId, 0);

    glGenFramebuffers(1, &framebufferDesc.m_nResolveFramebufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nResolveFramebufferId);

    glGenTextures(1, &framebufferDesc.m_nResolveTextureId);
    glBindTexture(GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId, 0);

    // check FBO status
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::SetupStereoRenderTargets() {
    if (!m_pHMD) return false;

    m_pHMD->GetRecommendedRenderTargetSize(&m_nRenderWidth, &m_nRenderHeight);

    CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, leftEyeDesc);
    CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, rightEyeDesc);

    return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::SetupCompanionWindow() {
    if (!m_pHMD) return;

    std::vector<VertexDataWindow> vVerts;

    // left eye verts
    vVerts.push_back(VertexDataWindow(vec2(-1, -1), vec2(0, 1)));
    vVerts.push_back(VertexDataWindow(vec2(0, -1), vec2(1, 1)));
    vVerts.push_back(VertexDataWindow(vec2(-1, 1), vec2(0, 0)));
    vVerts.push_back(VertexDataWindow(vec2(0, 1), vec2(1, 0)));

    // right eye verts
    vVerts.push_back(VertexDataWindow(vec2(0, -1), vec2(0, 1)));
    vVerts.push_back(VertexDataWindow(vec2(1, -1), vec2(1, 1)));
    vVerts.push_back(VertexDataWindow(vec2(0, 1), vec2(0, 0)));
    vVerts.push_back(VertexDataWindow(vec2(1, 1), vec2(1, 0)));

    GLushort vIndices[] = {0, 1, 3, 0, 3, 2, 4, 5, 7, 4, 7, 6};
    m_uiCompanionWindowIndexSize = _countof(vIndices);

    glGenVertexArrays(1, &m_unCompanionWindowVAO);
    glBindVertexArray(m_unCompanionWindowVAO);

    glGenBuffers(1, &m_glCompanionWindowIDVertBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_glCompanionWindowIDVertBuffer);
    glBufferData(GL_ARRAY_BUFFER, vVerts.size() * sizeof(VertexDataWindow), &vVerts[0], GL_STATIC_DRAW);

    glGenBuffers(1, &m_glCompanionWindowIDIndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glCompanionWindowIDIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_uiCompanionWindowIndexSize * sizeof(GLushort), &vIndices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void *)offsetof(VertexDataWindow, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void *)offsetof(VertexDataWindow, texCoord));

    glBindVertexArray(0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RenderStereoTargets() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_MULTISAMPLE);

    // Left Eye
    glBindFramebuffer(GL_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId);
    glViewport(0, 0, m_nRenderWidth, m_nRenderHeight);
    RenderScene(vr::Eye_Left);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDisable(GL_MULTISAMPLE);

    // Copy pixels from the renderframebuffer to the resolveframebuffer
    // I think this has something to do with MSAA?
    // Though disabling it causes odd z-buffering-related things to happen
    glBindFramebuffer(GL_READ_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, leftEyeDesc.m_nResolveFramebufferId);

    glBlitFramebuffer(0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    glEnable(GL_MULTISAMPLE);

    // Right Eye
    glBindFramebuffer(GL_FRAMEBUFFER, rightEyeDesc.m_nRenderFramebufferId);
    glViewport(0, 0, m_nRenderWidth, m_nRenderHeight);
    RenderScene(vr::Eye_Right);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDisable(GL_MULTISAMPLE);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, rightEyeDesc.m_nRenderFramebufferId);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rightEyeDesc.m_nResolveFramebufferId);

    glBlitFramebuffer(0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

//-----------------------------------------------------------------------------
// Purpose: Renders a scene with respect to nEye.
//-----------------------------------------------------------------------------
void CMainApplication::RenderScene(vr::Hmd_Eye nEye) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glUseProgram(ShaderManager::Textured_Shader);
    glUniformMatrix4fv(m_nSceneMatrixLocation, 1, GL_FALSE, glm::value_ptr(GetCurrentViewProjectionMatrix(nEye)));
    TextureManager::Update();
    player->Update();
    camera.Update();
    glBindVertexArray(m_unSceneVAO);
    map->UpdateAll();
    glBindVertexArray(0);

    glUseProgram(0);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RenderCompanionWindow() {
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, m_nCompanionWindowWidth, m_nCompanionWindowHeight);
    glActiveTexture(GL_TEXTURE0);  // Must reset this, as this is where the companion window shader expects these textures

    glBindVertexArray(m_unCompanionWindowVAO);
    glUseProgram(ShaderManager::CompanionWindow_Shader);

    // render left eye (first half of index array )
    glBindTexture(GL_TEXTURE_2D, leftEyeDesc.m_nResolveTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glDrawElements(GL_TRIANGLES, m_uiCompanionWindowIndexSize / 2, GL_UNSIGNED_SHORT, 0);

    // render right eye (second half of index array )
    glBindTexture(GL_TEXTURE_2D, rightEyeDesc.m_nResolveTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glDrawElements(GL_TRIANGLES, m_uiCompanionWindowIndexSize / 2, GL_UNSIGNED_SHORT,
                   (const void *)(uintptr_t)(m_uiCompanionWindowIndexSize));

    glBindVertexArray(0);
    glUseProgram(0);
}

//-----------------------------------------------------------------------------
// Purpose: Gets a Matrix Projection Eye with respect to nEye.
//-----------------------------------------------------------------------------
mat4 CMainApplication::GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye) {
    if (!m_pHMD) return mat4();

    vr::HmdMatrix44_t mat = m_pHMD->GetProjectionMatrix(nEye, m_fNearClip, m_fFarClip);

    return mat4(mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0], mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1], mat.m[0][2],
                mat.m[1][2], mat.m[2][2], mat.m[3][2], mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]);
}

//-----------------------------------------------------------------------------
// Purpose: Gets an HMDMatrixPoseEye with respect to nEye.
//-----------------------------------------------------------------------------
mat4 CMainApplication::GetHMDMatrixPoseEye(vr::Hmd_Eye nEye) {
    if (!m_pHMD) return mat4();

    vr::HmdMatrix34_t matEyeRight = m_pHMD->GetEyeToHeadTransform(nEye);
    mat4 matrixObj(matEyeRight.m[0][0], matEyeRight.m[1][0], matEyeRight.m[2][0], 0.0, matEyeRight.m[0][1], matEyeRight.m[1][1],
                   matEyeRight.m[2][1], 0.0, matEyeRight.m[0][2], matEyeRight.m[1][2], matEyeRight.m[2][2], 0.0, matEyeRight.m[0][3],
                   matEyeRight.m[1][3], matEyeRight.m[2][3], 1.0f);

    return glm::inverse(matrixObj);
}

//-----------------------------------------------------------------------------
// Purpose: Gets a Current View Projection Matrix with respect to nEye,
//          which may be an Eye_Left or an Eye_Right.
//-----------------------------------------------------------------------------
mat4 CMainApplication::GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye) {
    mat4 matMVP;
    if (nEye == vr::Eye_Left) {
        matMVP = m_mat4ProjectionLeft * m_mat4eyePosLeft;
    } else if (nEye == vr::Eye_Right) {
        matMVP = m_mat4ProjectionRight * m_mat4eyePosRight;
    }

    matMVP = matMVP * glm::rotate(glm::scale(m_mat4HMDPose, vec3(1)), -(float)(M_PI / 2.0f), vec3(1, 0, 0)) *
             glm::translate(player->transform->WorldTransform(), vec3(-2.5, -3.0, 0));

    return matMVP;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::UpdateHMDMatrixPose() {
    if (!m_pHMD) return;

    vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

    m_iValidPoseCount = 0;
    m_strPoseClasses = "";
    for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice) {
        if (m_rTrackedDevicePose[nDevice].bPoseIsValid) {
            m_iValidPoseCount++;
            m_rmat4DevicePose[nDevice] = ConvertSteamVRMatrixToMat4(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
            if (m_rDevClassChar[nDevice] == 0) {
                switch (m_pHMD->GetTrackedDeviceClass(nDevice)) {
                    case vr::TrackedDeviceClass_Controller:
                        m_rDevClassChar[nDevice] = 'C';
                        break;
                    case vr::TrackedDeviceClass_HMD:
                        m_rDevClassChar[nDevice] = 'H';
                        break;
                    case vr::TrackedDeviceClass_Invalid:
                        m_rDevClassChar[nDevice] = 'I';
                        break;
                    case vr::TrackedDeviceClass_GenericTracker:
                        m_rDevClassChar[nDevice] = 'G';
                        break;
                    case vr::TrackedDeviceClass_TrackingReference:
                        m_rDevClassChar[nDevice] = 'T';
                        break;
                    default:
                        m_rDevClassChar[nDevice] = '?';
                        break;
                }
            }
            m_strPoseClasses += m_rDevClassChar[nDevice];
        }
    }

    if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid) {
        m_mat4HMDPose = m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd];
        // printf("HMD Pose: %f, %f, %f\n", m_mat4HMDPose[3][0], m_mat4HMDPose[3][1], m_mat4HMDPose[3][2]);
        m_mat4HMDPose = glm::inverse(m_mat4HMDPose);
    }
}

//-----------------------------------------------------------------------------
// Purpose: Converts a SteamVR matrix to our local matrix class
//-----------------------------------------------------------------------------
mat4 CMainApplication::ConvertSteamVRMatrixToMat4(const vr::HmdMatrix34_t &matPose) {
    mat4 matrixObj(matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0, matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
                   matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0, matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f);
    return matrixObj;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    CMainApplication *pMainApplication = new CMainApplication(argc, argv);

    if (!pMainApplication->BInit()) {
        pMainApplication->Shutdown();
        return 1;
    }

    pMainApplication->RunMainLoop();

    pMainApplication->Shutdown();

    return 0;
}
