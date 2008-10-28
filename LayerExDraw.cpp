#pragma comment(lib, "gdiplus.lib")
#include "ncbind/ncbind.hpp"
#include "LayerExDraw.hpp"
#include <stdio.h>

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

/**
 * �摜�ǂݍ��ݏ���
 * @param name �t�@�C����
 * @return �摜���
 */
Image *loadImage(const tjs_char *name)
{
	Image *image = NULL;
	ttstr filename = TVPGetPlacedPath(name);
	if (filename.length()) {
		if (false && !wcschr(filename.c_str(), '>')) {// �ɂ�����ςɂȂ�̂ł��
			// ���t�@�C��������
			TVPGetLocalName(filename);
			image = Image::FromFile(filename.c_str(),false);
		} else {
			// ���ڋg���g�������������X�g���[�����g���ƂȂ���wmf/emf��OutOfMemory
			// �Ȃ�ꍇ������悤�Ȃ̂ł������񃁃����Ƀ������ɓW�J���Ă���g��
			IStream *in = TVPCreateIStream(filename, TJS_BS_READ);
			if (in) {
				STATSTG stat;
				in->Stat(&stat, STATFLAG_NONAME);
				// �T�C�Y���ӂꖳ������
				ULONG size = (ULONG)stat.cbSize.QuadPart;
				HGLOBAL hBuffer = ::GlobalAlloc(GMEM_MOVEABLE, size);
				if (hBuffer)	{
					void* pBuffer = ::GlobalLock(hBuffer);
					if (pBuffer) {
						if (in->Read(pBuffer, size, &size) == S_OK) {
							IStream* pStream = NULL;
							if(::CreateStreamOnHGlobal(hBuffer, FALSE, &pStream) == S_OK) 	{
								image = Image::FromStream(pStream,false);
								pStream->Release();
							}
						}
						::GlobalUnlock(hBuffer);
					}
					::GlobalFree(hBuffer);
				}
				in->Release();
			}
		}
	}
	if (image && image->GetLastStatus() != Ok) {
		delete image;
		image = NULL;
	}
	if (!image) {
		TVPThrowExceptionMessage((ttstr(L"cannot load : ") + name).c_str());
	}
	return image;
}

