#include <d2d1_3.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <cmath>
#include <numbers>
#include <array>

namespace {
	// Deklaracje użycia pomocniczych funkcji
	using D2D1::RenderTargetProperties;
	using D2D1::HwndRenderTargetProperties;
	using D2D1::SizeU;
	using D2D1::Point2F;
	using D2D1::StrokeStyleProperties;
	using D2D1::ColorF;
	using D2D1::LinearGradientBrushProperties;
	using D2D1::RadialGradientBrushProperties;
	using D2D1::Matrix3x2F;
	using D2D1::RectF;
	using D2D1::Ellipse;
	using D2D1::BezierSegment;
	using D2D1::QuadraticBezierSegment;
	using D2D1::BitmapProperties;
	using D2D1::PixelFormat;
	using std::sin;
	using std::array;


	// Interfejsy potrzebne do zainicjowania Direct2D
	ID2D1Factory7* d2d_factory = nullptr;
	ID2D1HwndRenderTarget* d2d_render_target = nullptr;

	// Zmienne wymiarów obszaru roboczego
	RECT rc;
	FLOAT window_hight;
	FLOAT window_width;

	// Interfejsy DirectWrite
	IDWriteFactory* write_factory = nullptr;
	IDWriteTextFormat* text_format = nullptr;
	// Dane napisu w UNICODE
	WCHAR const NAPIS[] = L"和敬清寂";
	FLOAT font_size;

	// Bitmapy
	ID2D1Bitmap* bitmap_chasen = nullptr;
	ID2D1Bitmap* bitmap_bubbles = nullptr;
	ID2D1Bitmap* bitmap_moving_bubble = nullptr;
	ID2D1Bitmap* bitmap_wagashi1 = nullptr;
	ID2D1Bitmap* bitmap_wagashi2 = nullptr;
	ID2D1Bitmap* bitmap_wagashi3 = nullptr;
	ID2D1Bitmap* bitmap_wagashi4 = nullptr;
	ID2D1Bitmap* bitmap_scroll = nullptr;
	// Interfejs fabryki WIC
	IWICImagingFactory* wic_factory = nullptr;
	// Docelowe obszary umiejscowania poszczególnych elementów
	D2D1_RECT_F chasen_rect;
	D2D1_RECT_F wagashi_rect;
	D2D1_RECT_F scroll_rect;
	D2D1_RECT_F bubbles_rect;
	D2D1_RECT_F text_rect;

	// Interfejsy potrzebne do rysowania
	ID2D1SolidColorBrush* brush = nullptr;

	// Pędzel gradientu promienistego (herbata)
	ID2D1RadialGradientBrush* rad_brush_tea = nullptr;
	ID2D1GradientStopCollection* rad_stops_tea = nullptr;
	UINT const NUM_RAD_STOPS_TEA = 3;
	D2D1_GRADIENT_STOP rad_stops_data_tea[NUM_RAD_STOPS_TEA];

	// Geometria sceny
	D2D1_POINT_2F teabowl_center;
	FLOAT teabowl_edge_radii;
	FLOAT teabowl_radii;
	FLOAT tea_radii;
	FLOAT const contour_width = 2.0f;
	D2D1_POINT_2F const teaspoon_center = { .x = 600, .y = 235 };
	FLOAT const teaspoon_rad_x = 105.f;
	FLOAT const teaspoon_rad_y = 115.f;


	// Stałe kolorów
	D2D1_COLOR_F const contour_color =
	{ .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };
	D2D1_COLOR_F const table_color =
	{ .r = 0.6f, .g = 0.5f, .b = 0.3f, .a = 1.0f };
	D2D1_COLOR_F const teabowl_edge_color =
	{ .r = 0.4f, .g = 0.4f, .b = 0.15f, .a = 1.0f };
	D2D1_COLOR_F const teabowl_color =
	{ .r = 0.25f, .g = 0.25f, .b = 0.1f, .a = 1.0f };
	D2D1_COLOR_F const spoon_color =
	{ .r = 0.95f, .g = 0.75f, .b = 0.35f, .a = 1.0f };
	D2D1_COLOR_F const tea_color =
	{ .r = 0.25f, .g = 0.8f, .b = 0.35f, .a = 1.0f };

