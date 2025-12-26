#include <cstdio>
#include <array>

#include "utility.h"

#include <GLFW/glfw3.h>

#include <glm/ext.hpp> // for matrix calculation


// Merhabalar hocam simulasyonda istediginiz degiskenleri degistrebilirsiniz 
// Sizler icin bazi degistirmek isteyebileceginizi dusunduhumuz yerlere isaret koyduk
// Bunlari bulmak icin dosya icinde "abc" aratabilirsiniz.


void MouseMoveCallback(GLFWwindow* wnd, double x, double y){
    
    const float changeSpeed = 0.2f;    //abc  // This for the panning the camera.

    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));

    if (!state->leftButtonPressed) {
        state->lastX = x;
        state->lastY = y;
        return;
    }

    float xoffset = (float)(x - state->lastX) * changeSpeed;
    float yoffset = (float)(state->lastY - y) * changeSpeed;
    state->lastX = x;
    state->lastY = y;

    state->yaw   += xoffset;
    state->pitch += yoffset;

    if (state->pitch > 89.0f)  state->pitch = 89.0f;
    if (state->pitch < -89.0f) state->pitch = -89.0f;
}

void MouseButtonCallback(GLFWwindow* wnd, int button, int action, int mods){
    
    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            state->leftButtonPressed = true;
            glfwGetCursorPos(wnd, &state->lastX, &state->lastY);
        } else if (action == GLFW_RELEASE) {
            state->leftButtonPressed = false;
        }
    }
}

void MouseScrollCallback(GLFWwindow* wnd, double dx, double dy)
{
    
    const float changeSpeed = 1.0f; //abc  // This is for the zooming in/out
    const float minFOV = 5.0f;
    const float maxFOV = 120.0f;

    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));

    state->FOV -= (float)dy * changeSpeed;

    if (state->FOV < minFOV) state->FOV = minFOV;   
    if (state->FOV > maxFOV) state->FOV =  maxFOV; 
}

void FramebufferChangeCallback(GLFWwindow* wnd, int w, int h)
{
    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));
    state->width = w;
    state->height = h;
}

void KeyboardCallback(GLFWwindow* wnd, int key, int scancode, int action, int modifier)
{
    
    const float changeSpeed = 0.2f;  //abc Simulation time settings
    const float maxSimSpeed = 3.0f;     
    const float minSimSpeed = -3.0f; // Recall that the simulation can do backwards
    
    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));
    if(action != GLFW_PRESS) return;

    uint32_t oldMode = state->mode;
    
    if(key == GLFW_KEY_P) state->mode = (state->mode == 3) ? 0 : (state->mode + 1);
    if(key == GLFW_KEY_O) state->mode = (state->mode == 0) ? 3 : (state->mode - 1);

    if(key == GLFW_KEY_L) {
        state->SimSpeed += changeSpeed;
        if(state->SimSpeed > maxSimSpeed) state->SimSpeed = maxSimSpeed;
    }

    if(key == GLFW_KEY_K) {
        state->SimSpeed -= changeSpeed;
        if(state->SimSpeed < minSimSpeed) state->SimSpeed = minSimSpeed;
    }
}


