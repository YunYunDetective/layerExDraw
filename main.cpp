#include "layerExDraw.hpp"
#include "ncbind/ncbind.hpp"

extern void initGdiPlus();
extern void deInitGdiPlus();

// ------------------------------------------------------
// �z�񑀍�p
// ------------------------------------------------------

NCB_REGISTER_SUBCLASS(FontInfo) {
	NCB_CONSTRUCTOR((const tjs_char *, REAL, INT));
	NCB_PROPERTY_WO(familyName, setFamilyName);
	NCB_PROPERTY(emSize, getEmSize, setEmSize);
	NCB_PROPERTY(style, getStyle, setStyle);
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
	
// statics
	NCB_METHOD(addPrivateFont);
	NCB_METHOD(showPrivateFontList);

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
	NCB_METHOD(drawEllipse);
	NCB_METHOD(drawLine);
	NCB_METHOD(drawRectangle);
	NCB_METHOD(drawString);
	NCB_METHOD(drawImage);
}

// ----------------------------------- �N���E�J������

NCB_PRE_REGIST_CALLBACK(initGdiPlus);
NCB_POST_UNREGIST_CALLBACK(deInitGdiPlus);