RectF *getBounds(Image *image)
{
	RectF srcRect;
	Unit srcUnit;
	image->GetBounds(&srcRect, &srcUnit);
	REAL dpix = image->GetHorizontalResolution();
	REAL dpiy = image->GetVerticalResolution();

	// �s�N�Z���ɕϊ�
	REAL x, y, width, height;
	switch (srcUnit) {
	case UnitPoint:		// 3 -- Each unit is a printer's point, or 1/72 inch.
		x = srcRect.X * dpix / 72;
		y = srcRect.Y * dpiy / 72;
		width  = srcRect.Width * dpix / 72;
		height = srcRect.Height * dpix / 72;
		break;
	case UnitInch:       // 4 -- Each unit is 1 inch.
		x = srcRect.X * dpix;
		y = srcRect.Y * dpiy;
		width  = srcRect.Width * dpix;
		height = srcRect.Height * dpix;
		break;
	case UnitDocument:   // 5 -- Each unit is 1/300 inch.
		x = srcRect.X * dpix / 300;
		y = srcRect.Y * dpiy / 300;
		width  = srcRect.Width * dpix / 300;
		height = srcRect.Height * dpix / 300;
		break;
	case UnitMillimeter: // 6 -- Each unit is 1 millimeter.
		x = srcRect.X * dpix / 25.4F;
		y = srcRect.Y * dpiy / 25.4F;
		width  = srcRect.Width * dpix / 25.4F;
		height = srcRect.Height * dpix / 25.4F;
		break;
	default:
		x = srcRect.X;
		y = srcRect.Y;
		width  = srcRect.Width;
		height = srcRect.Height;
		break;
	}
	return new RectF(x, y, width, height);
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
 * �z��Ƀt�H���g�̃t�@�~���[�����i�[
 * @param array �i�[��z��
 * @param fontCollection �t�H���g�����擾���錳�� FontCollection
 */
static void addFontFamilyName(iTJSDispatch2 *array, FontCollection *fontCollection)
{
	int count = fontCollection->GetFamilyCount();
	FontFamily *families = new FontFamily[count];
	if (families) {
		fontCollection->GetFamilies(count, families, &count);
		for (int i=0;i<count;i++) {
			WCHAR familyName[LF_FACESIZE];
			if (families[i].GetFamilyName(familyName) == Ok) {
				tTJSVariant name(familyName), *param = &name;
				array->FuncCall(0, TJS_W("add"), NULL, 0, 1, &param, array);
			}
		}
		delete families;
	}
}

/**
 * �t�H���g�ꗗ�̎擾
 * @param privateOnly true �Ȃ�v���C�x�[�g�t�H���g�̂ݎ擾
 */
tTJSVariant
GdiPlus::getFontList(bool privateOnly)
{
	iTJSDispatch2 *array = TJSCreateArrayObject();
	if (privateFontCollection)	{
		addFontFamilyName(array, privateFontCollection);
	}
	if (!privateOnly) {
		InstalledFontCollection installedFontCollection;
		addFontFamilyName(array, &installedFontCollection);
	}
	tTJSVariant ret(array,array);
	array->Release();
	return ret;
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

// --------------------------------------------------------
// �e�^�ϊ�����
// --------------------------------------------------------

extern bool IsArray(const tTJSVariant &var);

/**
 * ���W���̐���
 */
extern PointF getPoint(const tTJSVariant &var);

/**
 * �_�̔z����擾
 */
static void getPoints(const tTJSVariant &var, vector<PointF> &points)
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

// -----------------------------

/**
 * ��`���̐���
 */
extern RectF getRect(const tTJSVariant &var);

/**
 * ��`�̔z����擾
 */
static void getRects(const tTJSVariant &var, vector<RectF> &rects)
{
	ncbPropAccessor info(var);
	int c = info.GetArrayCount();
	for (int i=0;i<c;i++) {
		tTJSVariant p;
		if (info.checkVariant(i, p)) {
			rects.push_back(getRect(p));
		}
	}
}

// -----------------------------

/**
 * �����̔z����擾
 */
static void getReals(const tTJSVariant &var, vector<REAL> &points)
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

// -----------------------------

/**
 * �F�̔z����擾
 */
static void getColors(const tTJSVariant &var, vector<Color> &colors)
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
Brush* createBrush(const tTJSVariant colorOrBrush)
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

void
LayerExDraw::updateRect(RectF &rect)
{
	if (updateWhenDraw) {
		// �X�V����
		tTJSVariant  vars [4] = { rect.X, rect.Y, rect.Width, rect.Height };
		tTJSVariant *varsp[4] = { vars, vars+1, vars+2, vars+3 };
		_pUpdate(4, varsp);
	}
}

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
		graphics->SetPageUnit(UnitPixel);
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
 * �p�X�̗̈�����擾
 * @param app �\���\��
 * @param path �`�悷��p�X
 */
RectF
LayerExDraw::getPathExtents(const Appearance *app, const GraphicsPath *path)
{
	// �̈�L�^�p
	RectF rect;

	// �`������g���Ď��X�`��
	bool first = true;
	vector<Appearance::DrawInfo>::const_iterator i = app->drawInfos.begin();
	while (i != app->drawInfos.end()) {
		if (i->info) {
			Matrix matrix(1,0,0,1,i->ox,i->oy);
			graphics->SetTransform(&matrix);
			switch (i->type) {
			case 0:
				{
					Pen *pen = (Pen*)i->info;
					if (first) {
						path->GetBounds(&rect, &matrix, pen);
						first = false;
					} else {
						RectF r;
						path->GetBounds(&r, &matrix, pen);
						rect.Union(rect, rect, r);
					}
				}
				break;
			case 1:
				if (first) {
					path->GetBounds(&rect, &matrix, NULL);
					first = false;
				} else {
					RectF r;
					path->GetBounds(&r, &matrix, NULL);
					rect.Union(rect, rect, r);
				}
				break;
			}
		}
		i++;
	}
	return rect;
}

/**
 * �p�X��`�悷��
 * @param app �\���\��
 * @param path �`�悷��p�X
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawPath(const Appearance *app, const GraphicsPath *path)
{
	// �̈�L�^�p
	RectF rect;

	// �`������g���Ď��X�`��
	bool first = true;
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
					if (first) {
						path->GetBounds(&rect, &matrix, pen);
						first = false;
					} else {
						RectF r;
						path->GetBounds(&r, &matrix, pen);
						rect.Union(rect, rect, r);
					}
				}
				break;
			case 1:
				graphics->FillPath((Brush*)i->info, path);
				if (first) {
					path->GetBounds(&rect, &matrix, NULL);
					first = false;
				} else {
					RectF r;
					path->GetBounds(&r, &matrix, NULL);
					rect.Union(rect, rect, r);
				}
				break;
			}
		}
		i++;
	}
	updateRect(rect);
	return rect;
}

/**
 * �~�ʂ̕`��
 * @param x ������W
 * @param y ������W
 * @param width ����
 * @param height �c��
 * @param startAngle ���v�����~�ʊJ�n�ʒu
 * @param sweepAngle �`��p�x
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawArc(const Appearance *app, REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
{
	GraphicsPath path;
	path.AddArc(x, y, width, height, startAngle, sweepAngle);
	return drawPath(app, &path);
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
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawBezier(const Appearance *app, REAL x1, REAL y1, REAL x2, REAL y2, REAL x3, REAL y3, REAL x4, REAL y4)
{
	GraphicsPath path;
	path.AddBezier(x1, y1, x2, y2, x3, y3, x4, y4);
	return drawPath(app, &path);
}

/**
 * �A���x�W�F�Ȑ��̕`��
 * @param app �A�s�A�����X
 * @param points �_�̔z��
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawBeziers(const Appearance *app, tTJSVariant points)
{
	vector<PointF> ps;
	getPoints(points, ps);
	GraphicsPath path;
	path.AddBeziers(&ps[0], (int)ps.size());
	return drawPath(app, &path);
}

/**
 * Closed cardinal spline �̕`��
 * @param app �A�s�A�����X
 * @param points �_�̔z��
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawClosedCurve(const Appearance *app, tTJSVariant points)
{
	vector<PointF> ps;
	getPoints(points, ps);
	GraphicsPath path;
	path.AddClosedCurve(&ps[0], (int)ps.size());
	return drawPath(app, &path);
}

/**
 * Closed cardinal spline �̕`��
 * @param app �A�s�A�����X
 * @param points �_�̔z��
 * @pram tension tension
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawClosedCurve2(const Appearance *app, tTJSVariant points, REAL tension)
{
	vector<PointF> ps;
	getPoints(points, ps);
	GraphicsPath path;
	path.AddClosedCurve(&ps[0], (int)ps.size(), tension);
	return drawPath(app, &path);
}

/**
 * cardinal spline �̕`��
 * @param app �A�s�A�����X
 * @param points �_�̔z��
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawCurve(const Appearance *app, tTJSVariant points)
{
	vector<PointF> ps;
	getPoints(points, ps);
	GraphicsPath path;
	path.AddCurve(&ps[0], (int)ps.size());
	return drawPath(app, &path);
}

/**
 * cardinal spline �̕`��
 * @param app �A�s�A�����X
 * @param points �_�̔z��
 * @parma tension tension
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawCurve2(const Appearance *app, tTJSVariant points, REAL tension)
{
	vector<PointF> ps;
	getPoints(points, ps);
	GraphicsPath path;
	path.AddCurve(&ps[0], (int)ps.size(), tension);
	return drawPath(app, &path);
}

/**
 * cardinal spline �̕`��
 * @param app �A�s�A�����X
 * @param points �_�̔z��
 * @param offset
 * @param numberOfSegments
 * @param tension tension
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawCurve3(const Appearance *app, tTJSVariant points, int offset, int numberOfSegments, REAL tension)
{
	vector<PointF> ps;
	getPoints(points, ps);
	GraphicsPath path;
	path.AddCurve(&ps[0], (int)ps.size(), offset, numberOfSegments, tension);
	return drawPath(app, &path);
}

/**
 * �~���̕`��
 * @param x ������W
 * @param y ������W
 * @param width ����
 * @param height �c��
 * @param startAngle ���v�����~�ʊJ�n�ʒu
 * @param sweepAngle �`��p�x
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawPie(const Appearance *app, REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
{
	GraphicsPath path;
	path.AddPie(x, y, width, height, startAngle, sweepAngle);
	return drawPath(app, &path);
}

/**
 * �ȉ~�̕`��
 * @param app �A�s�A�����X
 * @param x
 * @param y
 * @param width
 * @param height
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawEllipse(const Appearance *app, REAL x, REAL y, REAL width, REAL height)
{
	GraphicsPath path;
	path.AddEllipse(x, y, width, height);
	return drawPath(app, &path);
}

/**
 * �����̕`��
 * @param app �A�s�A�����X
 * @param x1 �n�_X���W
 * @param y1 �n�_Y���W
 * @param x2 �I�_X���W
 * @param y2 �I�_Y���W
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawLine(const Appearance *app, REAL x1, REAL y1, REAL x2, REAL y2)
{
	GraphicsPath path;
	path.AddLine(x1, y1, x2, y2);
	return drawPath(app, &path);
}

/**
 * �A�������̕`��
 * @param app �A�s�A�����X
 * @param points �_�̔z��
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawLines(const Appearance *app, tTJSVariant points)
{
	vector<PointF> ps;
	getPoints(points, ps);
	GraphicsPath path;
	path.AddLines(&ps[0], (int)ps.size());
	return drawPath(app, &path);
}

/**
 * ���p�`�̕`��
 * @param app �A�s�A�����X
 * @param points �_�̔z��
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawPolygon(const Appearance *app, tTJSVariant points)
{
	vector<PointF> ps;
	getPoints(points, ps);
	GraphicsPath path;
	path.AddPolygon(&ps[0], (int)ps.size());
	return drawPath(app, &path);
}


/**
 * ��`�̕`��
 * @param app �A�s�A�����X
 * @param x
 * @param y
 * @param width
 * @param height
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawRectangle(const Appearance *app, REAL x, REAL y, REAL width, REAL height)
{
	GraphicsPath path;
	RectF rect(x, y, width, height);
	path.AddRectangle(rect);
	return drawPath(app, &path);
}

/**
 * ������`�̕`��
 * @param app �A�s�A�����X
 * @param rects ��`���̔z��
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawRectangles(const Appearance *app, tTJSVariant rects)
{
	vector<RectF> rs;
	getRects(rects, rs);
	GraphicsPath path;
	path.AddRectangles(&rs[0], (int)rs.size());
	return drawPath(app, &path);
}

/**
 * ������̕`��
 * @param font �t�H���g
 * @param app �A�s�A�����X
 * @param x �`��ʒuX
 * @param y �`��ʒuY
 * @param text �`��e�L�X�g
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawString(const FontInfo *font, const Appearance *app, REAL x, REAL y, const tjs_char *text)
{
	// ������̃p�X������
	GraphicsPath path;
	path.AddString(text, -1, font->fontFamily, font->style, font->emSize, PointF(x, y), StringFormat::GenericDefault());
	return drawPath(app, &path);
}


/**
 * ������̕`��̈���̎擾
 * @param font �t�H���g
 * @param app �A�s�A�����X
 * @param text �`��e�L�X�g
 * @return �`��̈���
 */
RectF
LayerExDraw::measureString(const FontInfo *font, const tjs_char *text)
{
	RectF rect;
	Font f(font->fontFamily, font->emSize, font->style, UnitPixel);
	graphics->MeasureString(text, -1, &f, PointF(0,0), StringFormat::GenericDefault(), &rect);
	return rect;
}


/**
 * �摜�̕`��B�R�s�[��͌��摜�� Bounds ��z�������ʒu�A�T�C�Y�� Pixel �w��ɂȂ�܂��B
 * @param x �R�s�[�挴�_
 * @param y  �R�s�[�挴�_
 * @param src �R�s�[���摜
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawImage(REAL x, REAL y, Image *src) 
{
	RectF rect;
	if (src) {
		RectF *bounds = getBounds(src);
		graphics->DrawImage(src, (rect.X = x + bounds->X), (rect.Y = y + bounds->Y), bounds->Width, bounds->Height);
		rect.Width  = bounds->Width;
		rect.Height = bounds->Height;
		updateRect(rect);
		delete bounds;
	}
	return rect;
}


/**
 * �摜�̋�`�R�s�[
 * @param dleft �R�s�[�捶�[
 * @param dtop  �R�s�[���[
 * @param src �R�s�[���摜
 * @param sleft ����`�̍��[
 * @param stop  ����`�̏�[
 * @param swidth ����`�̉���
 * @param sheight  ����`�̏c��
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawImageRect(REAL dleft, REAL dtop, Image *src, REAL sleft, REAL stop, REAL swidth, REAL sheight)
{
	return drawImageStretch(dleft, dtop, swidth , sheight, src, sleft, stop, swidth, sheight);
}

/**
 * �摜�̊g��k���R�s�[
 * @param dleft �R�s�[�捶�[
 * @param dtop  �R�s�[���[
 * @param dwidth �R�s�[��̉���
 * @param dheight  �R�s�[��̏c��
 * @param src �R�s�[���摜
 * @param sleft ����`�̍��[
 * @param stop  ����`�̏�[
 * @param swidth ����`�̉���
 * @param sheight  ����`�̏c��
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawImageStretch(REAL dleft, REAL dtop, REAL dwidth, REAL dheight, Image *src, REAL sleft, REAL stop, REAL swidth, REAL sheight)
{
	RectF destRect(dleft, dtop, dwidth, dheight);
	if (src) {
		graphics->DrawImage(src, destRect, sleft, stop, swidth, sheight, UnitPixel);
	}
	updateRect(destRect);
	return destRect;
}

static int compare_REAL(const REAL *a, const REAL *b)
{
	return (int)(*a - *b);
}

/**
 * �摜�̃A�t�B���ϊ��R�s�[
 * @param sleft ����`�̍��[
 * @param stop  ����`�̏�[
 * @param swidth ����`�̉���
 * @param sheight  ����`�̏c��
 * @param affine �A�t�B���p�����[�^�̎��(true:�ϊ��s��, false:���W�w��), 
 * @return �X�V�̈���
 */
RectF
LayerExDraw::drawImageAffine(Image *src, REAL sleft, REAL stop, REAL swidth, REAL sheight, bool affine, REAL A, REAL B, REAL C, REAL D, REAL E, REAL F)
{
	RectF rect;
	RectF srcRect(sleft, stop, swidth, sheight);
	REAL x[4], y[4]; // �����W�l
	if (affine) {
		REAL x2 = sleft+swidth;
		REAL y2 = stop +sheight;
#define AFFINEX(x,y) A*x+C*y+E
#define AFFINEY(x,y) B*x+D*y+F
		x[0] = AFFINEX(sleft,stop);
		y[0] = AFFINEY(sleft,stop);
		x[1] = AFFINEX(x2,stop);
		y[1] = AFFINEY(x2,stop);
		x[2] = AFFINEX(sleft,y2);
		y[2] = AFFINEY(sleft,y2);
		x[3] = AFFINEX(x2,y2);
		y[3] = AFFINEY(x2,y2);
	} else {
		x[0] = A;
		y[0] = B;
		x[1] = C;
		y[1] = D;
		x[2] = E;
		y[2] = F;
		x[3] = C-A+E;
		y[3] = D-B+F;
	}
	PointF dests[3] = { PointF(x[0],y[0]), PointF(x[1],y[1]), PointF(x[2],y[2]) };
	if (src) {
		graphics->DrawImage(src, dests, 3, sleft, stop, swidth, sheight, UnitPixel, NULL, NULL, NULL);
	}
	qsort(x, 4, sizeof(REAL), (int (*)(const void*, const void*))compare_REAL);
	qsort(y, 4, sizeof(REAL), (int (*)(const void*, const void*))compare_REAL);
	rect.X = x[0];
	rect.Y = y[0];
	rect.Width = x[3]-x[0];
	rect.Height = y[3]-y[0];
	updateRect(rect);
	return rect;
}
