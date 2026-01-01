#include <cstdio>
#include <array>

#include <iostream>

#include "utility.h"

#include <GLFW/glfw3.h>

#include <glm/ext.hpp> // for matrix calculation


// Merhabalar hocam simulasyonda istediginiz degiskenleri degistrebilirsiniz 
// Sizler icin bazi degistirmek isteyebileceginizi dusundugumuz yerlere isaret koyduk
// Bunlari bulmak icin dosya icinde "abc" aratabilirsiniz.


void MouseMoveCallback(GLFWwindow* wnd, double x, double y){
    
    const float changeSpeed = 0.2f;    //abc  // This for the panning the camera.

    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));

    if (!state->leftButtonPressed){
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

    if (button == GLFW_MOUSE_BUTTON_LEFT){
        if (action == GLFW_PRESS){
            state->leftButtonPressed = true;
            glfwGetCursorPos(wnd, &state->lastX, &state->lastY);
        } else if (action == GLFW_RELEASE){
            state->leftButtonPressed = false;
        }
    }
}

void MouseScrollCallback(GLFWwindow* wnd, double dx, double dy)
{
    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));

    const float zoomSpeed = 5.0f; //abc 

    if (state->cameraMode == 3) {
       
        glm::vec3 front;
        front.x = cos(glm::radians(state->yaw)) * cos(glm::radians(state->pitch));
        front.y = sin(glm::radians(state->pitch));
        front.z = sin(glm::radians(state->yaw)) * cos(glm::radians(state->pitch));
        glm::vec3 forward = glm::normalize(front);

        state->pos += forward * (float)dy * zoomSpeed; 
    }
    
    else {
        state->cameraDistance -= (float)dy * zoomSpeed ;

        if (state->cameraDistance < 5.0f) state->cameraDistance = 5.0f;
        if (state->cameraDistance > 150.0f) state->cameraDistance = 150.0f;
    }
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

    uint32_t oldMode = state->cameraMode;
    
    if(key == GLFW_KEY_P) state->cameraMode = (state->cameraMode == 3) ? 0 : (state->cameraMode + 1);
    if(key == GLFW_KEY_O) state->cameraMode = (state->cameraMode == 0) ? 3 : (state->cameraMode - 1);

    if(key == GLFW_KEY_L){
        state->SimSpeed += changeSpeed;
        if(state->SimSpeed > maxSimSpeed) state->SimSpeed = maxSimSpeed;
    }

    if(key == GLFW_KEY_K){
        state->SimSpeed -= changeSpeed;
        if(state->SimSpeed < minSimSpeed) state->SimSpeed = minSimSpeed;
    }
}

void assignMoonMatrix(GLState &state, float CurrentSimTime, int type, float orbitSize, float orbitSpeed, float scale){
    
    glm::mat4 model;

    if (type == 0){ 
        // moon
        model = glm::mat4(1.0f); 
        model = glm::rotate(model, CurrentSimTime * orbitSpeed, glm::vec3(0, 1, 0));
        model = glm::translate(model, glm::vec3(orbitSize, 1.0f * sin(CurrentSimTime * 0.7f), 0.0f)); // It oscillates(kind of)
        model = glm::scale(model, glm::vec3(scale));
        state.moonModel = model;
        state.moonVec  = glm::vec3(state.moonModel[3]);
    }
    else if (type == 1){ 
        //jupiter
        model = state.moonModel;
        model = glm::rotate(model, CurrentSimTime * orbitSpeed, glm::vec3(0, 1, 0));
        model = glm::translate(model, glm::vec3(orbitSize, 0.5 * sin(CurrentSimTime * 0.6f) + 0.25 * sin(CurrentSimTime * 1.2f), 0.0f)); // It oscillates(kind of)
        model = glm::scale(model, glm::vec3(scale)); // recall that this scale is wrt. to Moon, not Earth
        state.jupiterModel = model;
        state.jupiterVec = glm::vec3(state.jupiterModel[3]);
    }
    else if (type == 2){ 
        //sun
        model = glm::mat4(1.0f);
        model = glm::rotate(model, CurrentSimTime * orbitSpeed, glm::vec3(0, 1, 0));
        model = glm::translate(model, glm::vec3(orbitSize, 5 * sin(CurrentSimTime * 0.3) + 3 * cos(CurrentSimTime * 0.15) , 0.0f)); // It oscillates(kind of)
        model = glm::scale(model, glm::vec3(scale));
        state.sunModel = model;
        state.sunVec = glm::vec3(state.sunModel[3]);    
    }
}

