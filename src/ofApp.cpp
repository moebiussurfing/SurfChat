#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{
#if 1
	//w.doReset();
	//ofxSurfingHelpers::setMonitorsLayout(1, true, true);
#endif

	bGui.set("SurfChat", true);

	//--

	//ui.setImGuiViewPort(true);
	ui.setup();

	//--

	// Text input

	textInput.makeReferenceTo(bigTextInput.textInput);
	eTextInput = textInput.newListener([this](string& s)
		{
			ui.AddToLog("Prompt: " + s, OF_LOG_WARNING);
			//doGptSendMessage(s);

			doAttendCallbackTextInput();
		});

	// Optional customizations
	// Custom path for multi-instances 
	// avoid "collide folders".
	// or to organize bin/data
	// bigTextInput.setPathGlobal("Gpt");
	// bigTextInput.setName("Prompt");
	// Change the hint text:
	// bigTextInput.setHint("Type search");
	// Change the submit button text:
	 //bigTextInput.setSubmit("Send");
	callback_t myFunctionCallbackClear = std::bind(&ofApp::doAttendCallbackClear, this);
	bigTextInput.setFunctionCallbackClear(myFunctionCallbackClear);

	bWaitingGpt.setSerializable(0);

	// Link toggles
	bigTextInput.bDebug.makeReferenceTo(ui.bDebug);
	////bigTextInput.bWaiting.makeReferenceTo(bWaitingGpt);

	ui.setDisableStartupResetLayout();

	//--

	subs.setUiPtr(&ui);
	subs.setup(); // Startup with no subs mode

	//--

#ifdef USE_WHISPER
	whisper.setup();
	whisper.vCallback.addListener(this, &ofApp::doUpdatedWhisper);
#endif

	//ui.ClearLogDefaultTags();
	//ui.AddLogTag(ofColor::white);

	//--

	// Editors

#ifdef USE_EDITOR_INPUT
	editorInput.setup("Input");
	editorInput.setCustomFonts(ui.getFontsPtr(), ui.getFontsNames());

	//// custom keyword
	//editorInput.addKeyword("\"user\":");
	//editorInput.addKeyword("\"assistant\":");

	// Advanced
	callback_t myFunctionDraw = std::bind(&ofApp::drawWidgetsEditor, this);
	//std::function<void()> myFunctionDraw = std::bind(&ofApp::drawWidgetsEditor, this);
	editorInput.setDrawWidgetsFunction(myFunctionDraw);
#endif

	//-

#ifdef USE_EDITOR_RESPONSE
	editorResponse.setup("Last Response");
	editorResponse.setCustomFonts(ui.getFontsPtr(), ui.getFontsNames());
#endif

	//--

	params.add(bGui);
	params.add(ui.bGui_GameMode);
	params.add(bLock);
	params.add(bigTextInput.bGui);
	params.add(subs.bGui);
	params.add(bModeConversation);
	params.add(bModeOneSlide);
	params.add(sizeFontConv);
	params.add(bLastBigger);
	params.add(bLastBlink);
	params.add(bGui_GptConversation);
	params.add(indexPrompt);
	params.add(typeSpin);
	//params.add(fontR);
	//params.add(bGui_GptLastReply);

	ofAddListener(params.parameterChangedE(), this, &ofApp::Changed_Params);

	//--

	// Sounds
	setupSounds();

	//--

	startup();
}

//--------------------------------------------------------------
void ofApp::setupSounds()
{
	ofSoundPlayer s0;//intro
	s0.load("assets/sounds/0.wav");
	sounds.push_back(s0);

	ofSoundPlayer s1;//send
	s1.load("assets/sounds/1.wav");
	sounds.push_back(s1);

	ofSoundPlayer s2;//receive
	s2.load("assets/sounds/2.wav");
	sounds.push_back(s2);

	ofSoundPlayer s3;//error
	s3.load("assets/sounds/3.wav");
	sounds.push_back(s3);

	ofSoundPlayer s4;//clear
	s4.load("assets/sounds/4.ogg");
	sounds.push_back(s4);

	ofSoundPlayer s5;//error
	s5.load("assets/sounds/5.mp3");
	sounds.push_back(s5);

	for (auto& s : sounds)
	{
		s.setVolume(1.f);
		s.setMultiPlay(false);
	}

	sounds[0].play();
}

//--------------------------------------------------------------
void ofApp::startup()
{
	setupGpt();

	ofxSurfingHelpers::load(params);

	//TODO:
	bigTextInput.typeWaiting = typeSpin;

	//ui.ClearLogDefaultTags();
}

//--------------------------------------------------------------
void ofApp::setupGpt()
{
	//if (apiKey.get() == "")
	//{
	//	ui.AddToLog("No settled API key to run Setup GPT", OF_LOG_ERROR);
	//	return;
	//}

	{
		ofSetLogLevel(OF_LOG_VERBOSE);
		ui.setLogLevel(OF_LOG_VERBOSE);

		ui.AddToLog("setupGpt()", OF_LOG_WARNING);

		//--

		// GPT

		// Load file settings

		ofFile f;
		if (f.doesFileExist(pathGptSettings)) {
			ofJson configJson = ofLoadJson(pathGptSettings);
			//will fail if file do not exist
			if (configJson.find("apiKey") != configJson.end())
				apiKey = configJson["apiKey"].get<string>();
			if (configJson.find("model") != configJson.end())
				model = configJson["model"].get<string>();
		}
		else {
			apiKey = "your-api-key";
			model = "gpt-3.5-turbo";
		}

		//--

		ofxChatGPT chatGpt_;
		chatGpt_.setup(apiKey.get());
		vector<string> models;

		ofxChatGPT::ErrorCode errorCode;
		tie(models, errorCode) = chatGpt_.getModelList();

		ui.AddToLog("Available OpenAI GPT models:");
		for (auto m : models)
		{
			if (ofIsStringInString(m, "gpt")) {
				ui.AddToLog(m, OF_LOG_WARNING);
			}
			else {
				ui.AddToLog(m, OF_LOG_VERBOSE);
			}
		}

		//--

		// Initialize
		chatGpt.setup(model, apiKey);
	}

	//--

	setupGptPrompts();
}

