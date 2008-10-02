#ifndef _layerExText_hpp_
#define _layerExText_hpp_

#include <windows.h>
#include <gdiplus.h>
using namespace Gdiplus;

#include <vector>
using namespace std;

#include "layerExBase.hpp"

/**
 * GDIPlus �ŗL�����p
 */
struct GdiPlus {
	/**
	 * �v���C�x�[�g�t�H���g�̒ǉ�
	 * @param fontFileName �t�H���g�t�@�C����
	 */
	static void addPrivateFont(const tjs_char *fontFileName);

	/**
	 * �v���C�x�[�g�t�H���g�ꗗ�����O�ɏo��
	 */
	static void showPrivateFontList();
};

/**
 * �t�H���g���
 */
class FontInfo {
	friend class LayerExDraw;

protected:
	FontFamily *fontFamily; //< �t�H���g�t�@�~���[
	ttstr familyName;
	REAL emSize; //< �t�H���g�T�C�Y 
	INT style; //< �t�H���g�X�^�C��

	/**
	 * �t�H���g���̃N���A
	 */
	void clear();

public:

	FontInfo();
	/**
	 * �R���X�g���N�^
	 * @param familyName �t�H���g�t�@�~���[
	 * @param emSize �t�H���g�̃T�C�Y
	 * @param style �t�H���g�X�^�C��
	 */
	FontInfo(const tjs_char *familyName, REAL emSize, INT style);
	FontInfo(const FontInfo &orig);

	/**
	 * �f�X�g���N�^
	 */
	virtual ~FontInfo();

	void setFamilyName(const tjs_char *familyName);
	const tjs_char *getFamilyName() { return familyName.c_str(); }
	void setEmSize(REAL emSize) { this->emSize = emSize; }
	REAL getEmSize() {  return emSize; }
	void setStyle(INT style) { this->style = style; }
	INT getStyle() { return style; }

	REAL getAscent() {
		if (fontFamily) {
			return fontFamily->GetCellAscent(style) * emSize / fontFamily->GetEmHeight(style);
		} else {
			return 0;
		}
	}

	REAL getDescent() {
		if (fontFamily) {
			return fontFamily->GetCellDescent(style) * emSize / fontFamily->GetEmHeight(style);
		} else {
			return 0;
		}
	}

	REAL getLineSpacing() {
		if (fontFamily) {
			return fontFamily->GetLineSpacing(style) * emSize / fontFamily->GetEmHeight(style);
		} else {
			return 0;
		}
	}
};

/**
 * �`��O�Ϗ��
 */
class Appearance {
	friend class LayerExDraw;
public:
	// �`����
	struct DrawInfo{
		int type;   // 0:�u���V 1:�y��
		void *info; // ���I�u�W�F�N�g
		REAL ox; //< �\���I�t�Z�b�g
		REAL oy; //< �\���I�t�Z�b�g
		DrawInfo() : ox(0), oy(0), type(0), info(NULL) {}
		DrawInfo(REAL ox, REAL oy, Pen *pen) : ox(ox), oy(oy), type(0), info(pen) {}
		DrawInfo(REAL ox, REAL oy, Brush *brush) : ox(ox), oy(oy), type(1), info(brush) {}
		DrawInfo(const DrawInfo &orig) {
			ox = orig.ox;
			oy = orig.oy;
			type = orig.type;
			if (orig.info) {
				switch (type) {
				case 0:
					info = (void*)((Pen*)orig.info)->Clone();
					break;
				case 1:
					info = (void*)((Brush*)orig.info)->Clone();
					break;
				}
			} else {
				info = NULL;
			}
		}
		virtual ~DrawInfo() {
			if (info) {
				switch (type) {
				case 0:
					delete (Pen*)info;
					break;
				case 1:
					delete (Brush*)info;
					break;
				}
			}
		}	
	};
	vector<DrawInfo>drawInfos;

public:
	Appearance();
	virtual ~Appearance();

	/**
	 * ���̃N���A
	 */
	void clear();
	
	/**
	 * �u���V�̒ǉ�
	 * @param colorOrBrush ARGB�F�w��܂��̓u���V���i�����j
	 * @param ox �\���I�t�Z�b�gX
	 * @param oy �\���I�t�Z�b�gY
	 */
	void addBrush(tTJSVariant colorOrBrush, REAL ox=0, REAL oy=0);
	
	/**
	 * �y���̒ǉ�
	 * @param colorOrBrush ARGB�F�w��܂��̓u���V���i�����j
	 * @param widthOrOption �y�����܂��̓y�����i�����j
	 * @param ox �\���I�t�Z�b�gX
	 * @param oy �\���I�t�Z�b�gY
	 */
	void addPen(tTJSVariant colorOrBrush, tTJSVariant widthOrOption, REAL ox=0, REAL oy=0);
};