	// Interfejsy do obsługi ścieżki
	ID2D1PathGeometry* spoon_path = nullptr;
	ID2D1GeometrySink* spoon_path_sink = nullptr;

	// - Macierz do połączenia transformacji
	Matrix3x2F transformation;

	// Zmienne działania programu
	BOOL clicked = FALSE;
	FLOAT time = 0.0f;
	FLOAT angle = 0.0f;
	D2D1_POINT_2F mouse_position = { .x = 0, .y = 0 };
	INT wagashi = 0;
}

HRESULT LoadBitmapFromFile(
	ID2D1RenderTarget* pRenderTarget,
	IWICImagingFactory* pIWICFactory,
	PCWSTR uri,
	ID2D1Bitmap** ppBitmap
)
{
	IWICBitmapDecoder* pDecoder = NULL;
	IWICBitmapFrameDecode* pSource = NULL;
	IWICStream* pStream = NULL;
	IWICFormatConverter* pConverter = NULL;
	IWICBitmapScaler* pScaler = NULL;

	HRESULT hr = pIWICFactory->CreateDecoderFromFilename(
		uri,
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&pDecoder
	);

	if (SUCCEEDED(hr))
	{
		hr = pDecoder->GetFrame(0, &pSource);
	}

	if (SUCCEEDED(hr))
	{
		hr = pIWICFactory->CreateFormatConverter(&pConverter);
	}

	if (SUCCEEDED(hr))
	{
		hr = pConverter->Initialize(
			pSource,
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			NULL,
			0.f,
			WICBitmapPaletteTypeMedianCut
		);
	}

	if (SUCCEEDED(hr))
	{
		hr = pRenderTarget->CreateBitmapFromWicBitmap(
			pConverter,
			NULL,
			ppBitmap
		);
	}

	if (pDecoder) pDecoder->Release();
	if (pSource) pSource->Release();
	if (pStream) pStream->Release();
	if (pConverter) pConverter->Release();
	if (pScaler) pScaler->Release();

	return hr;
}

void setTeabowlGeometry() {
	teabowl_center = { .x = window_width / 4, .y = window_hight / 3 };
	teabowl_edge_radii = window_hight * 4 / 15;
	teabowl_radii = teabowl_edge_radii * 14 / 15;
	tea_radii = teabowl_edge_radii * 11 / 15;
}

void setChasenGeometry() {
	FLOAT chasen_top = teabowl_center.y + teabowl_edge_radii;
	chasen_rect = RectF(teabowl_center.x - teabowl_edge_radii,
		chasen_top,
		teabowl_center.x + teabowl_edge_radii,
		chasen_top + teabowl_edge_radii * 5 / 4);
}

void setWagashiGeometry() {
	wagashi_rect = RectF(teabowl_center.x + teabowl_edge_radii,
		chasen_rect.bottom - (2.5f * teabowl_edge_radii * 128 / 120),
		teabowl_center.x + 3.5f * teabowl_edge_radii,
		chasen_rect.bottom);
}

void setScrollGeometry() {
	FLOAT scroll_right = wagashi_rect.right + 0.25f * (wagashi_rect.right - wagashi_rect.left);
	scroll_rect = RectF(wagashi_rect.left,
		0,
		scroll_right,
		(scroll_right - wagashi_rect.left) * 50 / 128);
}

void setBubblesGeometry() {
	bubbles_rect = RectF(teabowl_center.x - teabowl_edge_radii,
		teabowl_center.y - teabowl_edge_radii,
		teabowl_center.x + teabowl_edge_radii,
		teabowl_center.y + teabowl_edge_radii);
}

void setTextGeometry() {
	text_rect = RectF(
		scroll_rect.left + 0.5f * (scroll_rect.right - scroll_rect.left) - 2.f * font_size,
		scroll_rect.top + 0.5f * (scroll_rect.bottom - scroll_rect.top - 1.6f * font_size),
		scroll_rect.right,
		scroll_rect.bottom
	);
}

