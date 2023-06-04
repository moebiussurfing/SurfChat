#pragma once

/*

	TODO

	add preset

	make a class for main gpt methods

	add post linke withoput request gpt

	fix threads for tts/ learn with color quatizer

	add tokens for longer replay

	ui docking
		add common menus to addon (exit, full screen, )
		use modes like: conversation, prompt selector etc
		for different layouts

	add ui menus: full screen, copy

	scroll tween when added new messages

	gpt setup/restart, reconnect.
		make new class SurfGPT.
		model and error/state

	add temperature, tokens..

	fix < > slides
		fix 1st/end slide.
	add dual window 1: gui / 2: out
	add prompts manager:
		save on json file.
		create prompt struct
		add new. edit.

	catch reg/numers like 1. 2. and convert to buttons
		to click and re submit as a new request

*/


//--

// Optional Modules
#define USE_OFX_ELEVEN_LABS
//#define USE_SURF_SUBTITLES
//#define USE_EDITOR_INPUT
#define USE_EDITOR_RESPONSE
//#define USE_WHISPER
#define USE_PRESETS

//--

#include "ofMain.h"

// //TODO:
// #define IMGUI_DEFINE_MATH_OPERATORS
// #include "imgui_demo.cpp"

#include "ChatThread.h"

#include "ofxSurfingHelpers.h"
#include "ofxAutosaveGroupTimer.h"
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

#ifdef USE_OFX_ELEVEN_LABS
#include "ofxElevenLabs.h"
#endif

#include "ofxWindowApp.h"
#include "surfingSounds.h"

#ifdef USE_PRESETS
#include "ofxSurfingPresetsLite.h"
#endif

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
    void setupInputText();
    void startup();
    void startupDelayed();
    void drawBg();

    ofxSurfingGui ui;

    ofParameter<bool> bGui;
    ofParameterGroup params{"surfCHAT"};
    void Changed_Params(ofAbstractParameter& e);

    void drawImGui();
    void drawImGuiMain();
    void drawImGuiGpt2();
    void drawImGuiGpt1();
    void drawImGuiConversation(ofxSurfingGui& ui);