void drawEarth(GLState& state, const MeshGL& mesh, const TextureGL& daytexture,const TextureGL& nighttexture, const TextureGL& specTex,GLuint vShaderId, GLuint fShaderId, glm::mat4 model, glm::mat4 view, glm::mat4 proj, float simTime,GLuint shadowMapTexId){
  
    const float spinSpeed = 0.5f; //abc 

    glm::mat4 finalModel = glm::rotate(model, simTime * spinSpeed, glm::vec3(0, 1, 0));
    glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(finalModel)));


    glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShaderId);
    glActiveShaderProgram(state.renderPipeline, vShaderId);
    
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(finalModel));
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj));
    glUniformMatrix3fv(3, 1, GL_FALSE, glm::value_ptr(normalMat));
    glUniform1ui(4, state.mode); 

    glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShaderId);
    glActiveShaderProgram(state.renderPipeline, fShaderId);
    glUniform1ui(0, state.mode); 

    if (state.mode == 2){
   
        glm::vec3 lightDirection = glm::normalize(state.sunVec); 
    
        glUniform3fv(1, 1, glm::value_ptr(lightDirection)); 
        glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(state.lightSpaceMatrix)); 
        glm::vec3 camPos = state.pos;
        glUniform3fv(2,1,glm::value_ptr(camPos));
        glUniform1ui(6, 1u);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, daytexture.textureId);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexId);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, nighttexture.textureId);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, specTex.textureId);

    glBindVertexArray(mesh.vaoId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.iBufferId);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

}

void drawClouds(GLState& state,
                const MeshGL& mesh,
                const TextureGL& cloudTex,
                GLuint vShaderId,
                GLuint fShaderId,
                glm::mat4 earthModel,
                glm::mat4 view,
                glm::mat4 proj,
                float simTime)
{
    const float cloudSpeed = 0.8f;
    glm::mat4 model = earthModel;
    model = glm::scale(model, glm::vec3(1.02f));
    model = glm::rotate(model, simTime * cloudSpeed, glm::vec3(0,1,0));

    glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(model)));

    // vertex shader
    glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShaderId);
    glActiveShaderProgram(state.renderPipeline, vShaderId);
    glUniformMatrix4fv(0,1,false,glm::value_ptr(model));
    glUniformMatrix4fv(1,1,false,glm::value_ptr(view));
    glUniformMatrix4fv(2,1,false,glm::value_ptr(proj));
    glUniformMatrix3fv(3,1,false,glm::value_ptr(normalMat));
    glUniform1ui(4,4); // <<<<<< cloud mode

    // fragment shader
    glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShaderId);
    glActiveShaderProgram(state.renderPipeline, fShaderId);
    glUniform1ui(0,4);

    // light
    glm::vec3 lightDir = normalize(state.sunVec);
    glUniform3fv(1,1,glm::value_ptr(lightDir));

    // alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // texture bind
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, cloudTex.textureId);

    glBindVertexArray(mesh.vaoId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.iBufferId);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT,nullptr);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
