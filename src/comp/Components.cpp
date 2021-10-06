//***********************************************************************************************
//Impromptu Modular: Modules for VCV Rack by Marc Boulé
//
//See ./LICENSE.md for all licenses
//***********************************************************************************************


#include "Components.hpp"


// Dark management
// ----------------------------------------

void saveDarkAsDefault(bool darkAsDefault) {
	json_t *settingsJ = json_object();
	json_object_set_new(settingsJ, "darkAsDefault", json_boolean(darkAsDefault));
	std::string settingsFilename = asset::user("ImpromptuModular.json");
	FILE *file = fopen(settingsFilename.c_str(), "w");
	if (file) {
		json_dumpf(settingsJ, file, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
		fclose(file);
	}
	json_decref(settingsJ);
}


bool loadDarkAsDefault() {
	bool ret = false;
	std::string settingsFilename = asset::user("ImpromptuModular.json");
	FILE *file = fopen(settingsFilename.c_str(), "r");
	if (!file) {
		saveDarkAsDefault(false);
		return ret;
	}
	json_error_t error;
	json_t *settingsJ = json_loadf(file, 0, &error);
	if (!settingsJ) {
		// invalid setting json file
		fclose(file);
		saveDarkAsDefault(false);
		return ret;
	}
	json_t *darkAsDefaultJ = json_object_get(settingsJ, "darkAsDefault");
	if (darkAsDefaultJ)
		ret = json_boolean_value(darkAsDefaultJ);
	
	fclose(file);
	json_decref(settingsJ);
	return ret;
}



// Helpers
// ----------------------------------------

// none


// Variations on existing knobs, lights, etc
// ----------------------------------------

// Screws
// ----------

void DynamicSVGScrew::addFrame(std::shared_ptr<Svg> svg) {
	frames.push_back(svg);
    if(frames.size() == 1) {
        setSvg(svg);
	}
}

void DynamicSVGScrew::step() {
    int newMode = isDark(mode) ? 1 : 0;
	if (newMode != oldMode) {
        if (newMode > 0 && !frameAltName.empty()) {// JIT loading of alternate skin
			frames.push_back(APP->window->loadSvg(frameAltName));
			frameAltName.clear();// don't reload!
		}
        setSvg(frames[newMode]);
        oldMode = newMode;
    }
	SvgWidget::step();
}



// Ports
// ----------



// Buttons and switches
// ----------

void IMSwitch2V::draw(const DrawArgs& args) {
	if (isDark(mode)) {
		nvgBeginPath(args.vg);
		NVGpaint grad = nvgLinearGradient(args.vg, 0, 0, 0, box.size.y, colTop, colBot);	
		nvgRoundedRect(args.vg, -1.0f, -1.0f, box.size.x + 2.0f, box.size.y + 2.0f, 1.5f);
		nvgFillPaint(args.vg, grad);
		nvgFill(args.vg);
	}	
	CKSS::draw(args);
}


struct SwitchOutlineWidget : Widget {
	int** mode = NULL;
	Margins* margins;
	
	SwitchOutlineWidget(int** _mode, Margins* _margins, Vec _size) {
		mode = _mode;
		margins = _margins;
		box.size = _size;
	}
	void draw(const DrawArgs& args) override {
		if (mode && isDark(*mode)) {
			// DEBUG("dbx: %g, %g, %g, %g", box.size.x, box.size.y, box.pos.x, box.pos.y);
			nvgBeginPath(args.vg);
			NVGpaint grad = nvgLinearGradient(args.vg, 0, 0, 0, box.size.y, colTop, colBot);	
			nvgRoundedRect(args.vg, -margins->l, -margins->t, box.size.x + margins->l + margins->r, box.size.y + margins->t + margins->b, 1.5f);
			nvgFillPaint(args.vg, grad);
			nvgFill(args.vg);
		}
		Widget::draw(args);
	}
};



IMSwitch2H::IMSwitch2H() {
	margins.l = 0.5f;
	margins.r = 0.5f;
	margins.t = 0.5f;
	margins.b = 0.5f;

	shadow->opacity = 0.0;
	// DEBUG("i  : %f, %f", sw->box.size.x, sw->box.size.y);
	
	TransformWidget *tw = new TransformWidget();
	tw->box.size = sw->box.size;
	fb->removeChild(sw);
	tw->addChild(sw);
	fb->addChild(tw);

	tw->rotate(float(M_PI_2));
	tw->translate(Vec(0, -sw->box.size.y));
	
	sw->box.size = sw->box.size.flip();
	tw->box.size = sw->box.size;
	fb->box.size = sw->box.size;
	box.size = sw->box.size;
	
	// add margins:
	fb->box.size = fb->box.size.plus(Vec(margins.l + margins.r, margins.t + margins.b));
	box.size = fb->box.size;
	tw->box.pos = tw->box.pos.plus(Vec(margins.l, margins.t));

	// switch outline (BUG!! use draw() code version for now, but not framebuffered):
	// DEBUG("  f: %f, %f", box.size.x, box.size.y);
	// SwitchOutlineWidget* sow = new SwitchOutlineWidget(&mode, &margins, box.size.mult(0.5f));
	// sow->box.pos = Vec(0, 0);
	// fb->addChildBottom(sow);
}


void IMSwitch2H::draw(const DrawArgs& args) {
	if (isDark(mode)) {
		nvgBeginPath(args.vg);
		NVGpaint grad = nvgLinearGradient(args.vg, 0, 0, 0, box.size.y, colTop, colBot);	
		nvgRoundedRect(args.vg, -margins.l, -margins.t, box.size.x + margins.l + margins.r, box.size.y + margins.t + margins.b, 1.5f);
		nvgFillPaint(args.vg, grad);
		nvgFill(args.vg);
	}
	CKSS::draw(args);
}


void IMSwitch3VInv::draw(const DrawArgs& args) {
	if (isDark(mode)) {
		nvgBeginPath(args.vg);
		NVGpaint grad = nvgLinearGradient(args.vg, 0, 0, 0, box.size.y, colTop, colBot);	
		nvgRoundedRect(args.vg, -1.0f, -1.0f, box.size.x + 2.0f, box.size.y + 2.0f, 1.5f);
		nvgFillPaint(args.vg, grad);
		nvgFill(args.vg);
	}
	SvgSwitch::draw(args);
}



LEDBezelBig::LEDBezelBig() {
	momentary = true;
	float ratio = 2.13f;
	addFrame(APP->window->loadSvg(asset::system("res/ComponentLibrary/LEDBezel.svg")));
	sw->box.size = sw->box.size.mult(ratio);
	fb->removeChild(sw);
	tw = new TransformWidget();
	tw->addChild(sw);
	tw->scale(Vec(ratio, ratio));
	tw->box.size = sw->box.size; 
	fb->addChild(tw);
	box.size = sw->box.size; 
	shadow->box.size = sw->box.size; 
}


void KeyboardBig::draw(const DrawArgs& args) {
	if (isDark(mode)) {
		nvgBeginPath(args.vg);
		NVGpaint grad = nvgLinearGradient(args.vg, 0, 0, 0, box.size.y, colTopD, colBotD);	
		nvgRoundedRect(args.vg, -1.0f, -1.0f, box.size.x + 2.0f, box.size.y + 2.0f, 1.5f);
		nvgFillPaint(args.vg, grad);
		nvgFill(args.vg);
	}
	SvgWidget::draw(args);
}


void KeyboardMed::draw(const DrawArgs& args) {
	if (isDark(mode)) {
		nvgBeginPath(args.vg);
		NVGpaint grad = nvgLinearGradient(args.vg, 0, 0, 0, box.size.y, colTopD, colBotD);	
		nvgRoundedRect(args.vg, -1.0f, -1.0f, box.size.x + 2.0f, box.size.y + 2.0f, 1.5f);
		nvgFillPaint(args.vg, grad);
		nvgFill(args.vg);
	}
	SvgWidget::draw(args);
}

void TactPadSvg::draw(const DrawArgs& args) {
	if (isDark(mode)) {
		nvgBeginPath(args.vg);
		NVGpaint grad = nvgLinearGradient(args.vg, 0, 0, 0, box.size.y, colTopD, colBotD);	
		nvgRoundedRect(args.vg, -1.0f, -1.0f, box.size.x + 2.0f, box.size.y + 2.0f, 5.0f);
		nvgFillPaint(args.vg, grad);
		nvgFill(args.vg);
	}
	SvgWidget::draw(args);
}

void CvPadSvg::draw(const DrawArgs& args) {
	if (isDark(mode)) {
		nvgBeginPath(args.vg);
		NVGpaint grad = nvgLinearGradient(args.vg, 0, 0, 0, box.size.y, colTopD, colBotD);	
		nvgRoundedRect(args.vg, -1.0f, -1.0f, box.size.x + 2.0f, box.size.y + 2.0f, 5.0f);
		nvgFillPaint(args.vg, grad);
		nvgFill(args.vg);
	}
	SvgWidget::draw(args);
}



// Knobs
// ----------

// none



// Lights
// ----------

// none



// Svg Widgets
// ----------

// none

