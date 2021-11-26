#pragma once
#include "CUDA/CUDAModule.h"
#include "CUDA/CUDAKernel.h"
#include "CUDA/CUDAMemory.h"
#include "CUDA/CUDAEvent.h"

#include "Assets/Material.h"

#include "BVH/Builders/BVHBuilder.h"
#include "BVH/Builders/SBVHBuilder.h"
#include "BVH/Builders/QBVHBuilder.h"
#include "BVH/Builders/CWBVHBuilder.h"

#include "Scene.h"

#include "Util/PMJ.h"

// Mirror CUDA vector types
struct alignas(8)  float2 { float x, y; };
struct             float3 { float x, y, z; };
struct alignas(16) float4 { float x, y, z, w; };

struct alignas(8)  int2 { int x, y; };
struct             int3 { int x, y, z; };
struct alignas(16) int4 { int x, y, z, w; };

struct CUDAVector3_SoA {
	CUDAMemory::Ptr<float> x;
	CUDAMemory::Ptr<float> y;
	CUDAMemory::Ptr<float> z;

	inline void init(int buffer_size) {
		x = CUDAMemory::malloc<float>(buffer_size);
		y = CUDAMemory::malloc<float>(buffer_size);
		z = CUDAMemory::malloc<float>(buffer_size);
	}

	inline void free() {
		CUDAMemory::free(x);
		CUDAMemory::free(y);
		CUDAMemory::free(z);
	}
};

struct TraceBuffer {
	CUDAVector3_SoA origin;
	CUDAVector3_SoA direction;

	CUDAMemory::Ptr<float2> cone;
	CUDAMemory::Ptr<float4> hits;

	CUDAMemory::Ptr<int> pixel_index_and_last_material;
	CUDAVector3_SoA      throughput;

	CUDAMemory::Ptr<float> last_pdf;

	inline void init(int buffer_size) {
		origin   .init(buffer_size);
		direction.init(buffer_size);

		cone = CUDAMemory::malloc<float2>(buffer_size);
		hits = CUDAMemory::malloc<float4>(buffer_size);

		pixel_index_and_last_material = CUDAMemory::malloc<int>(buffer_size);
		throughput.init(buffer_size);

		last_pdf = CUDAMemory::malloc<float>(buffer_size);
	}

	inline void free() {
		origin.free();
		direction.free();

		CUDAMemory::free(cone);
		CUDAMemory::free(hits);

		CUDAMemory::free(pixel_index_and_last_material);
		throughput.free();

		CUDAMemory::free(last_pdf);
	}
};

struct MaterialBuffer {
	CUDAVector3_SoA direction;

	CUDAMemory::Ptr<float2> cone;
	CUDAMemory::Ptr<float4> hits;

	CUDAMemory::Ptr<int> pixel_index;
	CUDAVector3_SoA      throughput;

	inline void init(int buffer_size) {
		direction.init(buffer_size);

		cone = CUDAMemory::malloc<float2>(buffer_size);
		hits = CUDAMemory::malloc<float4>(buffer_size);

		pixel_index = CUDAMemory::malloc<int>(buffer_size);
		throughput.init(buffer_size);
	}

	inline void free() {
		direction.free();

		CUDAMemory::free(cone);
		CUDAMemory::free(hits);

		CUDAMemory::free(pixel_index);
		throughput.free();
	}
};

struct ShadowRayBuffer {
	CUDAVector3_SoA ray_origin;
	CUDAVector3_SoA ray_direction;

	CUDAMemory::Ptr<float>  max_distance;
	CUDAMemory::Ptr<float4> illumination_and_pixel_index;

	inline void init(int buffer_size) {
		ray_origin   .init(buffer_size);
		ray_direction.init(buffer_size);

		max_distance                 = CUDAMemory::malloc<float> (buffer_size);
		illumination_and_pixel_index = CUDAMemory::malloc<float4>(buffer_size);
	}

	inline void free() {
		ray_origin.free();
		ray_direction.free();

		CUDAMemory::free(max_distance);
		CUDAMemory::free(illumination_and_pixel_index);
	}
};

struct BufferSizes {
	int trace     [MAX_BOUNCES];
	int diffuse   [MAX_BOUNCES];
	int plastic   [MAX_BOUNCES];
	int dielectric[MAX_BOUNCES];
	int conductor [MAX_BOUNCES];
	int shadow    [MAX_BOUNCES];

	int rays_retired       [MAX_BOUNCES];
	int rays_retired_shadow[MAX_BOUNCES];

	void reset(int batch_size) {
		memset(trace,               0, sizeof(trace));
		memset(diffuse,             0, sizeof(diffuse));
		memset(plastic,             0, sizeof(plastic));
		memset(dielectric,          0, sizeof(dielectric));
		memset(conductor,           0, sizeof(conductor));
		memset(shadow,              0, sizeof(shadow));
		memset(rays_retired,        0, sizeof(rays_retired));
		memset(rays_retired_shadow, 0, sizeof(rays_retired_shadow));

		trace[0] = batch_size;
	}
};

