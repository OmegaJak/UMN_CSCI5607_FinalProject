#pragma once

#include <OpenVR/openvr.h>
#include <SDL.h>
#include <glm.hpp>
#include "camera.h"
#include "map.h"
#include "map_loader.h"
#include "player.h"

class VRManager {
   public:
    VRManager(int argc, char *argv[]);
    virtual ~VRManager();

    bool Init();
    bool InitGL();
    bool InitCompositor();

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

    glm::mat4 GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye);
    glm::mat4 GetHMDMatrixPoseEye(vr::Hmd_Eye nEye);
    glm::mat4 GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye);
    void UpdateHMDMatrixPose();

    glm::mat4 ConvertSteamVRMatrixToMat4(const vr::HmdMatrix34_t &matPose);

   private:
    MapLoader map_loader;
    Map *map;
    Camera camera;
    Player *player;

    vr::IVRSystem *m_pHMD;
    std::string m_strDriver;
    std::string m_strDisplay;
    vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
    glm::mat4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];

    struct ControllerInfo_t {
        vr::VRInputValueHandle_t m_source = vr::k_ulInvalidInputValueHandle;
        vr::VRActionHandle_t m_actionPose = vr::k_ulInvalidActionHandle;
        vr::VRActionHandle_t m_actionHaptic = vr::k_ulInvalidActionHandle;
        glm::mat4 m_rmat4Pose;
        std::string m_sRenderModelName;
        bool m_bShowController;
    };

    enum EHand {
        Left = 0,
        Right = 1,
    };
    ControllerInfo_t m_rHand[2];

   private:
    // SDL bookkeeping //
    SDL_Window *m_pCompanionWindow;
    uint32_t m_nCompanionWindowWidth;
    uint32_t m_nCompanionWindowHeight;

    SDL_GLContext m_pContext;

    // OpenGL bookkeeping //
    int m_iValidPoseCount;
    glm::vec2 m_vAnalogValue;

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

    glm::mat4 m_mat4HMDPose;
    glm::mat4 m_mat4eyePosLeft;
    glm::mat4 m_mat4eyePosRight;

    glm::mat4 m_mat4ProjectionCenter;
    glm::mat4 m_mat4ProjectionLeft;
    glm::mat4 m_mat4ProjectionRight;

    struct VertexDataScene {
        glm::vec3 position;
        glm::vec2 texCoord;
    };

    struct VertexDataWindow {
        glm::vec2 position;
        glm::vec2 texCoord;

        VertexDataWindow(const glm::vec2 &pos, const glm::vec2 tex) : position(pos), texCoord(tex) {}
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
