#include "Speedometer.h"
#include "bakkesmod\wrappers\includes.h"
#include "bakkesmod\wrappers\arraywrapper.h"

using namespace std;

BAKKESMOD_PLUGIN(Speedometer, "Show car speed in freeplay", "1.3", PLUGINTYPE_FREEPLAY)




/*
TO-DO
	- Show BMGraph of speed over time

	- ITS DRAWING IN SPECTATOR ONLINE EVEN WITH NO TARGET
*/





void Speedometer::onLoad()
{
	showSpeed = make_shared<bool>(false);
	showThrottle = make_shared<bool>(false);
	showSpectate = make_shared<bool>(false);
	showValues = make_shared<bool>(false);
	useMetric = make_shared<bool>(false);
	cvarManager->registerCvar("Speedometer_Speed", "1", "Show car speed").bindTo(showSpeed);
	cvarManager->registerCvar("Speedometer_Throttle", "1", "Show throttle input").bindTo(showThrottle);
	cvarManager->registerCvar("Speedometer_Spectator", "1", "Show while spectating").bindTo(showSpectate);
	cvarManager->registerCvar("Speedometer_Values", "0", "Show numerical values").bindTo(showValues);
	cvarManager->registerCvar("Speedometer_Metric", "0", "Show speed in KPH or MPH").bindTo(useMetric);

	prefPosition = make_shared<float>(0.f);
	prefWidth = make_shared<float>(0.f);
	prefOpacity = make_shared<float>(0.f);
	cvarManager->registerCvar("Speedometer_Position", "90", "Vertical position of speedometer", true, true, 0, true, 100).bindTo(prefPosition);
	cvarManager->registerCvar("Speedometer_Width", "60", "Width of speedometer", true, true, 0, true, 100).bindTo(prefWidth);
	cvarManager->registerCvar("Speedometer_Opacity", "100", "Opacity of speedometer", true, true, 0, true, 1).bindTo(prefOpacity);

	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.ReplayPlayback.BeginState", [&](std::string eventName){isInGoalReplay = true;});
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.ReplayPlayback.EndState", [&](std::string eventName){isInGoalReplay = false;});

	gameWrapper->RegisterDrawable(bind(&Speedometer::Render, this, std::placeholders::_1));
}
void Speedometer::onUnload(){}


