#include <cstdio>
#include <array>

#include "utility.h"

#include <GLFW/glfw3.h>

#include <glm/ext.hpp> // for matrix calculation

void MouseMoveCallback(GLFWwindow* wnd, double x, double y)
{
    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));

    if (!state->leftButtonPressed) {
        state->lastX = x;
        state->lastY = y;
        return;
    }

    float xoffset = (float)(x - state->lastX) * 0.1f;
    float yoffset = (float)(state->lastY - y) * 0.1f;
    state->lastX = x;
    state->lastY = y;

    state->yaw   += xoffset;
    state->pitch += yoffset;

    // Pitch sınırını koru (Kutuplarda takla atmamak için)
    if (state->pitch > 89.0f)  state->pitch = 89.0f;
    if (state->pitch < -89.0f) state->pitch = -89.0f;
}

void MouseButtonCallback(GLFWwindow* wnd, int button, int action, int mods)
{
    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            state->leftButtonPressed = true;
            // Tıkladığı anda farenin o anki konumunu "ilk konum" olarak alalım ki sıçrama yapmasın
            glfwGetCursorPos(wnd, &state->lastX, &state->lastY);
        } else if (action == GLFW_RELEASE) {
            state->leftButtonPressed = false;
        }
    }
}
void MouseScrollCallback(GLFWwindow* wnd, double dx, double dy)
{
    // Pencereye bağlı olan GLState pointer'ını alıyoruz
    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));

    // FOV değerini dy miktarına göre değiştir (Hızı ayarlamak için 2.0f ile çarptık)
    state->FOV -= (float)dy * 2.0f;

    // Kamera açısının çok fazla bozulmaması için sınırlar koyalım
    if (state->FOV < 5.0f)   state->FOV = 5.0f;   // Maksimum zoom
    if (state->FOV > 120.0f) state->FOV = 120.0f; // Maksimum geniş açı
}

void FramebufferChangeCallback(GLFWwindow* wnd, int w, int h)
{
    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));
    state->width = w;
    state->height = h;
}

