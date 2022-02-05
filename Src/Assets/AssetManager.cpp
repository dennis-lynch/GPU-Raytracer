#include "AssetManager.h"

#include "Core/IO.h"
#include "Core/Timer.h"

#include "Math/Vector4.h"

#include "BVHLoader.h"
#include "TextureLoader.h"

#include "Util/Util.h"
#include "Util/StringUtil.h"
#include "Util/ThreadPool.h"

AssetManager::AssetManager() : thread_pool(make_owned<ThreadPool>()) {
	Material default_material = { };
	default_material.name    = "Default";
	default_material.diffuse = Vector3(1.0f, 0.0f, 1.0f);
	add_material(default_material);

	Medium default_medium = { };
	default_medium.name = "Default";
	add_medium(default_medium);
}

// NOTE: Seemingly pointless desctructor needed here since ThreadPool is
// forward declared, so its destructor is not available in the header file
AssetManager::~AssetManager() { }

MeshDataHandle AssetManager::add_mesh_data(MeshData mesh_data) {
	MeshDataHandle mesh_data_id = { int(mesh_datas.size()) };
	mesh_datas.push_back(std::move(mesh_data));

	return mesh_data_id;
}

MeshDataHandle AssetManager::add_mesh_data(Array<Triangle> triangles) {
	BVH2 bvh = BVH::create_from_triangles(triangles);

	MeshData mesh_data = { };
	mesh_data.triangles = std::move(triangles);
	mesh_data.bvh = BVH::create_from_bvh2(std::move(bvh));

	return add_mesh_data(std::move(mesh_data));
}

MaterialHandle AssetManager::add_material(const Material & material) {
	MaterialHandle material_id = { int(materials.size()) };
	materials.push_back(material);

	return material_id;
}

MediumHandle AssetManager::add_medium(const Medium & medium) {
	MediumHandle medium_id = { int(media.size()) };
	media.push_back(medium);

	return medium_id;
}

TextureHandle AssetManager::add_texture(const String & filename) {
	TextureHandle & texture_id = texture_cache[filename];

	// If the cache already contains this Texture simply return its index
	if (texture_id.handle != INVALID) return texture_id;

	// Otherwise, create new Texture and load it from disk
	textures_mutex.lock();
	texture_id.handle = textures.size();
	textures.emplace_back();
	textures_mutex.unlock();

	thread_pool->submit([this, filename, texture_id]() {
		StringView name = Util::remove_directory(filename.view());

		Texture texture = { };
		texture.name = name;

		bool success = false;

		StringView file_extension = Util::get_file_extension(filename.view());
		if (!file_extension.is_empty()) {
			if (file_extension == "dds") {
				success = TextureLoader::load_dds(filename, texture); // DDS is loaded using custom code
			} else {
				success = TextureLoader::load_stb(filename, texture); // other file formats use stb_image
			}
		}

		if (!success) {
			IO::print("WARNING: Failed to load Texture '{}'!\n"_sv, filename);

			// Use a default 1x1 pink Texture
			Vector4 pink = Vector4(1.0f, 0.0f, 1.0f, 1.0f);
			texture.data.resize(sizeof(Vector4));
			memcpy(texture.data.data(), &pink, sizeof(Vector4));

			texture.format = Texture::Format::RGBA;
			texture.width  = 1;
			texture.height = 1;
			texture.channels = 4;
			texture.mip_offsets = { 0 };
		}

		textures_mutex.lock();
		textures[texture_id.handle] = std::move(texture);
		textures_mutex.unlock();
	});

	return texture_id;
}

void AssetManager::wait_until_loaded() {
	if (assets_loaded) return; // Only necessary (and valid) to do this once

	thread_pool->sync();
	thread_pool.release();

	mesh_data_cache.clear();
	texture_cache  .clear();

	assets_loaded = true;
}
