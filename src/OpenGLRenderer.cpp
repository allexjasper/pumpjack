#include <GL/glew.h>
#include "OpenGLRenderer.h"
#include <boost/log/trivial.hpp>
#include <SDL.h>
#include <SDL_opengl.h>
#include <iostream>
#include <vector>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <algorithm>
#include <boost/regex.hpp>
#include <fstream>
#include <gli.hpp>


TimeSeries OpenGLRenderer::createTextureTimeSeries(const std::vector<OpenGLRenderer::TextureInfo>& textures, std::chrono::milliseconds totalDuration, TimeSeries::Timestamp startTime) {
    TimeSeries timeSeries;

    if (textures.empty()) {
        return timeSeries;
    }

    std::chrono::milliseconds interval = totalDuration / textures.size();
    TimeSeries::Timestamp lastTimestamp = startTime;

    // Add the first set of textures
    for (size_t i = 0; i < textures.size(); ++i) {
        TimeSeries::Timestamp timestamp = startTime + interval * i;
        const TextureInfo& textureInfo = textures[i];
        float angle = std::get<1>(textureInfo);
        int index = std::get<2>(textureInfo); // Use index instead of texture name
        TimeSeries::Sample sample = std::make_tuple(angle, timestamp, index);
        timeSeries.add(sample);
        lastTimestamp = timestamp;
    }

    // Add the second set of textures with continuously increasing timestamps
    for (size_t i = 0; i < textures.size(); ++i) {
        lastTimestamp += interval;
        const TextureInfo& textureInfo = textures[i];
        float angle = std::get<1>(textureInfo);
        int index = std::get<2>(textureInfo); // Use index instead of texture name
        TimeSeries::Sample sample = std::make_tuple(angle, lastTimestamp, index);
        timeSeries.add(sample);
    }

    return timeSeries;
}


