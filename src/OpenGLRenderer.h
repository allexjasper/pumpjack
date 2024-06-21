#pragma once
#include "Renderer.h"


#include <atomic>
#include <functional>
#include <thread>

struct SDL_Texture;
namespace gli
{
	class texture;
}

class OpenGLRenderer : public Renderer
{
public:
	OpenGLRenderer(const std::string& fileName, int zeroAnglePos, bool fullscreen, float scale, std::chrono::milliseconds transmissionDelay, bool angleMatchFrames);
	virtual ~OpenGLRenderer();

public:
	virtual void feedData(const TimeSeries& ts, std::chrono::milliseconds periodicity, TimeSeries::Timestamp, std::chrono::milliseconds, const std::string& overlay);
	void shutdown();
	void render(std::function<void(const std::string, TimeSeries&)> monitor);
	void renderThread(std::function<void(const std::string, TimeSeries&)> monitor);
	static void blockingMainLoop();
	typedef std::tuple<std::string, float, int> FrameInfo;
	typedef std::tuple<gli::texture*, float, int, int> TextureInfo;

private:
	void renderThread();
	void frameMatchtingThread();
	bool init();
	bool loadMedia(const std::string& directory);
	void close();
	std::vector<OpenGLRenderer::FrameInfo> getFilesSorted(const std::string& directory);
	int findFrameToRender(std::optional<int> prevFrame, float angle, std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds> ts, TimeSeries& localTS);
	void CreateTexture(const OpenGLRenderer::FrameInfo& info);
	void renderQuad(int textureID, int width, int height, float angle);
	TimeSeries createTextureTimeSeries(const std::vector<OpenGLRenderer::TextureInfo>& textures, 
						std::chrono::milliseconds totalDuration, TimeSeries::Timestamp startTime);


public:
	std::atomic<bool> _shutdownRequested;
	static std::atomic<bool> _quit;
	std::thread _renderThread;
	std::thread _frameMatchingThread;
	static TimeSeries _curTimeSeries;
	static std::mutex _mutTimeSeries;
	bool _fullscreen;
	float _scale;
	std::chrono::milliseconds _transmissionDelay;
	std::string _fileName;
	int _zeroAnglePos;
	float screenWidth;
	float screenHeight;
	bool _angleMatchFrames;

	TimeSeries _textureTimeSeries;
	std::mutex _mutTextureTimeSeries;
	
	
	std::vector<TextureInfo> _textures;
	TimeSeries::Timestamp _lastPeriodBegin;
	std::chrono::milliseconds _curRoationOffset;
	std::chrono::milliseconds _periodicity;

};