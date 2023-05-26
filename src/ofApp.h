#pragma once

/*

	WIP Example

	TODO

	textInput bubble
		fix clear. make button bigger
		add wait spin

	ui docking
		add common menus to addon (exit, full screen, )
		use modes like: conversation, prompt selector etc
		for different layouts
	gpt setup/restart, reconnect.
		make new class SurfGPT.
		model and error/state

	conversation window
		auto scroll down when response (tween)
		lock mode
		auto go to text input when get response
		make bigger scroll

	fix < > slides
		fix 1st/end slide.
	add dual window 1: gui / 2: out
	fix subtitle paragraph clamp. reset box centered
	add prompts manager:
		save on json file.
		create prompt struct
		add new. edit.

*/


//--

// Optional Modules
//#define USE_WHISPER
//#define USE_EDITOR_INPUT
#define USE_EDITOR_RESPONSE
#define USE_SURF_TTF

//--

#include "ofMain.h"

#include "ofxSurfingTextSubtitle.h"
#include "ofxSurfingImGui.h"
#include "surfingTextEditor.h"
#include "BigTextInput.h"
#include "ChatThread.h"
#include "surfingSceneTesters.h"

#include "Spinners2.h"
//#include "imspinner.h"

#include "ofxWindowApp.h"

#ifdef USE_WHISPER
#include "surfingWhisper.h"
#endif
#ifdef USE_SURF_TTF
#include "ofxSurfingTTS.h"
#endif

#include <curl/curl.h>

class ofApp : public ofBaseApp
{
public:
	void setup();
	void update();
	void draw();
	void drawBg();
	void exit();
	void keyPressed(int key);
	void startup();
	void setupSounds();

	ofParameter<bool> bGui;
	ofParameterGroup params{ "ofApp" };
	void Changed_Params(ofAbstractParameter& e);

	ofxSurfingGui ui;
	void drawImGui();
	void drawImGuiMain();
	//void drawImGuiReply(ofxSurfingGui& ui);
	void drawImGuiConversation(ofxSurfingGui& ui);
	bool bResetWindowConversation = 0;

	bool bFlagGoBottom = 0;

	ofxSurfingTextSubtitle subs;
	string path;
	void doPopulateText(string s = "");
	void doPopulateTextBlocks();
	void doClearSubsList();

	ChatThread chatGpt;
	void setupGpt();
	string pathGptSettings = "GptChat_ConfigKey.json";
	void doGptSendMessage(string message);
	void doGptRegenerate();
	void doGptGetMessage();
	ofParameter<bool> bWaitingGpt{ "GPT WAITING", 0 };
	// Error codes for various error conditions.
	vector<string> errorCodesNames{
			"Success",//0
			"InvalidAPIKey",
			"NetworkError",
			"ServerError",
			"RateLimitExceeded",
			"TokenLimitExceeded",
			"InvalidModel",
			"BadRequest",
			"Timeout",//8
			"UnknownError"
	};
	ofParameter<int> indexErrorCode{ "State", 9, 0, errorCodesNames.size() - 1 };
	bool bGptError = false;
	int getErrorCodeByCode(ofxChatGPT::ErrorCode e);
	string gptErrorMessage = "";

	// The used server sometimes requires some IP reseting.
	bool doGptResetEndpointIP();

	ofParameter<string> apiKey{ "API key","" };
	ofParameter<string> model{ "Model","" };
	ofParameter<bool> bModeConversation{ "Conversation", false };

	ofJson jConversationHistory = ofJson();
	ofParameter<bool> bGui_GptConversation{ "GPT Conversation",false };
	ofParameter<int> sizeFontConv{ "SizeFontConv", 0, 0, 3 };
	ofParameter<bool> bLastBigger{ "LastBigger", false };
	ofParameter<bool> bLastBlink{ "LastBlink", false };

	ofParameter<bool> bModeOneSlide{ "OneSlide", false };
	ofParameter<bool> bLock{ "Lock", false };

	// Text input bubble
	BigTextInput bigTextInput;
	void doAttendCallbackTextInput();
	ofEventListener eTextInput;
	ofParameter<string> textInput{ "TextInput", "" };
	void doAttendCallbackClear();

	// Text Editors
#ifdef USE_EDITOR_INPUT
	SurfingTextEditor editorInput;
#endif
#ifdef USE_EDITOR_RESPONSE
	SurfingTextEditor editorResponse;
#endif

	//ofParameter<int> fontR{ "FontR", 0, 0, 3 };
	//ofParameter<bool> bGui_GptLastReply{ "GPT Last Reply",false };

	void drawWidgetsEditor(); // Advanced: inserted widgets

	string textLastResponse;

	ofJson jQuestion;
	ofJson jResponse;

	string strBandname;

	//--

	void doClear();

	void doRandomInput();

	//--

	void setupGptPrompts();
	void doSwapGptPrompt();

	string strPrompt;
	ofParameter<int> indexPrompt{ "Prompt", 0, 0, 3 };
	string promptName;
	vector<pair<string, string> > prompts;
	vector<string> promptsNames;
	void setGptPrompt(int index);

	// "10 short sentences."
	static string GPT_Prompt_0() {
		return R"(I want you to act as a music band advertiser. 
You will create a campaign to promote that band.
That campaign consists of 10 short sentences.
These sentences must define the band's career highlights, 
the best albums or the more important musicians members.
The sentences will be short: less than 5 words each sentence.
)";
	}

	// "10 words list."
	static string GPT_Prompt_1() {
		return R"(I want you to act as a music band critic. 
I will pass you a band music name. You will return a list of 10 words.
You will only reply with that words list, and nothing else. 
Words will be sorted starting from less to more relevance.
The format of the response, will be with one line per each word.
These lines will be starting with the first char uppercased, 
and without a '.' at the end of the line, just include the break line char.
)";
	}

	// "10 Similar bands"
	static string GPT_Prompt_2() {
		return R"(I want you to act as a music critic.
As a LastFm maintainer.
I will give you a band name. You will list the 10 more similar bands.
You will only reply that band names list, and nothing else. 
But you must sort that bands, from older to newer. 
)";
	}

	// "Default"
	static string GPT_Prompt_3() {
		return R"(Act as your default ChatGPT behavior following the conversation.
)";
	}

	//--

	// Bg flash. set to 1 to start.
	float v = 0;

	ofParameter<int>typeSpin{ "typeSpin", 8, 0, 9 };

	vector<ofSoundPlayer> sounds;

	//--

#ifdef USE_WHISPER
	surfingWhisper whisper;
	void doUpdatedWhisper();
	void drawImGuiWidgetsWhisper();
#endif

#ifdef USE_SURF_TTF
	ofxSurfingTTS TTS;
#endif

	//--

	ofxWindowApp w;
};