//--------------------------------------------------------------
void ofApp::setupGptPrompts()
{
	//TODO:
	// Create prompts
	promptsNames.clear();
	promptsNames.push_back("10 short sentences.");
	promptsNames.push_back("10 words list.");
	promptsNames.push_back("10 Similar bands");
	promptsNames.push_back("Default");
	indexPrompt.setMax(promptsNames.size() - 1);

	prompts.clear();
	prompts.push_back(std::pair<std::string, std::string> { promptsNames[0], GPT_Prompt_0() });
	prompts.push_back(std::pair<std::string, std::string> { promptsNames[1], GPT_Prompt_1() });
	prompts.push_back(std::pair<std::string, std::string> { promptsNames[2], GPT_Prompt_2() });
	prompts.push_back(std::pair<std::string, std::string> { promptsNames[3], GPT_Prompt_3() });

	setGptPrompt(3);//default
}

//--------------------------------------------------------------
void ofApp::setGptPrompt(int index)
{
	//if (indexPrompt == index) return;//skip if not changed

	ofLogNotice(__FUNCTION__) << index;

	ui.AddToLog("setGptPrompt(" + ofToString(index) + ")", OF_LOG_WARNING);

	if (index > prompts.size() - 1) {
		ui.AddToLog("Index " + ofToString(index) + " out of range!", OF_LOG_ERROR);
		return;
	}

	indexPrompt = index;
	//promptName = ofToString(promptsNames[indexPrompt]);
	promptName = ofToString(prompts[indexPrompt].first);
	strPrompt = prompts[indexPrompt].second;

	chatGpt.setSystemMessage(strPrompt);

	ui.AddToLog("Prompt: " + promptName, OF_LOG_VERBOSE);
	ui.AddToLog(strPrompt, OF_LOG_VERBOSE);
}

//--------------------------------------------------------------
void ofApp::Changed_Params(ofAbstractParameter& e)
{
	string n = e.getName();
	ofLogNotice(__FUNCTION__) << n << " : " << e;

	if (n == bLock.getName())
	{
		bigTextInput.bGui_LockMove = bLock;
	}

	else if (n == bWaitingGpt.getName())
	{
		if (bWaitingGpt) bigTextInput.bWaiting = bWaitingGpt;
	}
	else if (n == indexPrompt.getName())
	{
		setGptPrompt(indexPrompt);
	}
	else if (n == bModeConversation.getName())
	{
		//workflow
		if (bModeConversation) {
			subs.bGui = 0;
			subs.bGui_List = 0;
			subs.bGui_Paragraph = 0;
		}
	}

	//else if (n == bWaitingGpt.getName())
	//{
	//	bigTextInput.bWaiting = bWaitingGpt;
	//}

	else if (n == ui.bGui_GameMode.getName())
	{
		if (ui.bGui_GameMode) {
			bLock = 1;
			ui.bMinimize = 1;
			ui.bDebug = 0;
			ui.bLog = 0;
			ui.bExtra = 0;
			bigTextInput.bGui = 1;
			bigTextInput.bGui_Config = 0;
			bGui_GptConversation = 1;
			bModeConversation = 1;
			editorResponse.bGui = 0;
		}
	}
}

//--------------------------------------------------------------
void ofApp::update()
{
	string s = "SurfChat | " + ofToString(ofGetFrameRate(), 0) + "FPS";
	ofSetWindowTitle(s);

	//--

	if (chatGpt.hasMessage())
	{
		doGptGetMessage();
	}

	//--

#ifdef USE_WHISPER
	whisper.update();
#endif

#ifdef USE_SURF_TTF
	if (TTS.isWaiting()) bigTextInput.bWaiting = 1;
#endif

	//--

	ofSoundUpdate();
}

//--------------------------------------------------------------
void ofApp::drawBg()
{
	int bgMin = 100;

	// Blink when waiting. 
	// Red if error.
	if (bGptError) {
		float v = glm::cos(10 * ofGetElapsedTimef());
		float a1 = ofMap(v, -1, 1, 100, 200, true);
		ofColor c = ofColor(a1, 0, 0);
		ofClear(c);
	}
	else if (bWaitingGpt) {
		ofColor c = bigTextInput.getColor();
		auto br = c.getBrightness();
		float v = glm::cos(10 * ofGetElapsedTimef());
		float a1 = ofMap(v, -1, 1, 100, 115, true);
		//float a1 = ofMap(v, -1, 1, 100, 150, true);
		c.setBrightness(a1);
		ofClear(c);
	}
	else {
		// Flash when submit
		if (v > 0) v -= 0.05f;
		else v = 0;

		// Use color from subtitler when no flash
		if (v > 0) ofClear(bgMin + (255 - bgMin) * v);
		else ofClear(subs.getColorBg());
	}
}

