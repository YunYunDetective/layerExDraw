#pragma comment(lib, "gdiplus.lib")
#include "LayerExDraw.hpp"
#include "ncbind/ncbind.hpp"

#include <stdio.h>
/**
 * ���O�o�͗p
 */
static void log(const tjs_char *format, ...)
{
	va_list args;
	va_start(args, format);
	tjs_char msg[1024];
	_vsnwprintf_s(msg, 1024, _TRUNCATE, format, args);
	TVPAddLog(msg);
	va_end(args);
}

// GDI+ ��{���
static GdiplusStartupInput gdiplusStartupInput;
static ULONG_PTR gdiplusToken;

/// �v���C�x�[�g�t�H���g���
static PrivateFontCollection *privateFontCollection = NULL;
static vector<void*> fontDatas;

// GDI+ ������
void initGdiPlus()
{
	// Initialize GDI+.
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

// GDI+ �I��
void deInitGdiPlus()
{
	// �t�H���g�f�[�^�̉��
	delete privateFontCollection;
	vector<void*>::const_iterator i = fontDatas.begin();
	while (i != fontDatas.end()) {
		delete[] *i;
		i++;
	}
	fontDatas.clear();
	GdiplusShutdown(gdiplusToken);
}

// --------------------------------------------------------
// �t�H���g���
// --------------------------------------------------------

/**
 * �v���C�x�[�g�t�H���g�̒ǉ�
 * @param fontFileName �t�H���g�t�@�C����
 */
void
GdiPlus::addPrivateFont(const tjs_char *fontFileName)
{
	if (!privateFontCollection) {
		privateFontCollection = new PrivateFontCollection();
	}
	ttstr filename = TVPGetPlacedPath(fontFileName);
	if (filename.length()) {
		if (!wcschr(filename.c_str(), '>')) {
			// ���t�@�C��������
			TVPGetLocalName(filename);
			privateFontCollection->AddFontFile(filename.c_str());
			return;
		} else {
			// �������Ƀ��[�h���ēW�J
			IStream *in = TVPCreateIStream(filename, TJS_BS_READ);
			if (in) {
				STATSTG stat;
				in->Stat(&stat, STATFLAG_NONAME);
				// �T�C�Y���ӂꖳ������
				ULONG size = (ULONG)stat.cbSize.QuadPart;
				char *data = new char[size];
				if (in->Read(data, size, &size) == S_OK) {
					privateFontCollection->AddMemoryFont(data, size);
					fontDatas.push_back(data);					
				} else {
					delete[] data;
				}
				in->Release();
				return;
			}
		}
	}
	TVPThrowExceptionMessage((ttstr(TJS_W("cannot open : ")) + fontFileName).c_str());
}

/**
 * �v���C�x�[�g�t�H���g�ꗗ�����O�ɏo��
 */
void
GdiPlus::showPrivateFontList()
{
	if (!privateFontCollection)	return;
	int count = privateFontCollection->GetFamilyCount();

	// fontCollection.
	FontFamily *families = new FontFamily[count];
	if (families) {
		TVPAddLog("--- private font families ---");
		privateFontCollection->GetFamilies(count, families, &count);
		for (int i=0;i<count;i++) {
			WCHAR familyName[LF_FACESIZE];
			if (families[i].GetFamilyName(familyName) == Ok) {
				TVPAddLog(familyName);
			}
		}
	}
}

// --------------------------------------------------------
// �t�H���g���
// --------------------------------------------------------

/**
 * �R���X�g���N�^
 */
FontInfo::FontInfo() : fontFamily(NULL), emSize(12), style(0) {}

/**
 * �R���X�g���N�^
 * @param familyName �t�H���g�t�@�~���[
 * @param emSize �t�H���g�̃T�C�Y
 * @param style �t�H���g�X�^�C��
 */
FontInfo::FontInfo(const tjs_char *familyName, REAL emSize, INT style) : fontFamily(NULL)
{
	setFamilyName(familyName);
	setEmSize(emSize);
	setStyle(style);
}

/**
 * �R�s�[�R���X�g���N�^
 */
FontInfo::FontInfo(const FontInfo &orig)
{
	fontFamily = orig.fontFamily ? orig.fontFamily->Clone() : NULL;
	emSize = orig.emSize;
	style = orig.style;
}

/**
 * �f�X�g���N�^
 */
FontInfo::~FontInfo()
{
	clear();
}

/**
 * �t�H���g���̃N���A
 */
void
FontInfo::clear()
{
	delete fontFamily;
	fontFamily = NULL;
	familyName = "";
}

/**
 * �t�H���g�̎w��
 */
void
FontInfo::setFamilyName(const tjs_char *familyName)
{
	if (familyName) {
		clear();
		if (privateFontCollection) {
			fontFamily = new FontFamily(familyName, privateFontCollection);
			if (fontFamily->IsAvailable()) {
				this->familyName = familyName;
				return;
			} else {
				clear();
			}
		}
		fontFamily = new FontFamily(familyName);
		if (fontFamily->IsAvailable()) {
			this->familyName = familyName;
			return;
		} else {
			clear();
		}
	}
}

// --------------------------------------------------------
// �A�s�A�����X���
// --------------------------------------------------------

Appearance::Appearance() {}

Appearance::~Appearance()
{
	clear();
}

/**
 * ���̃N���A
 */
void
Appearance::clear()
{
	drawInfos.clear();
}

/**
 * �摜�̐���
 */
static Image *loadImage(const tjs_char *name)
{
	// �摜�ǂݍ���
	Image *image = NULL;
	IStream *in = TVPCreateIStream(name, TJS_BS_READ);
	if (in == NULL) {
		TVPThrowExceptionMessage((ttstr(L"cannot open : ") + name).c_str());
	} else {
		// �摜����
		try {
			image = new Image(in);
			in->Release();
			if (image->GetLastStatus() != Ok) {
				delete image;
				TVPThrowExceptionMessage((ttstr(L"cannot load : ") + name).c_str());
			}
		} catch(...) {
			delete image;
			in->Release();
			throw;
		}
	}
	return image;
}

static bool IsArray(tTJSVariant &var)
{
	if (var.Type() == tvtObject) {
		iTJSDispatch2 *obj = var.AsObjectNoAddRef();
		return obj->IsInstanceOf(0, NULL, NULL, L"Array", obj) == TJS_S_TRUE;
	}
	return false;
}

/**
 * ���W���̐���
 */
static PointF getPoint(tTJSVariant &var)
{
	ncbPropAccessor info(var);
	if (IsArray(var)) {
		return PointF((REAL)info.getRealValue(0),
					  (REAL)info.getRealValue(1));
	} else {
		return PointF((REAL)info.getRealValue(L"x"),
					  (REAL)info.getRealValue(L"y"));
	}
}

/**
 * �F�̔z����擾
 */
static void getPoints(tTJSVariant &var, vector<PointF> &points)
{
	ncbPropAccessor info(var);
	int c = info.GetArrayCount();
	for (int i=0;i<c;i++) {
		tTJSVariant p;
		if (info.checkVariant(i, p)) {
			points.push_back(getPoint(p));
		}
	}
}

static void getPoints(ncbPropAccessor &info, int n, vector<PointF> &points)
{
	tTJSVariant var;
	if (info.checkVariant(n, var)) {
		getPoints(var, points);
	}
}

static void getPoints(ncbPropAccessor &info, const tjs_char *n, vector<PointF> &points)
{
	tTJSVariant var;
	if (info.checkVariant(n, var)) {
		getPoints(var, points);
	}
}

/**
 * ��`���̐���
 */
static RectF getRect(tTJSVariant &var)
{
	ncbPropAccessor info(var);
	if (IsArray(var)) {
		return RectF((REAL)info.getRealValue(0),
					 (REAL)info.getRealValue(1),
					 (REAL)info.getRealValue(2),
					 (REAL)info.getRealValue(3));
	} else {
		return RectF((REAL)info.getRealValue(L"x"),
					 (REAL)info.getRealValue(L"y"),
					 (REAL)info.getRealValue(L"width"),
					 (REAL)info.getRealValue(L"height"));
	}
}

/**
 * �����̔z����擾
 */
static void getReals(tTJSVariant &var, vector<REAL> &points)
{
	ncbPropAccessor info(var);
	int c = info.GetArrayCount();
	for (int i=0;i<c;i++) {
		points.push_back((REAL)info.getRealValue(i));
	}
}

static void getReals(ncbPropAccessor &info, int n, vector<REAL> &points)
{
	tTJSVariant var;
	if (info.checkVariant(n, var)) {
		getReals(var, points);
	}
}

static void getReals(ncbPropAccessor &info, const tjs_char *n, vector<REAL> &points)
{
	tTJSVariant var;
	if (info.checkVariant(n, var)) {
		getReals(var, points);
	}
}

/**
 * �F�̔z����擾
 */
static void getColors(tTJSVariant &var, vector<Color> &colors)
{
	ncbPropAccessor info(var);
	int c = info.GetArrayCount();
	for (int i=0;i<c;i++) {
		colors.push_back(Color((ARGB)info.getIntValue(i)));
	}
}

static void getColors(ncbPropAccessor &info, int n, vector<Color> &colors)
{
	tTJSVariant var;
	if (info.checkVariant(n, var)) {
		getColors(var, colors);
	}
}

static void getColors(ncbPropAccessor &info, const tjs_char *n, vector<Color> &colors)
{
	tTJSVariant var;
	if (info.checkVariant(n, var)) {
		getColors(var, colors);
	}
}

template <class T>
void commonBrushParameter(ncbPropAccessor &info, T *brush)
{
	tTJSVariant var;
	// SetBlend
	if (info.checkVariant(L"blend", var)) {
		vector<REAL> factors;
		vector<REAL> positions;
		ncbPropAccessor binfo(var);
		if (IsArray(var)) {
			getReals(binfo, 0, factors);
			getReals(binfo, 1, positions);
		} else {
			getReals(binfo, L"blendFactors", factors);
			getReals(binfo, L"blendPositions", positions);
		}
		int count = (int)factors.size();
		if ((int)positions.size() > count) {
			count = (int)positions.size();
		}
		if (count > 0) {
			brush->SetBlend(&factors[0], &positions[0], count);
		}
	}
	// SetBlendBellShape
	if (info.checkVariant(L"blendBellShape", var)) {
		ncbPropAccessor sinfo(var);
		if (IsArray(var)) {
			brush->SetBlendBellShape((REAL)sinfo.getRealValue(0),
									 (REAL)sinfo.getRealValue(1));
		} else {
			brush->SetBlendBellShape((REAL)info.getRealValue(L"focus"),
									 (REAL)info.getRealValue(L"scale"));
		}
	}
	// SetBlendTriangularShape
	if (info.checkVariant(L"blendTriangularShape", var)) {
		ncbPropAccessor sinfo(var);
		if (IsArray(var)) {
			brush->SetBlendTriangularShape((REAL)sinfo.getRealValue(0),
										   (REAL)sinfo.getRealValue(1));
		} else {
			brush->SetBlendTriangularShape((REAL)info.getRealValue(L"focus"),
										   (REAL)info.getRealValue(L"scale"));
		}
	}
	// SetGammaCorrection
	if (info.checkVariant(L"useGammaCorrection", var)) {
		brush->SetGammaCorrection((BOOL)var);
	}
	// SetInterpolationColors
	if (info.checkVariant(L"interpolationColors", var)) {
		vector<Color> colors;
		vector<REAL> positions;
		ncbPropAccessor binfo(var);
		if (IsArray(var)) {
			getColors(binfo, 0, colors);
			getReals(binfo, 1, positions);
		} else {
			getColors(binfo, L"presetColors", colors);
			getReals(binfo, L"blendPositions", positions);
		}
		int count = (int)colors.size();
		if ((int)positions.size() > count) {
			count = (int)positions.size();
		}
		if (count > 0) {
			brush->SetInterpolationColors(&colors[0], &positions[0], count);
		}
	}
}

/**
 * �u���V�̐���
 */
Brush* createBrush(tTJSVariant colorOrBrush)
{
	Brush *brush;
	if (colorOrBrush.Type() != tvtObject) {
		brush = new SolidBrush(Color((tjs_int)colorOrBrush));
	} else {
		// ��ʂ��Ƃɍ�蕪����
		ncbPropAccessor info(colorOrBrush);
		BrushType type = (BrushType)info.getIntValue(L"type", BrushTypeSolidColor);
		switch (type) {
		case BrushTypeSolidColor:
			brush = new SolidBrush(Color((ARGB)info.getIntValue(L"color", 0xffffffff)));
			break;
		case BrushTypeHatchFill:
			brush = new HatchBrush((HatchStyle)info.getIntValue(L"hatchStyle", HatchStyleHorizontal),
								   Color((ARGB)info.getIntValue(L"foreColor", 0xffffffff)),
								   Color((ARGB)info.getIntValue(L"backColor", 0xff000000)));
			break;
		case BrushTypeTextureFill:
			{
				ttstr imgname = info.GetValue(L"image", ncbTypedefs::Tag<ttstr>());
				Image *image = loadImage(imgname.c_str());
				WrapMode wrapMode = (WrapMode)info.getIntValue(L"wrapMode", WrapModeTile);
				tTJSVariant dstRect;
				if (info.checkVariant(L"dstRect", dstRect)) {
					brush = new TextureBrush(image, wrapMode, getRect(dstRect));
				} else {
					brush = new TextureBrush(image, wrapMode);
				}
				delete image;
				break;
			}
		case BrushTypePathGradient:
			{
				PathGradientBrush *pbrush;
				vector<PointF> points;
				getPoints(info, L"points", points);
				WrapMode wrapMode = (WrapMode)info.getIntValue(L"wrapMode", WrapModeTile);
				pbrush = new PathGradientBrush(&points[0], (int)points.size(), wrapMode);

				// ���ʃp�����[�^
				commonBrushParameter(info, pbrush);

				tTJSVariant var;
				//SetCenterColor
				if (info.checkVariant(L"centerColor", var)) {
					pbrush->SetCenterColor(Color((ARGB)(tjs_int)var));
				}
				//SetCenterPoint
				if (info.checkVariant(L"centerPoint", var)) {
					pbrush->SetCenterPoint(getPoint(var));
				}
				//SetFocusScales
				if (info.checkVariant(L"focusScales", var)) {
					ncbPropAccessor sinfo(var);
					if (IsArray(var)) {
						pbrush->SetFocusScales((REAL)sinfo.getRealValue(0),
											   (REAL)sinfo.getRealValue(1));
					} else {
						pbrush->SetFocusScales((REAL)info.getRealValue(L"xScale"),
											   (REAL)info.getRealValue(L"yScale"));
					}
				}
				//SetSurroundColors
				if (info.checkVariant(L"interpolationColors", var)) {
					vector<Color> colors;
					getColors(var, colors);
					int size = (int)colors.size();
					pbrush->SetSurroundColors(&colors[0], &size);
				}
				brush = pbrush;
			}
			break;
		case BrushTypeLinearGradient:
			{
				LinearGradientBrush *lbrush;
				Color color1((ARGB)info.getIntValue(L"color1", 0));
				Color color2((ARGB)info.getIntValue(L"color2", 0));

				tTJSVariant var;
				if (info.checkVariant(L"point1", var)) {
					PointF point1 = getPoint(var);
					info.checkVariant(L"point2", var);
					PointF point2 = getPoint(var);
					lbrush = new LinearGradientBrush(point1, point2, color1, color2);
				} else if (info.checkVariant(L"rect", var)) {
					RectF rect = getRect(var);
					if (info.HasValue(L"angle")) {
						// �A���O���w�肪����ꍇ
						lbrush = new LinearGradientBrush(rect, color1, color2,
														 (REAL)info.getRealValue(L"angle", 0),
														 (BOOL)info.getIntValue(L"isAngleScalable", 0));
					} else {
						// �����ꍇ�̓��[�h���Q��
						lbrush = new LinearGradientBrush(rect, color1, color2,
														 (LinearGradientMode)info.getIntValue(L"mode", LinearGradientModeHorizontal));
					}
				} else {
					TVPThrowExceptionMessage(L"must set point1,2 or rect");
				}

				// ���ʃp�����[�^
				commonBrushParameter(info, lbrush);

				// SetWrapMode
				if (info.checkVariant(L"wrapMode", var)) {
					lbrush->SetWrapMode((WrapMode)(tjs_int)var);
				}
				brush = lbrush;
			}
			break;
		default:
			TVPThrowExceptionMessage(L"invalid brush type");
			break;
		}
	}
	return brush;
}

/**
 * �u���V�̒ǉ�
 * @param colorOrBrush ARGB�F�w��܂��̓u���V���i�����j
 * @param ox �\���I�t�Z�b�gX
 * @param oy �\���I�t�Z�b�gY
 */
void
Appearance::addBrush(tTJSVariant colorOrBrush, REAL ox, REAL oy)
{
	drawInfos.push_back(DrawInfo(ox, oy, createBrush(colorOrBrush)));
}

/**
 * �y���̒ǉ�
 * @param colorOrBrush ARGB�F�w��܂��̓u���V���i�����j
 * @param widthOrOption �y�����܂��̓y�����i�����j
 * @param ox �\���I�t�Z�b�gX
 * @param oy �\���I�t�Z�b�gY
 */
void
Appearance::addPen(tTJSVariant colorOrBrush, tTJSVariant widthOrOption, REAL ox, REAL oy)
{
	Pen *pen;
	REAL width = 1.0;
	if (colorOrBrush.Type() == tvtObject) {
		Brush *brush = createBrush(colorOrBrush);
		pen = new Pen(brush, width);
		delete brush;
	} else {
		pen = new Pen(Color((ARGB)(tjs_int)colorOrBrush), width);
	}
	if (widthOrOption.Type() != tvtObject) {
		pen->SetWidth((REAL)(tjs_real)widthOrOption);
	} else {
		ncbPropAccessor info(widthOrOption);
		
		tTJSVariant var;

		// SetWidth
		if (info.checkVariant(L"width", var)) {
			pen->SetWidth((REAL)(tjs_real)var);
		}
		
		// SetAlignment
		if (info.checkVariant(L"alignment", var)) {
			pen->SetAlignment((PenAlignment)(tjs_int)var);
		}
		// SetCompoundArray
		if (info.checkVariant(L"compoundArray", var)) {
			vector<REAL> reals;
			getReals(var, reals);
			pen->SetCompoundArray(&reals[0], (int)reals.size());
		}

		// SetDashCap
		if (info.checkVariant(L"dashCap", var)) {
			pen->SetDashCap((DashCap)(tjs_int)var);
		}
		// SetDashOffset
		if (info.checkVariant(L"dashOffset", var)) {
			pen->SetDashOffset((REAL)(tjs_real)var);
		}

		// SetDashStyle
		// SetDashPattern
		if (info.checkVariant(L"dashStyle", var)) {
			if (IsArray(var)) {
				vector<REAL> reals;
				getReals(var, reals);
				pen->SetDashStyle(DashStyleCustom);
				pen->SetDashPattern(&reals[0], (int)reals.size());
			} else {
				pen->SetDashStyle((DashStyle)(tjs_int)var);
			}
		}

		// SetStartCap
		// SetCustomStartCap XXX
		if (info.checkVariant(L"startCap", var)) {
			pen->SetStartCap((LineCap)(tjs_int)var);
		}

		// SetEndCap
		// SetCustomEndCap XXX
		if (info.checkVariant(L"endCap", var)) {
			pen->SetEndCap((LineCap)(tjs_int)var);
		}

		// SetLineJoin
		if (info.checkVariant(L"lineJoin", var)) {
			pen->SetLineJoin((LineJoin)(tjs_int)var);
		}
		
		// SetMiterLimit
		if (info.checkVariant(L"miterLimit", var)) {
			pen->SetMiterLimit((REAL)(tjs_real)var);
		}
	}
	drawInfos.push_back(DrawInfo(ox, oy, pen));
}

// --------------------------------------------------------
// �t�H���g�`��n
// --------------------------------------------------------

/**
 * �R���X�g���N�^
 */
LayerExDraw::LayerExDraw(DispatchT obj)
	: layerExBase(obj), width(-1), height(-1), pitch(0), buffer(NULL), bitmap(NULL), graphics(NULL), updateWhenDraw(true)
{
}

/**
 * �f�X�g���N�^
 */
LayerExDraw::~LayerExDraw()
{
	delete graphics;
	delete bitmap;
}

void
LayerExDraw::reset()
{
	layerExBase::reset();
	// �ύX����Ă���ꍇ�͂���Ȃ���
	if (!(graphics &&
		  width  == _width &&
		  height == _height &&
		  pitch  == _pitch &&
		  buffer == _buffer)) {
		delete graphics;
		delete bitmap;
		width  = _width;
		height = _height;
		pitch  = _pitch;
		buffer = _buffer;
		bitmap = new Bitmap(width, height, pitch, PixelFormat32bppARGB, (unsigned char*)buffer);
		graphics = new Graphics(bitmap);
		graphics->SetSmoothingMode(SmoothingModeAntiAlias);
		graphics->SetCompositingMode(CompositingModeSourceOver);
		graphics->SetTextRenderingHint(TextRenderingHintAntiAlias);
	}
}

/**
 * ��ʂ̏���
 * @param argb �����F
 */
void
LayerExDraw::clear(ARGB argb)
{
	graphics->Clear(Color(argb));
}

/**
 * �p�X��`�悷��
 * @param app �\���\��
 * @param path �`�悷��p�X
 */
void
LayerExDraw::drawPath(const Appearance *app, const GraphicsPath *path)
{
	// �̈�L�^�p
	Rect unionRect;
	Rect rect;

	// �`������g���Ď��X�`��
	vector<Appearance::DrawInfo>::const_iterator i = app->drawInfos.begin();
	while (i != app->drawInfos.end()) {
		if (i->info) {
			Matrix matrix(1,0,0,1,i->ox,i->oy);
			graphics->SetTransform(&matrix);
			switch (i->type) {
			case 0:
				{
					Pen *pen = (Pen*)i->info;
					graphics->DrawPath(pen, path);
					if (updateWhenDraw) {
						path->GetBounds(&rect, &matrix, pen);
						unionRect.Union(unionRect, unionRect, rect);
					}
				}
				break;
			case 1:
				graphics->FillPath((Brush*)i->info, path);
				if (updateWhenDraw) {
					path->GetBounds(&rect, &matrix, NULL);
					unionRect.Union(unionRect, unionRect, rect);
				}
				break;
			}
		}
		i++;
	}

	if (updateWhenDraw) {
		// �X�V����
		tTJSVariant  vars [4] = { unionRect.X, unionRect.Y, unionRect.Width, unionRect.Height };
		tTJSVariant *varsp[4] = { vars, vars+1, vars+2, vars+3 };
		_pUpdate(4, varsp);
	}
}

/**
 * �~�ʂ̕`��
 * @param x ������W
 * @param y ������W
 * @param width ����
 * @param height �c��
 * @param startAngle ���v�����~�ʊJ�n�ʒu
 * @param sweepAngle �`��p�x
 */
void
LayerExDraw::drawArc(const Appearance *app, REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
{
	GraphicsPath path;
	path.AddArc(x, y, width, height, startAngle, sweepAngle);
	drawPath(app, &path);
}

/**
 * �x�W�F�Ȑ��̕`��
 * @param app �A�s�A�����X
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 * @param x3
 * @param y3
 * @param x4
 * @param y4
 */
void
LayerExDraw::drawBezier(const Appearance *app, REAL x1, REAL y1, REAL x2, REAL y2, REAL x3, REAL y3, REAL x4, REAL y4)
{
	GraphicsPath path;
	path.AddBezier(x1, y1, x2, y2, x3, y3, x4, y4);
	drawPath(app, &path);
}

/**
 * �~���̕`��
 * @param x ������W
 * @param y ������W
 * @param width ����
 * @param height �c��
 * @param startAngle ���v�����~�ʊJ�n�ʒu
 * @param sweepAngle �`��p�x
 */
void
LayerExDraw::drawPie(const Appearance *app, REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
{
	GraphicsPath path;
	path.AddPie(x, y, width, height, startAngle, sweepAngle);
	drawPath(app, &path);
}

/**
 * �ȉ~�̕`��
 * @param app �A�s�A�����X
 * @param x
 * @param y
 * @param width
 * @param height
 */
void
LayerExDraw::drawEllipse(const Appearance *app, REAL x, REAL y, REAL width, REAL height)
{
	GraphicsPath path;
	path.AddEllipse(x, y, width, height);
	drawPath(app, &path);
}

/**
 * �����̕`��
 * @param app �A�s�A�����X
 * @param x1 �n�_X���W
 * @param y1 �n�_Y���W
 * @param x2 �I�_X���W
 * @param y2 �I�_Y���W
 */
void
LayerExDraw::drawLine(const Appearance *app, REAL x1, REAL y1, REAL x2, REAL y2)
{
	GraphicsPath path;
	path.AddLine(x1, y1, x2, y2);
	drawPath(app, &path);
}

/**
 * ��`�̕`��
 * @param app �A�s�A�����X
 * @param x
 * @param y
 * @param width
 * @param height
 */
void
LayerExDraw::drawRectangle(const Appearance *app, REAL x, REAL y, REAL width, REAL height)
{
	GraphicsPath path;
	RectF rect(x, y, width, height);
	path.AddRectangle(rect);
	drawPath(app, &path);
}

/**
 * ������̕`��
 * @param font �t�H���g
 * @param app �A�s�A�����X
 * @param x �`��ʒuX
 * @param y �`��ʒuY
 * @param text �`��e�L�X�g
 */
void
LayerExDraw::drawString(const FontInfo *font, const Appearance *app, REAL x, REAL y, const tjs_char *text)
{
	// ������̃p�X������
	GraphicsPath path;
	path.AddString(text, -1, font->fontFamily, font->style, font->emSize, PointF(x, y), NULL);
	drawPath(app, &path);
}

/**
 * �摜�̕`��
 * @param name �摜��
 * @param x �\���ʒuX
 * @param y �\���ʒuY
 */
void
LayerExDraw::drawImage(REAL x, REAL y, const tjs_char *name)
{
	Image *image = loadImage(name);
	graphics->DrawImage(image, x, y);
	delete image;
}
