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



int positive_mod(int a, int b) {
    int result = a % b;
    if (result < 0) {
        result += b;
    }
    return result;
}

typedef std::tuple<int, float, int> FrameMatch; // frame index, distance angle, distance time
int distanceWeight(const FrameMatch& a)
{
    return std::get<1>(a) * 120.0 + std::get<2>(a);
}

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


void OpenGLRenderer::frameMatchtingThread()
{
    while (!_shutdownRequested)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        
#if 0
        TimeSeries localTS;

        {
            std::scoped_lock lock(_mutTimeSeries);
            localTS = _curTimeSeries;
        }

        auto frameTS = createTextureTimeSeries(_textures, _periodicity + _curRoationOffset, _lastPeriodBegin);
        auto resampledFrameTS = frameTS.resample();


        const auto  correlationTimeSeriesLength = std::chrono::milliseconds(800);
        const auto  correlationSearchWindowSize = std::chrono::milliseconds(300);

        auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());

        TimeSeries slicedFrameTS = resampledFrameTS.slice(now, now + correlationTimeSeriesLength);

        if (localTS.getVector().empty() || slicedFrameTS.getVector().empty())
            continue;


        auto bestOffset = localTS.bestMatch(correlationSearchWindowSize, slicedFrameTS);

        

        slicedFrameTS.shift(bestOffset);

        BOOST_LOG_TRIVIAL(info) << "texture frames offset: " << bestOffset.count() << std::endl;

        {
            std::scoped_lock lock(_mutTextureTimeSeries);
            _textureTimeSeries = slicedFrameTS;
        }
#endif
     

    }
}



int OpenGLRenderer::findFrameToRender(std::optional<int> prevFrame, float angle, 
                                    std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds> refNow, TimeSeries& localTS)
{




    if (_angleMatchFrames)
    {
#if 0
        std::optional<size_t> ind;
        std::optional<int> frameToRenderFromMatch;

        {
            std::scoped_lock lock(_mutTextureTimeSeries);
            ind = _textureTimeSeries.findIndex(refNow);
            if (ind)
            {
                frameToRenderFromMatch = std::get<2>(_textureTimeSeries.getVector()[*ind]);
                BOOST_LOG_TRIVIAL(info) << "frameToRenderFromMatch: " << *frameToRenderFromMatch;
            }
            else
            {
                BOOST_LOG_TRIVIAL(info) << "frame match not found" << _periodicity.count();
            }
        }
#endif
        BOOST_LOG_TRIVIAL(info) << "cur rotation offset: " << _curRoationOffset << std::endl;
        auto posInCurRotation = std::chrono::milliseconds ((refNow - _lastPeriodBegin + _curRoationOffset).count() % (_periodicity.count() - _curRoationOffset.count()));
        //BOOST_LOG_TRIVIAL(info) << "posInCurRotation: " << posInCurRotation.count() << std::endl;
        auto time_per_frame = (float)_periodicity.count() / (float)_textures.size();
        float relative_pos = (float)posInCurRotation.count() / ((float)_periodicity.count() - (float)_curRoationOffset.count());
        int frameToRenderRawNotWrapped = relative_pos * (float)_textures.size() + _zeroAnglePos;
        int frameToRenderRaw = frameToRenderRawNotWrapped;

        // to speed adjustment to avoid skips
        int speedCompPrevFrame;
        if (prevFrame ) 
        {
            int speedCompPrevFrame = * prevFrame;
            if (std::abs(frameToRenderRawNotWrapped - *prevFrame) > _textures.size() / 2) // roll over case
            {
                frameToRenderRaw = frameToRenderRawNotWrapped;
                if (speedCompPrevFrame < frameToRenderRaw)
                {
                    speedCompPrevFrame = *prevFrame + _textures.size();
                }
                else
                {
                    frameToRenderRaw += _textures.size();
                }
            }

            if (std::abs(frameToRenderRaw - speedCompPrevFrame) * time_per_frame > 500) // skips bigger 500 ms are seeked
            {
                BOOST_LOG_TRIVIAL(info) << "seeking to due to large time gap: " << std::abs(frameToRenderRaw - speedCompPrevFrame) * time_per_frame << std::endl;
            }
            else
            {
                int maxSkip = (1000.0 / 40.0) / time_per_frame; // preserve at least 30 FPS
                frameToRenderRaw = std::min(std::max(*prevFrame + 3, frameToRenderRaw), *prevFrame + maxSkip);
            }

        }








        int frameToRender = positive_mod(frameToRenderRaw, _textures.size());;




        //if (frameToRenderFromMatch)
        //    frameToRender = *frameToRenderFromMatch;
        
        BOOST_LOG_TRIVIAL(info) << "periodicity: " << _periodicity;
        BOOST_LOG_TRIVIAL(info) << "pos in period: " << posInCurRotation << " rel pos: " << relative_pos;
        BOOST_LOG_TRIVIAL(info) << "frame_to_render: " << frameToRender << " frame to render angel: " << std::get<1>(_textures[frameToRender]) << " rot angele: " << angle << " frame_as_per_time: " << frameToRenderRawNotWrapped << std::endl;
        if (prevFrame && frameToRender - *prevFrame > 10)
            BOOST_LOG_TRIVIAL(info) << "large frame skip from: " << *prevFrame << " to " << frameToRender << std::endl;
        prevFrame = frameToRender;
        return frameToRender;
    }
    else
    {
        if (prevFrame)
        {
            int frameToRender = (*prevFrame + 1) % _textures.size();
            BOOST_LOG_TRIVIAL(info) << "frame_to_render: " << frameToRender << std::endl;
            return frameToRender;
        }
        else
        {
            return 0;
        }
    }
}