//--------------------------------------------------------------
void ofApp::draw()
{
	// Bg
	drawBg();

	subs.draw();

	//--

	drawImGui();

	//--

#ifdef USE_WHISPER
	ofPushMatrix();
	ofTranslate(-15, ofGetHeight() * 0.7);
	whisper.draw();
	ofPopMatrix();
#endif

#ifdef USE_SURF_TTF
	//if (ui.bDebug) TTS.drawDebug();
#endif
}

//--------------------------------------------------------------
void ofApp::drawImGuiMain()
{
	if (!bGui) return;

	ImGui::SetNextWindowSize(ImVec2(230, 0), ImGuiCond_FirstUseEver);

	// Constraints
	ImVec2 size_min = ImVec2(140, 300);
	ImVec2 size_max = ImVec2(FLT_MAX, FLT_MAX);
	ImGui::SetNextWindowSizeConstraints(size_min, size_max);

	if (ui.BeginWindow(bGui))
	{
		string s;

		ui.PushFont(SurfingFontTypes(1));
		ui.Add(ui.bGui_GameMode, OFX_IM_TOGGLE_BIG_BORDER_BLINK);
		ui.PopFont();

		if (!ui.isGameMode()) {
			ui.AddMinimizerToggle();
			ui.AddLogToggle();
			if (ui.isMaximized())
			{
				ui.AddDebugToggle();
				ui.AddExtraToggle();
			}
			if (ui.isMaximized()) {
				ui.Add(bLock, OFX_IM_TOGGLE);
			}
		}
		ui.AddSpacingBigSeparated();

		//--

		if (!ui.isGameMode()) {
			ui.AddLabelHuge("ChatGPT");

			static ofParameter<bool> b{ "SERVER",0 };
			if (ui.isMaximized()) ui.Add(b, OFX_IM_TOGGLE_ROUNDED_MINI);
			if (ui.isMaximized() && b)
			{
				ui.AddLabelBig("API KEY");
				ui.Add(apiKey, OFX_IM_TEXT_INPUT_NO_NAME);
				ui.AddLabelBig("MODEL");
				ui.Add(model, OFX_IM_TEXT_DISPLAY);

				if (ui.AddButton("Restart"))
				{
					setupGpt();
				}
				if (ui.AddButton("ResetIP"))
				{
					doGptResetEndpointIP();
				}
			}
			ui.AddSpacingSeparated();
		}

		//ui.PushFont(OFX_IM_FONT_BIG);
		{
#ifdef USE_EDITOR_INPUT
			if (ui.AddButton("Send"))
			{
				doGptSendMessage(editorInput.getText(), bModeConversation);
			}
#endif
			if (ui.isMaximized() && ui.bDebug)
			{
				static bool bTester = 0;
				ImGui::Checkbox("Tester##1", &bTester);
				if (bTester) {
					if (ui.AddButton("Random", OFX_IM_BUTTON_BIG)) {
						doRandomInput();
					}
				}
			}
		}
		//ui.PopFont();

		if (ui.AddButton("Clear##2", OFX_IM_BUTTON))
		{
			// Clear
			doClear();
		}

		if (ui.AddButton("Regenerate", OFX_IM_BUTTON)) {
			doGptRegenerate();
		}

		ui.AddSpacingSeparated();

		if (!ui.isGameMode()) {
			ui.AddLabelBig("Prompt");
			ui.AddSpacing();

			ui.Add(bigTextInput.bGui, OFX_IM_TOGGLE_ROUNDED);
			if (ui.isMaximized()) ui.Add(bigTextInput.bGui_Config, OFX_IM_TOGGLE_ROUNDED_SMALL);
			ui.AddSpacing();
		}

		if (ui.isGameMode()) {
			ui.AddLabel("FontSize", 1);
			ui.DrawWidgetsFonts(sizeFontConv);
			ui.AddSpacingSeparated();
		}

		ui.AddLabel("Role Prompt", 1);
		ui.PushFont(SurfingFontTypes(1));
		ui.AddTooltip(strPrompt);
		ui.PopFont();
		ui.AddCombo(indexPrompt, promptsNames);
		ui.AddSpacing();

		//--

#ifdef USE_EDITOR_INPUT
		ui.AddLabelHuge("EDITORS");
		ui.AddSpacing();
		ui.Add(editorInput.bGui, OFX_IM_TOGGLE_ROUNDED);
#endif
		//if (subs.bExtra)
		if (ui.isMaximized())
		{
			ui.Add(bModeConversation, OFX_IM_TOGGLE);
			if (bModeConversation) {
				ui.Add(bGui_GptConversation, OFX_IM_TOGGLE_ROUNDED_MINI);
				if (bGui_GptConversation) {
					if (ui.AddButton("Reset window")) {
						bResetWindowConversation = 1;
					}
					ui.DrawWidgetsFonts(sizeFontConv);
					ui.Add(bLastBlink, OFX_IM_TOGGLE_ROUNDED_MINI);
					s = "Last block will blink";
					ui.AddTooltip(s);
					ui.Add(bLastBigger, OFX_IM_TOGGLE_ROUNDED_MINI);
					s = "Last block will be bigger";
					ui.AddTooltip(s);
				}
	}
			ui.AddSpacing();
			//ui.Add(bGui_GptLastReply, OFX_IM_TOGGLE_ROUNDED_MINI);

#ifdef USE_EDITOR_RESPONSE
			ui.AddSpacingBigSeparated();
			ui.Add(editorResponse.bGui, OFX_IM_TOGGLE_ROUNDED_MINI);
#endif
			ui.AddSpacingBigSeparated();

			//s = errorCodesNames[indexErrorCode.get()];
			//ui.AddLabelBig(s);
			s = gptErrorMessage;
			ui.AddLabelBig(s);
#if(1)
			if (ui.bDebug) {
				ui.Add(bWaitingGpt, OFX_IM_CHECKBOX);
				if (ui.Add(typeSpin, OFX_IM_STEPPER)) {
					bigTextInput.typeWaiting = typeSpin;
				};
			}
#endif
			ImSpinner::Spinner(bWaitingGpt, typeSpin);
			ui.AddSpacingBigSeparated();
}

		//--

		{
#ifdef USE_SURF_TTF
			s = "TTS";
			ui.AddSpacingSeparated();
			ui.AddLabelHuge(s);
			ui.Add(TTS.bEnable, OFX_IM_TOGGLE);
			if (TTS.bEnable) {
				if (ui.AddButton("Send", OFX_IM_BUTTON, 2, true)) {
					TTS.send(textLastResponse);
				}
				s = TTS.getText();
				ui.PushFont(SurfingFontTypes(1));
				ui.AddTooltip(s);
				ui.PopFont();

				if (ui.AddButton("Cancel", OFX_IM_BUTTON, 2)) {
					TTS.cancel();
				}
				ui.AddCombo(TTS.indexVoice, TTS.voices);
				ui.AddTooltip("voices");

				if (ui.AddButton("Replay", OFX_IM_BUTTON)) {
					TTS.replayAudio();
				}
				ui.AddTooltip("Replay last audio");
				ImSpinner::Spinner(TTS.isWaiting(), typeSpin);
				if (ui.bDebug) {
					s = TTS.getTextDisplay();
					ui.AddLabel(s);
				}
			}
#endif
		}

		//--

#ifdef USE_WHISPER
		drawImGuiWidgetsWhisper();
#endif
		//--

		if (!ui.isGameMode())
			if (!bModeConversation) {
				ui.AddSpacingBigSeparated();
				ui.AddLabelHuge("Titles");
				ui.AddSpacing();

				ui.Add(subs.bGui, OFX_IM_TOGGLE_BUTTON_ROUNDED_MEDIUM);
				if (subs.bGui)
				{
					ui.AddSpacing();
					ui.AddSpacingDouble();
					ui.Add(bModeOneSlide, OFX_IM_CHECKBOX);

					//ui.AddLabelBig("Tester", false, true);
					static bool bTester = 0;
					ImGui::Checkbox("Tester##2", &bTester);
					//ui.PushFont(OFX_IM_FONT_BIG);
					{
						if (bTester) {
							s = "Random\nText!";
							s = ofToUpper(s);
							if (ui.AddButton(s, OFX_IM_BUTTON_BIG))
							{
								doPopulateText();
							}
							s = "Do\nBlock!";
							s = ofToUpper(s);
							if (ui.AddButton(s, OFX_IM_BUTTON_BIG))
							{
								doPopulateTextBlocks();
							}
							s = "Clear\nList";
							s = ofToUpper(s);
							if (ui.AddButton(s, OFX_IM_BUTTON_BIG))
							{
								doClearSubsList();
							}
						}
					}
					//ui.PopFont();
				}
			}

		ui.EndWindow();
}
}

