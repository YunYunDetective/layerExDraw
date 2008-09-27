#include "layerExDraw.hpp"
#include "ncbind/ncbind.hpp"

extern void initGdiPlus();
extern void deInitGdiPlus();

// ------------------------------------------------------
// �z�񑀍�p
// ------------------------------------------------------

// Array �N���X�����o
static iTJSDispatch2 *ArrayCountProp   = NULL;   // Array.count

tjs_int64 getArrayCount(iTJSDispatch2 *array)
{
	tTJSVariant result;
	if (TJS_SUCCEEDED(ArrayCountProp->PropGet(0, NULL, NULL, &result, array))) {
		return result.AsInteger();
	}
	return 0;
}

iTJSDispatch2*
getMember(iTJSDispatch2 *dispatch, int num)
{
	tTJSVariant val;
	if (TJS_FAILED(dispatch->PropGetByNum(TJS_IGNOREPROP,
										  num,
										  &val,
										  dispatch))) {
		ttstr msg = TJS_W("can't get array index:");
		msg += num;
		TVPThrowExceptionMessage(msg.c_str());
	}
	return val.AsObject();
}

REAL 
getRealValue(iTJSDispatch2 *obj, const tjs_char *name)
{
	tTJSVariant val;
	if (TJS_FAILED(obj->PropGet(0,
								name,
								NULL,
								&val,
								obj))) {
		ttstr msg = TJS_W("can't get member:");
		msg += name;
		TVPThrowExceptionMessage(msg.c_str());
	}
	return (REAL)val.AsReal();
}

// ----------------------------------- �N���X�̓o�^

NCB_REGISTER_SUBCLASS(FontInfo) {
	NCB_CONSTRUCTOR((const tjs_char *, REAL, INT));
	NCB_PROPERTY_WO(familyName, setFamilyName);
	NCB_PROPERTY(emSize, getEmSize, setEmSize);
	NCB_PROPERTY(style, getStyle, setStyle);
};

NCB_REGISTER_SUBCLASS(Appearance) {
	NCB_CONSTRUCTOR(());
	NCB_METHOD(clear);
	NCB_METHOD(addSolidBrush);
	NCB_METHOD(addLinearGradientBrush);
	NCB_METHOD(addColorPen);
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

/**
 * �o�^�����O
 */
void PreRegistCallback()
{
	initGdiPlus();

	// Array.count ���擾
	{
		tTJSVariant varScripts;
		TVPExecuteExpression(TJS_W("Array"), &varScripts);
		iTJSDispatch2 *dispatch = varScripts.AsObjectNoAddRef();
		tTJSVariant val;
		if (TJS_FAILED(dispatch->PropGet(TJS_IGNOREPROP,
										 TJS_W("count"),
										 NULL,
										 &val,
										 dispatch))) {
			TVPThrowExceptionMessage(L"can't get Array.count");
		}
		ArrayCountProp = val.AsObject();
	}
}

/**
 * �J��������
 */
void PostUnregistCallback()
{
	if (ArrayCountProp) {
		ArrayCountProp->Release();
		ArrayCountProp = NULL;
	}
	deInitGdiPlus();
}

NCB_PRE_REGIST_CALLBACK(PreRegistCallback);
NCB_POST_UNREGIST_CALLBACK(PostUnregistCallback);
