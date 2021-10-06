//***********************************************************************************************
//Impromptu Modular: Modules for VCV Rack by Marc Boulé
//
//Based on code from the Fundamental and AudibleInstruments plugins by Andrew Belt 
//and graphics from the Component Library by Wes Milholen 
//See ./LICENSE.md for all licenses
//See ./res/fonts/ for font licenses
//***********************************************************************************************


#include "ImpromptuModular.hpp"


Plugin *pluginInstance;


void init(rack::Plugin *p) {
	pluginInstance = p;

	p->addModel(modelBigButtonSeq);
	p->addModel(modelBigButtonSeq2);
	p->addModel(modelChordKey);
	p->addModel(modelChordKeyExpander);
	p->addModel(modelClocked);
	p->addModel(modelClockedExpander);
	p->addModel(modelClkd);
	p->addModel(modelCvPad);
	p->addModel(modelFoundry);
	p->addModel(modelFoundryExpander);
	p->addModel(modelFourView);
	p->addModel(modelGateSeq64);
	p->addModel(modelGateSeq64Expander);
	p->addModel(modelHotkey);
	p->addModel(modelPart);
	p->addModel(modelPhraseSeq16);
	p->addModel(modelPhraseSeq32);
	p->addModel(modelPhraseSeqExpander);
	p->addModel(modelProbKey);
	// p->addModel(modelProbKeyExpander);
	p->addModel(modelSemiModularSynth);
	p->addModel(modelTact);
	p->addModel(modelTact1);
	p->addModel(modelTactG);
	p->addModel(modelTwelveKey);
	p->addModel(modelWriteSeq32);
	p->addModel(modelWriteSeq64);
	p->addModel(modelBlankPanel);
}


// General objects

void InverterWidget::draw(const DrawArgs& args) {
	TransparentWidget::draw(args);
	if (isDark(panelThemeSrc)) {
		// nvgSave(args.vg);
		nvgBeginPath(args.vg);
		nvgFillColor(args.vg, SCHEME_WHITE);// this is the source, the current framebuffer is the dest	
		nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
		nvgGlobalCompositeBlendFuncSeparate(args.vg, 
			NVG_ONE_MINUS_DST_COLOR,// srcRGB
			NVG_ZERO,// dstRGB
			NVG_ONE_MINUS_DST_COLOR,// srcAlpha
			NVG_ONE);// dstAlpha
		// blend factor: https://github.com/memononen/nanovg/blob/master/src/nanovg.h#L86
		// OpenGL blend doc: https://www.khronos.org/opengl/wiki/Blending
		nvgFill(args.vg);
		nvgClosePath(args.vg);
		// nvgRestore(args.vg);
	}			
}


ClockMaster clockMaster;  



// General functions


NVGcolor prepareDisplay(NVGcontext *vg, Rect *box, int fontSize, int* mode) {
	if (mode && *mode != 0) {
		nvgBeginPath(vg);
		NVGpaint grad = nvgLinearGradient(vg, 0, 0, 0, box->size.y, colTopD, colBotD);	
		nvgRoundedRect(vg, -1.5f, -1.5f, box->size.x + 3.0f, box->size.y + 3.0f, 5.0f);
		nvgFillPaint(vg, grad);
		nvgFill(vg);
	}

	NVGcolor backgroundColor = nvgRGB(0x38, 0x38, 0x38); 
	NVGcolor borderColor = nvgRGB(0x10, 0x10, 0x10);
	nvgBeginPath(vg);
	nvgRoundedRect(vg, 0.0, 0.0, box->size.x, box->size.y, 5.0f);
	nvgFillColor(vg, backgroundColor);
	nvgFill(vg);
	nvgStrokeWidth(vg, 1.0);
	nvgStrokeColor(vg, borderColor);
	nvgStroke(vg);
	nvgFontSize(vg, fontSize);
	NVGcolor textColor = nvgRGB(0xaf, 0xd2, 0x2c);
	nvgGlobalTint(vg, color::WHITE);
	return textColor;
}


static const char noteLettersSharp[12] = {'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B'};
static const char noteLettersFlat [12] = {'C', 'D', 'D', 'E', 'E', 'F', 'G', 'G', 'A', 'A', 'B', 'B'};
static const char isBlackKey      [12] = { 0,   1,   0,   1,   0,   0,   1,   0,   1,   0,   1,   0 };

void printNoteNoOct(int note, char* text, bool sharp) {// text must be at least 3 chars long (three displayed chars plus end of string)
	// given note is a pitch CV multiplied by 12 and rounded to nearest integer
	note = eucMod(note, 12);
	text[0] = sharp ? noteLettersSharp[note] : noteLettersFlat[note];// note letter
	text[1] = (isBlackKey[note] == 1) ? (sharp ? '\"' : 'b' ) : ' ';// sharp/flat
	text[2] = 0;
}


int printNote(float cvVal, char* text, bool sharp) {// text must be at least 4 chars long (three displayed chars plus end of string)
	// return cursor position of eos
	
	int indexNote;
	int octave;
	calcNoteAndOct(cvVal, &indexNote, &octave);
	
	// note letter
	text[0] = sharp ? noteLettersSharp[indexNote] : noteLettersFlat[indexNote];
	
	// octave number
	int cursor = 1;
	octave += 4;
	if (octave >= 0 && octave <= 9) {
		text[1] = (char) ( 0x30 + octave);
		cursor++;
	}
	
	// sharp/flat
	if (isBlackKey[indexNote] == 1) {
		text[cursor] = (sharp ? '\"' : 'b' );
		cursor++;
	}
	
	text[cursor] = 0;
	return cursor;
}


int moveIndex(int index, int indexNext, int numSteps) {
	if (indexNext < 0)
		index = numSteps - 1;
	else
	{
		if (indexNext - index >= 0) { // if moving right or same place
			if (indexNext >= numSteps)
				index = 0;
			else
				index = indexNext;
		}
		else { // moving left 
			if (indexNext >= numSteps)
				index = numSteps - 1;
			else
				index = indexNext;
		}
	}
	return index;
}


void InstantiateExpanderItem::onAction(const event::Action &e) {
	// Create Module and ModuleWidget
	engine::Module* module = model->createModule();
	APP->engine->addModule(module);

	ModuleWidget* mw = model->createModuleWidget(module);
	// APP->scene->rack->addModuleAtMouse(mw);
	
	// ModuleWidget *mw = model->createModuleWidget();
	if (mw) {
		APP->scene->rack->setModulePosNearest(mw, posit);
		APP->scene->rack->addModule(mw);
		history::ModuleAdd *h = new history::ModuleAdd;
		h->name = "create expander module";
		h->setModule(mw);
		APP->history->push(h);
	}
}