//--------------------------------------------------------------
void ofApp::drawImGui()
{
	subs.drawGui();

	ui.Begin();
	{
		//TODO:
		//ImSpinner::demoSpinners();

		//--

		// TextInput bubble widget

		bigTextInput.draw(ui);

		//--

		drawImGuiMain();

		//--

		if (bGui)
		{
			//--

			// Editor Input

#ifdef USE_EDITOR_INPUT
			editorInput.drawImGui();
#endif
			//--

			//if (ui.isMaximized() && ui.isExtraEnabled())
			//if (ui.isExtraEnabled())
			//if (ui.isDebug())

			//drawImGuiReply(ui);

			//--

#ifdef USE_EDITOR_RESPONSE
			// Editor Response
			editorResponse.drawImGui();
#endif
			//--

			subs.drawImGui();
		}

		//--

		drawImGuiConversation(ui);
	}
	ui.End();
}

/*
//--------------------------------------------------------------
void ofApp::drawImGuiReply(ofxSurfingGui& ui)
{
	// Gpt last reply
	if (ui.BeginWindow(bGui_GptLastReply, ImGuiWindowFlags_None))
	{
		if (ui.isDebug()) ui.AddComboFontsSelector(fontR);

		ui.PushFont(SurfingFontTypes(fontR.get()));

		string s = textLastResponse;

		ImGui::TextWrapped(s.c_str());

		ui.PopFont();

		ui.EndWindow();
	}
}
*/