struct Pathtracer {
	Scene scene;

	bool invalidated_scene     = true;
	bool invalidated_materials = true;
	bool invalidated_camera    = true;
	bool invalidated_config    = true;

	enum struct PixelQueryStatus {
		INACTIVE,
		PENDING,
		OUTPUT_READY
	} pixel_query_status = PixelQueryStatus::INACTIVE;

	int frames_accumulated = -1;

	PixelQuery pixel_query = { INVALID, INVALID, INVALID, INVALID };

	int * reverse_indices;

	int * mesh_data_bvh_offsets;
	int * mesh_data_triangle_offsets;

	CUDAEventPool event_pool;

	void init(const SceneConfig & scene_config, unsigned frame_buffer_handle, int width, int height);

	void cuda_init(unsigned frame_buffer_handle, int screen_width, int screen_height);
	void cuda_free();

	void resize_init(unsigned frame_buffer_handle, int width, int height); // Part of resize that initializes new size
	void resize_free();                                                    // Part of resize that cleans up old size

	void svgf_init();
	void svgf_free();

	void update(float delta);
	void render();

	void set_pixel_query(int x, int y);

private:
	int screen_width;
	int screen_height;
	int screen_pitch;

	int pixel_count;

	BVH        tlas_raw;
	BVH        tlas;
	BVHBuilder tlas_bvh_builder;

	QBVHBuilder  tlas_converter_qbvh;
	CWBVHBuilder tlas_converter_cwbvh;

	CUDAModule cuda_module;

	CUstream stream_memset;

	CUDAKernel kernel_generate;
	CUDAKernel kernel_trace_bvh;
	CUDAKernel kernel_trace_qbvh;
	CUDAKernel kernel_trace_cwbvh;
	CUDAKernel kernel_sort;
	CUDAKernel kernel_shade_diffuse;
	CUDAKernel kernel_shade_plastic;
	CUDAKernel kernel_shade_dielectric;
	CUDAKernel kernel_shade_conductor;
	CUDAKernel kernel_trace_shadow_bvh;
	CUDAKernel kernel_trace_shadow_qbvh;
	CUDAKernel kernel_trace_shadow_cwbvh;

	CUDAKernel * kernel_trace;
	CUDAKernel * kernel_trace_shadow;

	CUDAKernel kernel_svgf_reproject;
	CUDAKernel kernel_svgf_variance;
	CUDAKernel kernel_svgf_atrous;
	CUDAKernel kernel_svgf_finalize;

	CUDAKernel kernel_taa;
	CUDAKernel kernel_taa_finalize;

	CUDAKernel kernel_accumulate;

	CUgraphicsResource resource_accumulator;
	CUsurfObject       surf_accumulator;

	TraceBuffer     ray_buffer_trace;
	MaterialBuffer  ray_buffer_shade_diffuse_and_plastic;
	MaterialBuffer  ray_buffer_shade_dielectric_and_conductor;
	ShadowRayBuffer ray_buffer_shadow;

	CUDAModule::Global global_ray_buffer_shade_diffuse_and_plastic;
	CUDAModule::Global global_ray_buffer_shade_dielectric_and_conductor;
	CUDAModule::Global global_ray_buffer_shadow;

	BufferSizes * pinned_buffer_sizes;

	CUDAModule::Global global_camera;
	CUDAModule::Global global_buffer_sizes;
	CUDAModule::Global global_config;
	CUDAModule::Global global_svgf_data;

	CUDAModule::Global global_pixel_query;

	CUarray array_gbuffer_normal_and_depth;
	CUarray array_gbuffer_mesh_id_and_triangle_id;
	CUarray array_gbuffer_screen_position_prev;

	CUsurfObject surf_gbuffer_normal_and_depth;
	CUsurfObject surf_gbuffer_mesh_id_and_triangle_id;
	CUsurfObject surf_gbuffer_screen_position_prev;

	CUDAMemory::Ptr<float4> ptr_frame_buffer_albedo;
	CUDAMemory::Ptr<float4> ptr_frame_buffer_moment;

	CUDAMemory::Ptr<float4> ptr_frame_buffer_direct;
	CUDAMemory::Ptr<float4> ptr_frame_buffer_indirect;
	CUDAMemory::Ptr<float4> ptr_frame_buffer_direct_alt;
	CUDAMemory::Ptr<float4> ptr_frame_buffer_indirect_alt;

	CUDAMemory::Ptr<int>    ptr_history_length;
	CUDAMemory::Ptr<float4> ptr_history_direct;
	CUDAMemory::Ptr<float4> ptr_history_indirect;
	CUDAMemory::Ptr<float4> ptr_history_moment;
	CUDAMemory::Ptr<float4> ptr_history_normal_and_depth;

