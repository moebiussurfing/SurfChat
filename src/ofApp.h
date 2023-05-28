#pragma once

/*

	WIP Example

	TODO

	scroll tween

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
//#define USE_SURF_TTF
//#define USE_SURF_SUBTITLES

//--

#include "ofMain.h"

#include "ChatThread.h"

#include "ofxSurfingHelpers.h"
#include "surfingStrings.h"
#include "surfingSceneTesters.h"

#include "ofxSurfingImGui.h"
#include "BigTextInput.h"
#include "Spinners2.h"
#include "surfingTextEditor.h"

#ifdef USE_SURF_SUBTITLES
#include "ofxSurfingTextSubtitle.h"
#endif

#ifdef USE_WHISPER
#include "surfingWhisper.h"
#endif

#ifdef USE_SURF_TTF
#include "ofxSurfingTTS.h"
#endif

#include "ofxWindowApp.h"

#include <curl/curl.h>

#include <functional>
using callback_t = std::function<void()>;

class ofApp : public ofBaseApp
{

public:
	void setup();
	void update();
	void draw();
	void exit();
	void keyPressed(int key);

	void setupParams();
	void setupSounds();
	void startup();
	void startupDelayed();
	void drawBg();

	ofParameter<bool> bGui;
	ofParameterGroup params{ "surfCHAT" };
	void Changed_Params(ofAbstractParameter& e);

	ofxSurfingGui ui;
	void drawImGui();
	void drawImGuiMain();
	void drawImGuiConversation(ofxSurfingGui& ui);

	bool bResetWindowConversation = 0;
	bool bFlagGoBottom = 0;

	//--

	string textLastResponse;
	ofJson jQuestion;
	ofJson jResponse;

	vector<string> textHistory;

	//--

	ChatThread chatGpt;
	void setupGpt();
	string pathGptSettings = "GptChat_ConfigKey.json";
	void doGptSendMessage(string message);
	void doGptRegenerate();
	void doGptResend();
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

	ofParameterGroup paramsConversations{ "Conversations" };
	ofJson jConversationHistory = ofJson();
	ofParameter<bool> bGui_GptConversation{ "GPT Conversation",false };
	ofParameter<int> sizeFontConv{ "SizeFontConv", 0, 0, 3 };
	ofParameter<bool> bLastBigger{ "LastBigger", false };
	ofParameter<bool> bLastBlink{ "LastBlink", false };

	ofParameter<bool> bModeOneSlide{ "OneSlide", false };
	ofParameter<bool> bLock{ "Lock", false };

	//--

	// Text input bubble
	BigTextInput bigTextInput;
	void doAttendCallbackTextInput();
	ofEventListener eTextInput;
	ofParameter<string> textInput{ "TextInput", "" };
	void doAttendCallbackClear();
	void drawWidgetsToTextInput()
	{
		//ui.SameLine();
		//ui.AddSpacingY(10);
		ui.Add(bGui, OFX_IM_TOGGLE_ROUNDED_MINI_XS);
	}

	//--

	// Text Editors
#ifdef USE_EDITOR_INPUT
	SurfingTextEditor editorInput;
#endif
#ifdef USE_EDITOR_RESPONSE
	SurfingTextEditor editorLastResponse;
#endif

#ifdef USE_EDITOR_INPUT
	void drawWidgetsEditor(); // Advanced: inserted widgets
#endif

	//--

	void doClear();

	//--

	// Prompts and Roles
	void setupGptPrompts();
	void doSetGptPrompt(int index);
	void doSwapGptPrompt();//next
	ofParameterGroup paramsPrompts{ "Prompts" };
	string strPrompt;
	ofParameter<int> indexPrompt{ "IndexPrompt", 0, 0, 3 };
	string promptName;
	vector<string> promptsNames;
	vector<string> promptsContents;
	ofParameter<int> amountResultsPrompt{ "Amount", 10, 1, 100 };
	vector<string> tags{ "music band", "novelist", "screenwriter", "film director" };
	ofParameter<int> indexTagWord{ "Tag", 0, 0, tags.size() - 1 };
	ofParameter<string> tagWord{ "Tag Word", "music band" };

	// Roles (system prompts)

	// "Default"
	static string doCreateGptRolePrompt0() {
		return string("Act as your default ChatGPT behavior \nfollowing the conversation.");
	}

	// "Short sentences from an advertiser."
	string doCreateGptRolePrompt1() {
		string s0 = "From now on, I want you to act as a " + tagWord.get() + " advertiser.\n";
		string s1 = "You will create a campaign to promote that " + tagWord.get() + "\n";
		string s2 = "That campaign consists of " + ofToString(amountResultsPrompt.get()) + " short sentences.\n";
		s2 += "These sentences must define " + tagWord.get() + "'s career highlights, the best edited releases or the more important  members in case the authors worked in collaboration of many members or as collective.\n";
		s2 += "The sentences will be short: less than 5 words each sentence.";
		return string(s0 + s1 + s2);
	}

	// "Words list from a critic."
	string doCreateGptRolePrompt2() {
		string s0 = "I want you to act as a " + tagWord.get() + " critic. I will pass you a " + tagWord.get() + " name.\n";
		string s1 = "You will return a list of " + ofToString(amountResultsPrompt.get()) + " words.\n";
		string s2 = R"(You will only reply with that words list, and nothing else. Words will be sorted starting from less to more relevance.
The format of the response, will be with one line per each word. These lines will be starting with the first char uppercased, 
and without a '.' at the end of the line, just include the break line char.)";
		return string(s0 + s1 + s2);
	}

	// "Similar authors from a critic."
	string doCreateGptRolePrompt3() {
		string s0 = "I want you to act as a " + tagWord.get() + " critic. I will pass you a " + tagWord.get()
			+ " name. ";
		string s1 = "You will return a list of " + ofToString(amountResultsPrompt.get()) + " words.\n";
		string s2 = R"(You will only reply with that words list, and nothing else, no explanations. 
Words will be sorted starting from less to more relevance. 
The format of the response, will be with one line per each word. These lines will be starting with the first char uppercased, 
and without a '.' at the end of the line, just include the break line char.)";
		return string(s0 + s1 + s2);
	}

	//--

	// Bg flash. set to 1 to start.
	float v = 0;

	ofParameter<int>typeSpin{ "typeSpin", 8, 0, 9 };

	vector<ofSoundPlayer> sounds;

	//--

#ifdef USE_SURF_SUBTITLES
	ofxSurfingTextSubtitle subs;
	string path;
	void doPopulateText(string s = "");
	void doPopulateTextBlocks();
	void doClearSubsList();
#endif

#ifdef USE_WHISPER
	surfingWhisper whisper;
	void doUpdatedWhisper();
	void drawImGuiWidgetsWhisper();
#endif

#ifdef USE_SURF_TTF
	ofxSurfingTTS TTS;
#endif

	//--

	ofParameter<ofColor> colorBg{ "ColorBg", ofColor::grey, ofColor(), ofColor() };
	ofParameter<ofColor> colorAccent{ "ColorAccent", ofColor::grey, ofColor(), ofColor() };
	ofParameter<ofColor> colorTxt{ "ColorText", ofColor::grey, ofColor(), ofColor() };

	//// Tester
	//string strBandname;
	//void doRandomInput();

	//ofParameter<int> fontR{ "FontR", 0, 0, 3 };
	//ofParameter<bool> bGui_GptLastReply{ "GPT Last Reply",false };

	bool bDoneStartup = 0;
	bool bDoneStartupDelayed = 0;

	void doReset();

	ofxWindowApp w;
};
