#include <iostream>
#include <array>

#ifdef __WIN32__
#else
//#include <unistd.h>
#endif

#include "vkapp.h"
#include "app.h"
#include "extensions_vk.hpp"

extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

// GLFW Callback functions
static void onErrorCallback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

#ifdef GUI
void drawGUI(VkApp& VK)
{

    // Create a dockspace over the mainviewport so that we can dock stuff
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), 
        ImGuiDockNodeFlags_PassthruCentralNode // make the dockspace transparent
        | ImGuiDockNodeFlags_NoDockingInCentralNode // dont allow docking in the central area
    );

    // This needs a window if we want to dock it.
    ImGui::Begin("Debug");
    ImGui::Checkbox("Raytrace", &VK.useRaytracer);
    ImGui::SliderFloat("RussianRoulette", &VK.m_pcRay.rr,0.0f, 1.0f);
    ImGui::Text("Iterations %d", VK.currIterations);
    ImGui::Text("Rate %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::InputInt([=]() {
        if (VK.BRDF_var < 0) return "Phong";
        if (VK.BRDF_var > 0) return "Beckham";
        if (VK.BRDF_var == 0) return "GGX";
        }(), &VK.BRDF_var);
    ImGui::End();
}
#endif

//////////////////////////////////////////////////////////////////////////
static int const WIDTH  = 10*128;
static int const HEIGHT = 6*128;
static std::string PROJECT = "rtrt";

App* app;  // The app, declared here so static callback functions can find it.

//---------------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    app =  new App(argc, argv); // Constructs the glfw window and sets UI callbacks

    // TODO: TEMP pls remove
    app->doApiDump = false;
    app->m_show_gui = true;

    VkApp VK(app); // Creates and manages all things Vulkan.

    // The draw loop
    printf("looping =======================================\n");
    while(!glfwWindowShouldClose(app->GLFW_window)) {
        glfwPollEvents();
        app->updateCamera();
        
        #ifdef GUI
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if(app->m_show_gui)
            drawGUI(VK);       
        #endif

        VK.drawFrame();

#ifdef GUI
        // Now we need to call EndFrame and allow all the other windows to process and update
        ImGui::EndFrame();
        // Update and Render additional Platform Windows
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {            
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
#endif // GUI
    }

    // Cleanup

    VK.destroyAllVulkanResources();
    
    glfwDestroyWindow(app->GLFW_window);
    glfwTerminate();
}

void framebuffersize_cb(GLFWwindow* window, int w, int h)
{
    assert(false && "Not ready for window resize events.");
}

void scroll_cb(GLFWwindow* window, double x, double y)
{
    #ifdef GUI
    if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse)
        return;
    #endif

    int delta = y;
    if(delta != 0)
        app->myCamera.wheel(delta > 0 ? 1 : -1);
}

void mousebutton_cb(GLFWwindow* window, int button, int action, int mods)
{
    #ifdef GUI
    if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse)
        return;
    #endif

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    app->myCamera.setMousePosition(x, y);
}

void cursorpos_cb(GLFWwindow* window, double x, double y)
{
    #ifdef GUI
    if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse)
        return;
    #endif

    if (app->myCamera.lmb || app->myCamera.rmb || app->myCamera.mmb)
        app->myCamera.mouseMove(x, y);
}

void char_cb(GLFWwindow* window, unsigned int key)
{
    #ifdef GUI
    if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard)
        return;
    #endif
}

void key_cb(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    const bool pressed = action != GLFW_RELEASE;

    if (pressed && key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, 1);
}

static float lastTime = 0;

void App::updateCamera()
{
    float now = glfwGetTime();
    float dt = now-lastTime;
    lastTime = now;

    float dist = 0.7*dt;

    float rad = 3.14159/180;
  
    myCamera.lmb   = glfwGetMouseButton(GLFW_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        
    auto oldCam = myCamera.eye;
    if (glfwGetKey(GLFW_window, GLFW_KEY_W) == GLFW_PRESS)
        myCamera.eye += dist*glm::vec3(sin(myCamera.spin*rad), 0.0, -cos(myCamera.spin*rad));
    if (glfwGetKey(GLFW_window, GLFW_KEY_S) == GLFW_PRESS)
        myCamera.eye -= dist*glm::vec3(sin(myCamera.spin*rad), 0.0, -cos(myCamera.spin*rad));
    if (glfwGetKey(GLFW_window, GLFW_KEY_A) == GLFW_PRESS)
        myCamera.eye -= dist*glm::vec3(cos(myCamera.spin*rad), 0.0, sin(myCamera.spin*rad));
    if (glfwGetKey(GLFW_window, GLFW_KEY_D) == GLFW_PRESS)
        myCamera.eye += dist*glm::vec3(cos(myCamera.spin*rad), 0.0, sin(myCamera.spin*rad));
    if (glfwGetKey(GLFW_window, GLFW_KEY_SPACE) == GLFW_PRESS || glfwGetKey(GLFW_window, GLFW_KEY_E)== GLFW_PRESS )
        myCamera.eye += dist*glm::vec3(0,-1,0);
    if (glfwGetKey(GLFW_window, GLFW_KEY_C) == GLFW_PRESS || glfwGetKey(GLFW_window, GLFW_KEY_Q)== GLFW_PRESS)
        myCamera.eye += dist*glm::vec3(0,1,0);

    if (oldCam != myCamera.eye)
    {
        myCamera.moved = true;
    }

}

App::App(int argc, char** argv)
{
    doApiDump = false;
    m_show_gui = true;

    int argi = 1;
    while (argi<argc) {
        std::string arg = argv[argi++];
        if (arg == "-d")
            doApiDump = true;
        else {
            printf("Unknown argument: %s\n", arg.c_str());
            exit(-1); } }

    glfwSetErrorCallback(onErrorCallback);

    if(!glfwInit()) {
        printf("Could not initialize GLFW.");
        exit(1); }
  
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFW_window = glfwCreateWindow(WIDTH, HEIGHT, PROJECT.c_str(), nullptr, nullptr);

    if(!glfwVulkanSupported()) {
        printf("GLFW: Vulkan Not Supported\n");
        exit(1); }

    glfwSetWindowUserPointer(GLFW_window, this);
    glfwSetKeyCallback(GLFW_window, &key_cb);
    glfwSetCharCallback(GLFW_window, &char_cb);
    glfwSetCursorPosCallback(GLFW_window, &cursorpos_cb);
    glfwSetMouseButtonCallback(GLFW_window, &mousebutton_cb);
    glfwSetScrollCallback(GLFW_window, &scroll_cb);
    glfwSetFramebufferSizeCallback(GLFW_window, &framebuffersize_cb);
}