void drawMoon(GLState& state, const MeshGL& mesh, const TextureGL& texture, GLuint vShaderId, GLuint fShaderId, glm::mat4 model, glm::mat4 view, glm::mat4 proj, float simTime){
  
    const float spinSpeed = 1.5f; //abc

    glm::mat4 finalModel = glm::rotate(model, simTime * spinSpeed, glm::vec3(0, 1, 0));
    glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(finalModel)));

    glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShaderId);
    glActiveShaderProgram(state.renderPipeline, vShaderId);

    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(finalModel));
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj));
    glUniformMatrix3fv(3, 1, GL_FALSE, glm::value_ptr(normalMat));
    glUniform1ui(4, state.mode); 

    glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShaderId);
    glActiveShaderProgram(state.renderPipeline, fShaderId);
    glUniform1ui(0, state.mode); 

    if (state.mode == 2){
    
        glm::vec3 lightDirection = glm::normalize(state.sunVec); 
        
        glUniform3fv(1, 1, glm::value_ptr(lightDirection)); 
        glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(state.lightSpaceMatrix)); 
        glm::vec3 camPos = state.pos;
        glUniform3fv(2,1,glm::value_ptr(camPos));
        glUniform1ui(6, 0u);
    
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.textureId);

    glBindVertexArray(mesh.vaoId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.iBufferId);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

}

void drawBackground(GLState& state, const MeshGL& mesh, const TextureGL& texture, GLuint vShaderId, GLuint fShaderId, glm::mat4 view, glm::mat4 proj){
    
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);     

    glm::mat4 skyView = glm::mat4(glm::mat3(view));

    glm::mat4 skyProj = glm::perspective(glm::radians(45.0f), (float)state.width / (float)state.height, 0.1f, 1000.0f);  // It is perspective but meets the conditions

    glm::mat4 skyModel = glm::scale(glm::mat4(1.0f), glm::vec3(500.0f));

    glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShaderId);
    glActiveShaderProgram(state.renderPipeline, vShaderId);
    
    glUniformMatrix4fv(0, 1, false, glm::value_ptr(skyModel)); 
    glUniformMatrix4fv(1, 1, false, glm::value_ptr(skyView));  
    glUniformMatrix4fv(2, 1, false, glm::value_ptr(skyProj));
    glUniform1ui(4, state.mode);    

    glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShaderId);
    glActiveShaderProgram(state.renderPipeline, fShaderId);
    glUniform1ui(0, state.mode);
    
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

void drawSun(GLState& state, const MeshGL& mesh, const TextureGL& texture, GLuint vShaderId, GLuint fShaderId, glm::mat4 view, glm::mat4 proj, float simTime){  
    
    //glDepthMask(GL_FALSE);
    //glDisable(GL_CULL_FACE); // We have worried about if it is too far so we disabled just for the sun

    glm::mat4 sunView = glm::mat4(glm::mat3(view));
    glm::mat4 sunProj = glm::perspective(glm::radians(45.0f), (float)state.width / state.height, 0.1f, 1000.0f); // It is perspective but meets the conditions

    glm::mat4 sunModel = state.sunModel;

    glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShaderId);
    glActiveShaderProgram(state.renderPipeline, vShaderId);
    glUniformMatrix4fv(0, 1, false, glm::value_ptr(sunModel));
    glUniformMatrix4fv(1, 1, false, glm::value_ptr(sunView));
    glUniformMatrix4fv(2, 1, false, glm::value_ptr(sunProj));
    //No need for normal since it is just white
    glUniform1ui(4, state.mode);  

    glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShaderId);
    glActiveShaderProgram(state.renderPipeline, fShaderId);
    glUniform1ui(0, state.mode);


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.textureId);

    glBindVertexArray(mesh.vaoId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.iBufferId);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // glEnable(GL_CULL_FACE);
    // glDepthMask(GL_TRUE);
 
}