void KeyboardCallback(GLFWwindow* wnd, int key, int scancode, int action, int modifier)
{
    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));
    if(action != GLFW_PRESS) return;

    uint32_t oldMode = state->mode;
    
    // Modu değiştir
    if(key == GLFW_KEY_P) state->mode = (state->mode == 3) ? 0 : (state->mode + 1);
    if(key == GLFW_KEY_O) state->mode = (state->mode == 0) ? 3 : (state->mode - 1);

    // EĞER ŞİMDİ MOD 3'E GİRDİYSEK (Kritik Düzeltme)
    if (state->mode == 3 && oldMode != 3) {
        // Yörünge kamerasında (Orbit) bakış yönümüz merkeze doğrudur.
        // Serbest kameraya geçtiğimizde kameranın kafasını 180 derece çevirmeliyiz 
        // ki baktığımız gezegeni görmeye devam edelim.
        state->yaw += 180.0f;
        
        // Pitch değerini tersine çevirerek bakış açısını dikeyde de hizalarız.
        state->pitch = -state->pitch;
    }
        if(key == GLFW_KEY_L) {
        state->SimSpeed += 0.1f;
        
        // Üst limit koyalım ki gezegenler sapıtmasın (örneğin 10 kat hız)
        if(state->SimSpeed > 5.0f) state->SimSpeed = 5.0f;
    }

    if(key == GLFW_KEY_K) {
        state->SimSpeed -= 0.1f;
        
        // Alt limit koyalım (örneğin -10 kat geri sarma)
        if(state->SimSpeed < 0.0f) state->SimSpeed = 0.0f;
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
    model = glm::scale(model, glm::vec3(1.0f)); 


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
    glm::mat4 parentMatrix, // Kimin etrafında dönecek?
    float orbitRadius,      // Ne kadar uzakta dönecek?
    float orbitSpeed,       // Ne kadar hızlı dönecek? (Bunu da ekledim, çok lazım olacak)
    float scaleSize)        // Boyutu ne olacak?
{
    // --- Ayarlar ---
    // Sabit hız yerine parametreden gelen orbitSpeed'i kullanacağız.
    // Kendi ekseninde dönüş hızı (Spin) sabit kalabilir veya onu da parametre yapabilirsin.
    const float spinSpeed = 1.0f; 

    // Shader Location Tanımları (Aynı)
    static constexpr GLuint U_TRANSFORM_MODEL   = 0;
    static constexpr GLuint U_TRANSFORM_VIEW    = 1;
    static constexpr GLuint U_TRANSFORM_PROJ    = 2;
    static constexpr GLuint U_TRANSFORM_NORMAL  = 3;
    static constexpr GLuint U_MODE = 0;
    static constexpr GLuint T_ALBEDO = 0;

    // --- 1. MATEMATİKSEL MODELLEME ---
    
    // A. Parent Pozisyonu
    glm::mat4 model = parentMatrix; 

    // B. Yörünge Dönüşü (Orbit Rotation)
    // Parametreden gelen 'orbitSpeed' kullanılıyor
    float orbitAngle = simTime * orbitSpeed;
    model = glm::rotate(model, orbitAngle, glm::vec3(0, 1, 0));

    // C. Uzaklaşma (Translation)
    // Parametreden gelen 'orbitRadius' kullanılıyor
    model = glm::translate(model, glm::vec3(orbitRadius, 0, 0));

    // --- MERKEZİ KAYDET ---
    glm::mat4 centerMatrix = model;

    // D. Kendi Ekseninde Dönme (Spin)
    float spinAngle = simTime * spinSpeed;
    model = glm::rotate(model, spinAngle, glm::vec3(0, 1, 0));

    // E. Küçülme (Scale)
    // Parametreden gelen 'scaleSize' kullanılıyor
    model = glm::scale(model, glm::vec3(scaleSize));


    // --- 2. ÇİZİM (Aynı) ---
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
    glBindVertexArray(0);

    return centerMatrix;
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

    //Textures
    TextureGL EarthTex = TextureGL("textures/2k_earth_daymap.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL MoonTex = TextureGL("textures/2k_moon.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL JupiterTex = TextureGL("textures/2k_jupiter.jpg", TextureGL::LINEAR, TextureGL::REPEAT);

    // --- DEĞİŞİKLİK 1: Zaman Değişkenleri ---
    
    float CurrentSimTime = 0.0f;   // Simülasyonda geçen toplam zaman
    float lastFrameTime = 0.0f;    // Delta time hesabı için

    // --- DEĞİŞİKLİK 2: Kamerayı Başlat ---
    glm::vec3 CameraStartPos = glm::vec3(0.0f, 0.0f, 5.0f);
    state.pos = CameraStartPos;    // state içindeki pozisyonu güncelle
    

    // =============== //
    //   RENDER LOOP   //
    // =============== //

    while(!glfwWindowShouldClose(state.window))
    {

        glfwPollEvents();

        

        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        
        // Simülasyon zamanını ilerlet (Speed 0 ise zaman durur)
        CurrentSimTime += deltaTime * state.SimSpeed;

        if (state.mode == 3) {
        float cameraSpeed = 5.0f * deltaTime; // deltaTime ile çarpmak hızı sabitler
        
        // Bakış yönünü (forward) hesapla: Gaze bir nokta olduğu için, 
        // bakış yönü = Gaze - Pos
        glm::vec3 forward = glm::normalize(state.gaze - state.pos);
        // Sağ yönü (right) hesapla
        glm::vec3 right = glm::normalize(glm::cross(forward, state.up));

        if (glfwGetKey(state.window, GLFW_KEY_W) == GLFW_PRESS)
            state.pos += forward * cameraSpeed;
        if (glfwGetKey(state.window, GLFW_KEY_S) == GLFW_PRESS)
            state.pos -= forward * cameraSpeed;
        if (glfwGetKey(state.window, GLFW_KEY_A) == GLFW_PRESS)
            state.pos -= right * cameraSpeed;
        if (glfwGetKey(state.window, GLFW_KEY_D) == GLFW_PRESS)
            state.pos += right * cameraSpeed;

        // Kameranın baktığı yeri de konumla beraber kaydır ki bakış açısı bozulmasın
        // (Eğer gaze hep merkeze baksın istiyorsan bu satırı sil)
        state.gaze = state.pos + forward; 
    }

        glViewport(0, 0, state.width, state.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4x4 proj;
        glm::mat4x4 view;

        proj = glm::perspective(glm::radians(state.FOV), (float)state.width / (float)state.height, 0.1f, 1000.0f);
        // Bu yardımcı hesaplamayı switch/if-else bloğunun hemen üstünde yapabilirsin
        glm::vec3 dir;
        dir.x = cos(glm::radians(state.yaw)) * cos(glm::radians(state.pitch));
        dir.y = sin(glm::radians(state.pitch));
        dir.z = sin(glm::radians(state.yaw)) * cos(glm::radians(state.pitch));

        // 2. MODLARA GÖRE DAVRAN
        if (state.mode == 0) { // DÜNYA
            state.pos = state.earthVec + dir * 6.0f; 
            view = glm::lookAt(state.pos, state.earthVec, state.up);
        }
        else if (state.mode == 1) { // AY
            state.pos = state.moonVec + dir * 2.0f;
            view = glm::lookAt(state.pos, state.moonVec, state.up);
        }
        else if (state.mode == 2) { // UYDU
            state.pos = state.jupiterVec + dir * 0.5f;
            view = glm::lookAt(state.pos, state.jupiterVec, state.up);
        }
        else if (state.mode == 3) { // SERBEST (WASD BURADA YAŞAYACAK)
            float cameraSpeed = 5.0f * deltaTime;
            
            // Yön vektörlerini hesapla (FPS tarzı)
            glm::vec3 forward = glm::normalize(dir); // dir zaten bakış yönümüz
            glm::vec3 right = glm::normalize(glm::cross(forward, state.up));

            // Klavyeyi kontrol et
            if (glfwGetKey(state.window, GLFW_KEY_W) == GLFW_PRESS) state.pos += forward * cameraSpeed;
            if (glfwGetKey(state.window, GLFW_KEY_S) == GLFW_PRESS) state.pos -= forward * cameraSpeed;
            if (glfwGetKey(state.window, GLFW_KEY_A) == GLFW_PRESS) state.pos -= right * cameraSpeed;
            if (glfwGetKey(state.window, GLFW_KEY_D) == GLFW_PRESS) state.pos += right * cameraSpeed;

            // Bakış noktasını önümüze koy
            state.gaze = state.pos + forward;
            view = glm::lookAt(state.pos, state.gaze, state.up);
        }

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