//--------------------------------------------------------------
void ofApp::drawImGuiConversation(ofxSurfingGui& ui)
{
	if (!bGui_GptConversation) return;

	//static ofParameter<bool> bGui_Headers{ "Headers", 0 };
	//static ofParameter<bool> bGui_Bg{ "Bg", 0 };
	//static ofParameter<bool> bGui_ResizePin{ "Resizer", 0 };
	//static ofParameter<bool> bGui_LockMove{ "Lock", 0 };

	//ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
	//window_flags |= ImGuiWindowFlags_NoScrollbar;
	//if (!bGui_Bg) window_flags |= ImGuiWindowFlags_NoBackground;
	//if (!bGui_Headers) window_flags |= ImGuiWindowFlags_NoTitleBar;
	//if (!bGui_ResizePin) window_flags |= ImGuiWindowFlags_NoResize;
	//if (bGui_LockMove) window_flags |= ImGuiWindowFlags_NoMove;

	bool b = ui.bDebug;
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
	if (!b) window_flags |= ImGuiWindowFlags_NoBackground;
	if (!b) window_flags |= ImGuiWindowFlags_NoTitleBar;
	if (!b) window_flags |= ImGuiWindowFlags_NoResize;
	if (bLock) window_flags |= ImGuiWindowFlags_NoMove;
	//if (!b) window_flags |= ImGuiWindowFlags_NoScrollbar;
	//window_flags |= ImGuiWindowFlags_NoScrollbar;

	//--

	// Reset
	{
		float padx = 100;
		float pady = ofGetHeight() * 0.15;
		float w = ofGetWidth() - 2 * padx;
		float h = ofGetHeight() - pady - 100;
		float x = padx;
		float y = pady;
		ImGui::SetNextWindowSize(ImVec2(w, h), bResetWindowConversation ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(x, y), bResetWindowConversation ? ImGuiCond_Always : ImGuiCond_FirstUseEver);

		if (bResetWindowConversation) bResetWindowConversation = 0;
	}

	float scrollbarSize = ImGui::GetStyle().ScrollbarSize;
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, scrollbarSize * 1.25f); // scale the scrollbar size

	if (ui.BeginWindow(bGui_GptConversation, window_flags))
	{
		if (ui.isDebug()) ui.AddComboFontsSelector(sizeFontConv);
		ui.PushFont(SurfingFontTypes(sizeFontConv.get()));

		//--

		// Colorized by roles

		ImU32 c1 = ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_Text));
		ImU32 c2 = ImGui::GetColorU32(bigTextInput.getColor());

		//try 
		{
			int i = 0;
			for (auto& m : jConversationHistory) {
				string role = m["message"]["role"].get<std::string>();
				string content = m["message"]["content"].get<std::string>();

				bool bLast = jConversationHistory.size() - 1 == i;

				ImU32 color = (role == "user") ? c1 : c2;
				ImGui::PushStyleColor(ImGuiCol_Text, color);

				// Customize last text block
				if (bLast) {
					if (bLastBlink) ui.BeginBlinkText(true, true);
					if (bLastBigger) ui.PushFont(SurfingFontTypes(MAX(3, sizeFontConv.get() + 1)));
				}

				ImGui::TextWrapped("%s", content.c_str());

				if (bLast) {
					if (bLastBigger) ui.popStyleFont();
					if (bLastBlink) ui.EndBlinkText();
				}
				ImGui::PopStyleColor();

				i++;
			}
		}
		//catch (const std::exception& e) {
		//	ofLogError("Error reading message history JSON data: ") << e.what();
		//	ofLogError("JSON data that caused the exception: ") << jConversationHistory.dump();
		//}

		ui.PopFont();

		if (bFlagGoBottom) {
			bFlagGoBottom = 0;
			// Scroll to the bottom of the window
			ImGui::SetScrollHereY(1.0f);
		}

		ui.EndWindow();
	}

	ImGui::PopStyleVar();
}

//--------------------------------------------------------------
void ofApp::doPopulateText(string s)
{
	//workflow
	doClearSubsList();

	// auto generate a random text
	if (s == "")
	{
		s = ofxSurfingHelpers::getTextRandom();

		//workaround log fix
		s = " " + s;
	}

	////TODO:
	////trick
	//ui.ClearLogDefaultTags();
	//ofColor c = ofColor(subs.getColorText(), 255);
	//ui.AddLogTag(c);

	//ofLogNotice() << s;
	subs.doSetTextSlideStart(s);
	ui.AddToLog(s);

	// Spacing
	for (size_t i = 0; i < 10; i++)
	{
		ofLogNotice("ofApp") << "|";
	}
}