	CUDAMemory::Ptr<float4> ptr_taa_frame_prev;
	CUDAMemory::Ptr<float4> ptr_taa_frame_curr;

	struct Matrix3x4 {
		float cells[12];
	};

	int       * pinned_mesh_bvh_root_indices;
	int       * pinned_mesh_material_ids;
	Matrix3x4 * pinned_mesh_transforms;
	Matrix3x4 * pinned_mesh_transforms_inv;
	Matrix3x4 * pinned_mesh_transforms_prev;
	ProbAlias * pinned_light_mesh_prob_alias;
	int2      * pinned_light_mesh_first_index_and_triangle_count;
	int       * pinned_light_mesh_transform_index;

	double * light_mesh_probabilites; // Scratch memory used to compute pinned_light_mesh_prob_alias

	union alignas(float4) CUDAMaterial {
		struct {
			Vector3 emission;
		} light;
		struct {
			Vector3 diffuse;
			int     texture_id;
		} diffuse;
		struct {
			Vector3 diffuse;
			int     texture_id;
			float   roughness;
		} plastic;
		struct {
			Vector3 negative_absorption;
			float   index_of_refraction;
			float   roughness;
		} dielectric;
		struct {
			Vector3 diffuse;
			int     texture_id;
			Vector3 eta;
			Vector3 k;
			float   roughness;
		} conductor;
	};

	CUDAMemory::Ptr<Material::Type> ptr_material_types;
	CUDAMemory::Ptr<CUDAMaterial>   ptr_materials;

	struct CUDATexture {
		CUtexObject texture;
		float2 size;
	};

	CUDATexture      * textures;
	CUmipmappedArray * texture_arrays;

	CUDAMemory::Ptr<CUDATexture>  ptr_textures;

	struct CUDATriangle {
		Vector3 position_0;
		Vector3 position_edge_1;
		Vector3 position_edge_2;

		Vector3 normal_0;
		Vector3 normal_edge_1;
		Vector3 normal_edge_2;

		Vector2 tex_coord_0;
		Vector2 tex_coord_edge_1;
		Vector2 tex_coord_edge_2;
	};

	CUDAMemory::Ptr<CUDATriangle> ptr_triangles;

	CUDAMemory::Ptr<BVHNode2>    ptr_bvh_nodes_2;
	CUDAMemory::Ptr<BVHNode4>    ptr_bvh_nodes_4;
	CUDAMemory::Ptr<BVHNode8>    ptr_bvh_nodes_8;
	CUDAMemory::Ptr<int>         ptr_mesh_bvh_root_indices;
	CUDAMemory::Ptr<int>         ptr_mesh_material_ids;
	CUDAMemory::Ptr<Matrix3x4>   ptr_mesh_transforms;
	CUDAMemory::Ptr<Matrix3x4>   ptr_mesh_transforms_inv;
	CUDAMemory::Ptr<Matrix3x4>   ptr_mesh_transforms_prev;

	CUDAModule::Global global_lights_total_weight;

	CUDAMemory::Ptr<int>       ptr_light_indices;
	CUDAMemory::Ptr<ProbAlias> ptr_light_prob_alias;

	CUDAMemory::Ptr<ProbAlias> ptr_light_mesh_prob_alias;
	CUDAMemory::Ptr<int2>      ptr_light_mesh_first_index_and_triangle_count;
	CUDAMemory::Ptr<int>       ptr_light_mesh_transform_index;

	CUDAMemory::Ptr<Vector3> ptr_sky_data;

	CUDAMemory::Ptr<PMJ::Point>     ptr_pmj_samples;
	CUDAMemory::Ptr<unsigned short> ptr_blue_noise_textures;

	// Timing Events
	CUDAEvent::Desc event_desc_primary;
	CUDAEvent::Desc event_desc_trace[MAX_BOUNCES];
	CUDAEvent::Desc event_desc_sort [MAX_BOUNCES];
	CUDAEvent::Desc event_desc_shade_diffuse   [MAX_BOUNCES];
	CUDAEvent::Desc event_desc_shade_plastic   [MAX_BOUNCES];
	CUDAEvent::Desc event_desc_shade_dielectric[MAX_BOUNCES];
	CUDAEvent::Desc event_desc_shade_conductor [MAX_BOUNCES];
	CUDAEvent::Desc event_desc_shadow_trace[MAX_BOUNCES];
	CUDAEvent::Desc event_desc_svgf_reproject;
	CUDAEvent::Desc event_desc_svgf_variance;
	CUDAEvent::Desc event_desc_svgf_atrous[MAX_ATROUS_ITERATIONS];
	CUDAEvent::Desc event_desc_svgf_finalize;
	CUDAEvent::Desc event_desc_taa;
	CUDAEvent::Desc event_desc_reconstruct;
	CUDAEvent::Desc event_desc_accumulate;
	CUDAEvent::Desc event_desc_end;

	void calc_light_power();

	void build_tlas();
};