void setupShadowMap(GLuint& fbo, GLuint& colorTex, GLuint& depthTex, int res){
    
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    
    glGenTextures(1, &colorTex);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, res, res, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; 
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

   
    glGenTextures(1, &depthTex);
    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, res, res, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

int main(int argc, const char* argv[])
{
    GLState state = GLState("Planet Renderer", 1280, 720, CallbackPointersGLFW());
    ShaderGL vShader = ShaderGL(ShaderGL::VERTEX, "shaders/generic.vert");
    ShaderGL fShader = ShaderGL(ShaderGL::FRAGMENT, "shaders/debug.frag");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    //Objects
    MeshGL Earth = MeshGL("meshes/sphere_20k.obj");
    MeshGL Moon = MeshGL("meshes/sphere_5k.obj");
    MeshGL Jupiter = MeshGL("meshes/sphere_2k.obj");
    MeshGL Sky = MeshGL("meshes/sphere_80k.obj");   
    MeshGL Sun = MeshGL("meshes/sphere_2k.obj");

    //Textures
    TextureGL EarthSpecTex = TextureGL("textures/2k_earth_specular_map.png", TextureGL::LINEAR, TextureGL::REPEAT);

    TextureGL EarthNightTex = TextureGL("textures/2k_earth_nightmap_alpha.png", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL CloudTex      = TextureGL("textures/2k_earth_clouds_alpha.png",   TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL EarthTex = TextureGL("textures/2k_earth_daymap.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL MoonTex = TextureGL("textures/2k_moon.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL JupiterTex = TextureGL("textures/2k_jupiter.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL SkyTex = TextureGL("textures/8k_stars_milky_way.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL SunTex = TextureGL("textures/2k_moon.jpg", TextureGL::LINEAR, TextureGL::REPEAT); //random as sun tex is not provided and it would be white anyway

    //Starting setup

    state.earthModel = glm::scale(state.earthModel, glm::vec3(3.0f)); //abc This is the scale for the Earth

    float CurrentSimTime = 0.0f;   
    float lastFrameTime = 0.0f;    
    
    GLuint shadowFBO, shadowColorTex, shadowDepthTex;
    const int SHADOW_RES = 2048; 
    setupShadowMap(shadowFBO, shadowColorTex, shadowDepthTex, SHADOW_RES);

    // =============== //
    //   RENDER LOOP   //
    // =============== //

    while(!glfwWindowShouldClose(state.window)){
    
        glfwPollEvents();

        // Time management
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        CurrentSimTime += deltaTime * state.SimSpeed;

        // Calculate and save the positions  
        
        //abc                               Planet OrbitSize    OrbitSpeed   Scale
        assignMoonMatrix(state, CurrentSimTime, 0, 12.0f,       0.8f,        1.0f); // Moon
        assignMoonMatrix(state, CurrentSimTime, 1, 3.0f,        1.5,        0.33f);  // Jupiter // Look for the function for the value of Scale
        assignMoonMatrix(state, CurrentSimTime, 2, 300.0f,      0.005f,      2.0f); // Sun

        // Cam matrices

        glm::mat4 proj = glm::perspective(glm::radians(state.FOV), (float)state.width / state.height, 0.1f, 1000.0f);

        //dir = Planet - Camera
        glm::vec3 dir; 
        dir.x = cos(glm::radians(state.yaw)) * cos(glm::radians(state.pitch));
        dir.y = sin(glm::radians(state.pitch));
        dir.z = sin(glm::radians(state.yaw)) * cos(glm::radians(state.pitch));
        dir = glm::normalize(dir);

        glm::mat4 view;

        if (state.cameraMode == 0){ // Earth 
            state.pos = state.earthVec + dir * state.cameraDistance; 
            view = glm::lookAt(state.pos, state.earthVec, state.up);
        } 
        else if (state.cameraMode == 1){ // Moon 
            state.pos = state.moonVec + dir * state.cameraDistance;
            view = glm::lookAt(state.pos, state.moonVec, state.up);
        }
        else if (state.cameraMode == 2){ // Jupiter 
            state.pos = state.jupiterVec + dir * state.cameraDistance;
            view = glm::lookAt(state.pos, state.jupiterVec, state.up);
        }
        
    
        else if (state.cameraMode == 3){ 
            
            glm::vec3 front;
            front.x = cos(glm::radians(state.yaw)) * cos(glm::radians(state.pitch));
            front.y = sin(glm::radians(state.pitch));
            front.z = sin(glm::radians(state.yaw)) * cos(glm::radians(state.pitch));
            glm::vec3 forward = glm::normalize(-front); // we need cam - planet so its -front
            
            glm::vec3 right = glm::normalize(glm::cross(forward, state.up));

            float cameraSpeed = 15.0f * deltaTime; 

            if (glfwGetKey(state.window, GLFW_KEY_W) == GLFW_PRESS) state.pos += forward * cameraSpeed;
            if (glfwGetKey(state.window, GLFW_KEY_S) == GLFW_PRESS) state.pos -= forward * cameraSpeed;
            if (glfwGetKey(state.window, GLFW_KEY_A) == GLFW_PRESS) state.pos -= right * cameraSpeed;
            if (glfwGetKey(state.window, GLFW_KEY_D) == GLFW_PRESS) state.pos += right * cameraSpeed;

            view = glm::lookAt(state.pos, state.pos + forward, state.up);
        }
        // Shadow matrix
        
        glm::mat4 lightView = glm::lookAt(state.sunVec,glm::vec3(0,0,0) , glm::vec3(0, 1, 0));
        glm::mat4 lightProj = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, 0.1f, 1000.0f);
        state.lightSpaceMatrix = lightProj * lightView;

        // Shadow mapping 
        state.mode = 3; // In this mode we are saving only z-depths
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO); // <- Warning from here program/shader state performance warning: Vertex shader in program 2 is being recompiled based on GL state.
                                                      // Couldnt find a solution :(
        glViewport(0, 0, SHADOW_RES, SHADOW_RES);
        
        float largeVal[] ={ 1.0f, 0.0f, 0.0f, 0.0f };   
        glClearBufferfv(GL_COLOR, 0, largeVal);
        glClear(GL_DEPTH_BUFFER_BIT);

        drawEarth(state, Earth, EarthTex,EarthNightTex,EarthSpecTex, vShader.shaderId, fShader.shaderId, state.earthModel, lightView, lightProj, CurrentSimTime,shadowColorTex);
        drawMoon(state, Moon, MoonTex, vShader.shaderId, fShader.shaderId, state.moonModel, lightView, lightProj, CurrentSimTime);
        drawMoon(state, Jupiter, JupiterTex, vShader.shaderId, fShader.shaderId, state.jupiterModel, lightView, lightProj, CurrentSimTime);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);           
        glViewport(0, 0, state.width, state.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, shadowColorTex);

        // Rendering 

        state.mode = 1; drawBackground(state, Sky, SkyTex, vShader.shaderId, fShader.shaderId, view, proj);
        state.mode = 0; drawSun(state, Sun, SunTex, vShader.shaderId, fShader.shaderId, view, proj, CurrentSimTime);

        state.mode = 2;
        
        drawEarth(state, Earth, EarthTex,EarthNightTex, EarthSpecTex,vShader.shaderId, fShader.shaderId, state.earthModel, view, proj, CurrentSimTime,shadowColorTex);
        drawClouds(state, Earth, CloudTex,
                   vShader.shaderId, fShader.shaderId,
                   state.earthModel,
                   view, proj,
                   CurrentSimTime);
        state.mode = 5;
        drawMoon(state, Moon, MoonTex, vShader.shaderId, fShader.shaderId, state.moonModel, view, proj, CurrentSimTime);
        drawMoon(state, Jupiter, JupiterTex, vShader.shaderId, fShader.shaderId, state.jupiterModel, view, proj, CurrentSimTime);
        state.mode = 2;
        
        glfwSwapBuffers(state.window);
    
    }

}


// int main(int argc, const char* argv[])
//{
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
//  {
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
//      {
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
//      {
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