void Speedometer::Render(CanvasWrapper canvas)
{
	bool freeplay = gameWrapper->IsInFreeplay();
	bool training = gameWrapper->IsInCustomTraining();
	bool online = gameWrapper->IsInOnlineGame();
	bool replay = gameWrapper->IsInReplay();

	if(!freeplay && !training && !online && !replay) return;//make sure its in a valid gamestate first
	if(online && isInGoalReplay) return;//return if camera is in goal replay
	CameraWrapper camera = gameWrapper->GetCamera();
	if(camera.IsNull()) return;
	if(!*showSpeed && !*showThrottle) return;//return if user doesnt want to draw either graph
	if(!*showSpectate)
	{
		if(replay || online)
			return;//return if user doesnt want it to work when spectating
	}
	else
	{
		if(online && !gameWrapper->GetLocalCar().IsNull())
			return;//only allow it in online if player is spectating (controllable car does not exist)
	}
	PriWrapper target = PriWrapper(reinterpret_cast<std::uintptr_t>(gameWrapper->GetCamera().GetViewTarget().PRI));
	if(target.IsNull()) return;
	CarWrapper car = target.GetCar();
	if(car.IsNull()) return;


	float SCREENWIDTH = canvas.GetSize().X;
	float SCREENHEIGHT = canvas.GetSize().Y;
	float HALFWIDTH = SCREENWIDTH/2.f;

	float positionPerc = *prefPosition / 100.f;
	float widthPerc = *prefWidth / 100.f;

	float relativeHeight = SCREENHEIGHT / 1080.f;
	float boxHeight = 40.f * relativeHeight;

	Vector2 startPosition = {(int)(HALFWIDTH - HALFWIDTH * widthPerc), (int)(SCREENHEIGHT * positionPerc)};
	Vector2 boxSize = {(int)(SCREENWIDTH * widthPerc), boxHeight};

	if(*showSpeed)
	{
		Vector velocity = car.GetVelocity();
		float speed = velocity.magnitude();
		float speedPerc = speed/2300.f;
		const float supersonicIn = 2200.f/2300.f;
		const float supersonicOut = 2100.f/2300.f;
		const float engineOut = 1410.f/2300.f;

		if(speed >= 2200)
			wasSuperSonic = true;
		else
		{
			if(wasSuperSonic && (speed >= 2100 && speed < 2200))
				timeStartedBuffer = chrono::steady_clock::now();
			wasSuperSonic = false;
		}
		chrono::duration<double> bufferDuration = chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - timeStartedBuffer);
		float bufferPerc = 1 - bufferDuration.count();
		if(bufferPerc < 0 || speed < 2100) bufferPerc = 0;

		//draw base color
		canvas.SetColor(255,255,255,(char)(100**prefOpacity));
		canvas.SetPosition(startPosition);
		canvas.FillBox(boxSize);

		//draw label
		canvas.SetColor(255,255,255,(char)(255**prefOpacity));
		canvas.SetPosition(startPosition.minus(Vector2{60,(int)(-10.f * relativeHeight)}));
		canvas.DrawString("Speed");
		if(*showValues)
		{
			canvas.SetPosition(Vector2{startPosition.X + boxSize.X + 5, startPosition.Y + (int)(10.f * relativeHeight)});
			if(*useMetric)
				canvas.DrawString(to_string((int)(speed* 0.036)) + " KPH");//KPH
			else
				canvas.DrawString(to_string((int)(speed/44.704)) + " MPH");//MPH
		}

		//draw speed amount
		if(car.GetbSuperSonic())
			canvas.SetColor(255,255,50,(char)(255**prefOpacity));
		else
			canvas.SetColor(255,50,50,(char)(255**prefOpacity));
		canvas.SetPosition(startPosition);
		canvas.FillBox(Vector2{(int)((float)boxSize.X * speedPerc), boxSize.Y});

		//draw outline
		canvas.SetColor(255,255,255,(char)(255**prefOpacity));
		canvas.SetPosition(startPosition.minus(Vector2{1,1}));
		canvas.DrawBox(boxSize.minus(Vector2{-2,-2}));

		//draw supersonic thresholds
		int XposIn = startPosition.X + (float)boxSize.X * supersonicIn;
		int XposOut = startPosition.X + (float)boxSize.X * supersonicOut;
		canvas.DrawLine(Vector2{XposIn, startPosition.Y}, Vector2{XposIn, startPosition.Y + (int)boxHeight}, 2.f);
		canvas.DrawLine(Vector2{XposOut, startPosition.Y}, Vector2{XposOut, startPosition.Y + (int)boxHeight}, 2.f);

		//draw engine threshold
		int XposEngine = startPosition.X + (float)boxSize.X * engineOut;
		canvas.DrawLine(Vector2{ XposEngine, startPosition.Y }, Vector2{ XposEngine, startPosition.Y + (int)boxHeight }, 2.f);

		//draw supersonic buffer timer
		//base color
		Vector2 bufferTimerSize = {startPosition.X + boxSize.X - XposOut + 2, (int)(boxHeight/2)};
		Vector2 bufferTimerStart = Vector2{startPosition.X + boxSize.X - bufferTimerSize.X + 1, startPosition.Y - bufferTimerSize.Y + 1};
		canvas.SetColor(255,255,255,(char)(100**prefOpacity));
		canvas.SetPosition(bufferTimerStart);
		canvas.FillBox(bufferTimerSize);

		canvas.SetColor(255,50,50,(char)(255**prefOpacity));
		canvas.SetPosition(bufferTimerStart);
		canvas.FillBox(Vector2{(int)((float)bufferTimerSize.X * bufferPerc), bufferTimerSize.Y});

		canvas.SetColor(255,255,255,(char)(255**prefOpacity));
		canvas.SetPosition(bufferTimerStart);
		canvas.DrawBox(bufferTimerSize);
	}
	if(*showThrottle)
	{
		ControllerInput inputs = car.GetInput();
		startPosition.Y += (int)(50.f * relativeHeight);
		Vector2 newStartPosition = {HALFWIDTH, startPosition.Y};

		//draw base color
		canvas.SetColor(255,255,255,(char)(100**prefOpacity));
		canvas.SetPosition(startPosition);
		canvas.FillBox(boxSize);

		//draw label
		canvas.SetColor(255,255,255,(char)(255**prefOpacity));
		canvas.SetPosition(startPosition.minus(Vector2{60,(int)(-10.f * relativeHeight)}));
		canvas.DrawString("Throttle");
		if(*showValues)
		{
			canvas.SetPosition(Vector2{startPosition.X + boxSize.X + 5, startPosition.Y + (int)(10.f * relativeHeight)});
			canvas.DrawString(to_string((int)(inputs.Throttle*100)));
		}

		//draw throttle amount
		if(inputs.Throttle > 0.f)
		{
			canvas.SetColor(50,255,50,(char)(255**prefOpacity));
			canvas.SetPosition(newStartPosition);
			canvas.FillBox(Vector2{(int)((float)boxSize.X * (inputs.Throttle / 2)), boxSize.Y});
		}
		else
		{
			canvas.SetColor(50,50,255,(char)(255**prefOpacity));
			float fillWidth = (float)boxSize.X * (abs(inputs.Throttle / 2));
			canvas.SetPosition(newStartPosition.minus(Vector2{(int)fillWidth,0}));
			canvas.FillBox(Vector2{(int)fillWidth, boxSize.Y});
		}

		//draw outline
		canvas.SetColor(255,255,255,(char)(255**prefOpacity));
		canvas.SetPosition(startPosition.minus(Vector2{1,1}));
		canvas.DrawBox(boxSize.minus(Vector2{-2,-2}));
		canvas.DrawLine(Vector2{(int)HALFWIDTH, startPosition.Y}, Vector2{(int)HALFWIDTH, startPosition.Y + (int)boxHeight}, 2.f);
	}
}