/*
 * �A�E�g���C���x�[�X�̃e�L�X�g�`�惁�\�b�h�̒ǉ�
 */
class LayerExDraw : public layerExBase
{
protected:
	// ���ێ��p
	GeometryT width, height;
	BufferT   buffer;
	PitchT    pitch;
	
	/// ���C�����Q�Ƃ���r�b�g�}�b�v
	Bitmap *bitmap;
	/// ���C���ɑ΂��ĕ`�悷��R���e�L�X�g
	Graphics *graphics;

	bool updateWhenDraw;

public:
	void setUpdateWhenDraw(int updateWhenDraw) {
		this->updateWhenDraw = updateWhenDraw != 0;
	}
	int getUpdateWhenDraw() { return updateWhenDraw ? 1 : 0; }
	
public:	
	LayerExDraw(DispatchT obj);
	~LayerExDraw();
	virtual void reset();

	// ------------------------------------------------------------------
	// �`�惁�\�b�h�Q
	// ------------------------------------------------------------------

protected:

	/**
	 * �p�X�̗̈�����擾
	 * @param app �\���\��
	 * @param path �`�悷��p�X
	 * @return �̈���̎��� left, top, width, height
	 */
	tTJSVariant getPathExtents(const Appearance *app, const GraphicsPath *path);

	/**
	 * �p�X�̕`��
	 * @param app �A�s�A�����X
	 * @param path �`�悷��p�X
	 * @return �̈���̎��� left, top, width, height
	 */
	tTJSVariant drawPath(const Appearance *app, const GraphicsPath *path);


public:
	/**
	 * ��ʂ̏���
	 * @param argb �����F
	 */
	void clear(ARGB argb);

	/**
	 * �~�ʂ̕`��
	 * @param app �A�s�A�����X
	 * @param x ������W
	 * @param y ������W
	 * @param width ����
	 * @param height �c��
	 * @param startAngle ���v�����~�ʊJ�n�ʒu
	 * @param sweepAngle �`��p�x
	 * @return �̈���̎��� left, top, width, height
	 */
	tTJSVariant drawArc(const Appearance *app, REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle);

	/**
	 * �~���̕`��
	 * @param app �A�s�A�����X
	 * @param x ������W
	 * @param y ������W
	 * @param width ����
	 * @param height �c��
	 * @param startAngle ���v�����~�ʊJ�n�ʒu
	 * @param sweepAngle �`��p�x
	 * @return �̈���̎��� left, top, width, height
	 */
	tTJSVariant drawPie(const Appearance *app, REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle);
	
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
	 * @return �̈���̎��� left, top, width, height
	 */
	tTJSVariant drawBezier(const Appearance *app, REAL x1, REAL y1, REAL x2, REAL y2, REAL x3, REAL y3, REAL x4, REAL y4);
	
	/**
	 * �ȉ~�̕`��
	 * @param app �A�s�A�����X
	 * @param x
	 * @param y
	 * @param width
	 * @param height
	 * @return �̈���̎��� left, top, width, height
	 */
	tTJSVariant drawEllipse(const Appearance *app, REAL x, REAL y, REAL width, REAL height);

	/**
	 * �����̕`��
	 * @param app �A�s�A�����X
	 * @param x1 �n�_X���W
	 * @param y1 �n�_Y���W
	 * @param x2 �I�_X���W
	 * @param y2 �I�_Y���W
	 * @return �̈���̎��� left, top, width, height
	 */
	tTJSVariant drawLine(const Appearance *app, REAL x1, REAL y1, REAL x2, REAL y2);

	/**
	 * ��`�̕`��
	 * @param app �A�s�A�����X
	 * @param x
	 * @param y
	 * @param width
	 * @param height
	 * @return �̈���̎��� left, top, width, height
	 */
	tTJSVariant drawRectangle(const Appearance *app, REAL x, REAL y, REAL width, REAL height);
	
	/**
	 * ������̕`��
	 * @param font �t�H���g
	 * @param app �A�s�A�����X
	 * @param x �`��ʒuX
	 * @param y �`��ʒuY
	 * @param text �`��e�L�X�g
	 * @return �̈���̎��� left, top, width, height
	 */
	tTJSVariant drawString(const FontInfo *font, const Appearance *app, REAL x, REAL y, const tjs_char *text);

	/**
	 * �摜�̕`��
	 * @param x �\���ʒuX
	 * @param y �\���ʒuY
	 * @param name �摜��
	 */
	void drawImage(REAL x, REAL y, const tjs_char *name);

	// -----------------------------------------------------------------------------

	/**
	 * ������̕`��̈���̎擾
	 * @param font �t�H���g
	 * @param text �`��e�L�X�g
	 * @return �̈���̎��� left, top, width, height
	 */
	tTJSVariant measureString(const FontInfo *font, const tjs_char *text);
};

#endif