bool bIsInteractingWindowConversation=0;
    bool bRefreshWindowConversation = 0;
    void doRefreshWindowConversationIfFlagged();

    bool bFlagGoBottom = 0;
    bool bFlagGoTop = 0;
    //TODO: centralize commands. same approach
    //use home/end keys for top/bottom full
    bool bFlagGoBottomStp = 0;
    bool bFlagGoTopStp = 0;
    bool bFlagGoBottomPage = 0;
    bool bFlagGoTopPage = 0;

    //--

    string textLastResponse;
    // ofJson jQuestion;
    ofJson jResponse;

    string textLastTrick; //tricks could be jokes or summarize spceial commands.
    bool bLastWasATrick = 0;

    vector<string> textHistory;
    int i_hist = 0;

    //--

    void doSendSilentMessageToConversation(string message);
    // Not sent to gpt just to chat window to notify some commands to the user
    
    ChatThread chatGpt;
    void doGptRestart();
    void setupGpt(bool bSilent = 0);
    string pathGptSettings = "GptChat_ConfigKey.json";
    void doGptSendMessage(string message);
    void doGptRegenerate();
    void doGptResend();
    void doGptGetMessage();
    ofParameter<bool> bWaitingGpt{"GPT WAITING", 0};
    // Error codes for various error conditions.
    vector<string> errorCodesNames{
        "Success", //0
        "InvalidAPIKey",
        "NetworkError",
        "ServerError",
        "RateLimitExceeded",
        "TokenLimitExceeded",
        "InvalidModel",
        "BadRequest",
        "Timeout", //8
        "UnknownError"
    };
    ofParameter<int> indexErrorCode{"State", 9, 0, errorCodesNames.size() - 1};
    bool bGptError = false;
    int getErrorCodeByCode(ofxChatGPT::ErrorCode e);
    string gptErrorMessage = "";

    vector<string> modelsNames;
    ofParameter<int> indexModel{"IndexModel", 0, 0, 0};

    // The used server sometimes requires some IP reseting.
    bool doGptResetEndpointIP();

    ofParameter<string> apiKey{"API key", ""};
    ofParameter<string> model{"Model", ""};
    ofParameter<bool> bModeConversation{"Conversation", false};

    ofParameterGroup paramsConversations{"Conversations"};
    ofJson jConversationHistory = ofJson();
    ofParameter<bool> bGui_GptConversation{"GPT Conversation", false};
    ofParameter<int> sizeFontConv{"SizeFontConv", 0, 0, 3};
    ofParameter<bool> bLastBigger{"LastBigger", false};
    ofParameter<bool> bLastBlink{"LastBlink", false};

    ofParameter<bool> bModeOneSlide{"OneSlide", false}; //for subtitles module
    ofParameter<bool> bLock{"Lock", false};

    void doGptDoAJoke(); //do a joke related to last text
    void doGptDoASummarization(); //summarize last text in 6 words

    //--

    // Text input bubble
    BigTextInput bigTextInput;
    void doAttendCallbackTextInput();
    ofEventListener eTextInput;
    ofParameter<string> textInput{"TextInput", ""};
    void doAttendCallbackClear();
    void doAttendCallbackKeys();
    void drawWidgetsToTextInput();
    ofParameter<float> spacingY{"SpacingY", 0, 0, 1};
    ofParameter<float> spacingX{"SpacingX", 0, 0, 1};
    //spacing between text input and window conversation
    ofParameter<bool> bBottomTextInput{"Bottom", false};
    void drawWidgetsContextMenu1();
    void drawWidgetsContextMenu2();
    ofParameter<bool> bGui_WindowContextMenu;

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

    void doClear(bool bSilent = 0);
    void doClearAll();

    //--

    // Prompts and Roles
    void setupGptPrompts();
    void doRefreshGptPrompts();
    void doGptSetPrompt(int index);
    void doSwapGptPrompt(); //next
    ofParameterGroup paramsPrompts{"Prompts"};
    string strPrompt;
    ofParameter<int> indexPrompt{"IndexPrompt", 0, 0, 0};
    string promptName;
    vector<string> promptsNames;
    vector<string> promptsContents;
    ofParameter<int> amountResultsPrompt{"Results", 10, 1, 100};
    vector<string> tagsNames{
        "Music Band", "Book Writer", "Film director", "Film Actor", "Film Title", "Illustrator", "Painter",
        "Philosopher"
    };
    ofParameter<int> indexTags{"IndexTag", 0, 0, tagsNames.size() - 1};
    string tagWord = "-1";

    // Roles (system prompts)

    // Default
    static string doCreateGptRolePrompt0()
    {
        return string("Act as your default ChatGPT behavior \nfollowing the conversation.");
    }

    // Words
    string doCreateGptRolePrompt1()
    {
        string s0 = "From now on, I want you to act as a " + tagWord + " critic.\n";
        s0 += "I will pass you a " + tagWord + " name.";
        string s1 = "You will return a list of " + ofToString(amountResultsPrompt.get()) + " words.\n";
        string s2 =
            "You will only reply with that words list, and nothing else, no explanations. \nWords will be sorted starting from less to more relevance. \n";
        s2 +=
            "The format of the response, will be with one line per each word. \nThese lines will be starting with the first char uppercased, \n";
        //s2 += "and without a '.' at the end of the line, just include the break line char.";
        s2 += "start each line with a number and a '.' starting at '1.'";
        return string(s0 + s1 + s2);
    }

    // Sentences
    int max_words = 7;

    string doCreateGptRolePrompt2()
    {
        string s0 = "From now on, I want you to act as a " + tagWord + " advertiser.\n";
        string s1 = "You will create a campaign to promote that " + tagWord + "\n";
        s1 += "That campaign consists of " + ofToString(amountResultsPrompt.get()) + " short sentences.\n";
        s1 += "These sentences must define " + tagWord +
            "'s career highlights, \nthe best edited releases or the more important  members in case the authors worked in collaboration of many members or as collective.\n";
        string s2 = "The sentences will be short: less than " + ofToString(max_words) + " words each sentence.";
        return string(s0 + s1 + s2);
    }

    // Other similar
    string doCreateGptRolePrompt3()
    {
        string s0 = "From now on, I want you to act as a " + tagWord + " critic.\n";
        s0 += "I will pass you a " + tagWord + " name. ";
        string s1 = "You will return a list of " + ofToString(amountResultsPrompt.get()) + " names of similar " +
            tagWord + "s creators.\n";
        string s2 =
            "You will only reply with that words list, and nothing else, no explanations. \nWords will be sorted starting from less to more relevance. \n";
        s2 +=
            "The format of the response, will be with one line per each word. \nThese lines will be starting with the first char uppercased, \n";
        //s2 += "and without a '.' at the end of the line, just include the break line char.";
        s2 += "start each line with a number and a '.' starting at '1.'";
        return string(s0 + s1 + s2);
    }

    //--

    SurfingSounds sounds;

    // Bg flash. set to 1 to start.
    float v = 0;

    ofParameter<int> typeSpin{"typeSpin", 0, 0, 0};

    //--

#ifdef USE_OFX_ELEVEN_LABS
    ofxElevenLabs tts;
    void drawImGuiElebenLabs1();
    void drawImGuiElebenLabs2();
    void drawImGuiElebenLabsExtras();
#endif

    //--

    ofParameter<ofColor> colorBg{"ColorBg", ofColor::grey, ofColor(), ofColor()};
    ofParameter<ofColor> colorAccent{"ColorBubbleAccent", ofColor::grey, ofColor(), ofColor()};
    ofParameter<ofColor> colorUser{"ColorTxtUser", ofColor::grey, ofColor(), ofColor()};
    ofParameter<ofColor> colorAssistant{"ColorTxtAssistant", ofColor::grey, ofColor(), ofColor()};

    // Tester
    string strBandname;
    void doRandomInput();

    bool bDoneStartup = 0;
    bool bDoneStartupDelayed = 0;

    void doResetAll(bool bSilent = 0);

    ofxWindowApp w;
    ofxAutosaveGroupTimer g;

    void windowResized(ofResizeEventArgs& resize);

    void doRefreshWindowConversation();
    void doResetWindowConversation();

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

#ifdef USE_PRESETS
    ofxSurfingPresetsLite presets;
    ofParameterGroup paramsPreset;
#endif
};
