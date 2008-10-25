#include "layerExDraw.hpp"
#include "ncbind/ncbind.hpp"

extern void initGdiPlus();
extern void deInitGdiPlus();

// ------------------------------------------------------
// �^�R���o�[�^�o�^
// ------------------------------------------------------

struct RectConvertor { // �R���o�[�^
	void operator ()(tTJSVariant &dst, const RectF &src) {
		iTJSDispatch2 *dict = TJSCreateDictionaryObject();
		if (dict != NULL) {
			tTJSVariant left(src.X);
			tTJSVariant top(src.Y);
			tTJSVariant width(src.Width);
			tTJSVariant height(src.Height);
			dict->PropSet(TJS_MEMBERENSURE, L"left",   NULL, &left, dict);
			dict->PropSet(TJS_MEMBERENSURE, L"top",    NULL, &top, dict);
			dict->PropSet(TJS_MEMBERENSURE, L"width",  NULL, &width, dict);
			dict->PropSet(TJS_MEMBERENSURE, L"height", NULL, &height, dict);
			dst = tTJSVariant(dict, dict);
			dict->Release();
		}
	}
};
NCB_SET_TOVARIANT_CONVERTOR(RectF, RectConvertor);

// ------------------------------------------------------
// �N���X�o�^
// ------------------------------------------------------

NCB_REGISTER_SUBCLASS(FontInfo) {
	NCB_CONSTRUCTOR((const tjs_char *, REAL, INT));
	NCB_PROPERTY_WO(familyName, setFamilyName);
	NCB_PROPERTY(emSize, getEmSize, setEmSize);
	NCB_PROPERTY(style, getStyle, setStyle);
	NCB_PROPERTY_RO(ascent, getAscent);
	NCB_PROPERTY_RO(descent, getDescent);
	NCB_PROPERTY_RO(lineSpacing, getLineSpacing);
};

NCB_REGISTER_SUBCLASS(Appearance) {
	NCB_CONSTRUCTOR(());
	NCB_METHOD(clear);
	NCB_METHOD(addBrush);
	NCB_METHOD(addPen);
};

#define ENUM(n) Variant(#n, (int)n)

NCB_REGISTER_CLASS(GdiPlus)
{
// enums
	ENUM(FontStyleRegular);
	ENUM(FontStyleBold);
	ENUM(FontStyleItalic);
	ENUM(FontStyleBoldItalic);
	ENUM(FontStyleUnderline);
	ENUM(FontStyleStrikeout);

	ENUM(BrushTypeSolidColor);
	ENUM(BrushTypeHatchFill);
	ENUM(BrushTypeTextureFill);
	ENUM(BrushTypePathGradient);
	ENUM(BrushTypeLinearGradient);

	ENUM(DashCapFlat);
	ENUM(DashCapRound);
	ENUM(DashCapTriangle);
	
	ENUM(DashStyleSolid);
	ENUM(DashStyleDash);
	ENUM(DashStyleDot);
	ENUM(DashStyleDashDot);
	ENUM(DashStyleDashDotDot);

	ENUM(HatchStyleHorizontal);
	ENUM(HatchStyleVertical);
	ENUM(HatchStyleForwardDiagonal);
	ENUM(HatchStyleBackwardDiagonal);
	ENUM(HatchStyleCross);
	ENUM(HatchStyleDiagonalCross);
	ENUM(HatchStyle05Percent);
	ENUM(HatchStyle10Percent);
	ENUM(HatchStyle20Percent);
	ENUM(HatchStyle25Percent);
	ENUM(HatchStyle30Percent);
	ENUM(HatchStyle40Percent);
	ENUM(HatchStyle50Percent);
	ENUM(HatchStyle60Percent);
	ENUM(HatchStyle70Percent);
	ENUM(HatchStyle75Percent);
	ENUM(HatchStyle80Percent);
	ENUM(HatchStyle90Percent);
	ENUM(HatchStyleLightDownwardDiagonal);
	ENUM(HatchStyleLightUpwardDiagonal);
	ENUM(HatchStyleDarkDownwardDiagonal);
	ENUM(HatchStyleDarkUpwardDiagonal);
	ENUM(HatchStyleWideDownwardDiagonal);
	ENUM(HatchStyleWideUpwardDiagonal);
	ENUM(HatchStyleLightVertical);
	ENUM(HatchStyleLightHorizontal);
	ENUM(HatchStyleNarrowVertical);
	ENUM(HatchStyleNarrowHorizontal);
	ENUM(HatchStyleDarkVertical);
	ENUM(HatchStyleDarkHorizontal);
	ENUM(HatchStyleDashedDownwardDiagonal);
	ENUM(HatchStyleDashedUpwardDiagonal);
	ENUM(HatchStyleDashedHorizontal);
	ENUM(HatchStyleDashedVertical);
	ENUM(HatchStyleSmallConfetti);
	ENUM(HatchStyleLargeConfetti);
	ENUM(HatchStyleZigZag);
	ENUM(HatchStyleWave);
	ENUM(HatchStyleDiagonalBrick);
	ENUM(HatchStyleHorizontalBrick);
	ENUM(HatchStyleWeave);
	ENUM(HatchStylePlaid);
	ENUM(HatchStyleDivot);
	ENUM(HatchStyleDottedGrid);
	ENUM(HatchStyleDottedDiamond);
	ENUM(HatchStyleShingle);
	ENUM(HatchStyleTrellis);
	ENUM(HatchStyleSphere);
	ENUM(HatchStyleSmallGrid);
	ENUM(HatchStyleSmallCheckerBoard);
	ENUM(HatchStyleLargeCheckerBoard);
	ENUM(HatchStyleOutlinedDiamond);
	ENUM(HatchStyleSolidDiamond);
	ENUM(HatchStyleTotal);
	ENUM(HatchStyleLargeGrid);
	ENUM(HatchStyleMin);
	ENUM(HatchStyleMax);

	ENUM(LinearGradientModeHorizontal);
	ENUM(LinearGradientModeVertical);
	ENUM(LinearGradientModeForwardDiagonal);
	ENUM(LinearGradientModeBackwardDiagonal);

	ENUM(LineCapFlat);
	ENUM(LineCapSquare);
	ENUM(LineCapRound);
	ENUM(LineCapTriangle);
	ENUM(LineCapNoAnchor);
	ENUM(LineCapSquareAnchor);
	ENUM(LineCapRoundAnchor);
	ENUM(LineCapDiamondAnchor);
	ENUM(LineCapArrowAnchor);

	ENUM(LineJoinMiter);
	ENUM(LineJoinBevel);
	ENUM(LineJoinRound);
	ENUM(LineJoinMiterClipped);
	
	ENUM(PenAlignmentCenter);
	ENUM(PenAlignmentInset);

	ENUM(WrapModeTile);
	ENUM(WrapModeTileFlipX);
	ENUM(WrapModeTileFlipY);
	ENUM(WrapModeTileFlipXY);
	ENUM(WrapModeClamp);
	
// statics
	NCB_METHOD(addPrivateFont);
	NCB_METHOD(getFontList);

// classes
	NCB_SUBCLASS(Font,FontInfo);
	NCB_SUBCLASS(Appearance,Appearance);
}

