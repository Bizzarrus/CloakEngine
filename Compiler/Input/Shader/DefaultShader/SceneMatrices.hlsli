struct BufferProj {
	matrix ViewToProjection;
	matrix ProjectionToView;
};
struct BufferView {
	matrix WorldToView;
	matrix ViewToWorld;
	matrix WorldToProjection;
	matrix ProjectionToWorld;
	float3 ViewPos;
};
ConstantBuffer<BufferProj> Projection : register(b0, space0);
ConstantBuffer<BufferView> View : register(b1, space0);