// Function to process a full file and split into blocks/slides.
//--------------------------------------------------------------
void ofApp::doPopulateTextBlocks() {
	string path = "files/txt/text2.txt";
	subs.setupTextBlocks(path);
}
//--------------------------------------------------------------
void ofApp::doClearSubsList() {
	subs.doClearList();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
	if (ui.isOverInputText()) return; // skip when editing
	if (chatGpt.isWaiting()) return;

	if (key == '1') { strBandname = "Jane's Addiction"; doGptSendMessage(strBandname); return; }
	if (key == '2') { strBandname = "Fugazi"; doGptSendMessage(strBandname); return; }
	if (key == '3') { strBandname = "Joy Division"; doGptSendMessage(strBandname); return; }
	if (key == '4') { strBandname = "The Smiths";  doGptSendMessage(strBandname); return; }
	if (key == '5') { strBandname = "Radio Futura"; doGptSendMessage(strBandname); return; }
	if (key == '6') { strBandname = "John Frusciante"; doGptSendMessage(strBandname); return; }
	if (key == '7') { strBandname = "Primus"; doGptSendMessage(strBandname); return; }
	if (key == '8') { strBandname = "Kraftwerk"; doGptSendMessage(strBandname); return; }
	if (key == '9') { strBandname = "Portishead"; doGptSendMessage(strBandname); return; }

	if (0) return;

	else if (key == 'g') {
		bGui = !bGui;

		//workflow
		if (!bGui && bigTextInput.bGui_Config) bigTextInput.bGui_Config = 0;
	}

	//// next prompt
	//else if (key == OF_KEY_TAB) { doSwapGptPrompt(); }

	//// regenerate
	//else if (key == OF_KEY_RETURN) { doGptRegenerate(); }

	// Clear
	else if (key == OF_KEY_BACKSPACE) { doClear(); }

	else if (key == 'd') {
		ui.bDebug = !ui.bDebug;
		bigTextInput.bDebug = ui.bDebug;
	}

	else if (key == 'l') { bLock = !bLock; }

	else if (key == ' ') { doGptRegenerate(); }

	//--

	else if (key == 'e') { subs.setToggleEdit(); }
	//else if (key == 'l') { subs.setToggleLive(); }
	//else if (key == 'g') { subs.setToggleVisibleGui(); }
	//else if (key == ' ') { doPopulateText(); }

	//else if (key == ' ') { subs.setTogglePlay(); }
	//else if (key == OF_KEY_RETURN) { subs.setTogglePlayForced(); }
	//else if (key == OF_KEY_LEFT) { subs.setSubtitlePrevious(); }
	//else if (key == OF_KEY_RIGHT) { subs.setSubtitleNext(); }
	//else if (key == OF_KEY_BACKSPACE) { subs.setSubtitleRandomIndex(); };

	//--

	//TODO:
#ifdef USE_EDITOR_INPUT
	//switch (key)
	//{
	//case '1': {
	//	string path = ofToDataPath("text1.txt", true);
	//	editorInput.loadText(path);
	//	break;
	//}
	//case '2': {
	//	string path = ofToDataPath("text2.txt", true);
	//	editorInput.loadText(path);
	//	break;
	//}
	//case '3': {
	//	string str = "Garc�a Castell�n pone la X de Kitchen a Fern�ndez D�az\n y tapona la investigaci�n a Rajoy, \nla c�pula del PP y \nel CNI El juez \ndetermina que la decisi�n \nde espiar a B�rcenas con \nfondos reservados para evitar problemas judiciales \nal presidente y a Cospedal no \ntrascendi� del Ministerio del Interior.\nEl cierre de la instrucci�n llega \ncuando Anticorrupci�n apunta al CNI en \nel episodio del 'falso cura\n' e investiga una segunda Kitchen \nen la c�rcel";
	//	editorInput.setText(str);
	//	break;
	//}
	//}
#endif
}

//--------------------------------------------------------------
void ofApp::drawWidgetsEditor()
{
	//--

#ifdef USE_EDITOR_INPUT
	//if (editorInput.bExtra || !editorInput.bMenus) ui.AddSpacingSeparated();

	//float w = 200;
	float w = ui.getWidgetsWidth(4);
	float h = 2 * ui.getWidgetsHeightUnit();
	ImVec2 sz{ w, h };

	if (ui.AddButton("SEND", sz))
	{
		string s = editorInput.getText();
		ui.AddToLog(s, OF_LOG_NOTICE);
		doGptSendMessage(s, bModeConversation);

		//workflow
		//editorInput.clearText();
	};
	ui.SameLine();
	if (ui.AddButton("CLEAR", sz))
	{
		editorInput.clearText();
	};

	ui.Add(bModeConversation);

	//editorInput.drawImGuiWidgetsFonts();

	ui.AddSpacingSeparated();
#endif

	//--

	// Catch from previous widget
	//bOver = ImGui::IsItemHovered();
	ImGuiIO& io = ImGui::GetIO();

	bool bMouseLeft = io.MouseClicked[0];
	bool bMouseRight = io.MouseClicked[1];
	float mouseWheel = io.MouseWheel;

	//bool bKeyReturn = ImGui::IsKeyboardKey(ImGuiKey_Enter);
	bool bKeyReturn = ImGui::IsKeyPressed(ImGuiKey_Enter);
	bool bCtrl = io.KeyCtrl;
	bool bShift = io.KeyShift;

	// Enter
	if (bCtrl && bKeyReturn)
	{
#ifdef USE_EDITOR_INPUT
		string s = editorInput.getText();
		ui.AddToLog(s, OF_LOG_NOTICE);
		doGptSendMessage(s, bModeConversation);
		editorInput.clearText();
#endif
	}
}

//--------------------------------------------------------------
void ofApp::doGptSendMessage(string message) {
	ofLogNotice(__FUNCTION__) << message;

	// Clear

	ofxChatGPT::ErrorCode errorCode;
	bGptError = false;

	ofJson jMsg;
	jMsg["message"]["role"] = "user";
	jMsg["message"]["content"] = message;

	jConversationHistory.push_back(jMsg);

	ofLogVerbose("ofApp") << "User: " << message;

	jQuestion = jMsg;
	jResponse = ofJson();

	bWaitingGpt = 1;

	// clear
	textLastResponse = "";

	if (!bModeConversation) {
#ifdef USE_EDITOR_RESPONSE
		editorResponse.clearText();//workflow
#endif
	}

	ui.AddToLog("doGptSendMessage()", OF_LOG_WARNING);
	ui.AddToLog(message);

	//--

	// Submit
	message = "\n" + message;
	if (bModeConversation) chatGpt.chatWithHistoryAsync(message);
	else chatGpt.chatAsync(message);

	sounds[1].play();

	// workflow

	// scroll conversation
	bFlagGoBottom = 1;

	// focus in text input
	bigTextInput.setFocus();
}

//--------------------------------------------------------------
void ofApp::doGptRegenerate() {
	ui.AddToLog("doGptRegenerate()", OF_LOG_WARNING);

	chatGpt.regenerateAsync();
}