NCB_GET_INSTANCE_HOOK(LayerExDraw)
{
	// �C���X�^���X�Q�b�^
	NCB_INSTANCE_GETTER(objthis) { // objthis �� iTJSDispatch2* �^�̈����Ƃ���
		ClassT* obj = GetNativeInstance(objthis);	// �l�C�e�B�u�C���X�^���X�|�C���^�擾
		if (!obj) {
			obj = new ClassT(objthis);				// �Ȃ��ꍇ�͐�������
			SetNativeInstance(objthis, obj);		// objthis �� obj ���l�C�e�B�u�C���X�^���X�Ƃ��ēo�^����
		}
		obj->reset();
		return obj;
	}
	// �f�X�g���N�^�i���ۂ̃��\�b�h���Ă΂ꂽ��ɌĂ΂��j
	~NCB_GET_INSTANCE_HOOK_CLASS () {
	}
};


// �t�b�N���A�^�b�`
NCB_ATTACH_CLASS_WITH_HOOK(LayerExDraw, Layer) {
	NCB_PROPERTY(updateWhenDraw, getUpdateWhenDraw, setUpdateWhenDraw);
	NCB_METHOD(clear);
	NCB_METHOD(drawArc);
	NCB_METHOD(drawPie);
	NCB_METHOD(drawBezier);
	NCB_METHOD(drawBeziers);
	NCB_METHOD(drawClosedCurve);
	NCB_METHOD(drawClosedCurve2);
	NCB_METHOD(drawCurve);
	NCB_METHOD(drawCurve2);
	NCB_METHOD(drawCurve3);
	NCB_METHOD(drawEllipse);
	NCB_METHOD(drawLine);
	NCB_METHOD(drawLines);
	NCB_METHOD(drawPolygon);
	NCB_METHOD(drawRectangle);
	NCB_METHOD(drawRectangles);
	NCB_METHOD(drawString);
	NCB_METHOD(drawImage);
	NCB_METHOD(measureString);
}

// ----------------------------------- �N���E�J������

NCB_PRE_REGIST_CALLBACK(initGdiPlus);
NCB_POST_UNREGIST_CALLBACK(deInitGdiPlus);