void InitDirect2D(HWND hwnd) {
	GetClientRect(hwnd, &rc);
	window_hight = static_cast<FLOAT>(rc.bottom) - static_cast<FLOAT>(rc.top);
	window_width = static_cast<FLOAT>(rc.right) - static_cast<FLOAT>(rc.left);

	// Ustawienie geometrii elementów sceny
	setTeabowlGeometry();
	setChasenGeometry();
	setWagashiGeometry();
	setScrollGeometry();
	setBubblesGeometry();

	// Utworzenie fabryki Direct2D
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
		&d2d_factory);

	// Utworzenie celu renderowania w oknie Windows
	d2d_factory->CreateHwndRenderTarget(
		RenderTargetProperties(),
		HwndRenderTargetProperties(hwnd,
			SizeU(static_cast<UINT32>(rc.right) -
				static_cast<UINT32>(rc.left),
				static_cast<UINT32>(rc.bottom) -
				static_cast<UINT32>(rc.top))),
		&d2d_render_target);

	// Utworzenie pędzla
	d2d_render_target->CreateSolidColorBrush(contour_color, &brush);

	// Utworzenie fabryki DirectWrite
	DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(&write_factory)
	);


	// Utworzenie formatu napisu DirectWrite
	font_size = 0.3f * (scroll_rect.bottom - scroll_rect.top);
	setTextGeometry();
	write_factory->CreateTextFormat(
		L"Times New Roman",
		nullptr,
		DWRITE_FONT_WEIGHT_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		font_size,
		L"en-us",
		&text_format
	);


	// Ustawienia gradientu promienistego
	rad_stops_data_tea[0] =
	{ .position = 0.0f, .color = ColorF(0.25f, 0.8f, 0.35f, 1.0f) };
	rad_stops_data_tea[1] =
	{ .position = 0.7f, .color = ColorF(0.25f, 0.8f, 0.35f, 1.0f) };
	rad_stops_data_tea[2] =
	{ .position = 0.95f, .color = ColorF(ColorF::ForestGreen, 1.0f) };

	d2d_render_target->CreateGradientStopCollection(
		rad_stops_data_tea, NUM_RAD_STOPS_TEA, &rad_stops_tea);
	if (rad_stops_tea) {
		d2d_render_target->CreateRadialGradientBrush(
			RadialGradientBrushProperties(teabowl_center,
				Point2F(0, 0), tea_radii, tea_radii),
			rad_stops_tea, &rad_brush_tea);
	}

	// Utworzenie i zbudowanie geometrii ścieżki
	d2d_factory->CreatePathGeometry(&spoon_path);
	spoon_path->Open(&spoon_path_sink);
	spoon_path_sink->BeginFigure(Point2F(500, 300), D2D1_FIGURE_BEGIN_FILLED);
	spoon_path_sink->AddBezier(BezierSegment(
		Point2F(400, 50), Point2F(800, 50), Point2F(700, 300)));
	spoon_path_sink->AddQuadraticBezier(QuadraticBezierSegment(
		Point2F(670, 350), Point2F(650, 380)));
	spoon_path_sink->AddQuadraticBezier(QuadraticBezierSegment(
		Point2F(630, 410), Point2F(630, 420)));
	spoon_path_sink->AddQuadraticBezier(QuadraticBezierSegment(
		Point2F(635, 580), Point2F(640, 740)));
	spoon_path_sink->AddQuadraticBezier(QuadraticBezierSegment(
		Point2F(645, 765), Point2F(600, 760)));
	spoon_path_sink->AddQuadraticBezier(QuadraticBezierSegment(
		Point2F(555, 765), Point2F(560, 740)));
	spoon_path_sink->AddQuadraticBezier(QuadraticBezierSegment(
		Point2F(565, 580), Point2F(570, 420)));
	spoon_path_sink->AddQuadraticBezier(QuadraticBezierSegment(
		Point2F(570, 410), Point2F(550, 380)));
	spoon_path_sink->AddQuadraticBezier(QuadraticBezierSegment(
		Point2F(530, 350), Point2F(500, 300)));
	spoon_path_sink->EndFigure(D2D1_FIGURE_END_OPEN);
	spoon_path_sink->Close();

	// Utworzenie fabryki WIC
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&wic_factory)
	);

	// Załadowanie bitmap
	LoadBitmapFromFile(d2d_render_target, wic_factory, L"chasen2.png", &bitmap_chasen);
	LoadBitmapFromFile(d2d_render_target, wic_factory, L"bubbles.png", &bitmap_bubbles);
	LoadBitmapFromFile(d2d_render_target, wic_factory, L"moving_bubble.png", &bitmap_moving_bubble);
	LoadBitmapFromFile(d2d_render_target, wic_factory, L"wagashi1.png", &bitmap_wagashi1);
	LoadBitmapFromFile(d2d_render_target, wic_factory, L"wagashi2.png", &bitmap_wagashi2);
	LoadBitmapFromFile(d2d_render_target, wic_factory, L"wagashi3.png", &bitmap_wagashi3);
	LoadBitmapFromFile(d2d_render_target, wic_factory, L"wagashi4.png", &bitmap_wagashi4);
	LoadBitmapFromFile(d2d_render_target, wic_factory, L"scroll.png", &bitmap_scroll);
}

