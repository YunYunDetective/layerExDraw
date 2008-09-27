#pragma comment(lib, "gdiplus.lib")
#include "LayerExDraw.hpp"

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
 * �Œ�F�u���V�̒ǉ�
 * @param argb �F�w��
 * @param ox �\���I�t�Z�b�gX
 * @param oy �\���I�t�Z�b�gY
 */
void
Appearance::addSolidBrush(ARGB argb, REAL ox, REAL oy)
{
	drawInfos.push_back(DrawInfo(ox, oy, new SolidBrush(Color(argb))));
}


/**
 * �O���f�[�V�����u���V�̒ǉ�
 * @param x1
 * @param y1
 * @param argb1 �F�w�肻�̂P
 * @param x1
 * @param y1
 * @param argb1 �F�w�肻�̂Q
 * @param ox �\���I�t�Z�b�gX
 * @param oy �\���I�t�Z�b�gY
 */
void 
Appearance::addLinearGradientBrush(REAL x1, REAL y1, ARGB argb1, REAL x2, REAL y2, ARGB argb2, REAL ox, REAL oy)
{
	drawInfos.push_back(DrawInfo(ox, oy, new LinearGradientBrush(PointF(x1,y1), PointF(x2,y2), Color(argb1), Color(argb2))));
}

/**
 * �Œ�F�y���̒ǉ�
 * @param ox �\���I�t�Z�b�gX
 * @param oy �\���I�t�Z�b�gY
 * @param argb �F�w��
 * @param width �y����
 */
void 
Appearance::addColorPen(ARGB argb, REAL width, REAL ox, REAL oy)
{
	Pen *pen = new Pen(Color(argb), width);
	// �Ƃ肠�����܂邭�ڑ�
	pen->SetLineCap(LineCapRound, LineCapRound, DashCapRound);
	pen->SetLineJoin(LineJoinRound);
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
	// �摜�ǂݍ���
	IStream *in = TVPCreateIStream(name, TJS_BS_READ);
	if(!in) {
		TVPThrowExceptionMessage((ttstr(TJS_W("cannot open : ")) + ttstr(name)).c_str());
	}
	
	try {
		// �摜����
		Image image(in);
		int ret;
		if ((ret = image.GetLastStatus()) != Ok) {
			TVPThrowExceptionMessage((ttstr(TJS_W("cannot load : ")) + ttstr(name) + ttstr(L" : ") + ttstr(ret)).c_str());
		}
		// �`�揈��
		graphics->DrawImage(&image, x, y);
	} catch (...) {
		in->Release();
		throw;
	}
	in->Release();
}