glm::mat4 drawEarth(
    const GLState& state,
    const MeshGL& mesh,
    const TextureGL& texture,
    GLuint vShaderId,
    GLuint fShaderId,
    glm::mat4 view,
    glm::mat4 proj,
    float simTime)
{
    // --- Ayarlar ---
    static constexpr GLuint U_TRANSFORM_MODEL   = 0;
    static constexpr GLuint U_TRANSFORM_VIEW    = 1;
    static constexpr GLuint U_TRANSFORM_PROJ    = 2;
    static constexpr GLuint U_TRANSFORM_NORMAL  = 3;
    static constexpr GLuint U_MODE = 0;
    static constexpr GLuint T_ALBEDO = 0;

    const float earthSpinSpeed = 0.5f; 

    // --- 1. MATEMATİKSEL MODELLEME ---
    
    // Adım A: Pozisyon Belirleme
    // Dünya merkezde (0,0,0) olduğu için Identity Matrix ile başlıyoruz.
    glm::mat4 model = glm::mat4(1.0f);
    
    // NOT: Eğer ileride Dünya'yı hareket ettirmek istersen (Translate),
    // o işlemi BURADA yapmalısın.
    
    // --- KRİTİK NOKTA: MERKEZİ KAYDETMEK ---
    // Henüz dünyayı kendi etrafında döndürmedik (Spin atmadı).
    // Ay, Dünya'nın "dönüşünü" değil "konumunu" takip etmeli.
    // O yüzden bu saf halini saklıyoruz.
    glm::mat4 earthCenterMatrix = model;


    // Adım B: Görsel Dönüş (Spin) ve Ölçek
    // Bu işlemler sadece Dünya'nın grafiğini etkiler, konumunu değil.
    float angle = simTime * earthSpinSpeed;
    model = glm::rotate(model, angle, glm::vec3(0, 1, 0));
     


    // --- 2. ÇİZİM İŞLEMLERİ (Aynı) ---
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

    glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShaderId);
    glActiveShaderProgram(state.renderPipeline, vShaderId);
    glUniformMatrix4fv(U_TRANSFORM_MODEL, 1, false, glm::value_ptr(model));
    glUniformMatrix4fv(U_TRANSFORM_VIEW, 1, false, glm::value_ptr(view));
    glUniformMatrix4fv(U_TRANSFORM_PROJ, 1, false, glm::value_ptr(proj));
    glUniformMatrix3fv(U_TRANSFORM_NORMAL, 1, false, glm::value_ptr(normalMatrix));

    glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShaderId);
    glActiveShaderProgram(state.renderPipeline, fShaderId);
    glActiveTexture(GL_TEXTURE0 + T_ALBEDO);
    glBindTexture(GL_TEXTURE_2D, texture.textureId);
    glUniform1ui(U_MODE, state.mode);

    glBindVertexArray(mesh.vaoId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.iBufferId);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // --- SONUÇ: DÜNYANIN MERKEZİNİ DÖNDÜR ---
    // Ay fonksiyonu bunu "Parent" olarak kullanacak.
    return earthCenterMatrix; 
}

glm::mat4 drawMoon(
    const GLState& state,
    const MeshGL& mesh,
    const TextureGL& texture,
    GLuint vShaderId,
    GLuint fShaderId,
    glm::mat4 view,
    glm::mat4 proj,
    float simTime,
    glm::mat4 parentMatrix, 
    float orbitRadius,      
    float orbitSpeed,       
    float scaleSize)        
{
    
    const float spinSpeed = 1.0f; //abc //Not self spin, spin speed of orbiting around the center planet 

    static constexpr GLuint U_TRANSFORM_MODEL   = 0;
    static constexpr GLuint U_TRANSFORM_VIEW    = 1;
    static constexpr GLuint U_TRANSFORM_PROJ    = 2;
    static constexpr GLuint U_TRANSFORM_NORMAL  = 3;
    static constexpr GLuint U_MODE = 0;
    static constexpr GLuint T_ALBEDO = 0;

    glm::mat4 model = parentMatrix; 

    float orbitAngle = simTime * orbitSpeed;
    model = glm::rotate(model, orbitAngle, glm::vec3(0, 1, 0));
    model = glm::translate(model, glm::vec3(orbitRadius, 0, 0));

    glm::mat4 centerMatrix = model;

    float spinAngle = simTime * spinSpeed;
    model = glm::rotate(model, spinAngle, glm::vec3(0, 1, 0));

    model = glm::scale(model, glm::vec3(scaleSize));

    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

    glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShaderId);
    glActiveShaderProgram(state.renderPipeline, vShaderId);
    glUniformMatrix4fv(U_TRANSFORM_MODEL, 1, false, glm::value_ptr(model));
    glUniformMatrix4fv(U_TRANSFORM_VIEW, 1, false, glm::value_ptr(view));
    glUniformMatrix4fv(U_TRANSFORM_PROJ, 1, false, glm::value_ptr(proj));
    glUniformMatrix3fv(U_TRANSFORM_NORMAL, 1, false, glm::value_ptr(normalMatrix));

    glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShaderId);
    glActiveShaderProgram(state.renderPipeline, fShaderId);
    glActiveTexture(GL_TEXTURE0 + T_ALBEDO);
    glBindTexture(GL_TEXTURE_2D, texture.textureId);
    glUniform1ui(U_MODE, state.mode);

    glBindVertexArray(mesh.vaoId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.iBufferId);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return centerMatrix;
}