#if 0
int OpenGLRenderer::findFrameToRender(std::optional<int> prevFrame, float angle, std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds> refNow)
{
    if (_angleMatchFrames)
    {

        auto posInCurRotation = refNow - _lastPeriodBegin + _curRoationOffset  + _transmissionDelay;
        //BOOST_LOG_TRIVIAL(info) << "posInCurRotation: " << posInCurRotation.count() << std::endl;
        auto time_frame_per_rot = (float)_periodicity.count() / (float)_textures.size();
        float relative_pos = (float)posInCurRotation.count() / ((float)_periodicity.count() - (float)_curRoationOffset.count());
        int frameAsPerTime = positive_mod(((int)(relative_pos * _textures.size()) + _zeroAnglePos), _textures.size());
        int frameAsPerTimeBuffered = frameAsPerTime - 0.1 * _textures.size(); // 20% buffer for this rotation deviationg, but not falling in the other half wave to find the same angle

        int searchWndBegin = prevFrame ? *prevFrame : frameAsPerTimeBuffered;
        int searchWndSize = 0.3 * _textures.size();

        std::vector<FrameMatch> frameMatches;

        for (int i = 0; i < searchWndSize; i++)
        {
            FrameMatch fm;
            int curIndex = positive_mod(searchWndBegin + i, _textures.size());
            std::get<0>(fm) = curIndex;
            std::get<1>(fm) = fabs(std::get<1>(_textures[curIndex]) - angle);
            //std::get<2>(fm) = std::abs(i - 18.0 / time_frame_per_rot);
            std::get<2>(fm) = std::abs(searchWndBegin + i - frameAsPerTime);

            frameMatches.push_back(fm);
        }


        std::sort(frameMatches.begin(), frameMatches.end(), [](const FrameMatch& a, const FrameMatch& b)
            {
                return distanceWeight(a) < distanceWeight(b);
            });

        int frameToRender = frameAsPerTime;//std::get<0>(frameMatches.front());
        BOOST_LOG_TRIVIAL(info) << "periodicity: " << _periodicity.count();
        BOOST_LOG_TRIVIAL(info) << "frame_to_render: " << frameToRender << " frame to render angel: " << std::get<1>(_textures[frameToRender]) << " rot angele: " << angle << " frame_as_per_time: " << frameAsPerTime << std::endl;
        if(prevFrame && frameToRender -  *prevFrame > 10 )
            BOOST_LOG_TRIVIAL(info) << "large frame skip from: " << *prevFrame << " to " << frameToRender  << std::endl;
        prevFrame = frameToRender;
        return frameToRender;
    }
    else
    {
        if (prevFrame)
        {
            int frameToRender = (*prevFrame + 1) % _textures.size();
            BOOST_LOG_TRIVIAL(info) << "frame_to_render: " << frameToRender << std::endl;
            return frameToRender;
        }
        else
        {
            return 0;
        }
    }
}
#endif



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

std::atomic<bool> OpenGLRenderer::_quit(false);


OpenGLRenderer::OpenGLRenderer(const std::string& fileName, int zeroAnglePos, bool fullscreen,
    float scale, std::chrono::milliseconds transmissionDelay, bool angleMatchFrames) : _fileName(fileName),
    _zeroAnglePos(zeroAnglePos),
    _fullscreen(fullscreen),
    _scale(scale),
    _transmissionDelay(transmissionDelay),
    _angleMatchFrames(angleMatchFrames)
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
    _renderThread.join();
    _frameMatchingThread.join();
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
        if (!loadMedia(_fileName) || _textures.empty()) {
            BOOST_LOG_TRIVIAL(info) << "Failed to load media!\n";
        }
        else {  
           
          
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
                        _quit = true;
                    }
                    // User presses a key
                    else if (e.type == SDL_KEYDOWN) {
                        switch (e.key.keysym.sym) {
                        case SDLK_q:
                            _quit = true;
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

                int frame_to_render = 0;
                auto ind = localTS.findIndex(now + _transmissionDelay);
                if (ind)
                {
                    angle = std::get<0>(localTS.getVector()[*ind]);
                    frame_to_render = findFrameToRender(prevFrame, angle, now + _transmissionDelay, localTS);

                }



                
                
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

                //BOOST_LOG_TRIVIAL(info) << "frame time: " << std::chrono::duration_cast<std::chrono::milliseconds>(ts_frame_end - ts_frame_begin).count() << std::endl;
            }
        }

    }
    glDisable(GL_TEXTURE_2D);
    // Free resources and close SDL
    close();
}

void OpenGLRenderer::blockingMainLoop()
{
    
    while (!_quit) {
        
        SDL_Delay(200);
    }
}

void OpenGLRenderer::render(std::function<void(const std::string, TimeSeries&)> monitor)
{

    //_renderThread = std::thread([this, monitor]() {
    //    renderThread(monitor);
    //    });
    renderThread(monitor);


}