//--------------------------------------------------------------
void ofApp::doGptGetMessage()
{
	ui.AddToLog("doGptGetMessage()", OF_LOG_WARNING);

	// Get
	string strGptResponse;
	ofxChatGPT::ErrorCode errorCode;
	tie(strGptResponse, errorCode) = chatGpt.getMessage();

	indexErrorCode = getErrorCodeByCode(errorCode);
	if (indexErrorCode > 0 && indexErrorCode < 9) bGptError = true;
	else bGptError = false;

	bWaitingGpt = 0;

	if (errorCode == ofxChatGPT::Success) // Success
	{
		// Get response

		ofLogNotice("ofApp") << "ofxChatGPT Success.";
		bGptError = false;
		gptErrorMessage = "State: Success";
		indexErrorCode = 0;

		ofJson jMsg;
		jMsg["message"]["role"] = "assistant";
		jMsg["message"]["content"] = strGptResponse;

		jConversationHistory.push_back(jMsg);

		ofLogNotice("ofApp") << "Assistant: " << strGptResponse;

		jResponse = jMsg;

		//--

		//TODO:
		// Process response
		for (auto& content : jMsg["content"])
		{
			strGptResponse += content.get<std::string>() + "\n";
		}
		//ofLogNotice("ofxSurfingTextSubtitle") << "strGptResponse:" << strGptResponse;

		//--

		textLastResponse = ofxSurfingHelpers::removeNumbersStartingLines(strGptResponse);
		ofLogNotice("ofxSurfingTextSubtitle") << endl << textLastResponse;

		//--

		// Build slides
		if (!bModeConversation) {
			if (bModeOneSlide) subs.doBuildDataTextOneSlideOnly(textLastResponse);
			else subs.doBuildDataText(textLastResponse);
		}

		/*
		//TODO:
		//there's no new line \n marks. so we assume the blocks will be numbered 1., 2., 3. etc
		size_t sz = ofxSurfingHelpers::countNewlines(textLastResponse);
		bool b = true;
		//b = sz == 0;
		if (b) {
			subs.doBuildDataTextBlocks(textLastResponse, true);
		}
		else {//we found \n tags. so we assume blocks ends with \n.
			subs.doBuildDataTextBlocks(textLastResponse);
		}
		*/

		//--

#ifdef USE_EDITOR_RESPONSE
		// Here textLastResponse is already cached 
		editorResponse.clearText();//workflow
		editorResponse.addText(textLastResponse + "\n");
#endif
		sounds[3].play();

#ifdef USE_SURF_TTF
		TTS.send(textLastResponse);
#endif
	}
	else // Error
	{
		//tuple<string, ofxChatGPT::ErrorCode> m = ofxChatGPT::getMessage();

		indexErrorCode = getErrorCodeByCode(errorCode);
		bGptError = true;
		gptErrorMessage = "Error: " + ofxChatGPT::getErrorMessage(errorCode);
		ofLogError("ofApp") << "ofxChatGPT has an error: " << gptErrorMessage;
		ui.AddToLog(gptErrorMessage, OF_LOG_ERROR);

		//--

		// workaround to try to fix the error on the fly
		// bc error could like new IP renew for our endpoint server.
		bool b = doGptResetEndpointIP();
		if (b) ui.AddToLog("doGptResetEndpointIP() Success", OF_LOG_WARNING);
		else ui.AddToLog("doGptResetEndpointIP() Error", OF_LOG_ERROR);

		sounds[3].play();
	}

	// workflow

	// scroll conversation
	bFlagGoBottom = 1;

	// focus in text input
	bigTextInput.setFocus();
}

//--------------------------------------------------------------
void ofApp::doRandomInput()
{
	ui.AddToLog("doRandomInput()", OF_LOG_WARNING);

	//workflow
	doClearSubsList();

	size_t sz = 9;
	float r = ofRandom(sz);
	if (r < 1) { strBandname = "Jane's Addiction"; }
	else if (r < 2) { strBandname = "Fugazi"; }
	else if (r < 3) { strBandname = "Joy Division"; }
	else if (r < 4) { strBandname = "The Smiths"; }
	else if (r < 5) { strBandname = "Radio Futura"; }
	else if (r < 6) { strBandname = "John Frusciante"; }
	else if (r < 7) { strBandname = "Primus"; }
	else if (r < 8) { strBandname = "Kraftwerk"; }
	else if (r < 9) { strBandname = "Portishead"; }
	string s = "";
	s = strBandname;
	ui.AddToLog(s, OF_LOG_NOTICE);

	bigTextInput.setText(s);
	doGptSendMessage(s);

#ifdef USE_EDITOR_INPUT
	editorInput.setText(s);
	ui.AddToLog("editorInput.setText()");
	ui.AddToLog(s, OF_LOG_NOTICE);
	doGptSendMessage(editorInput.getText(), bModeConversation);
#endif
};

//--------------------------------------------------------------
void ofApp::doSwapGptPrompt() {
	if (indexPrompt == 0) indexPrompt = 1;
	else if (indexPrompt == 1) indexPrompt = 2;
	else if (indexPrompt == 2) indexPrompt = 3;
	else if (indexPrompt == 3) indexPrompt = 0;
	//setGptPrompt(indexPrompt);
}

//--------------------------------------------------------------
void ofApp::exit()
{
	ofLogVerbose() << "exit()";

	ofRemoveListener(params.parameterChangedE(), this, &ofApp::Changed_Params);

	//sounds[0].play();//can't be played bc just closes

	ofxSurfingHelpers::save(params);

	ofJson configJson;
	configJson["apiKey"] = apiKey;
	configJson["model"] = model;
	ofSavePrettyJson(pathGptSettings, configJson);
}