void DestroyDirect2D() {
	if (brush) brush->Release();
	brush = nullptr;
	if (rad_brush_tea) rad_brush_tea->Release();
	rad_brush_tea = nullptr;

	if (text_format) text_format->Release();
	text_format = nullptr;
	if (write_factory) write_factory->Release();
	write_factory = nullptr;

	if (bitmap_chasen) bitmap_chasen->Release();
	bitmap_chasen = nullptr;
	if (bitmap_bubbles) bitmap_bubbles->Release();
	bitmap_bubbles = nullptr;
	if (bitmap_wagashi1) bitmap_wagashi1->Release();
	bitmap_wagashi1 = nullptr;
	if (bitmap_wagashi2) bitmap_wagashi2->Release();
	bitmap_wagashi2 = nullptr;
	if (bitmap_wagashi3) bitmap_wagashi3->Release();
	bitmap_wagashi3 = nullptr;
	if (bitmap_wagashi4) bitmap_wagashi4->Release();
	bitmap_wagashi4 = nullptr;
	if (bitmap_scroll) bitmap_scroll->Release();
	bitmap_scroll = nullptr;

	if (d2d_render_target) d2d_render_target->Release();
	d2d_render_target = nullptr;
	if (d2d_factory) d2d_factory->Release();
	d2d_factory = nullptr;
}

void drawTeabowl() {
	d2d_render_target->SetTransform(Matrix3x2F::Identity());

	brush->SetColor(teabowl_edge_color);
	d2d_render_target->FillEllipse(
		Ellipse(teabowl_center, teabowl_edge_radii, teabowl_edge_radii),
		brush);

	brush->SetColor(contour_color);
	d2d_render_target->DrawEllipse(
		Ellipse(teabowl_center, teabowl_edge_radii, teabowl_edge_radii),
		brush, contour_width);


	brush->SetColor(teabowl_color);
	d2d_render_target->FillEllipse(
		Ellipse(teabowl_center, teabowl_radii, teabowl_radii),
		brush);

	brush->SetColor(contour_color);
	d2d_render_target->DrawEllipse(
		Ellipse(teabowl_center, teabowl_radii, teabowl_radii),
		brush, contour_width);


	d2d_render_target->FillEllipse(
		Ellipse(teabowl_center, tea_radii, tea_radii),
		rad_brush_tea);

	brush->SetColor(contour_color);
	d2d_render_target->DrawEllipse(
		Ellipse(teabowl_center, tea_radii, tea_radii),
		brush, contour_width);

}

