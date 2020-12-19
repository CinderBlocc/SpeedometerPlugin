#pragma once
#pragma comment(lib, "PluginSDK.lib")
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include <chrono>

class Speedometer : public BakkesMod::Plugin::BakkesModPlugin
{
private:
	bool isInGoalReplay = false;
	bool wasSuperSonic = false;
	std::chrono::steady_clock::time_point timeStartedBuffer;
	std::shared_ptr<bool>  showSpeed;
	std::shared_ptr<bool>  showThrottle;
	std::shared_ptr<bool>  showSpectate;
	std::shared_ptr<bool>  showValues;
	std::shared_ptr<bool>  useMetric;
	std::shared_ptr<float> prefPosition;
	std::shared_ptr<float> prefWidth;
	std::shared_ptr<float> prefOpacity;

public:
	void onLoad() override;
	void onUnload() override;

	void Render(CanvasWrapper canvas);
};