void drawBackground(//Not a perspective, instead it a ortho
    const GLState& state, 
    const MeshGL& mesh,
    const TextureGL& texture,
    GLuint vShaderId,
    GLuint fShaderId,
    glm::mat4 view,
    glm::mat4 proj){
    
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);     

    glm::mat4 skyView = glm::mat4(glm::mat3(view));

    // In sim, the zoom in/out didnt change the background so we added a const FOV
    glm::mat4 skyProj = glm::perspective(glm::radians(60.0f), (float)state.width / (float)state.height, 0.1f, 1000.0f);  

    glm::mat4 skyModel = glm::scale(glm::mat4(1.0f), glm::vec3(500.0f));

    glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShaderId);
    glActiveShaderProgram(state.renderPipeline, vShaderId);
    
    glUniformMatrix4fv(0, 1, false, glm::value_ptr(skyModel)); 
    glUniformMatrix4fv(1, 1, false, glm::value_ptr(skyView));  
    glUniformMatrix4fv(2, 1, false, glm::value_ptr(skyProj));    

    glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShaderId);
    glActiveShaderProgram(state.renderPipeline, fShaderId);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.textureId);

    glBindVertexArray(mesh.vaoId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.iBufferId);

    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}

int main(int argc, const char* argv[])
{
    GLState state = GLState("Planet Renderer", 1280, 720,
                            CallbackPointersGLFW());
    ShaderGL vShader = ShaderGL(ShaderGL::VERTEX, "shaders/generic.vert");
    ShaderGL fShader = ShaderGL(ShaderGL::FRAGMENT, "shaders/debug.frag");
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    //Objects
    MeshGL Earth = MeshGL("meshes/sphere_20k.obj");
    MeshGL Moon = MeshGL("meshes/sphere_5k.obj");
    MeshGL Jupiter = MeshGL("meshes/sphere_2k.obj");
    MeshGL Sky = MeshGL("meshes/sphere_80k.obj");

    //Textures
    TextureGL EarthTex = TextureGL("textures/2k_earth_daymap.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL MoonTex = TextureGL("textures/2k_moon.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL JupiterTex = TextureGL("textures/2k_jupiter.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL SkyTex = TextureGL("textures/8k_stars_milky_way.jpg", TextureGL::LINEAR, TextureGL::REPEAT);

    float CurrentSimTime = 0.0f;   
    float lastFrameTime = 0.0f;       

    // =============== //
    //   RENDER LOOP   //
    // =============== //

    while(!glfwWindowShouldClose(state.window))
    {

        glfwPollEvents();

        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        
        CurrentSimTime += deltaTime * state.SimSpeed;

        glViewport(0, 0, state.width, state.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4x4 proj;
        glm::mat4x4 view;

        proj = glm::perspective(glm::radians(state.FOV), (float)state.width / (float)state.height, 0.1f, 1000.0f);
         
        glm::vec3 dir; // panning calcutalions // it stays after changing the view mode
        dir.x = cos(glm::radians(state.yaw)) * cos(glm::radians(state.pitch));
        dir.y = sin(glm::radians(state.pitch));
        dir.z = sin(glm::radians(state.yaw)) * cos(glm::radians(state.pitch));
   
        if (state.mode == 0) { 
            state.pos = state.earthVec + dir * 6.0f; 
            view = glm::lookAt(state.pos, state.earthVec, state.up);
        }
        else if (state.mode == 1) { 
            state.pos = state.moonVec + dir * 2.0f;
            view = glm::lookAt(state.pos, state.moonVec, state.up);
        }
        else if (state.mode == 2) { 
            state.pos = state.jupiterVec + dir * 0.5f;
            view = glm::lookAt(state.pos, state.jupiterVec, state.up);
        }
        else if (state.mode == 3) {
            
            float cameraSpeed = 5.0f * deltaTime;
            
            glm::vec3 forward = glm::normalize(-dir); 
            glm::vec3 right = glm::normalize(glm::cross(forward, state.up));

            if (glfwGetKey(state.window, GLFW_KEY_W) == GLFW_PRESS) state.pos += forward * cameraSpeed;
            if (glfwGetKey(state.window, GLFW_KEY_S) == GLFW_PRESS) state.pos -= forward * cameraSpeed;
            if (glfwGetKey(state.window, GLFW_KEY_A) == GLFW_PRESS) state.pos -= right * cameraSpeed;
            if (glfwGetKey(state.window, GLFW_KEY_D) == GLFW_PRESS) state.pos += right * cameraSpeed;

            state.gaze = state.pos + forward;
            view = glm::lookAt(state.pos, state.gaze, state.up);
        }

        drawBackground(state, Sky, SkyTex, vShader.shaderId, fShader.shaderId, view, proj);

        glm::mat4 EarthPos = drawEarth(state, Earth, EarthTex, 
                                  vShader.shaderId, fShader.shaderId, 
                                  view, proj, 
                                  CurrentSimTime);
        
        glm::mat4 MoonPos = drawMoon(state, Moon, MoonTex, 
                             vShader.shaderId, fShader.shaderId, 
                             view, proj, CurrentSimTime, 
                             EarthPos, 
                             4.0f,     
                             0.8f,     
                             0.3f);   

        glm::mat4 JupiterPos = drawMoon(state, Jupiter, JupiterTex, 
                             vShader.shaderId, fShader.shaderId, 
                             view, proj, CurrentSimTime, 
                             MoonPos, 
                             1.5,     
                             2.5f,     
                             0.1f);
        
        glfwSwapBuffers(state.window);

        state.earthVec   = glm::vec3(EarthPos[3]);
        state.moonVec    = glm::vec3(MoonPos[3]);
        state.jupiterVec = glm::vec3(JupiterPos[3]);
    }

}






// int main(int argc, const char* argv[])
// {
//     GLState state = GLState("Planet Renderer", 1280, 720,
//                             CallbackPointersGLFW());
//     ShaderGL vShader = ShaderGL(ShaderGL::VERTEX, "shaders/generic.vert");
//     ShaderGL fShader = ShaderGL(ShaderGL::FRAGMENT, "shaders/debug.frag");
//     MeshGL mesh = MeshGL("meshes/tri.obj");
//     TextureGL tex = TextureGL("textures/2k_earth_daymap.jpg",
//                               TextureGL::LINEAR, TextureGL::REPEAT);
//     // Set unchanged state(s)
//     glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//     glEnable(GL_DEPTH_TEST);

//     // =============== //
//     //   RENDER LOOP   //
//     // =============== //
//     while(!glfwWindowShouldClose(state.window))
//     {
//         // Poll inputs from the OS via GLFW
//         glfwPollEvents();

//         // Object-common matrices
//         glm::mat4x4 proj = glm::perspective(glm::radians(50.0f),
//                                             float(state.width) / float(state.height),
//                                             0.01f, 100.0f);
//         glm::mat4x4 view = glm::lookAt(state.pos, state.gaze, state.up);

//         // Start rendering
//         glViewport(0, 0, state.width, state.height);
//         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//         glEnable(GL_CULL_FACE);

//         // Bind shaders and related uniforms / textures
//         // Move this somewhere proper later.
//         // These must match the uniform "location" at the shader(s).
//         // Vertex
//         static constexpr GLuint U_TRANSFORM_MODEL   = 0;
//         static constexpr GLuint U_TRANSFORM_VIEW    = 1;
//         static constexpr GLuint U_TRANSFORM_PROJ    = 2;
//         static constexpr GLuint U_TRANSFORM_NORMAL  = 3;
//         // Fragment
//         static constexpr GLuint U_MODE = 0;
//         static constexpr GLuint T_ALBEDO = 0;
//         // glActiveShaderProgram makes "glUniform" family of commands
//         // to effect the selected shader
//         glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShader.shaderId);
//         glActiveShaderProgram(state.renderPipeline, vShader.shaderId);
//         {
//             // Don't move the triangle
//             glm::mat4x4 model = glm::identity<glm::mat4x4>();
//             // Normal local->world matrix of the object
//             glm::mat3x3 normalMatrix = glm::inverseTranspose(model);
//             glUniformMatrix4fv(U_TRANSFORM_MODEL, 1, false, glm::value_ptr(model));
//             glUniformMatrix4fv(U_TRANSFORM_VIEW, 1, false, glm::value_ptr(view));
//             glUniformMatrix4fv(U_TRANSFORM_PROJ, 1, false, glm::value_ptr(proj));
//             glUniformMatrix3fv(U_TRANSFORM_NORMAL, 1, false, glm::value_ptr(normalMatrix));
//         }
//         // Fragment shader
//         glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShader.shaderId);
//         glActiveShaderProgram(state.renderPipeline, fShader.shaderId);
//         {
//             // Bind texture(s)
//             // You can bind texture via GL_TEXTURE0 + x where x is the bind point at the shader
//             // you do not need to explicitly say GL_TEXTURE1 GL_TEXTURE2 etc.
//             // Here is a demonstration as static assertions.
//             static_assert(GL_TEXTURE0 +  1 == GL_TEXTURE1, "OGL API is wrong!");
//             static_assert(GL_TEXTURE0 +  2 == GL_TEXTURE2, "OGL API is wrong!");
//             static_assert(GL_TEXTURE0 + 16 == GL_TEXTURE16, "OGL API is wrong!");
//             glActiveTexture(GL_TEXTURE0 + T_ALBEDO);
//             glBindTexture(GL_TEXTURE_2D, tex.textureId);

//             glUniform1ui(U_MODE, state.mode);
//         }
//         // Bind VAO
//         glBindVertexArray(mesh.vaoId);
//         // Draw call!
//         glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
//         glfwSwapBuffers(state.window);
//     }

// }