void drawChasen() {
	d2d_render_target->SetTransform(Matrix3x2F::Identity());
	d2d_render_target->DrawBitmap(
		bitmap_chasen,
		chasen_rect,
		1.0f,
		D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
}

void drawWagashi() {
	d2d_render_target->SetTransform(Matrix3x2F::Identity());
	if (wagashi == 0) {
		d2d_render_target->DrawBitmap(
			bitmap_wagashi1,
			wagashi_rect,
			1.0f,
			D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
	}
	else if (wagashi == 1) {
		d2d_render_target->DrawBitmap(
			bitmap_wagashi2,
			wagashi_rect,
			1.0f,
			D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
	}
	else if (wagashi == 2) {
		d2d_render_target->DrawBitmap(
			bitmap_wagashi3,
			wagashi_rect,
			1.0f,
			D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
	}
	if (wagashi == 3) {
		d2d_render_target->DrawBitmap(
			bitmap_wagashi4,
			wagashi_rect,
			1.0f,
			D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
	}
}

void drawBubbles() {
	d2d_render_target->SetTransform(Matrix3x2F::Rotation(angle, teabowl_center));

	d2d_render_target->DrawBitmap(
		bitmap_bubbles,
		bubbles_rect,
		1.0f,
		D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
}

void drawScroll() {
	d2d_render_target->SetTransform(Matrix3x2F::Identity());
	d2d_render_target->DrawBitmap(
		bitmap_scroll,
		scroll_rect,
		1.0f,
		D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
}

void drawScrolltext() {
	d2d_render_target->SetTransform(Matrix3x2F::Identity());
	d2d_render_target->DrawText(
		NAPIS,
		sizeof(NAPIS) / sizeof(NAPIS[0]),
		text_format,
		text_rect,
		brush
	);
}

void drawSpoon() {
	FLOAT wagashi_spoon_ratio = 0.8f;
	FLOAT spoon_length = 700.f;
	FLOAT scale = (wagashi_rect.bottom - wagashi_rect.top) * wagashi_spoon_ratio / spoon_length;

	transformation = Matrix3x2F::Scale(scale, scale, Point2F(600, 765));
	transformation.SetProduct(transformation,
		Matrix3x2F::Translation(wagashi_rect.right + 0.25f * (wagashi_rect.right - wagashi_rect.left) - 600,
			-765 + 0.9f * wagashi_rect.bottom));
	d2d_render_target->SetTransform(transformation);

	d2d_render_target->DrawGeometry(spoon_path, brush, 2.f * contour_width / scale);
	brush->SetColor(spoon_color);
	d2d_render_target->FillGeometry(spoon_path, brush);

	brush->SetColor(tea_color);
	d2d_render_target->FillEllipse(
		Ellipse(teaspoon_center, teaspoon_rad_x, teaspoon_rad_y),
		brush);

	brush->SetColor(contour_color);
	d2d_render_target->DrawEllipse(
		Ellipse(teaspoon_center, teaspoon_rad_x, teaspoon_rad_y),
		brush, 0.7f * contour_width / scale);
}

D2D1::Matrix3x2F bubble_translation() {
	FLOAT x = mouse_position.x - teabowl_center.x;
	FLOAT y = mouse_position.y - teabowl_center.y;
	FLOAT inner_radii = 0.8f * tea_radii;

	if (x * x + y * y > inner_radii * inner_radii) {
		FLOAT scale = inner_radii / sqrt(x * x + y * y);
		x *= scale;
		y *= scale;
	}
	return Matrix3x2F::Translation(x, y);
}


void drawMovingBubble() {
	transformation = bubble_translation();
	d2d_render_target->SetTransform(transformation);
	FLOAT bubble_radii = 0.2f * tea_radii;

	d2d_render_target->DrawBitmap(
		bitmap_moving_bubble,
		RectF(teabowl_center.x - bubble_radii, teabowl_center.y - bubble_radii,
			teabowl_center.x + bubble_radii, teabowl_center.y + bubble_radii),
		1.0f,
		D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
}

void OnPaint(HWND hwnd, INT mouse_clicked, FLOAT mouse_x, FLOAT mouse_y) {

	if (mouse_clicked == 1) {
		if (!clicked)
			wagashi = (wagashi + 1) % 4;
		clicked = TRUE;
	}
	if (mouse_clicked == -1)
		clicked = FALSE;

	if (mouse_x >= 0) {
		mouse_position.x = mouse_x;
		mouse_position.y = mouse_y;
	}

	time += 0.05f;
	angle = -15.f + 5.0f * sin(time);

	d2d_render_target->BeginDraw();
	d2d_render_target->Clear(table_color);

	drawTeabowl();
	drawBubbles();
	drawMovingBubble();
	drawChasen();
	drawWagashi();
	drawScroll();
	drawScrolltext();
	drawSpoon();
	
	if (d2d_render_target->EndDraw() == D2DERR_RECREATE_TARGET) {
		DestroyDirect2D();
		InitDirect2D(hwnd);
	}
}