void OpenGLRenderer::CreateTexture(const OpenGLRenderer::FrameInfo& info)
{
    // Load the texture from file
    gli::texture& Texture = *new gli::texture();
    Texture = gli::load(std::get<0>(info));
    if (Texture.empty())
        return;

    // Translate the texture format and target for OpenGL
    gli::gl GL(gli::gl::PROFILE_GL33);
    gli::gl::format const Format = GL.translate(Texture.format(), Texture.swizzles());
    auto Target = GL.translate(Texture.target());

    // Ensure the texture is compressed and is a 2D texture
    assert(gli::is_compressed(Texture.format()));
    assert(Target == gli::gl::TARGET_2D);

    // Generate and bind a new texture object
    GLuint TextureName = 0;
    glGenTextures(1, &TextureName);
    glBindTexture(Target, TextureName);

    // Set texture parameters
    glTexParameteri(Target, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(Target, GL_TEXTURE_MAX_LEVEL, 0); // Only one level, set max level to 0
    glTexParameteriv(Target, GL_TEXTURE_SWIZZLE_RGBA, &Format.Swizzles[0]);

    // Get the extent of the base level (level 0)
    auto Extent = Texture.extent(0);

    // Allocate storage for the texture
    glTexStorage2D(GL_TEXTURE_2D, 1, Format.Internal, Extent.x, Extent.y);

    // Upload the texture data for the base level
    glCompressedTexSubImage2D(
        Target, 0, 0, 0, Extent.x, Extent.y,
        Format.Internal, static_cast<GLsizei>(Texture.size(0)), Texture.data(0, 0, 0)
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gli::texture* p = &Texture;

    TextureInfo ti = { p, std::get<1>(info), std::get<2>(info), (int)TextureName }; // gli::texture, angle, index, OpenGL texture
    _textures.push_back(ti);
}

namespace fs = boost::filesystem;

std::vector<OpenGLRenderer::FrameInfo> OpenGLRenderer::getFilesSorted(const std::string& directory)
{
    std::vector<FrameInfo> fileInfos;

    // Iterate over the files in the directory
    for (const auto& entry : boost::make_iterator_range(fs::directory_iterator(directory), {})) {
        if (!fs::is_regular_file(entry))
        {
            continue;
        }
        std::string curEntry = entry.path().string();
        boost::regex regExWithAngle(".*_(-?)(\\d+)_(\\d+)_(\\d+).*$");
        boost::regex regExWoAngle(".*(\\d\\d\\d\\d).*$");

        boost::smatch matches;

        if (boost::regex_search(curEntry, matches, regExWithAngle))
        {
            // Extract and convert the matched strings to integers
            int sign = 1;
            if (matches[1] == "-")
                sign = -1;
            int angleInt = std::stoi(matches[2].str());
            int angleDecimal = std::stoi(matches[3].str());
            int frameNumber = std::stoi(matches[4].str());

            float angle = sign * angleInt + sign*angleDecimal / 1000.0f;

            // Add the extracted information to the vector as a tuple
            fileInfos.push_back(std::make_tuple(curEntry, angle, frameNumber));

        } 
        else if(boost::regex_search(curEntry, matches, regExWoAngle))
        {
            // Extract and convert the matched strings to integers
            int frameNumber = std::stoi(matches[1].str());

            float angle = 0.0;

            // Add the extracted information to the vector as a tuple
            fileInfos.push_back(std::make_tuple(curEntry, angle, frameNumber));

        }
        else
        {
            BOOST_LOG_TRIVIAL(info) << "No match found for: " << curEntry << "\n";
        }
    }

    // Sort the vector by frame number
    std::sort(fileInfos.begin(), fileInfos.end(), [](const std::tuple<std::string, float, int>& a, const std::tuple<std::string, float, int>& b) {
        return std::get<2>(a) < std::get<2>(b);
        });

    return fileInfos;
}





// The window we'll be rendering to
SDL_Window* gWindow = NULL;

bool OpenGLRenderer::init() {
    // Initialization flag
    bool success = true;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        BOOST_LOG_TRIVIAL(info) << "SDL could not initialize! SDL Error:\n" << SDL_GetError();
        success = false;
    }
    else {
        // Create window
        SDL_DisplayMode dm;
        if (SDL_GetCurrentDisplayMode(0, &dm) != 0) {
            SDL_Quit();
            return false;
        }

        //SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 3); // Adjust the number of samples as needed


        if (_fullscreen)
        {
            screenWidth = dm.w;
            screenHeight = dm.h;
            gWindow = SDL_CreateWindow("Movement Machine Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth, screenHeight, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
        }
        else
        {
            screenWidth = dm.w * 0.7;
            screenHeight = dm.h * 0.7;
            gWindow = SDL_CreateWindow("Movement Machine Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth, screenHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL );
        }

     

        SDL_ShowCursor(SDL_DISABLE);

        // Create OpenGL context
        auto context = SDL_GL_CreateContext(gWindow);
        if (!context) {
            BOOST_LOG_TRIVIAL(info) <<  "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
            return false;
        }

        // Enable V-sync (Set swap interval to 1)
        if (SDL_GL_SetSwapInterval(1) < 0) {
            // Handle error
            BOOST_LOG_TRIVIAL(info) << "Warning: Unable to set VSync! SDL Error: "  << SDL_GetError();
        }

        // Initialize GLEW
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW" << std::endl;
            return false;
        }
        
        if (gWindow == NULL) {
            BOOST_LOG_TRIVIAL(info) << "Window could not be created! SDL Error: " << SDL_GetError();
            success = false;
        }

    }

    return success;
}

bool OpenGLRenderer::loadMedia(const std::string& directory) {
    // Loading success flag
    bool success = true;

    auto files = getFilesSorted(directory);

    // Load JPG texture
   
    for (const auto& file : files)
    {
        CreateTexture(file);
    }

    return success;
}

void OpenGLRenderer::close() {
    
    SDL_DestroyWindow(gWindow);
    gWindow = NULL;
    SDL_Quit();
}


TimeSeries OpenGLRenderer::_curTimeSeries;
std::mutex OpenGLRenderer::_mutTimeSeries;



OpenGLRenderer::OpenGLRenderer(const std::string& fileName, bool fullscreen,  float scale) : _fileName(fileName),
    _fullscreen(fullscreen),
    _scale(scale),
    _shutdownRequested(false)
{
    //gFileName = fileName;
    //gzeroAnglePos = zeroAnglePos;
}

OpenGLRenderer::~OpenGLRenderer()
{

}



void OpenGLRenderer::feedData(const TimeSeries& ts, std::chrono::milliseconds periodicity, TimeSeries::Timestamp lastPeriodBegin, std::chrono::milliseconds curRoationOffset, const std::string& overlay)
{
    std::scoped_lock lock(_mutTimeSeries);
    _periodicity = periodicity;
    _curTimeSeries = ts;
    _curRoationOffset = curRoationOffset;
    _lastPeriodBegin = lastPeriodBegin;
    //gOverlay = overlay;
}

void OpenGLRenderer::shutdown()
{
    _shutdownRequested = true;
    //_renderThread.join();
    //_frameMatchingThread.join();
}

//    if (!image.loadFromFile("C:\\Users\\alexa\\Downloads\\download.jpg"))









void OpenGLRenderer::renderQuad(int textureID, int width, int height, float angle)
{
    
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Calculate the position of the quad to be centered
    float quadLeft = (screenWidth - width * _scale) / 2.0f;
    float quadTop = (screenHeight - height * _scale) / 2.0f;
    float quadRight = quadLeft + width * _scale;
    float quadBottom = quadTop + height * _scale;

    // Apply rotation
    glTranslatef((quadLeft + quadRight) / 2.0f, (quadTop + quadBottom) / 2.0f, 0.0f);
    glRotatef(-angle, 0.0f, 0.0f, 1.0f);
    glTranslatef(-(quadLeft + quadRight) / 2.0f, -(quadTop + quadBottom) / 2.0f, 0.0f);

    // Draw the textured quad
    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f); // Set color to white to show the texture
    glTexCoord2f(0.0f, 0.0f); glVertex2f(quadLeft, quadTop); // Bottom-left corner
    glTexCoord2f(1.0f, 0.0f); glVertex2f(quadRight, quadTop); // Bottom-right corner
    glTexCoord2f(1.0f, 1.0f); glVertex2f(quadRight, quadBottom); // Top-right corner
    glTexCoord2f(0.0f, 1.0f); glVertex2f(quadLeft, quadBottom); // Top-left corner
    glEnd();
}

void OpenGLRenderer::renderThread(std::function<void(const std::string, TimeSeries&)> monitor)
{
    // Start up SDL and create window
    if (!init()) {
        printf("Failed to initialize!\n");
    }
    else {
        // Load media
        BOOST_LOG_TRIVIAL(info) << "loading load media!\n";
        if (!loadMedia(_fileName) || _textures.empty()) {
            BOOST_LOG_TRIVIAL(info) << "Failed to load media!\n";
        }
        else {  
           
            BOOST_LOG_TRIVIAL(info) << "init view port!\n";
            // Render texture to the center of the screen
            auto Extent = std::get<0>(_textures.front())->extent(0);
            int textureWidth = Extent.x;
            int textureHeight = Extent.y;
           

            // Get the viewport dimensions
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            int screenWidth = viewport[2];
            int screenHeight = viewport[3];

            glClear(GL_COLOR_BUFFER_BIT);

            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, screenWidth, screenHeight, 0, -1, 1); // Set up orthographic projection with viewport dimensions
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            glEnable(GL_TEXTURE_2D);

            std::optional<int> prevFrame;
            // While application is running
            while (!_shutdownRequested) {
                auto ts_frame_begin = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
                
                SDL_Event e;

                while (SDL_PollEvent(&e) != 0) {
                    // User requests quit
                    if (e.type == SDL_QUIT) {

                        _shutdownRequested = true;
                    }
                    // User presses a key
                    else if (e.type == SDL_KEYDOWN) {
                        switch (e.key.keysym.sym) {
                        case SDLK_q:
                            _shutdownRequested = true;
                            break;
                        default:
                            break;
                        }
                    }
                }
            

                float angle = 0.0f;


                TimeSeries localTS;

                {
                    std::scoped_lock lock(_mutTimeSeries);
                    localTS = _curTimeSeries;
                }


                auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());


                angle = findAngleToRender(now, localTS);
                

                auto frame_to_render = findFrameToRender(prevFrame, angle, now, localTS);
                
                
                prevFrame = frame_to_render;

                TimeSeries renderedAngleTs;
                renderedAngleTs.add({ angle, std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()), 0 });
                monitor("rendered", renderedAngleTs);



                glClear(GL_COLOR_BUFFER_BIT);
                glLoadIdentity();


                renderQuad(std::get<3>(_textures[frame_to_render]), textureWidth, textureHeight, angle);

                SDL_GL_SwapWindow(gWindow);
                SDL_Delay(5);

                auto ts_frame_end = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());

  //              BOOST_LOG_TRIVIAL(info) << "frame time: " << std::chrono::duration_cast<std::chrono::milliseconds>(ts_frame_end - ts_frame_begin).count() << std::endl;
            }
        }

    }
    glDisable(GL_TEXTURE_2D);
    // Free resources and close SDL
    close();
}



void OpenGLRenderer::render(std::function<void(const std::string, TimeSeries&)> monitor)
{

    //_renderThread = std::thread([this, monitor]() {
    //    renderThread(monitor);
    //    });
    renderThread(monitor);


}