#ifdef USE_WHISPER

//--------------------------------------------------------------
void ofApp::doUpdatedWhisper()
{
	string s = whisper.getTextLast();
	ofLogNotice() << "doUpdatedWhisper(): " << s;
	doPopulateText(s);
}

//--------------------------------------------------------------
void ofApp::drawImGuiWidgetsWhisper()
{
	ui.Add(ui.bMinimize, OFX_IM_TOGGLE_BUTTON_ROUNDED);
	ui.Add(ui.bLog, OFX_IM_TOGGLE_BUTTON_ROUNDED);
	if (ui.bLog || whisper.bDebug) {
		ui.AddSpacing();
		if (ui.Add(whisper.vClear, OFX_IM_BUTTON)) {
			ui.ClearLog();
		};
	}
	ui.AddSpacingBigSeparated();

	ui.AddLabelHuge("ofxWhisper");
	ui.AddSpacing();
	ui.Add(whisper.bEnable, OFX_IM_TOGGLE_BIG_BORDER_BLINK);
	ui.AddSpacing();

	if (ui.isMaximized())
	{
		ui.AddSpacing();
		ui.Add(whisper.bTimeStamps, OFX_IM_TOGGLE_BUTTON_ROUNDED_MINI);
		ui.Add(whisper.bSpanish, OFX_IM_TOGGLE_BUTTON_ROUNDED_MINI);
		s = "Uses another model\n";
		s += "Requires app restart!";
		ui.AddTooltip(s);
		ui.Add(whisper.bHighQuality, OFX_IM_TOGGLE_BUTTON_ROUNDED_MINI);
		s = "Uses a bigger model\n";
		s += "Requires app restart!";
		ui.AddTooltip(s);
		ui.Add(whisper.step_ms);
		s = "Default is 500ms\n";
		s += "Requires app restart!";
		ui.AddTooltip(s);
		ui.Add(whisper.length_ms);
		s = "Default is 5000ms\n";
		s += "Requires app restart!";
		ui.AddTooltip(s);
		ui.AddSpacing();
		ui.Add(whisper.bDebug, OFX_IM_TOGGLE_BUTTON_ROUNDED_MINI);
		if (whisper.bDebug) {
		}
		ui.AddSpacing();
		//ui.AddLabel(whisper.getTextLast());
	}

	ui.AddSpacingBigSeparated();
}

#endif

//--------------------------------------------------------------
int ofApp::getErrorCodeByCode(ofxChatGPT::ErrorCode errorCode)
{
	int i = -1;

	switch (errorCode)
	{
	case ofxChatGPT::Success: i = 0; break;
	case ofxChatGPT::InvalidAPIKey: i = 1; break;
	case ofxChatGPT::NetworkError: i = 2; break;
	case ofxChatGPT::ServerError: i = 3; break;
	case ofxChatGPT::RateLimitExceeded: i = 4; break;
	case ofxChatGPT::TokenLimitExceeded: i = 5; break;
	case ofxChatGPT::InvalidModel: i = 6; break;
	case ofxChatGPT::BadRequest: i = 7;		break;
	case ofxChatGPT::Timeout: i = 8; break;
	case ofxChatGPT::UnknownError: i = 9; break;
	default:i = 9; break;
	}

	return i;
}

//--------------------------------------------------------------
void ofApp::doAttendCallbackClear()
{
	ofLogNotice(__FUNCTION__);
	//TODO:
	doClear(); // crash
}

//--------------------------------------------------------------
void ofApp::doAttendCallbackTextInput()
{
	ofLogNotice(__FUNCTION__);

	//workflow
	//clear
	doClearSubsList();

	if (!bModeConversation) {
#ifdef USE_EDITOR_RESPONSE
		editorResponse.clearText();
#endif
	}

	//// will be called when submitted text changed!
	//text = bigTextInput.getText();
	//ofLogNotice(__FUNCTION__) << text;
	//ofSetWindowTitle(text);

	//ui.AddToLog("TextInput: " + textInput.get(), OF_LOG_NOTICE);

	v = 1;

	string s = textInput.get();
	doGptSendMessage(s);
}

//--------------------------------------------------------------
bool ofApp::doGptResetEndpointIP()
{
	CURL* curl;
	CURLcode res;
	struct curl_slist* headers = NULL;

	const std::string api_key = apiKey;

	// Set up the headers
	std::string authorization_header = "Authorization: Bearer " + api_key;
	headers = curl_slist_append(headers, authorization_header.c_str());

	// Set up the URL and payload
	std::string url = "https://api.pawan.krd/resetip";
	std::string payload = "";

	curl = curl_easy_init();
	if (curl) {
		// Set up the request
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());

		// Disable SSL certificate verification
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

		// Send the request
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;

			string s = "doGptResetEndpointIP() curl_easy_perform() failed: " + ofToString(curl_easy_strerror(res));
			ui.AddToLog(s, OF_LOG_ERROR);

			// Clean up
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);

			return false;
		}

		// Clean up
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);

		return true;
	}

	return false;
}

//--------------------------------------------------------------
void ofApp::doClear()
{
	ofLogNotice(__FUNCTION__);

	ui.AddToLog("Clear", OF_LOG_WARNING);

	chatGpt.clear();

	bGptError = 0;
	gptErrorMessage = "";

	jConversationHistory.clear();

	doClearSubsList();

	textLastResponse = "";

	if (!bModeConversation) {
#ifdef USE_EDITOR_RESPONSE
		editorResponse.clearText();//workflow
#endif
	}

	sounds[4].play();
}